#include "OrgasmControlAlgorithm.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/SensorInterface.h"
#include "../hardware/ClitoralOscillator.h"
#include "../hardware/TENSController.h"
#include "../hardware/HeartRateSensor.h"
#include "../hardware/FluidSensor.h"
#include <QDebug>
#include <QThread>
#include <algorithm>
#include <numeric>

OrgasmControlAlgorithm::OrgasmControlAlgorithm(HardwareManager* hardware, QObject* parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_sensorInterface(hardware ? hardware->getSensorInterface() : nullptr)
    , m_clitoralOscillator(hardware ? hardware->getClitoralOscillator() : nullptr)
    , m_tensController(hardware ? hardware->getTENSController() : nullptr)
    , m_heartRateSensor(nullptr)
    , m_fluidSensor(hardware ? hardware->getFluidSensor() : nullptr)
    , m_updateTimer(new QTimer(this))
    , m_safetyTimer(new QTimer(this))
    , m_state(ControlState::STOPPED)
    , m_mode(Mode::MANUAL)
    , m_emergencyStop(false)
    , m_tensEnabled(true)
    , m_antiEscapeEnabled(true)
    , m_heartRateEnabled(false)
    , m_verboseLogging(false)  // Non-critical fix: Default to non-verbose for production
    , m_arousalLevel(0.0)
    , m_smoothedArousal(0.0)
    , m_arousalState(ArousalState::BASELINE)
    , m_baselineClitoral(0.0)
    , m_baselineAVL(0.0)
    , m_historyIndex(0)
    , m_currentHeartRate(0)
    , m_baselineHeartRate(70)
    , m_heartRateContribution(0.0)
    , m_heartRateWeight(DEFAULT_HR_WEIGHT)
    , m_intensity(INITIAL_INTENSITY)
    , m_frequency(INITIAL_FREQUENCY)
    , m_tensAmplitude(0.0)
    , m_edgeCount(0)
    , m_orgasmCount(0)
    , m_targetEdges(5)
    , m_targetOrgasms(3)
    , m_maxDurationMs(MAX_SESSION_DURATION_MS)
    , m_highPressureDuration(0)
    , m_edgeThreshold(DEFAULT_EDGE_THRESHOLD)
    , m_orgasmThreshold(DEFAULT_ORGASM_THRESHOLD)
    , m_recoveryThreshold(DEFAULT_RECOVERY_THRESHOLD)
    , m_fluidTrackingEnabled(false)
    , m_sessionFluidMl(0.0)
    , m_lubricationMl(0.0)
    , m_orgasmicFluidMl(0.0)
    // Bug #1 fix: Initialize previously uninitialized member variables
    , m_previousArousal(0.0)
    , m_inOrgasm(false)
    , m_pointOfNoReturnReached(false)
    , m_unexpectedOrgasmCount(0)
    , m_cooldownStartIntensity(0.0)
    , m_cooldownStartFrequency(0.0)
    // Bug #14 fix: Initialize calibration state variables
    , m_calibSumClitoral(0.0)
    , m_calibSumAVL(0.0)
    , m_calibSamples(0)
    // Bug #15, #16, #17 fix: Initialize seal integrity tracking
    , m_sealLossCount(0)
    , m_resealAttemptInProgress(false)
    // Arousal-adaptive seal integrity tracking
    // Bug fix: Use -1.0 as sentinel for "uninitialized" since pressure can never be negative
    , m_previousAVLPressure(-1.0)
    , m_previousClitoralPressure(0.0)
    // Milking mode thresholds
    , m_milkingZoneLower(DEFAULT_MILKING_ZONE_LOWER)
    , m_milkingZoneUpper(DEFAULT_MILKING_ZONE_UPPER)
    , m_dangerThreshold(DEFAULT_DANGER_THRESHOLD)
    // Milking mode state
    , m_milkingFailureMode(0)
    , m_milkingOrgasmCount(0)
    , m_dangerZoneEntries(0)
    , m_milkingZoneTime(0)
    , m_milkingAvgArousal(0.0)
    , m_milkingAvgSamples(0)
    // Milking PID control state
    , m_milkingIntegralError(0.0)
    , m_milkingPreviousError(0.0)
    , m_milkingTargetArousal(MILKING_TARGET_AROUSAL)
{
    // Initialize history buffers
    m_pressureHistory.resize(HISTORY_SIZE);
    m_pressureHistory.fill(0.0);
    m_arousalHistory.resize(HISTORY_SIZE);
    m_arousalHistory.fill(0.0);

    // Bug #2 fix: Initialize QElapsedTimers to a known state
    // QElapsedTimer::elapsed() returns undefined values if start() was never called
    // Using invalidate() puts them in "not started" state; isValid() returns false
    m_sessionTimer.invalidate();
    m_stateTimer.invalidate();
    m_resealTimer.invalidate();

    // Connect timers
    connect(m_updateTimer, &QTimer::timeout, this, &OrgasmControlAlgorithm::onUpdateTick);
    connect(m_safetyTimer, &QTimer::timeout, this, &OrgasmControlAlgorithm::onSafetyCheck);

    // Connect fluid sensor signals if available
    if (m_fluidSensor && m_fluidSensor->isReady()) {
        m_fluidTrackingEnabled = true;
        connect(m_fluidSensor, &FluidSensor::volumeUpdated,
                this, [this](double current, double cumulative) {
            QMutexLocker locker(&m_mutex);
            m_sessionFluidMl = cumulative;
            emit fluidVolumeUpdated(current, cumulative);
        });
        connect(m_fluidSensor, &FluidSensor::orgasmicBurstDetected,
                this, [this](double volumeMl, double /*peakRate*/, int orgasmNum) {
            QMutexLocker locker(&m_mutex);
            m_orgasmicFluidMl += volumeMl;
            m_fluidPerOrgasm.append(volumeMl);
            emit fluidOrgasmBurst(volumeMl, orgasmNum);
        });
        connect(m_fluidSensor, &FluidSensor::lubricationRateChanged,
                this, &OrgasmControlAlgorithm::lubricationRateChanged);
        connect(m_fluidSensor, &FluidSensor::overflowWarning,
                this, [this](double volumeMl, double /*capacity*/) {
            QMutexLocker locker(&m_mutex);
            emit fluidOverflowWarning(volumeMl);
        });
        qDebug() << "OrgasmControlAlgorithm: Fluid sensor connected";
    }

    qDebug() << "OrgasmControlAlgorithm initialized";
}

OrgasmControlAlgorithm::~OrgasmControlAlgorithm()
{
    // CRITICAL-3 fix: Stop timers FIRST to prevent callbacks during destruction
    // This prevents use-after-free in onUpdateTick/onSafetyCheck
    m_updateTimer->stop();
    m_safetyTimer->stop();

    // CRITICAL-1 fix: Disconnect fluid sensor signals before destruction
    // Prevents use-after-free if FluidSensor outlives this object
    if (m_fluidSensor) {
        disconnect(m_fluidSensor, nullptr, this, nullptr);
    }

    // CRITICAL-1 fix: Also disconnect heart rate sensor (defensive)
    if (m_heartRateSensor) {
        disconnect(m_heartRateSensor, nullptr, this, nullptr);
    }

    stop();
}

// ============================================================================
// Control Methods
// ============================================================================

void OrgasmControlAlgorithm::startAdaptiveEdgingInternal(int targetCycles)
{
    if (m_state != ControlState::STOPPED) {
        qWarning() << "Cannot start: algorithm already running";
        return;
    }

    m_targetEdges = targetCycles;
    m_edgeCount = 0;
    m_orgasmCount = 0;
    m_intensity = INITIAL_INTENSITY;
    m_frequency = INITIAL_FREQUENCY;
    m_emergencyStop = false;

    // Bug #11 fix: Reset high pressure duration tracking for new session
    m_highPressureDuration = 0;

    // Reset seal integrity tracking for new session
    m_sealLossCount = 0;
    m_resealAttemptInProgress = false;
    // Bug fix: Use -1.0 as sentinel for "uninitialized" since pressure can never be negative
    m_previousAVLPressure = -1.0;
    m_previousClitoralPressure = -1.0;

    // Reset orgasm detection state for new session
    m_inOrgasm = false;
    m_pointOfNoReturnReached = false;
    m_unexpectedOrgasmCount = 0;

    // Bug #20 fix: Reset previous arousal to prevent stale value on first iteration
    m_previousArousal = 0.0;
    m_arousalLevel = 0.0;
    m_smoothedArousal = 0.0;

    // Reset fluid tracking
    m_sessionFluidMl = 0.0;
    m_lubricationMl = 0.0;
    m_orgasmicFluidMl = 0.0;
    m_fluidPerOrgasm.clear();

    // Bug #18 fix: Reset milking-specific state to prevent state leaks between sessions
    m_milkingZoneTime = 0;
    m_milkingAvgArousal = 0.0;
    m_milkingAvgSamples = 0;
    m_milkingOrgasmCount = 0;
    m_dangerZoneEntries = 0;
    m_milkingIntegralError = 0.0;
    m_milkingPreviousError = 0.0;

    // Start fluid sensor session
    if (m_fluidSensor && m_fluidTrackingEnabled) {
        m_fluidSensor->startSession();
        m_fluidSensor->setCurrentArousalLevel(0.0);
    }

    setMode(Mode::ADAPTIVE_EDGING);
    setState(ControlState::CALIBRATING);

    // Start timers
    // Bug #2 fix: Start m_stateTimer to ensure it's valid even during calibration
    m_sessionTimer.start();
    m_stateTimer.start();
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_safetyTimer->start(SAFETY_INTERVAL_MS);

    qDebug() << "Started Adaptive Edging with target cycles:" << targetCycles;

    // Reset calibration state
    m_calibSumClitoral = 0.0;
    m_calibSumAVL = 0.0;
    m_calibSamples = 0;
}

void OrgasmControlAlgorithm::startAdaptiveEdging(int targetCycles)
{
    QMutexLocker locker(&m_mutex);
    startAdaptiveEdgingInternal(targetCycles);
}

void OrgasmControlAlgorithm::startForcedOrgasm(int targetOrgasms, int maxDurationMs)
{
    QMutexLocker locker(&m_mutex);

    if (m_state != ControlState::STOPPED) {
        qWarning() << "Cannot start: algorithm already running";
        return;
    }

    m_targetOrgasms = targetOrgasms;
    m_maxDurationMs = maxDurationMs;
    m_edgeCount = 0;
    m_orgasmCount = 0;
    m_intensity = FORCED_BASE_INTENSITY;
    m_frequency = FORCED_BASE_FREQUENCY;
    m_tensAmplitude = FORCED_TENS_AMPLITUDE;
    m_emergencyStop = false;

    // Bug #11 fix: Reset high pressure duration tracking for new session
    m_highPressureDuration = 0;

    // Reset seal integrity tracking for new session
    m_sealLossCount = 0;
    m_resealAttemptInProgress = false;
    // Bug fix: Use -1.0 as sentinel for "uninitialized" since pressure can never be negative
    m_previousAVLPressure = -1.0;
    m_previousClitoralPressure = -1.0;

    // Reset orgasm detection state for new session
    m_inOrgasm = false;
    m_pointOfNoReturnReached = false;
    m_unexpectedOrgasmCount = 0;

    // Bug #20 fix: Reset previous arousal to prevent stale value / false anti-escape trigger
    m_previousArousal = 0.0;
    m_arousalLevel = 0.0;
    m_smoothedArousal = 0.0;

    // Reset fluid tracking
    m_sessionFluidMl = 0.0;
    m_lubricationMl = 0.0;
    m_orgasmicFluidMl = 0.0;
    m_fluidPerOrgasm.clear();

    // Bug #18 fix: Reset milking-specific state to prevent state leaks between sessions
    m_milkingZoneTime = 0;
    m_milkingAvgArousal = 0.0;
    m_milkingAvgSamples = 0;
    m_milkingOrgasmCount = 0;
    m_dangerZoneEntries = 0;
    m_milkingIntegralError = 0.0;
    m_milkingPreviousError = 0.0;

    // Start fluid sensor session
    if (m_fluidSensor && m_fluidTrackingEnabled) {
        m_fluidSensor->startSession();
        m_fluidSensor->setCurrentArousalLevel(0.0);
    }

    setMode(Mode::FORCED_ORGASM);
    setState(ControlState::CALIBRATING);

    // Bug #2 fix: Start m_stateTimer to ensure it's valid even during calibration
    m_sessionTimer.start();
    m_stateTimer.start();
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_safetyTimer->start(SAFETY_INTERVAL_MS);

    qDebug() << "Started Forced Orgasm with target:" << targetOrgasms
             << "max duration:" << maxDurationMs << "ms";

    // Reset calibration state
    m_calibSumClitoral = 0.0;
    m_calibSumAVL = 0.0;
    m_calibSamples = 0;
}

void OrgasmControlAlgorithm::startDenial(int durationMs)
{
    QMutexLocker locker(&m_mutex);

    // Bug #6 fix: Set mode AFTER calling startAdaptiveEdgingInternal,
    // which would otherwise overwrite it with ADAPTIVE_EDGING

    // Use edging logic but never allow release
    startAdaptiveEdgingInternal(999);  // Very high target - duration is the limit

    // Bug #7 fix: Denial-specific initialization AFTER base edging initialization
    // Set denial mode and duration AFTER initialization
    // (startAdaptiveEdgingInternal sets Mode::ADAPTIVE_EDGING, we override it here)
    m_maxDurationMs = durationMs;
    m_edgeCount = 0;
    m_orgasmCount = 0;  // Bug #7: Explicitly reset orgasm count for denial mode
    m_unexpectedOrgasmCount = 0;  // Bug #7: Reset unexpected orgasm tracking

    // Denial mode never transitions to forced orgasm, ensure these are clean
    m_inOrgasm = false;
    m_pointOfNoReturnReached = false;

    setMode(Mode::DENIAL);
}

void OrgasmControlAlgorithm::startMilking(int durationMs, int failureMode)
{
    QMutexLocker locker(&m_mutex);
    startMilkingInternal(durationMs, failureMode);
}

void OrgasmControlAlgorithm::startMilkingInternal(int durationMs, int failureMode)
{
    if (m_state != ControlState::STOPPED) {
        qWarning() << "Cannot start milking: algorithm already running";
        return;
    }

    qDebug() << "Starting milking session:" << durationMs << "ms, failure mode:" << failureMode;

    // Reset session state
    m_edgeCount = 0;
    m_orgasmCount = 0;
    m_emergencyStop = false;
    m_highPressureDuration = 0;

    // Reset seal integrity tracking
    m_sealLossCount = 0;
    m_resealAttemptInProgress = false;
    // Bug fix: Use -1.0 as sentinel for "uninitialized" since pressure can never be negative
    m_previousAVLPressure = -1.0;
    m_previousClitoralPressure = -1.0;

    // Reset orgasm detection state
    m_inOrgasm = false;
    m_pointOfNoReturnReached = false;
    m_unexpectedOrgasmCount = 0;

    // Bug #20 fix: Reset previous arousal to prevent stale value on first iteration
    m_previousArousal = 0.0;
    m_arousalLevel = 0.0;
    m_smoothedArousal = 0.0;

    // Reset fluid tracking
    m_sessionFluidMl = 0.0;
    m_lubricationMl = 0.0;
    m_orgasmicFluidMl = 0.0;
    m_fluidPerOrgasm.clear();

    // Reset milking-specific state
    m_milkingOrgasmCount = 0;
    m_dangerZoneEntries = 0;
    m_milkingZoneTime = 0;
    m_milkingAvgArousal = 0.0;
    m_milkingAvgSamples = 0;

    // Reset PID state
    m_milkingIntegralError = 0.0;
    m_milkingPreviousError = 0.0;
    m_milkingTargetArousal = MILKING_TARGET_AROUSAL;

    // Set session parameters
    m_maxDurationMs = qMin(durationMs, MILKING_MAX_SESSION_MS);
    // Bug #6 fix: Use qBound directly with int types instead of casting through double
    m_milkingFailureMode = qBound(0, failureMode, 3);

    // Set initial intensity for milking
    m_intensity = MILKING_BASE_INTENSITY;
    m_frequency = MILKING_BASE_FREQUENCY;
    if (m_tensEnabled) {
        m_tensAmplitude = MILKING_TENS_AMPLITUDE;
    }

    // Start fluid sensor session
    if (m_fluidSensor && m_fluidTrackingEnabled) {
        m_fluidSensor->startSession();
        m_fluidSensor->setCurrentArousalLevel(0.0);
    }

    // Set mode and state
    setMode(Mode::MILKING);
    setState(ControlState::CALIBRATING);

    // Reset calibration state
    m_calibSumClitoral = 0.0;
    m_calibSumAVL = 0.0;
    m_calibSamples = 0;

    // Start timers
    m_sessionTimer.start();
    m_stateTimer.start();
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_safetyTimer->start(SAFETY_INTERVAL_MS);
}

void OrgasmControlAlgorithm::stop()
{
    QMutexLocker locker(&m_mutex);

    m_updateTimer->stop();
    m_safetyTimer->stop();

    // Stop all stimulation gracefully
    if (m_clitoralOscillator) {
        m_clitoralOscillator->stop();
    }

    if (m_tensController) {
        m_tensController->stop();
    }

    if (m_hardware) {
        m_hardware->setSOL2(true);   // Vent outer chamber
        m_hardware->setSOL5(true);   // Vent clitoral chamber
    }

    // End fluid sensor session
    if (m_fluidSensor && m_fluidTrackingEnabled) {
        m_fluidSensor->endSession();
        qDebug() << "Fluid session ended: total=" << m_sessionFluidMl << "mL"
                 << "lubrication=" << m_lubricationMl << "mL"
                 << "orgasmic=" << m_orgasmicFluidMl << "mL";
    }

    setState(ControlState::STOPPED);
    setMode(Mode::MANUAL);

    qDebug() << "OrgasmControlAlgorithm stopped";
}

void OrgasmControlAlgorithm::emergencyStop()
{
    // CRITICAL-2 fix: Acquire mutex before accessing member variables
    // handleEmergencyStop() modifies m_emergencyStop, timers, and hardware state
    QMutexLocker locker(&m_mutex);
    handleEmergencyStop();
}

// ============================================================================
// Configuration
// ============================================================================

void OrgasmControlAlgorithm::setEdgeThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    double newValue = clamp(threshold, 0.5, 0.95);
    if (m_edgeThreshold != newValue) {
        m_edgeThreshold = newValue;
        emit edgeThresholdChanged(newValue);
    }
}

