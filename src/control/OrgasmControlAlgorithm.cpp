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
    , m_clitoralOscillator(nullptr)
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
{
    // Initialize history buffers
    m_pressureHistory.resize(HISTORY_SIZE, 0.0);
    m_arousalHistory.resize(HISTORY_SIZE, 0.0);

    // Connect timers
    connect(m_updateTimer, &QTimer::timeout, this, &OrgasmControlAlgorithm::onUpdateTick);
    connect(m_safetyTimer, &QTimer::timeout, this, &OrgasmControlAlgorithm::onSafetyCheck);

    // Connect fluid sensor signals if available
    if (m_fluidSensor && m_fluidSensor->isReady()) {
        m_fluidTrackingEnabled = true;
        connect(m_fluidSensor, &FluidSensor::volumeUpdated,
                this, [this](double current, double cumulative) {
            m_sessionFluidMl = cumulative;
            emit fluidVolumeUpdated(current, cumulative);
        });
        connect(m_fluidSensor, &FluidSensor::orgasmicBurstDetected,
                this, [this](double volumeMl, double /*peakRate*/, int orgasmNum) {
            m_orgasmicFluidMl += volumeMl;
            m_fluidPerOrgasm.append(volumeMl);
            emit fluidOrgasmBurst(volumeMl, orgasmNum);
        });
        connect(m_fluidSensor, &FluidSensor::lubricationRateChanged,
                this, &OrgasmControlAlgorithm::lubricationRateChanged);
        connect(m_fluidSensor, &FluidSensor::overflowWarning,
                this, [this](double volumeMl, double /*capacity*/) {
            emit fluidOverflowWarning(volumeMl);
        });
        qDebug() << "OrgasmControlAlgorithm: Fluid sensor connected";
    }

    qDebug() << "OrgasmControlAlgorithm initialized";
}

OrgasmControlAlgorithm::~OrgasmControlAlgorithm()
{
    stop();
}

// ============================================================================
// Control Methods
// ============================================================================

void OrgasmControlAlgorithm::startAdaptiveEdging(int targetCycles)
{
    QMutexLocker locker(&m_mutex);

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

    // Reset fluid tracking
    m_sessionFluidMl = 0.0;
    m_lubricationMl = 0.0;
    m_orgasmicFluidMl = 0.0;
    m_fluidPerOrgasm.clear();

    // Start fluid sensor session
    if (m_fluidSensor && m_fluidTrackingEnabled) {
        m_fluidSensor->startSession();
        m_fluidSensor->setCurrentArousalLevel(0.0);
    }

    setMode(Mode::ADAPTIVE_EDGING);
    setState(ControlState::CALIBRATING);

    // Start timers
    m_sessionTimer.start();
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_safetyTimer->start(SAFETY_INTERVAL_MS);

    qDebug() << "Started Adaptive Edging with target cycles:" << targetCycles;

    // Calibrate baseline (blocking for simplicity, could be async)
    locker.unlock();
    calibrateBaseline(BASELINE_DURATION_MS);
    locker.relock();

    setState(ControlState::BUILDING);
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

    // Reset fluid tracking
    m_sessionFluidMl = 0.0;
    m_lubricationMl = 0.0;
    m_orgasmicFluidMl = 0.0;
    m_fluidPerOrgasm.clear();

    // Start fluid sensor session
    if (m_fluidSensor && m_fluidTrackingEnabled) {
        m_fluidSensor->startSession();
        m_fluidSensor->setCurrentArousalLevel(0.0);
    }

    setMode(Mode::FORCED_ORGASM);
    setState(ControlState::CALIBRATING);

    m_sessionTimer.start();
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_safetyTimer->start(SAFETY_INTERVAL_MS);

    qDebug() << "Started Forced Orgasm with target:" << targetOrgasms
             << "max duration:" << maxDurationMs << "ms";

    locker.unlock();
    calibrateBaseline(BASELINE_DURATION_MS);
    locker.relock();

    setState(ControlState::FORCING);
    
    // Enable all stimulation
    if (m_clitoralOscillator) {
        m_clitoralOscillator->setFrequency(m_frequency);
        m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
        m_clitoralOscillator->start();
    }
    
    if (m_tensEnabled && m_tensController) {
        m_tensController->setFrequency(TENS_FORCED_FREQUENCY);
        m_tensController->setAmplitude(m_tensAmplitude * 100.0);
        m_tensController->start();
    }
}