void OrgasmControlAlgorithm::setOrgasmThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    double newValue = clamp(threshold, 0.85, 1.0);
    if (m_orgasmThreshold != newValue) {
        m_orgasmThreshold = newValue;
        emit orgasmThresholdChanged(newValue);
    }
}

void OrgasmControlAlgorithm::setRecoveryThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    double newValue = clamp(threshold, 0.3, 0.8);
    if (m_recoveryThreshold != newValue) {
        m_recoveryThreshold = newValue;
        emit recoveryThresholdChanged(newValue);
    }
}

void OrgasmControlAlgorithm::setTENSEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_tensEnabled = enabled;
}

void OrgasmControlAlgorithm::setAntiEscapeEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_antiEscapeEnabled = enabled;
}

void OrgasmControlAlgorithm::setMilkingZoneLower(double threshold)
{
    QMutexLocker locker(&m_mutex);
    // HIGH-4 fix: Ensure lower < upper with minimum gap of 0.05
    static constexpr double MIN_ZONE_GAP = 0.05;
    threshold = clamp(threshold, 0.50, 0.85);
    // Cap at upper threshold minus minimum gap to ensure valid range
    m_milkingZoneLower = qMin(threshold, m_milkingZoneUpper - MIN_ZONE_GAP);
}

void OrgasmControlAlgorithm::setMilkingZoneUpper(double threshold)
{
    QMutexLocker locker(&m_mutex);
    // HIGH-4 fix: Ensure lower < upper < danger with minimum gaps
    static constexpr double MIN_ZONE_GAP = 0.05;
    threshold = clamp(threshold, 0.80, 0.94);
    // Must be above lower threshold and below danger threshold
    threshold = qMax(threshold, m_milkingZoneLower + MIN_ZONE_GAP);
    m_milkingZoneUpper = qMin(threshold, m_dangerThreshold - MIN_ZONE_GAP);
}

void OrgasmControlAlgorithm::setDangerThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    // HIGH-4 fix: Ensure danger > upper with minimum gap of 0.02
    static constexpr double MIN_DANGER_GAP = 0.02;
    threshold = clamp(threshold, 0.88, 0.96);
    // Must be above upper threshold
    m_dangerThreshold = qMax(threshold, m_milkingZoneUpper + MIN_DANGER_GAP);
}

void OrgasmControlAlgorithm::setMilkingFailureMode(int mode)
{
    QMutexLocker locker(&m_mutex);
    // Bug #6 fix: Use qBound directly with int types instead of casting through double
    m_milkingFailureMode = qBound(0, mode, 3);
}

void OrgasmControlAlgorithm::setHeartRateSensor(HeartRateSensor* sensor)
{
    QMutexLocker locker(&m_mutex);

    // Bug #8 fix: Disconnect old sensor signals before connecting new one
    // This prevents stale connections and potential null dereference
    if (m_heartRateSensor) {
        disconnect(m_heartRateSensor, nullptr, this, nullptr);
    }

    m_heartRateSensor = sensor;

    if (sensor) {
        m_heartRateEnabled = true;

        // Connect to heart rate signals
        // Bug #8 fix: Lambdas check m_heartRateSensor validity before accessing
        connect(sensor, &HeartRateSensor::heartRateUpdated,
                this, [this](int bpm) {
                    QMutexLocker locker(&m_mutex);
                    if (!m_heartRateSensor) return;  // Sensor was removed
                    m_currentHeartRate = bpm;
                });

        connect(sensor, &HeartRateSensor::signalLost,
                this, [this]() {
                    QMutexLocker locker(&m_mutex);
                    if (!m_heartRateSensor) return;  // Sensor was removed
                    emit heartRateSensorLost();
                    // Reduce HR weight when signal is lost
                    m_heartRateWeight = 0.0;
                });

        connect(sensor, &HeartRateSensor::signalRecovered,
                this, [this]() {
                    QMutexLocker locker(&m_mutex);
                    if (!m_heartRateSensor) return;  // Sensor was removed
                    m_heartRateWeight = DEFAULT_HR_WEIGHT;
                });

        qDebug() << "Heart rate sensor connected to OrgasmControlAlgorithm";
    } else {
        m_heartRateEnabled = false;
        m_heartRateWeight = 0.0;
    }
}

void OrgasmControlAlgorithm::setHeartRateEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_heartRateEnabled = enabled && (m_heartRateSensor != nullptr);

    if (!m_heartRateEnabled) {
        m_heartRateWeight = 0.0;
    } else {
        m_heartRateWeight = DEFAULT_HR_WEIGHT;
    }
}

void OrgasmControlAlgorithm::setHeartRateWeight(double weight)
{
    QMutexLocker locker(&m_mutex);
    m_heartRateWeight = clamp(weight, 0.0, 0.5);  // Max 50% weight for HR
}

QVector<double> OrgasmControlAlgorithm::getArousalHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_arousalHistory;
}

// ============================================================================
// Timer Callbacks
// ============================================================================

void OrgasmControlAlgorithm::onUpdateTick()
{
    QMutexLocker locker(&m_mutex);
    if (m_emergencyStop) return;

    // Execute state-specific logic
    switch (m_state) {
        case ControlState::CALIBRATING: {
            if (m_sessionTimer.elapsed() < BASELINE_DURATION_MS) {
                // Collect data for calibration
                if (m_hardware) {
                    // Bug #12 fix: Only accumulate valid pressure readings
                    // MEDIUM-3 fix: Use class-level constants instead of local static constexpr
                    double clitoralReading = m_hardware->readClitoralPressure();
                    double avlReading = m_hardware->readAVLPressure();

                    // Only include valid readings in calibration
                    if (clitoralReading >= PRESSURE_MIN_VALID && clitoralReading <= PRESSURE_MAX_VALID &&
                        avlReading >= PRESSURE_MIN_VALID && avlReading <= PRESSURE_MAX_VALID) {
                        m_calibSumClitoral += clitoralReading;
                        m_calibSumAVL += avlReading;
                        m_calibSamples++;
                    } else {
                        qWarning() << "Invalid calibration reading - Clitoral:" << clitoralReading
                                   << "mmHg, AVL:" << avlReading << "mmHg";
                    }
                }
            } else {
                // Finalize calibration
                // HIGH-5 fix: Check for minimum valid samples before proceeding
                if (m_calibSamples < MIN_CALIBRATION_SAMPLES) {
                    qWarning() << "Calibration failed: only" << m_calibSamples
                               << "valid samples (need" << MIN_CALIBRATION_SAMPLES << ")";
                    emit sensorError("Calibration",
                        QString("Insufficient valid readings: %1/%2").arg(m_calibSamples).arg(MIN_CALIBRATION_SAMPLES));
                    handleEmergencyStop();
                    break;
                }

                m_baselineClitoral = m_calibSumClitoral / m_calibSamples;
                m_baselineAVL = m_calibSumAVL / m_calibSamples;

                // HIGH-5 fix: Validate baseline values are reasonable
                if (m_baselineClitoral < 1.0 || m_baselineAVL < 1.0) {
                    qWarning() << "Calibration failed: baseline too low - Clitoral:"
                               << m_baselineClitoral << "AVL:" << m_baselineAVL;
                    emit sensorError("Calibration",
                        QString("Baseline too low: Clitoral=%1, AVL=%2").arg(m_baselineClitoral).arg(m_baselineAVL));
                    handleEmergencyStop();
                    break;
                }

                std::fill(m_pressureHistory.begin(), m_pressureHistory.end(), m_baselineClitoral);
                m_historyIndex.store(0, std::memory_order_release);  // Bug #1 fix: atomic store
                qDebug() << "Baseline calibrated: Clitoral=" << m_baselineClitoral
                         << "mmHg, AVL=" << m_baselineAVL << "mmHg"
                         << "samples:" << m_calibSamples;

                // Transition to next state based on mode
                if (m_mode == Mode::ADAPTIVE_EDGING || m_mode == Mode::DENIAL) {
                    m_stateTimer.start();  // Bug #5 fix: Initialize state timer
                    setState(ControlState::BUILDING);
                } else if (m_mode == Mode::FORCED_ORGASM || m_mode == Mode::MULTI_ORGASM) {
                    m_stateTimer.start();  // Bug #5 fix: Initialize state timer for FORCING state
                    m_inOrgasm = false;    // Ensure clean orgasm detection state
                    setState(ControlState::FORCING);
                } else if (m_mode == Mode::MILKING) {
                    m_stateTimer.start();
                    setState(ControlState::BUILDING);
                }
            }
            break;
        }

        case ControlState::BUILDING:
            updateArousalLevel();
            // Milking mode uses BUILDING to ramp up to milking zone
            if (m_mode == Mode::MILKING) {
                runMilking();
            } else {
                runAdaptiveEdging();
            }
            break;

        case ControlState::BACKING_OFF:
        case ControlState::HOLDING:
            updateArousalLevel();
            runAdaptiveEdging();
            break;

        case ControlState::FORCING:
            updateArousalLevel();
            runForcedOrgasm();
            break;

        case ControlState::MILKING:
        case ControlState::DANGER_REDUCTION:
        case ControlState::ORGASM_FAILURE:
            updateArousalLevel();
            runMilking();
            break;

        case ControlState::COOLING_DOWN: {
            if (m_stateTimer.elapsed() < COOLDOWN_DURATION_MS) {
                double progress = static_cast<double>(m_stateTimer.elapsed()) / COOLDOWN_DURATION_MS;
                m_intensity = m_cooldownStartIntensity * (1.0 - progress);
                m_frequency = qMax(m_cooldownStartFrequency * (1.0 - progress), 3.0);

                if (m_clitoralOscillator) {
                    m_clitoralOscillator->setFrequency(m_frequency);
                    m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
                }
                if (m_tensController) {
                    m_tensController->setAmplitude(m_intensity * 50.0);
                }
            } else {
                stop(); // Cooldown finished
            }
            break;
        }

        case ControlState::STOPPED:
        case ControlState::ERROR:
        default:
            // Do nothing
            break;
    }
}

void OrgasmControlAlgorithm::onSafetyCheck()
{
    QMutexLocker locker(&m_mutex);
    performSafetyCheck();
}

// ============================================================================
// Arousal Detection
// ============================================================================