void OrgasmControlAlgorithm::startDenial(int durationMs)
{
    QMutexLocker locker(&m_mutex);

    m_maxDurationMs = durationMs;
    m_edgeCount = 0;
    setMode(Mode::DENIAL);

    // Use edging logic but never allow release
    locker.unlock();
    startAdaptiveEdging(999);  // Very high target - duration is the limit
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
    handleEmergencyStop();
}

// ============================================================================
// Configuration
// ============================================================================

void OrgasmControlAlgorithm::setEdgeThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_edgeThreshold = clamp(threshold, 0.5, 0.95);
}

void OrgasmControlAlgorithm::setOrgasmThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_orgasmThreshold = clamp(threshold, 0.85, 1.0);
}

void OrgasmControlAlgorithm::setRecoveryThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_recoveryThreshold = clamp(threshold, 0.3, 0.8);
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

void OrgasmControlAlgorithm::setHeartRateSensor(HeartRateSensor* sensor)
{
    QMutexLocker locker(&m_mutex);
    m_heartRateSensor = sensor;

    if (sensor) {
        m_heartRateEnabled = true;

        // Connect to heart rate signals
        connect(sensor, &HeartRateSensor::heartRateUpdated,
                this, [this](int bpm) {
                    m_currentHeartRate = bpm;
                });

        connect(sensor, &HeartRateSensor::signalLost,
                this, [this]() {
                    emit heartRateSensorLost();
                    // Reduce HR weight when signal is lost
                    m_heartRateWeight = 0.0;
                });

        connect(sensor, &HeartRateSensor::signalRecovered,
                this, [this]() {
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
    if (m_emergencyStop) return;

    // Update arousal estimation
    updateArousalLevel();

    // Execute mode-specific logic
    switch (m_mode) {
        case Mode::ADAPTIVE_EDGING:
        case Mode::DENIAL:
            runAdaptiveEdging(m_targetEdges);
            break;
        case Mode::FORCED_ORGASM:
        case Mode::MULTI_ORGASM:
            runForcedOrgasm(m_targetOrgasms, m_maxDurationMs);
            break;
        default:
            break;
    }
}

void OrgasmControlAlgorithm::onSafetyCheck()
{
    performSafetyCheck();
}

// ============================================================================
// Arousal Detection
// ============================================================================

void OrgasmControlAlgorithm::calibrateBaseline(int durationMs)
{
    setState(ControlState::CALIBRATING);

    qDebug() << "Calibrating baseline for" << durationMs << "ms";

    double sumClitoral = 0.0;
    double sumAVL = 0.0;
    int samples = 0;

    QElapsedTimer calibTimer;
    calibTimer.start();

    while (calibTimer.elapsed() < durationMs && !m_emergencyStop) {
        if (m_hardware) {
            sumClitoral += m_hardware->readClitoralPressure();
            sumAVL += m_hardware->readAVLPressure();
            samples++;
        }
        QThread::msleep(50);  // 20 Hz sampling
    }

    if (samples > 0) {
        m_baselineClitoral = sumClitoral / samples;
        m_baselineAVL = sumAVL / samples;
    }

    // Initialize history with baseline
    std::fill(m_pressureHistory.begin(), m_pressureHistory.end(), m_baselineClitoral);
    m_historyIndex = 0;

    qDebug() << "Baseline calibrated: Clitoral=" << m_baselineClitoral
             << "mmHg, AVL=" << m_baselineAVL << "mmHg";
}

void OrgasmControlAlgorithm::updateArousalLevel()
{
    double newArousal = calculateArousalLevel();

    // Store in history
    m_arousalHistory[m_historyIndex] = newArousal;
    m_historyIndex = (m_historyIndex + 1) % HISTORY_SIZE;

    // Update state if changed
    ArousalState oldState = m_arousalState;
    updateArousalState();

    if (m_arousalState != oldState) {
        emit arousalStateChanged(m_arousalState);
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

double OrgasmControlAlgorithm::calculateArousalLevel()
{
    if (!m_hardware) return 0.0;

    // Read current pressure
    double currentClitoral = m_hardware->readClitoralPressure();
    double currentAVL = m_hardware->readAVLPressure();

    // Store in pressure history
    m_pressureHistory[m_historyIndex] = currentClitoral;

    // Feature 1: Baseline deviation (tissue engorgement)
    double baselineDeviation = 0.0;
    if (m_baselineClitoral > 0.1) {
        baselineDeviation = (currentClitoral - m_baselineClitoral) / m_baselineClitoral;
    }

    // Feature 2: Pressure variance (arousal fluctuations)
    double pressureVariance = calculateVariance(m_pressureHistory, VARIANCE_WINDOW_SAMPLES);

    // Feature 3: Contraction band power (0.8-1.2 Hz = orgasmic contractions)
    double contractionPower = calculateBandPower(m_pressureHistory, 0.8, 1.2);

    // Feature 4: Rate of change
    double rateOfChange = calculateDerivative(m_pressureHistory);

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

    // Calculate pressure-based arousal component
    double pressureArousal =
        WEIGHT_DEVIATION * normDeviation +
        WEIGHT_VARIANCE * normVariance +
        WEIGHT_CONTRACTION * normContraction +
        WEIGHT_ROC * normROC;

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

        // Weighted combination of HR features
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
    // Adjust weights dynamically based on whether HR is available
    double effectivePressureWeight = 1.0 - m_heartRateWeight;
    double arousal = effectivePressureWeight * pressureArousal +
                     m_heartRateWeight * heartRateArousal;

    // Apply seal integrity penalty
    arousal *= sealIntegrity;

    // Exponential smoothing
    m_smoothedArousal = AROUSAL_ALPHA * arousal + (1.0 - AROUSAL_ALPHA) * m_smoothedArousal;

    return clamp(m_smoothedArousal, 0.0, 1.0);
}

double OrgasmControlAlgorithm::calculateVariance(const QVector<double>& data, int windowSize)
{
    if (data.isEmpty() || windowSize <= 1) return 0.0;

    int startIdx = (m_historyIndex - windowSize + HISTORY_SIZE) % HISTORY_SIZE;
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
                                                   double freqLow, double freqHigh)
{
    // Simplified band power using autocorrelation at target period
    // For 0.8-1.2 Hz at 10 Hz sampling, check lags 8-12 samples

    if (data.size() < 20) return 0.0;

    int lagLow = static_cast<int>(10.0 / freqHigh);   // ~8 samples for 1.2 Hz
    int lagHigh = static_cast<int>(10.0 / freqLow);  // ~12 samples for 0.8 Hz

    double maxCorrelation = 0.0;

    for (int lag = lagLow; lag <= lagHigh; ++lag) {
        double correlation = 0.0;
        int count = 0;

        for (int i = 0; i < HISTORY_SIZE - lag; ++i) {
            int idx1 = (m_historyIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
            int idx2 = (m_historyIndex - i - lag + HISTORY_SIZE) % HISTORY_SIZE;
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

double OrgasmControlAlgorithm::calculateDerivative(const QVector<double>& data)
{
    if (data.size() < 5) return 0.0;

    // Simple 5-point derivative
    double sum = 0.0;
    for (int i = 1; i <= 4; ++i) {
        int idx1 = (m_historyIndex - i + 1 + HISTORY_SIZE) % HISTORY_SIZE;
        int idx2 = (m_historyIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
        sum += data[idx1] - data[idx2];
    }

    // Convert to rate per second (10 Hz sampling = 0.1s per sample)
    return sum / 4.0 * 10.0;
}

bool OrgasmControlAlgorithm::detectContractions()
{
    // Check for rhythmic contractions in 0.8-1.2 Hz band
    double power = calculateBandPower(m_pressureHistory, 0.8, 1.2);
    return power > MAX_CONTRACTION_POWER * 0.5;  // Threshold at 50% of max
}

void OrgasmControlAlgorithm::updateArousalState()
{
    if (m_arousalLevel < 0.2) {
        m_arousalState = ArousalState::BASELINE;
    } else if (m_arousalLevel < 0.5) {
        m_arousalState = ArousalState::WARMING;
    } else if (m_arousalLevel < m_edgeThreshold) {
        m_arousalState = ArousalState::PLATEAU;
    } else if (m_arousalLevel < m_orgasmThreshold) {
        m_arousalState = ArousalState::PRE_ORGASM;
    } else {
        m_arousalState = ArousalState::ORGASM;
    }
}

// ============================================================================
// Algorithm Execution
// ============================================================================

void OrgasmControlAlgorithm::runAdaptiveEdging(int targetCycles)
{
    Q_UNUSED(targetCycles);

    static double previousArousal = 0.0;
    static QElapsedTimer backoffTimer;
    static bool inBackoff = false;

    switch (m_state) {
        case ControlState::BUILDING: {
            emit buildUpProgress(m_arousalLevel, m_edgeThreshold);

            if (m_arousalLevel >= m_edgeThreshold) {
                // EDGE DETECTED - back off!
                m_edgeCount++;
                emit edgeDetected(m_edgeCount, m_intensity);

                // Stop clitoral oscillation
                if (m_clitoralOscillator) {
                    m_clitoralOscillator->stop();
                }

                // Reduce outer chamber pressure
                if (m_hardware) {
                    m_hardware->setSOL2(true);  // Vent
                }

                // Stop TENS during backoff
                if (m_tensController) {
                    m_tensController->stop();
                }

                backoffTimer.start();
                inBackoff = true;
                setState(ControlState::BACKING_OFF);

            } else {
                // Continue ramping up
                if (m_arousalLevel < m_edgeThreshold * 0.9) {
                    m_intensity = qMin(m_intensity + RAMP_RATE, MAX_INTENSITY);
                    m_frequency = qMin(m_frequency + FREQ_RAMP_RATE, MAX_FREQUENCY);
                }

                // Apply stimulation
                if (m_hardware) {
                    // Outer chamber sustained vacuum
                    m_hardware->setSOL1(true);   // Vacuum on
                    m_hardware->setSOL2(false);  // Vent closed
                }

                if (m_clitoralOscillator) {
                    if (!m_clitoralOscillator->isRunning()) {
                        m_clitoralOscillator->start();
                    }
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

            if (m_arousalLevel < m_recoveryThreshold &&
                backoffTimer.elapsed() >= MIN_BACKOFF_MS) {
                // Recovery complete - enter hold phase
                setState(ControlState::HOLDING);

                // Light stimulation during hold
                if (m_clitoralOscillator) {
                    m_clitoralOscillator->setFrequency(HOLD_FREQUENCY);
                    m_clitoralOscillator->setAmplitude(HOLD_AMPLITUDE * MAX_CLITORAL_AMPLITUDE);
                    m_clitoralOscillator->start();
                }
            }
            break;
        }

        case ControlState::HOLDING: {
            static QElapsedTimer holdTimer;
            static bool holdStarted = false;

            if (!holdStarted) {
                holdTimer.start();
                holdStarted = true;
            }

            if (holdTimer.elapsed() >= HOLD_DURATION_MS) {
                holdStarted = false;

                // Check if we've reached target edges
                if (m_edgeCount >= m_targetEdges) {
                    if (m_mode == Mode::DENIAL) {
                        // Denial mode: just stop
                        emit edgingComplete(m_edgeCount);
                        runCoolDown(COOLDOWN_DURATION_MS);
                    } else {
                        // Allow orgasm
                        emit edgeCycleCompleted(m_edgeCount, m_targetEdges);
                        emit edgingComplete(m_edgeCount);

                        // Switch to forcing mode for release
                        m_targetOrgasms = 1;
                        setMode(Mode::FORCED_ORGASM);
                        setState(ControlState::FORCING);
                    }
                } else {
                    // More cycles to go
                    emit edgeCycleCompleted(m_edgeCount, m_targetEdges);

                    // Escalate base intensity for next cycle
                    m_intensity = qMin(m_intensity + ESCALATION_RATE, MAX_INTENSITY * 0.8);

                    setState(ControlState::BUILDING);
                }
            }
            break;
        }

        default:
            break;
    }

    previousArousal = m_arousalLevel;
}

void OrgasmControlAlgorithm::runForcedOrgasm(int targetOrgasms, int maxDurationMs)
{
    Q_UNUSED(targetOrgasms);
    Q_UNUSED(maxDurationMs);

    static double previousArousal = 0.0;
    static bool inOrgasm = false;
    static QElapsedTimer orgasmTimer;

    if (m_state != ControlState::FORCING) return;

    // Check session timeout
    if (m_sessionTimer.elapsed() >= m_maxDurationMs) {
        emit sessionTimeoutWarning();
        runCoolDown(COOLDOWN_DURATION_MS);
        return;
    }

    // Check target reached
    if (m_orgasmCount >= m_targetOrgasms) {
        emit forcedOrgasmComplete(m_orgasmCount, m_sessionTimer.elapsed());
        runCoolDown(COOLDOWN_DURATION_MS);
        return;
    }

    // ANTI-ESCAPE: If arousal dropping, escalate
    if (m_antiEscapeEnabled && m_arousalLevel < previousArousal - AROUSAL_DROP_THRESHOLD) {
        m_intensity = qMin(m_intensity + ANTI_ESCAPE_RATE, MAX_INTENSITY);
        m_frequency = qMin(m_frequency + ANTI_ESCAPE_FREQ_RATE, MAX_FREQUENCY);

        emit antiEscapeTriggered(m_intensity, m_frequency);
    }

    // Apply maximum stimulation
    if (m_hardware) {
        m_hardware->setSOL1(true);
        m_hardware->setSOL2(false);
    }

    if (m_clitoralOscillator) {
        m_clitoralOscillator->setFrequency(m_frequency);
        m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
    }

    // ORGASM DETECTION
    if (!inOrgasm && m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
        inOrgasm = true;
        orgasmTimer.start();
        m_orgasmCount++;

        emit orgasmDetected(m_orgasmCount, m_sessionTimer.elapsed());

        // Notify fluid sensor of orgasm event for correlation
        if (m_fluidSensor && m_fluidTrackingEnabled) {
            m_fluidSensor->recordOrgasmEvent(m_orgasmCount);
        }

        // BOOST through orgasm (do NOT back off)
        m_intensity = qMin(m_intensity + THROUGH_ORGASM_BOOST, MAX_INTENSITY);
    }

    // Post-orgasm handling
    if (inOrgasm && orgasmTimer.elapsed() > ORGASM_DURATION_MS) {
        if (orgasmTimer.elapsed() < ORGASM_DURATION_MS + POST_ORGASM_PAUSE_MS) {
            // Brief amplitude reduction
            if (m_clitoralOscillator) {
                m_clitoralOscillator->setAmplitude(m_intensity * 0.7 * MAX_CLITORAL_AMPLITUDE);
            }
        } else {
            // Resume full intensity
            inOrgasm = false;
            if (m_clitoralOscillator) {
                m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
                m_frequency = qMin(m_frequency + 0.5, MAX_FREQUENCY);
                m_clitoralOscillator->setFrequency(m_frequency);
            }
        }
    }

    emit forcedOrgasmProgress(m_orgasmCount, m_targetOrgasms,
                               m_sessionTimer.elapsed(), m_maxDurationMs);

    previousArousal = m_arousalLevel;
}

void OrgasmControlAlgorithm::runCoolDown(int durationMs)
{
    setState(ControlState::COOLING_DOWN);

    qDebug() << "Starting cooldown for" << durationMs << "ms";

    QElapsedTimer cooldownTimer;
    cooldownTimer.start();

    double startIntensity = m_intensity;
    double startFrequency = m_frequency;

    while (cooldownTimer.elapsed() < durationMs && !m_emergencyStop) {
        double progress = static_cast<double>(cooldownTimer.elapsed()) / durationMs;

        m_intensity = startIntensity * (1.0 - progress);
        m_frequency = qMax(startFrequency * (1.0 - progress), 3.0);

        if (m_clitoralOscillator) {
            m_clitoralOscillator->setFrequency(m_frequency);
            m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
        }

        if (m_tensController) {
            m_tensController->setAmplitude(m_intensity * 50.0);
        }

        QThread::msleep(100);
    }

    stop();
}

// ============================================================================
// Safety
// ============================================================================

void OrgasmControlAlgorithm::performSafetyCheck()
{
    if (!m_hardware) return;

    double avlPressure = m_hardware->readAVLPressure();
    double clitoralPressure = m_hardware->readClitoralPressure();

    // Check seal integrity
    if (avlPressure < SEAL_LOST_THRESHOLD) {
        emit sealIntegrityWarning(avlPressure);
        // Don't emergency stop, but reduce intensity
        m_intensity = qMax(m_intensity * 0.8, 0.1);
    }

    // Check overpressure
    if (clitoralPressure > MAX_SAFE_CLITORAL_PRESSURE) {
        emit overpressureWarning(clitoralPressure);
        handleEmergencyStop();
        return;
    }

    // Check session duration
    if (m_sessionTimer.elapsed() > MAX_SESSION_DURATION_MS) {
        emit sessionTimeoutWarning();
        runCoolDown(COOLDOWN_DURATION_MS);
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
        if (m_tensController->hasError()) {
            emit tensFault(m_tensController->getErrorMessage());
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
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
        qDebug() << "State changed to:" << static_cast<int>(m_state);
    }
}

void OrgasmControlAlgorithm::setMode(Mode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged(m_mode);
        qDebug() << "Mode changed to:" << static_cast<int>(m_mode);
    }
}

double OrgasmControlAlgorithm::clamp(double value, double min, double max)
{
    return qBound(min, value, max);
}