void OrgasmControlAlgorithm::updateArousalLevel()
{
    // Bug #3 fix: Load index ONCE at cycle start, pass to all functions
    // This prevents race condition where pressure and arousal histories
    // could be written with different indices
    int currentIdx = m_historyIndex.load(std::memory_order_acquire);

    // Pass index to calculateArousalLevel - it will use this for pressure history
    double newArousal = calculateArousalLevel(currentIdx);

    // Write arousal history with SAME index used for pressure history
    m_arousalHistory[currentIdx] = newArousal;

    // Advance index ONCE at cycle end
    m_historyIndex.store((currentIdx + 1) % HISTORY_SIZE, std::memory_order_release);

    // Update state if changed
    // Bug #11 fix: Use atomic load for thread-safe access
    ArousalState oldState = m_arousalState.load(std::memory_order_acquire);
    updateArousalState();
    ArousalState newState = m_arousalState.load(std::memory_order_acquire);

    if (newState != oldState) {
        emit arousalStateChanged(newState);
    }

    if (qAbs(newArousal - m_arousalLevel) > 0.01) {
        m_arousalLevel = newArousal;
        emit arousalLevelChanged(m_arousalLevel);

        // Update fluid sensor with current arousal level for correlation
        if (m_fluidSensor && m_fluidTrackingEnabled) {
            m_fluidSensor->setCurrentArousalLevel(m_arousalLevel);
        }
    }
}

double OrgasmControlAlgorithm::calculateArousalLevel(int currentIdx)
{
    if (!m_hardware) return 0.0;

    // Bug #12 fix: Read pressure values (units: mmHg, range: 0-75 mmHg for MPX5010DP)
    // Returns -1.0 on sensor error - must validate before use
    double currentClitoral = m_hardware->readClitoralPressure();
    double currentAVL = m_hardware->readAVLPressure();

    // Bug #12 fix: Validate sensor readings
    // Negative values indicate sensor error; use previous value or baseline
    // MEDIUM-3 fix: Use class-level PRESSURE_MIN_VALID/PRESSURE_MAX_VALID constants
    if (currentClitoral < PRESSURE_MIN_VALID || currentClitoral > PRESSURE_MAX_VALID) {
        qWarning() << "Invalid clitoral pressure reading:" << currentClitoral << "mmHg - using baseline";
        currentClitoral = m_baselineClitoral > 0.0 ? m_baselineClitoral : 0.0;
    }
    if (currentAVL < PRESSURE_MIN_VALID || currentAVL > PRESSURE_MAX_VALID) {
        qWarning() << "Invalid AVL pressure reading:" << currentAVL << "mmHg - using baseline";
        currentAVL = m_baselineAVL > 0.0 ? m_baselineAVL : 0.0;
    }

    // Bug #3 fix: Use passed currentIdx - DO NOT reload m_historyIndex here!
    // The caller (updateArousalLevel) loads the index once and passes it to ensure
    // consistent index usage across pressure history, arousal history, and all helper functions

    // Store in pressure history (validated values only)
    m_pressureHistory[currentIdx] = currentClitoral;

    // Feature 1: Baseline deviation (tissue engorgement)
    double baselineDeviation = 0.0;
    if (m_baselineClitoral > 0.1) {
        baselineDeviation = (currentClitoral - m_baselineClitoral) / m_baselineClitoral;
    }

    // Feature 2: Pressure variance (arousal fluctuations)
    // Bug #3 fix: Use passed currentIdx to ensure consistent index across all calculations
    double pressureVariance = calculateVariance(m_pressureHistory, VARIANCE_WINDOW_SAMPLES, currentIdx);

    // Feature 3: Contraction band power (0.8-1.2 Hz = orgasmic contractions)
    double contractionPower = calculateBandPower(m_pressureHistory, 0.8, 1.2, currentIdx);

    // Feature 4: Rate of change
    double rateOfChange = calculateDerivative(m_pressureHistory, currentIdx);

    // Feature 5: Seal integrity (reduces arousal if seal is poor)
    double sealIntegrity = 1.0;
    if (m_baselineAVL > 0.1) {
        sealIntegrity = clamp(currentAVL / m_baselineAVL, 0.0, 1.0);
    }

    // Normalize pressure features
    double normDeviation = clamp(qAbs(baselineDeviation) / MAX_DEVIATION, 0.0, 1.0);
    double normVariance = clamp(pressureVariance / MAX_VARIANCE, 0.0, 1.0);
    double normContraction = clamp(contractionPower / MAX_CONTRACTION_POWER, 0.0, 1.0);
    double normROC = clamp(qAbs(rateOfChange) / MAX_RATE_OF_CHANGE, 0.0, 1.0);

    // Calculate pressure-based arousal component (raw weighted sum)
    double pressureArousalRaw =
        WEIGHT_DEVIATION * normDeviation +
        WEIGHT_VARIANCE * normVariance +
        WEIGHT_CONTRACTION * normContraction +
        WEIGHT_ROC * normROC;

    // Bug #2 fix: Normalize pressure arousal to 0-1 range
    // The weights sum to 0.70, so we must normalize to allow reaching edge threshold
    // when heart rate sensor is disabled
    static constexpr double PRESSURE_WEIGHT_SUM = WEIGHT_DEVIATION + WEIGHT_VARIANCE +
                                                   WEIGHT_CONTRACTION + WEIGHT_ROC;  // = 0.70
    // Bug #7 fix: Defensive check for division by zero (should never happen with current constants)
    double pressureArousal = (PRESSURE_WEIGHT_SUM > 0.0)
                             ? (pressureArousalRaw / PRESSURE_WEIGHT_SUM)
                             : 0.0;

    // Calculate heart rate arousal component (if enabled)
    double heartRateArousal = 0.0;
    m_heartRateContribution = 0.0;

    if (m_heartRateEnabled && m_heartRateSensor && m_heartRateSensor->hasPulseSignal()) {
        // Get heart rate features
        double hrNormalized = m_heartRateSensor->getHeartRateNormalized();
        double hrvNormalized = m_heartRateSensor->getHRVNormalized();
        double hrAcceleration = m_heartRateSensor->getHeartRateAcceleration();

        // Normalize acceleration (typical max is 10 BPM/sec during orgasm)
        double normAccel = clamp(qAbs(hrAcceleration) / 10.0, 0.0, 1.0);

        // Weighted combination of HR features (weights sum to 1.0)
        heartRateArousal =
            WEIGHT_HR_ZONE * hrNormalized +
            WEIGHT_HRV * hrvNormalized +
            WEIGHT_HR_ACCEL * normAccel;

        m_heartRateContribution = heartRateArousal;
        m_currentHeartRate = m_heartRateSensor->getCurrentBPM();

        // Check for orgasm signature in heart rate
        if (m_heartRateSensor->isOrgasmSignature()) {
            emit heartRateOrgasmSignature();
        }

        emit heartRateUpdated(m_currentHeartRate, m_heartRateContribution);
    }

    // Combine pressure and heart rate components
    // When HR is enabled: 70% pressure + 30% HR (both normalized to 0-1)
    // When HR is disabled: 100% pressure (normalized to 0-1, can reach thresholds)
    //
    // Bug #12 fix: When heart rate is disabled (or no signal), use 100% pressure weight
    // Previously this was using m_heartRateWeight even when HR was disabled,
    // which capped arousal at 70% and prevented reaching the edge threshold of 85%
    double effectiveHRWeight = (m_heartRateEnabled && m_heartRateSensor &&
                                 m_heartRateSensor->hasPulseSignal())
                                ? m_heartRateWeight : 0.0;
    double effectivePressureWeight = 1.0 - effectiveHRWeight;
    double arousal = effectivePressureWeight * pressureArousal +
                     effectiveHRWeight * heartRateArousal;

    // Apply seal integrity penalty
    arousal *= sealIntegrity;

    // Exponential smoothing
    m_smoothedArousal = AROUSAL_ALPHA * arousal + (1.0 - AROUSAL_ALPHA) * m_smoothedArousal;

    return clamp(m_smoothedArousal, 0.0, 1.0);
}

double OrgasmControlAlgorithm::calculateVariance(const QVector<double>& data, int windowSize, int currentIdx)
{
    if (data.isEmpty() || windowSize <= 1) return 0.0;

    // Critical fix: Clamp windowSize to valid range to prevent negative index
    windowSize = qBound(2, windowSize, HISTORY_SIZE);

    // Bug #1 fix: currentIdx passed from caller to ensure consistent index across all calculations
    // Critical fix: Use modular arithmetic that handles large negative offsets correctly
    // Adding 2*HISTORY_SIZE ensures result is always positive before modulo
    int startIdx = ((currentIdx - windowSize) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
    double sum = 0.0;
    double sumSq = 0.0;
    int count = qMin(windowSize, data.size());

    for (int i = 0; i < count; ++i) {
        int idx = (startIdx + i) % HISTORY_SIZE;
        sum += data[idx];
        sumSq += data[idx] * data[idx];
    }

    double mean = sum / count;
    return (sumSq / count) - (mean * mean);
}

double OrgasmControlAlgorithm::calculateBandPower(const QVector<double>& data,
                                                   double freqLow, double freqHigh, int currentIdx)
{
    // Simplified band power using autocorrelation at target period
    // For 0.8-1.2 Hz at 10 Hz sampling, check lags 8-12 samples

    if (data.size() < 20) return 0.0;

    // Bug #4 fix: Validate frequency parameters to prevent division by zero or invalid lags
    if (freqLow <= 0.0 || freqHigh <= 0.0 || freqHigh < freqLow) {
        return 0.0;
    }

    int lagLow = static_cast<int>(10.0 / freqHigh);   // ~8 samples for 1.2 Hz
    int lagHigh = static_cast<int>(10.0 / freqLow);   // ~12 samples for 0.8 Hz

    // Bug #4 fix: Clamp lag values to valid buffer bounds
    // Lag must be at least 1 (can't correlate with self) and at most HISTORY_SIZE - 1
    lagLow = qBound(1, lagLow, HISTORY_SIZE - 1);
    lagHigh = qBound(1, lagHigh, HISTORY_SIZE - 1);

    // Bug fix: Ensure lagHigh >= lagLow after clamping to prevent empty loop range
    if (lagHigh < lagLow) {
        return 0.0;
    }

    // Bug #1 fix: currentIdx passed from caller to ensure consistent index across all calculations
    double maxCorrelation = 0.0;

    for (int lag = lagLow; lag <= lagHigh; ++lag) {
        double correlation = 0.0;
        int count = 0;

        // Critical fix: Limit loop to prevent (i + lag) from exceeding HISTORY_SIZE
        // This ensures idx2 calculation never goes negative even with single HISTORY_SIZE addition
        int maxI = qMin(HISTORY_SIZE - lag, HISTORY_SIZE);

        for (int i = 0; i < maxI; ++i) {
            // Critical fix: Use double-modulo pattern to handle negative values correctly
            // ((x % N) + N) % N guarantees positive result for any x
            int idx1 = ((currentIdx - i) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
            int idx2 = ((currentIdx - i - lag) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
            correlation += data[idx1] * data[idx2];
            count++;
        }

        if (count > 0) {
            correlation /= count;
            maxCorrelation = qMax(maxCorrelation, correlation);
        }
    }

    return maxCorrelation;
}

double OrgasmControlAlgorithm::calculateDerivative(const QVector<double>& data, int currentIdx)
{
    if (data.size() < 5) return 0.0;

    // Bug #1 fix: currentIdx passed from caller to ensure consistent index across all calculations
    // Simple 5-point derivative
    double sum = 0.0;
    for (int i = 1; i <= 4; ++i) {
        // Critical fix: Use double-modulo pattern to handle negative values correctly
        int idx1 = ((currentIdx - i + 1) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
        int idx2 = ((currentIdx - i) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
        sum += data[idx1] - data[idx2];
    }

    // Convert to rate per second (10 Hz sampling = 0.1s per sample)
    return sum / 4.0 * 10.0;
}

bool OrgasmControlAlgorithm::detectContractions()
{
    // Check for rhythmic contractions in 0.8-1.2 Hz band
    // Critical fix: Must pass currentIdx for thread-safe buffer access
    int currentIdx = m_historyIndex.load(std::memory_order_acquire);
    double power = calculateBandPower(m_pressureHistory, 0.8, 1.2, currentIdx);
    return power > MAX_CONTRACTION_POWER * 0.5;  // Threshold at 50% of max
}

void OrgasmControlAlgorithm::updateArousalState()
{
    // Bug #11 fix: Use atomic store for thread-safe state transitions
    ArousalState newState;
    if (m_arousalLevel < 0.2) {
        newState = ArousalState::BASELINE;
    } else if (m_arousalLevel < 0.5) {
        newState = ArousalState::WARMING;
    } else if (m_arousalLevel < m_edgeThreshold) {
        newState = ArousalState::PLATEAU;
    } else if (m_arousalLevel < m_orgasmThreshold) {
        newState = ArousalState::PRE_ORGASM;
    } else {
        newState = ArousalState::ORGASM;
    }
    m_arousalState.store(newState, std::memory_order_release);
}

// ============================================================================
// Algorithm Execution
// ============================================================================

void OrgasmControlAlgorithm::runAdaptiveEdging()
{
    // Bug #7 fix: Check session timeout for denial mode (duration-limited edging)
    if (m_mode == Mode::DENIAL && m_sessionTimer.elapsed() >= m_maxDurationMs) {
        emit sessionTimeoutWarning();
        emit edgingComplete(m_edgeCount);
        startCoolDown();
        return;
    }

    // ========================================================================
    // Handle ongoing orgasm (unexpected orgasm during edging)
    // This takes priority over all state logic
    // ========================================================================
    if (m_inOrgasm) {
        // Continue supporting the orgasm - don't fight it
        if (m_hardware) {
            m_hardware->setSOL1(true);
            m_hardware->setSOL2(false);
        }
        if (m_clitoralOscillator) {
            if (!m_clitoralOscillator->isRunning()) m_clitoralOscillator->start();
            // Gentle stimulation through orgasm
            m_clitoralOscillator->setFrequency(HOLD_FREQUENCY);
            m_clitoralOscillator->setAmplitude(m_intensity * 0.7 * MAX_CLITORAL_AMPLITUDE);
        }

        // Check if orgasm has completed
        if (m_stateTimer.elapsed() > ORGASM_DURATION_MS + POST_ORGASM_PAUSE_MS) {
            m_inOrgasm = false;
            m_pointOfNoReturnReached = false;
            qDebug() << "Unexpected orgasm during edging completed after"
                     << m_stateTimer.elapsed() << "ms";

            // Post-orgasm behavior depends on mode
            if (m_mode == Mode::DENIAL) {
                // Denial mode: orgasm was not supposed to happen, end session
                qDebug() << "Denial mode: unexpected orgasm occurred, ending session";
                emit edgingComplete(m_edgeCount);
                startCoolDown();
            } else {
                // Adaptive edging: recovery period then optionally continue
                qDebug() << "Adaptive edging: entering extended recovery after unexpected orgasm";
                setState(ControlState::BACKING_OFF);
                m_stateTimer.start();
                // Will need to wait longer than normal before resuming
                // The BACKING_OFF state will handle normal recovery
            }
        }
        m_previousArousal = m_arousalLevel;
        return;
    }

    switch (m_state) {
        case ControlState::BUILDING: {
            emit buildUpProgress(m_arousalLevel, m_edgeThreshold);

            // ================================================================
            // ORGASM DETECTION - Check before edge detection
            // If arousal exceeds orgasm threshold with contractions, this is
            // a real orgasm, not just an edge
            // ================================================================
            if (m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
                // Orgasm detected during edging - this is unexpected but must be handled
                m_inOrgasm = true;
                m_orgasmCount++;
                m_unexpectedOrgasmCount++;
                m_stateTimer.start();

                qDebug() << "Unexpected orgasm detected during edging!"
                         << "arousal:" << m_arousalLevel
                         << "edge count:" << m_edgeCount;

                emit unexpectedOrgasmDuringEdging(m_orgasmCount, m_edgeCount);
                emit orgasmDetected(m_orgasmCount, m_sessionTimer.elapsed());

                // Record fluid event if tracking
                if (m_fluidSensor && m_fluidTrackingEnabled) {
                    m_fluidSensor->recordOrgasmEvent(m_orgasmCount);
                }

                // Continue stimulation through orgasm (handled in m_inOrgasm block above)
                // Do NOT stop or back off - let the orgasm complete naturally

            } else if (m_arousalLevel >= m_edgeThreshold) {
                // EDGE DETECTED - back off!
                m_edgeCount++;
                emit edgeDetected(m_edgeCount, m_intensity);

                if (m_clitoralOscillator) m_clitoralOscillator->stop();
                // SAFETY: Only vent clitoral chamber (SOL5), NEVER outer chamber (SOL2)
                // Outer chamber maintains seal - venting SOL2 causes detachment
                if (m_hardware) m_hardware->setSOL5(true);  // Vent clitoral only
                if (m_tensController) m_tensController->stop();

                m_stateTimer.start();
                m_pointOfNoReturnReached = false;  // Reset for this back-off cycle
                setState(ControlState::BACKING_OFF);

            } else {
                // Normal ramping up when not near edge threshold
                // Anti-escape is intentionally NOT used in edging mode - arousal
                // fluctuations are expected and part of the natural edging experience
                if (m_arousalLevel < m_edgeThreshold * 0.9) {
                    m_intensity = qMin(m_intensity + RAMP_RATE, MAX_INTENSITY);
                    m_frequency = qMin(m_frequency + FREQ_RAMP_RATE, MAX_FREQUENCY);
                }

                if (m_hardware) {
                    m_hardware->setSOL1(true);
                    m_hardware->setSOL2(false);
                }
                if (m_clitoralOscillator) {
                    if (!m_clitoralOscillator->isRunning()) m_clitoralOscillator->start();
                    m_clitoralOscillator->setFrequency(m_frequency);
                    m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
                }
                if (m_tensEnabled && m_tensController && !m_tensController->isRunning()) {
                    m_tensController->setFrequency(20.0);
                    m_tensController->setAmplitude(m_intensity * 60.0);
                    m_tensController->start();
                }
            }
            break;
        }

        case ControlState::BACKING_OFF: {
            emit backOffProgress(m_arousalLevel, m_recoveryThreshold);

            // ================================================================
            // POINT OF NO RETURN DETECTION
            // If arousal is RISING during back-off (stimulation stopped),
            // orgasm is inevitable - stop fighting it
            // ================================================================
            if (!m_pointOfNoReturnReached &&
                m_arousalLevel > m_previousArousal + PONR_AROUSAL_RISE_THRESHOLD) {
                // Arousal rising despite no stimulation = point of no return
                m_pointOfNoReturnReached = true;
                qDebug() << "Point of no return reached during back-off!"
                         << "arousal rising from" << m_previousArousal
                         << "to" << m_arousalLevel;
                emit pointOfNoReturnReached(m_edgeCount);

                // Resume gentle stimulation to support the inevitable orgasm
                if (m_clitoralOscillator) {
                    m_clitoralOscillator->setFrequency(HOLD_FREQUENCY);
                    m_clitoralOscillator->setAmplitude(HOLD_AMPLITUDE * MAX_CLITORAL_AMPLITUDE);
                    m_clitoralOscillator->start();
                }
                if (m_hardware) {
                    m_hardware->setSOL1(true);
                    m_hardware->setSOL2(false);  // Close vent, resume vacuum
                }
            }

            // Check for orgasm during back-off (after point of no return)
            if (m_pointOfNoReturnReached &&
                m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
                // Orgasm is happening
                m_inOrgasm = true;
                m_orgasmCount++;
                m_unexpectedOrgasmCount++;
                m_stateTimer.start();

                qDebug() << "Orgasm detected after point of no return";
                emit unexpectedOrgasmDuringEdging(m_orgasmCount, m_edgeCount);
                emit orgasmDetected(m_orgasmCount, m_sessionTimer.elapsed());

                if (m_fluidSensor && m_fluidTrackingEnabled) {
                    m_fluidSensor->recordOrgasmEvent(m_orgasmCount);
                }
                // Orgasm handling continues in the m_inOrgasm block at top
            }
            // Normal back-off completion (only if not in PONR state)
            else if (!m_pointOfNoReturnReached &&
                     m_arousalLevel < m_recoveryThreshold &&
                     m_stateTimer.elapsed() >= MIN_BACKOFF_MS) {
                setState(ControlState::HOLDING);
                m_stateTimer.start();

                if (m_clitoralOscillator) {
                    m_clitoralOscillator->setFrequency(HOLD_FREQUENCY);
                    m_clitoralOscillator->setAmplitude(HOLD_AMPLITUDE * MAX_CLITORAL_AMPLITUDE);
                    m_clitoralOscillator->start();
                }
            }
            break;
        }

        case ControlState::HOLDING: {
            if (m_stateTimer.elapsed() >= HOLD_DURATION_MS) {
                if (m_edgeCount >= m_targetEdges) {
                    if (m_mode == Mode::DENIAL) {
                        emit edgingComplete(m_edgeCount);
                        startCoolDown();
                    } else {
                        emit edgeCycleCompleted(m_edgeCount, m_targetEdges);
                        emit edgingComplete(m_edgeCount);

                        // Bug #8, #9, #10 fixes: Properly initialize state for FORCING transition
                        m_targetOrgasms = 1;
                        m_inOrgasm = false;                          // Bug #9: Reset orgasm detection state
                        m_tensAmplitude = FORCED_TENS_AMPLITUDE;     // Bug #10: Set TENS amplitude for forced mode
                        m_stateTimer.start();                        // Bug #8: Restart timer for FORCING state

                        setMode(Mode::FORCED_ORGASM);
                        setState(ControlState::FORCING);
                    }
                } else {
                    emit edgeCycleCompleted(m_edgeCount, m_targetEdges);
                    m_intensity = qMin(m_intensity + ESCALATION_RATE, MAX_INTENSITY * 0.8);
                    setState(ControlState::BUILDING);
                }
            }
            break;
        }
        default: break;
    }
    m_previousArousal = m_arousalLevel;
}

void OrgasmControlAlgorithm::runForcedOrgasm()
{
    if (m_state != ControlState::FORCING) return;

    if (m_sessionTimer.elapsed() >= m_maxDurationMs) {
        emit sessionTimeoutWarning();
        startCoolDown();
        return;
    }

    if (m_orgasmCount >= m_targetOrgasms) {
        emit forcedOrgasmComplete(m_orgasmCount, m_sessionTimer.elapsed());
        startCoolDown();
        return;
    }

    if (m_antiEscapeEnabled && m_arousalLevel < m_previousArousal - AROUSAL_DROP_THRESHOLD) {
        m_intensity = qMin(m_intensity + ANTI_ESCAPE_RATE, MAX_INTENSITY);
        m_frequency = qMin(m_frequency + ANTI_ESCAPE_FREQ_RATE, MAX_FREQUENCY);
        emit antiEscapeTriggered(m_intensity, m_frequency);
    }

    if (m_hardware) {
        m_hardware->setSOL1(true);
        m_hardware->setSOL2(false);
    }

    // Bug #3 fix: Ensure clitoral oscillator is started, not just configured
    if (m_clitoralOscillator) {
        if (!m_clitoralOscillator->isRunning()) {
            m_clitoralOscillator->start();
        }
        m_clitoralOscillator->setFrequency(m_frequency);
        m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
    }

    // Bug #4 fix: Start TENS controller in forced orgasm mode
    if (m_tensEnabled && m_tensController) {
        if (!m_tensController->isRunning()) {
            m_tensController->setFrequency(TENS_FORCED_FREQUENCY);
            m_tensController->setAmplitude(m_tensAmplitude * 100.0);  // Convert to percentage
            m_tensController->start();
        }
    }

    if (!m_inOrgasm && m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
        m_inOrgasm = true;
        m_stateTimer.start();
        m_orgasmCount++;
        emit orgasmDetected(m_orgasmCount, m_sessionTimer.elapsed());
        if (m_fluidSensor && m_fluidTrackingEnabled) {
            m_fluidSensor->recordOrgasmEvent(m_orgasmCount);
        }
        m_intensity = qMin(m_intensity + THROUGH_ORGASM_BOOST, MAX_INTENSITY);
    }

    if (m_inOrgasm && m_stateTimer.elapsed() > ORGASM_DURATION_MS) {
        if (m_stateTimer.elapsed() < ORGASM_DURATION_MS + POST_ORGASM_PAUSE_MS) {
            if (m_clitoralOscillator) {
                m_clitoralOscillator->setAmplitude(m_intensity * 0.7 * MAX_CLITORAL_AMPLITUDE);
            }
        } else {
            m_inOrgasm = false;
            if (m_clitoralOscillator) {
                m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
                m_frequency = qMin(m_frequency + 0.5, MAX_FREQUENCY);
                m_clitoralOscillator->setFrequency(m_frequency);
            }
        }
    }

    emit forcedOrgasmProgress(m_orgasmCount, m_targetOrgasms,
                               m_sessionTimer.elapsed(), m_maxDurationMs);
    m_previousArousal = m_arousalLevel;
}

void OrgasmControlAlgorithm::startCoolDown()
{
    setState(ControlState::COOLING_DOWN);
    qDebug() << "Starting cooldown for" << COOLDOWN_DURATION_MS << "ms";
    m_stateTimer.start();
    m_cooldownStartIntensity = m_intensity;
    m_cooldownStartFrequency = m_frequency;
}

// ============================================================================
// Safety
// ============================================================================

void OrgasmControlAlgorithm::performSafetyCheck()
{
    if (!m_hardware) return;

    // Bug #12 fix: Read and validate pressure values (units: mmHg)
    double avlPressure = m_hardware->readAVLPressure();
    double clitoralPressure = m_hardware->readClitoralPressure();

    // Bug #12 fix: Validate sensor readings before safety checks
    // Invalid readings (-1.0) indicate sensor failure - treat as emergency
    // MEDIUM-3 fix: Use class-level PRESSURE_MIN_VALID/PRESSURE_MAX_VALID constants
    if (avlPressure < PRESSURE_MIN_VALID || avlPressure > PRESSURE_MAX_VALID) {
        qWarning() << "AVL sensor failure detected:" << avlPressure << "mmHg - triggering emergency stop";
        emit sensorError("AVL", QString("Invalid reading: %1 mmHg").arg(avlPressure));
        handleEmergencyStop();
        return;
    }
    if (clitoralPressure < PRESSURE_MIN_VALID || clitoralPressure > PRESSURE_MAX_VALID) {
        qWarning() << "Clitoral sensor failure detected:" << clitoralPressure << "mmHg - triggering emergency stop";
        emit sensorError("Clitoral", QString("Invalid reading: %1 mmHg").arg(clitoralPressure));
        handleEmergencyStop();
        return;
    }

    // ========================================================================
    // Arousal-Adaptive Seal Integrity Detection
    // Differentiates between normal tissue swelling and actual vacuum leaks
    // ========================================================================

    // Bug #4 fix: Seal detection uses FIXED threshold for rapid leaks
    // Arousal-adaptive threshold is ONLY for gradual leak detection where swelling is expected
    // Rapid pressure drops indicate mechanical failure, not physiological swelling
    double fixedThreshold = SEAL_LOST_THRESHOLD;  // Never adjusted - physical seal requirement

    // Arousal-adaptive threshold for gradual leak detection only
    // During high arousal, vulva swells and displaces air, lowering AVL pressure
    double adaptiveThreshold = SEAL_LOST_THRESHOLD;
    if (m_arousalLevel > 0.0) {
        // At full arousal, threshold can be reduced by up to 50% for gradual detection
        adaptiveThreshold = SEAL_LOST_THRESHOLD * (1.0 - SEAL_AROUSAL_COMPENSATION_FACTOR * m_arousalLevel);
    }

    // Calculate rate of pressure change (mmHg per 100ms)
    double rateOfChange = 0.0;
    // Bug #5 fix: Skip rate calculation on first tick when m_previousAVLPressure is uninitialized
    // Bug fix: Use >= 0.0 since -1.0 is the sentinel value for "uninitialized"
    // This allows 0.0 to be a valid previous pressure reading
    bool hasValidPreviousPressure = (m_previousAVLPressure >= 0.0);
    if (hasValidPreviousPressure) {
        // Rate = (previous - current) / time_in_seconds
        // Positive rate means pressure is dropping
        rateOfChange = (m_previousAVLPressure - avlPressure) / (SAFETY_INTERVAL_MS / 1000.0);
    }

    // Check if clitoral pressure is rising (indicates tissue swelling, not leak)
    bool clitoralPressureRising = hasValidPreviousPressure &&
                                  (clitoralPressure > m_previousClitoralPressure + 0.5);

    // Determine if this is likely a seal leak vs tissue swelling
    // Seal leak criteria - use OR logic for multiple detection paths:
    //   Path 1 (Rapid leak): Rapid pressure drop - ALWAYS uses fixed threshold (mechanical failure)
    //   Path 2 (Gradual leak): Uses adaptive threshold (physiological changes considered)
    // Bug #4 fix: Rapid leak uses fixedThreshold, gradual leak uses adaptiveThreshold
    bool rapidSealLeak = hasValidPreviousPressure &&
                         (avlPressure < fixedThreshold) &&  // Bug #4: Use FIXED threshold for rapid leaks
                         (rateOfChange > RAPID_PRESSURE_DROP_THRESHOLD);

    // Gradual leak: pressure low, not explained by tissue swelling
    // Only trigger if we have baseline data AND no swelling indicators
    bool gradualSealLeak = hasValidPreviousPressure &&
                           (avlPressure < adaptiveThreshold) &&  // Adaptive threshold OK here
                           (!clitoralPressureRising) &&
                           (m_arousalLevel < 0.5);  // High arousal explains low AVL via swelling

    // Also trigger on very low absolute pressure regardless of rate
    // (device completely off or severely compromised seal) - uses FIXED threshold
    bool criticalPressureLoss = (avlPressure < fixedThreshold * 0.3);

    // Combined seal loss detection - any path triggers
    bool sealLossDetected = rapidSealLeak || gradualSealLeak || criticalPressureLoss;

    // ========================================================================
    // Bug #15, #16, #17 Fix: Progressive seal integrity response
    // ========================================================================
    if (sealLossDetected) {
        // Seal is weakening or lost (verified by multi-signal check)
        m_sealLossCount++;
        emit sealIntegrityWarning(avlPressure);

        if (criticalPressureLoss) {
            qWarning() << "Critical pressure loss detected:" << avlPressure << "mmHg";
        } else if (rapidSealLeak) {
            qDebug() << "Rapid seal leak detected - AVL:" << avlPressure
                     << "fixed threshold:" << fixedThreshold
                     << "rate:" << rateOfChange << "mmHg/100ms";
        } else {
            qDebug() << "Gradual seal leak detected - AVL:" << avlPressure
                     << "adaptive threshold:" << adaptiveThreshold
                     << "rate:" << rateOfChange << "mmHg/100ms";
        }

        // Bug #17: Emergency stop on persistent seal loss (SEAL_EMERGENCY_DURATION_MS)
        if (m_sealLossCount >= SEAL_EMERGENCY_THRESHOLD) {
            qWarning() << "Seal lost for" << SEAL_EMERGENCY_DURATION_MS
                       << "ms - triggering emergency stop";
            emit sealLostEmergencyStop();
            handleEmergencyStop();
            // Update previous pressures before returning
            m_previousAVLPressure = avlPressure;
            m_previousClitoralPressure = clitoralPressure;
            return;
        }

        // Bug #16: Progressive response based on failure count
        if (m_sealLossCount <= RESEAL_ATTEMPT_THRESHOLD) {
            // Phase 1 (1-3 failures): Attempt re-seal by boosting outer vacuum
            if (!m_resealAttemptInProgress) {
                m_resealAttemptInProgress = true;
                m_resealTimer.start();
                emit resealAttemptStarted();
                qDebug() << "Attempting re-seal: boosting outer vacuum";
            }

            // Bug #15: Increase outer chamber vacuum to pull device back onto body
            // MEDIUM-5 fix: Add null check for m_hardware
            if (m_resealTimer.elapsed() < RESEAL_BOOST_DURATION_MS && m_hardware) {
                m_hardware->setSOL1(true);   // Ensure vacuum is on
                m_hardware->setSOL2(false);  // Ensure vent is closed
                // Temporarily increase intensity to boost vacuum
                m_intensity = qMin(m_intensity + 0.05, MAX_INTENSITY);
            }
        } else if (m_sealLossCount <= RESEAL_INTENSITY_THRESHOLD) {
            // Phase 2 (4-6 failures): Re-seal failed, reduce intensity
            m_resealAttemptInProgress = false;
            m_intensity = qMax(m_intensity * 0.8, 0.1);
            qDebug() << "Re-seal failed, reducing intensity to" << m_intensity;
        }
        // Phase 3 (7+ failures) handled above by emergency stop

    } else if (avlPressure < adaptiveThreshold) {
        // Bug #5 fix: Pressure is low but didn't trigger sealLossDetected
        // This means high arousal (>0.5) is explaining the low pressure via tissue swelling
        // OR clitoral pressure is rising (swelling indicator)
        // Non-critical fix: Only log in verbose mode (high-frequency path)
        if (m_verboseLogging) {
            qDebug() << "Low AVL pressure (" << avlPressure << ") attributed to tissue swelling"
                     << "(arousal:" << m_arousalLevel
                     << ", clitoral rising:" << clitoralPressureRising
                     << ", adaptive threshold:" << adaptiveThreshold << ")";
        }
        // Bug #10 fix: DON'T reset seal loss counter when pressure is still low!
        // Only decrement gradually to prevent oscillation between leak/swelling states
        // This prevents a real leak from being masked by intermittent high arousal readings
        if (m_sealLossCount > 0) {
            m_sealLossCount--;  // Gradual decrease, not instant reset
        }
        // Keep reseal attempt status - don't reset until pressure is truly good
    } else {
        // Seal is good - pressure is ABOVE threshold, safe to reset tracking
        if (m_sealLossCount > 0) {
            qDebug() << "Seal re-established after" << m_sealLossCount << "failures";
        }
        m_sealLossCount = 0;
        m_resealAttemptInProgress = false;
    }

    // Update previous pressure values for next rate calculation
    m_previousAVLPressure = avlPressure;
    m_previousClitoralPressure = clitoralPressure;

    // Check overpressure
    if (clitoralPressure > MAX_SAFE_CLITORAL_PRESSURE) {
        emit overpressureWarning(clitoralPressure);
        handleEmergencyStop();
        return;
    }

    // Check session duration
    if (m_sessionTimer.elapsed() > MAX_SESSION_DURATION_MS) {
        emit sessionTimeoutWarning();
        startCoolDown();
        return;
    }

    // Track high-pressure duration for tissue protection
    if (m_intensity > 0.7) {
        m_highPressureDuration += SAFETY_INTERVAL_MS;

        if (m_highPressureDuration > MAX_HIGH_PRESSURE_DURATION_MS) {
            emit tissueProtectionTriggered();
            // Force a brief reduction
            m_intensity = qMax(m_intensity - 0.2, 0.3);
            m_highPressureDuration = 0;
        }
    } else {
        m_highPressureDuration = qMax(m_highPressureDuration - SAFETY_INTERVAL_MS, 0LL);
    }

    // Check TENS safety
    if (m_tensEnabled && m_tensController) {
        if (m_tensController->isFaultDetected()) {
            emit tensFault(m_tensController->getFaultReason());
            m_tensController->stop();
            m_tensEnabled = false;
        }
    }
}

void OrgasmControlAlgorithm::handleEmergencyStop()
{
    m_emergencyStop = true;

    qWarning() << "EMERGENCY STOP ACTIVATED";

    // Stop all timers
    m_updateTimer->stop();
    m_safetyTimer->stop();

    // Immediately stop all stimulation
    if (m_clitoralOscillator) {
        m_clitoralOscillator->emergencyStop();
    }

    if (m_tensController) {
        m_tensController->emergencyStop();
    }

    // Vent all chambers
    if (m_hardware) {
        m_hardware->emergencyStop();
    }

    setState(ControlState::ERROR);
    emit emergencyStopActivated();
}

// ============================================================================
// Utility Methods
// ============================================================================

void OrgasmControlAlgorithm::setState(ControlState state)
{
    // LOW-3 fix: Use atomic operations for thread-safe state access
    ControlState currentState = m_state.load(std::memory_order_acquire);
    if (currentState != state) {
        m_state.store(state, std::memory_order_release);
        emit stateChanged(state);
        // Non-critical fix: Only log state changes in verbose mode
        if (m_verboseLogging) {
            qDebug() << "State changed to:" << static_cast<int>(state);
        }
    }
}

void OrgasmControlAlgorithm::setMode(Mode mode)
{
    // LOW-3 fix: Use atomic operations for thread-safe mode access
    Mode currentMode = m_mode.load(std::memory_order_acquire);
    if (currentMode != mode) {
        m_mode.store(mode, std::memory_order_release);
        emit modeChanged(mode);
        // Non-critical fix: Only log mode changes in verbose mode
        if (m_verboseLogging) {
            qDebug() << "Mode changed to:" << static_cast<int>(mode);
        }
    }
}

double OrgasmControlAlgorithm::clamp(double value, double min, double max)
{
    return qBound(min, value, max);
}

// ============================================================================
// Milking Mode Implementation
// ============================================================================

void OrgasmControlAlgorithm::runMilking()
{
    // Check session timeout
    if (m_sessionTimer.elapsed() >= m_maxDurationMs) {
        qDebug() << "Milking session complete - duration reached";
        emit milkingSessionComplete(m_sessionTimer.elapsed(),
                                    m_milkingOrgasmCount == 0,
                                    m_dangerZoneEntries);
        startCoolDown();
        return;
    }

    switch (m_state) {
    case ControlState::BUILDING: {
        // Increase stimulation until reaching milking zone
        if (m_arousalLevel >= m_milkingZoneLower) {
            qDebug() << "Entered milking zone at arousal:" << m_arousalLevel;
            emit milkingZoneEntered(m_arousalLevel);
            m_stateTimer.start();
            setState(ControlState::MILKING);
        } else {
            // Gradual intensity increase to reach zone
            m_intensity = clamp(m_intensity + RAMP_RATE, MILKING_MIN_INTENSITY, MILKING_MAX_INTENSITY);

            // Apply stimulation
            if (m_clitoralOscillator) {
                m_clitoralOscillator->setFrequency(m_frequency);
                // Bug #13 fix: Use setAmplitude() not setIntensity() (method doesn't exist)
                m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
                if (!m_clitoralOscillator->isRunning()) {
                    m_clitoralOscillator->start();
                }
            }
            if (m_tensController && m_tensEnabled) {
                m_tensController->setAmplitude(m_tensAmplitude);
                m_tensController->setFrequency(15.0);
                if (!m_tensController->isRunning()) {
                    m_tensController->start();
                }
            }
        }
        break;
    }

    case ControlState::MILKING: {
        // Check for orgasm (FAILURE condition)
        if (m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
            qDebug() << "Unwanted orgasm detected during milking!";
            handleMilkingOrgasmFailure();
            return;
        }

        // Check for danger zone (arousal too high)
        if (m_arousalLevel >= m_dangerThreshold) {
            m_dangerZoneEntries++;
            qDebug() << "Danger zone entered, arousal:" << m_arousalLevel
                     << "entries:" << m_dangerZoneEntries;
            emit dangerZoneEntered(m_arousalLevel);
            m_stateTimer.start();
            setState(ControlState::DANGER_REDUCTION);
            return;
        }

        // Check if fell below zone (need more stimulation)
        if (m_arousalLevel < m_milkingZoneLower) {
            // Increase stimulation to get back in zone
            m_intensity = clamp(m_intensity + RAMP_RATE * 2, MILKING_MIN_INTENSITY, MILKING_MAX_INTENSITY);
        } else {
            // In zone - use PID control to maintain
            double adjustment = calculateMilkingIntensityAdjustment();
            m_intensity = clamp(m_intensity + adjustment, MILKING_MIN_INTENSITY, MILKING_MAX_INTENSITY);

            // Track time in zone
            m_milkingZoneTime += UPDATE_INTERVAL_MS;

            // Update running average
            m_milkingAvgArousal = (m_milkingAvgArousal * m_milkingAvgSamples + m_arousalLevel)
                                 / (m_milkingAvgSamples + 1);
            m_milkingAvgSamples++;

            // Periodic status report
            if (m_stateTimer.elapsed() >= MILKING_ZONE_REPORT_INTERVAL_MS) {
                emit milkingZoneMaintained(m_milkingZoneTime, m_milkingAvgArousal);
                m_stateTimer.start();
            }
        }

        // Apply stimulation
        if (m_clitoralOscillator) {
            // Bug #9 fix: Ensure oscillator is running before setting parameters
            // It may have been stopped during ORGASM_FAILURE recovery
            if (!m_clitoralOscillator->isRunning()) {
                m_clitoralOscillator->start();
            }
            // Bug #13 fix: Use setAmplitude() not setIntensity() (method doesn't exist)
            m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
            m_clitoralOscillator->setFrequency(m_frequency);
        }
        if (m_tensController && m_tensEnabled) {
            m_tensController->setAmplitude(m_tensAmplitude * m_intensity);
        }
        break;
    }

    case ControlState::DANGER_REDUCTION: {
        // Check for orgasm even during danger reduction
        if (m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
            qDebug() << "Unwanted orgasm during danger reduction!";
            handleMilkingOrgasmFailure();
            return;
        }

        // Significantly reduce stimulation
        m_intensity = MILKING_MIN_INTENSITY;
        if (m_clitoralOscillator) {
            // Bug #9 fix: Ensure oscillator is running (at reduced amplitude) during danger reduction
            // We want minimal stimulation, not zero - complete stop could cause seal issues
            if (!m_clitoralOscillator->isRunning()) {
                m_clitoralOscillator->start();
            }
            // Bug #13 fix: Use setAmplitude() not setIntensity() (method doesn't exist)
            m_clitoralOscillator->setAmplitude(m_intensity * 0.5 * MAX_CLITORAL_AMPLITUDE);  // Half of minimum
        }
        if (m_tensController && m_tensEnabled) {
            m_tensController->stop();
        }

        // Check if safe to return to milking
        if (m_arousalLevel < DANGER_RECOVERY_THRESHOLD) {
            qDebug() << "Exited danger zone at arousal:" << m_arousalLevel;
            emit dangerZoneExited(m_arousalLevel);

            // Reset PID integral to prevent overshoot
            m_milkingIntegralError = 0.0;

            m_stateTimer.start();
            setState(ControlState::MILKING);
        }
        break;
    }

    case ControlState::ORGASM_FAILURE: {
        // Handle based on failure mode - this state is used for recovery
        qint64 elapsed = m_stateTimer.elapsed();

        // Bug #9 fix: Validate m_milkingFailureMode before switch
        int validatedMode = qBound(0, m_milkingFailureMode, 3);
        if (validatedMode != m_milkingFailureMode) {
            qWarning() << "Invalid m_milkingFailureMode:" << m_milkingFailureMode
                       << "- using" << validatedMode;
            m_milkingFailureMode = validatedMode;
        }

        switch (m_milkingFailureMode) {
        case 1:  // Ruined orgasm - wait for orgasm to subside, then rebuild
            if (elapsed >= ORGASM_DURATION_MS) {
                qDebug() << "Ruined orgasm recovery complete, resuming";
                m_stateTimer.start();
                setState(ControlState::BUILDING);
            }
            break;

        case 2:  // Punishment - continue intense stimulation
            if (elapsed >= POST_UNEXPECTED_ORGASM_RECOVERY_MS) {
                qDebug() << "Punishment phase complete, resuming milking";
                m_intensity = MILKING_BASE_INTENSITY;
                m_stateTimer.start();
                setState(ControlState::BUILDING);
            } else {
                // Maintain intense stimulation during punishment
                if (m_clitoralOscillator) {
                    // Bug #13 fix: Use setAmplitude() not setIntensity() (method doesn't exist)
                    m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
                    m_clitoralOscillator->setFrequency(m_frequency * 1.2);
                }
            }
            break;

        case 3:  // Continue - brief pause then resume
            if (elapsed >= POST_ORGASM_PAUSE_MS) {
                qDebug() << "Brief pause complete, resuming milking";
                m_stateTimer.start();
                setState(ControlState::BUILDING);
            }
            break;

        case 0:  // Stop - should have been handled in handleMilkingOrgasmFailure
        default:
            startCoolDown();
            break;
        }
        break;
    }

    default:
        break;
    }
}

double OrgasmControlAlgorithm::calculateMilkingIntensityAdjustment()
{
    // Calculate error (positive = need more stimulation, negative = too much)
    double error = m_milkingTargetArousal - m_arousalLevel;

    // Proportional term
    double pTerm = MILKING_PID_KP * error;

    // Integral term (accumulated error)
    m_milkingIntegralError += error * (UPDATE_INTERVAL_MS / 1000.0);
    // Anti-windup: clamp integral to prevent runaway
    m_milkingIntegralError = clamp(m_milkingIntegralError, -0.5, 0.5);
    double iTerm = MILKING_PID_KI * m_milkingIntegralError;

    // Derivative term (rate of change of error)
    double dError = (error - m_milkingPreviousError) / (UPDATE_INTERVAL_MS / 1000.0);
    double dTerm = MILKING_PID_KD * dError;
    m_milkingPreviousError = error;

    // Combined adjustment
    double adjustment = pTerm + iTerm + dTerm;

    // Bug fix: Output clamping to prevent unstable oscillations
    // Limit maximum adjustment per cycle to prevent sudden intensity swings
    // Max adjustment of 0.05 per 50ms tick = 1.0/sec maximum rate of change
    static constexpr double MAX_ADJUSTMENT_PER_TICK = 0.05;
    adjustment = clamp(adjustment, -MAX_ADJUSTMENT_PER_TICK, MAX_ADJUSTMENT_PER_TICK);

    // Emit for debugging/UI
    emit milkingIntensityAdjusted(m_intensity + adjustment, error);

    return adjustment;
}

void OrgasmControlAlgorithm::handleMilkingOrgasmFailure()
{
    m_milkingOrgasmCount++;
    qint64 elapsed = m_sessionTimer.elapsed();

    qDebug() << "Milking failure: orgasm" << m_milkingOrgasmCount
             << "at" << elapsed << "ms, mode:" << m_milkingFailureMode;

    emit unwantedOrgasm(m_milkingOrgasmCount, elapsed);
    emit orgasmDetected(m_milkingOrgasmCount, elapsed);

    // Bug #9 fix: Validate m_milkingFailureMode before switch
    int validatedMode = qBound(0, m_milkingFailureMode, 3);
    if (validatedMode != m_milkingFailureMode) {
        qWarning() << "Invalid m_milkingFailureMode:" << m_milkingFailureMode
                   << "- using" << validatedMode;
        m_milkingFailureMode = validatedMode;
    }

    switch (m_milkingFailureMode) {
    case 0:  // Stop - end session immediately (but maintain seal!)
        if (m_clitoralOscillator) m_clitoralOscillator->stop();
        if (m_tensController) m_tensController->stop();
        // SAFETY: Only vent clitoral chamber (SOL5), NEVER outer chamber (SOL2)
        // Outer chamber maintains seal - venting SOL2 causes detachment
        if (m_hardware) m_hardware->setSOL5(true);  // Vent clitoral only
        emit milkingSessionComplete(elapsed, false, m_dangerZoneEntries);
        setState(ControlState::COOLING_DOWN);
        break;

    case 1:  // Ruined orgasm - stop AT orgasm onset to deny full pleasure
        if (m_clitoralOscillator) m_clitoralOscillator->stop();
        if (m_tensController) m_tensController->stop();
        // SAFETY: Only vent clitoral chamber, maintain outer seal
        if (m_hardware) m_hardware->setSOL5(true);
        m_stateTimer.start();
        setState(ControlState::ORGASM_FAILURE);
        break;

    case 2:  // Punishment - intensify through hypersensitivity
        m_intensity = clamp(m_intensity + THROUGH_ORGASM_BOOST * 2, 0.0, MAX_INTENSITY);
        if (m_clitoralOscillator) {
            // Bug #13 fix: Use setAmplitude() not setIntensity() (method doesn't exist)
            m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
            m_clitoralOscillator->setFrequency(m_frequency * 1.2);
        }
        m_stateTimer.start();
        setState(ControlState::ORGASM_FAILURE);
        break;

    case 3:  // Continue - log and resume after brief pause
        if (m_clitoralOscillator) m_clitoralOscillator->stop();
        // SAFETY: Only vent clitoral chamber, maintain outer seal
        if (m_hardware) m_hardware->setSOL5(true);
        m_stateTimer.start();
        setState(ControlState::ORGASM_FAILURE);
        break;

    default:
        startCoolDown();
        break;
    }
}

