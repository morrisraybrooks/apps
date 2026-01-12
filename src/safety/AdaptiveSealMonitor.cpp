#include "AdaptiveSealMonitor.h"
#include "SafetyConstants.h"
#include "EmergencyStopCoordinator.h"
#include "../logging/ISafetyLogger.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <QJsonObject>
#include <cmath>
#include <algorithm>

AdaptiveSealMonitor::AdaptiveSealMonitor(HardwareManager* hardware, QObject* parent)
    : QObject(parent)
    , StatefulComponent<int>(SEALED, "AdaptiveSealMonitor")
    , m_hardware(hardware)
    , m_emergencyStopCoordinator(nullptr)
    , m_safetyLogger(nullptr)
    , m_active(false)
    , m_monitoring(false)
    , m_paused(false)
    , m_baselineThreshold(SafetyConstants::DEFAULT_DETACHMENT_THRESHOLD_MMHG)
    , m_hysteresis(SafetyConstants::DEFAULT_HYSTERESIS_MMHG)
    , m_monitoringRateHz(SafetyConstants::ANTI_DETACHMENT_MONITORING_RATE_HZ)
    , m_phasePriority(NORMAL)
    , m_arousalLevel(0.0)
    , m_clitoralPressure(0.0)
    , m_previousClitoralPressure(0.0)
    , m_clitoralPressureRising(false)
    , m_currentAVLPressure(0.0)
    , m_previousAVLPressure(0.0)
    , m_pressureRateOfChange(0.0)
    , m_lastReadingTime(0)
    , m_resealAttemptCount(0)
    , m_currentResealPower(0.0)
    , m_resealTimer(new QTimer(this))
    , m_lastLeakPath(NONE)
    , m_monitoringTimer(new QTimer(this))
    , m_totalLeakEvents(0)
    , m_successfulReseals(0)
    , m_failedReseals(0)
    , m_consecutiveErrors(0)
{
    // Set up monitoring timer for high-frequency monitoring
    m_monitoringTimer->setInterval(1000 / m_monitoringRateHz);
    m_monitoringTimer->setTimerType(Qt::PreciseTimer);
    connect(m_monitoringTimer, &QTimer::timeout, this, &AdaptiveSealMonitor::performMonitoringCycle);

    // Set up reseal timer
    m_resealTimer->setSingleShot(true);
    connect(m_resealTimer, &QTimer::timeout, this, &AdaptiveSealMonitor::onResealTimeout);

    // Register state transition callback
    registerTransitionCallback([this](int oldState, int newState) {
        onStateTransition(oldState, newState);
    });

    qDebug() << "AdaptiveSealMonitor initialized with baseline threshold:" << m_baselineThreshold << "mmHg";
}

AdaptiveSealMonitor::~AdaptiveSealMonitor()
{
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->unregisterHandler("AdaptiveSealMonitor");
    }
    shutdown();
}

bool AdaptiveSealMonitor::initialize()
{
    if (!m_hardware) {
        m_lastError = "Hardware manager not available";
        qCritical() << m_lastError;
        return false;
    }

    if (!m_hardware->isReady()) {
        m_lastError = "Hardware not ready";
        qCritical() << m_lastError;
        return false;
    }

    try {
        // Verify AVL pressure reading
        double testPressure = m_hardware->readAVLPressure();
        if (!SafetyConstants::isValidPressure(testPressure)) {
            throw std::runtime_error("Invalid AVL pressure reading during initialization");
        }

        setState(SEALED);
        m_active = true;

        qDebug() << "AdaptiveSealMonitor initialized successfully";
        return true;

    } catch (const std::exception& e) {
        m_lastError = QString("Initialization failed: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void AdaptiveSealMonitor::shutdown()
{
    if (!m_active) return;

    qDebug() << "Shutting down AdaptiveSealMonitor...";
    stopMonitoring();
    m_active = false;
    qDebug() << "AdaptiveSealMonitor shutdown complete";
}

void AdaptiveSealMonitor::startMonitoring()
{
    if (!m_active) {
        qWarning() << "Cannot start monitoring: System not initialized";
        return;
    }

    if (m_monitoring) {
        qWarning() << "Monitoring already active";
        return;
    }

    m_monitoring = true;
    m_paused = false;
    m_consecutiveErrors = 0;
    m_resealAttemptCount = 0;

    m_monitoringTimer->start();
    qDebug() << QString("AdaptiveSealMonitor started at %1 Hz").arg(m_monitoringRateHz);
}

void AdaptiveSealMonitor::stopMonitoring()
{
    if (!m_monitoring) return;

    m_monitoringTimer->stop();
    m_resealTimer->stop();
    m_monitoring = false;
    m_paused = false;

    setState(SEALED);
    qDebug() << "AdaptiveSealMonitor stopped";
}

void AdaptiveSealMonitor::pauseMonitoring()
{
    if (m_monitoring && !m_paused) {
        m_paused = true;
        m_monitoringTimer->stop();
        qDebug() << "AdaptiveSealMonitor paused";
    }
}

void AdaptiveSealMonitor::resumeMonitoring()
{
    if (m_monitoring && m_paused) {
        m_paused = false;
        m_monitoringTimer->start();
        qDebug() << "AdaptiveSealMonitor resumed";
    }
}

// ============================================================================
// Arousal Integration
// ============================================================================

void AdaptiveSealMonitor::setArousalLevel(double arousalLevel)
{
    m_arousalLevel = std::clamp(arousalLevel, 0.0, 1.0);
}

void AdaptiveSealMonitor::setClitoralPressure(double pressureMmHg)
{
    m_previousClitoralPressure = m_clitoralPressure;
    m_clitoralPressure = pressureMmHg;
    m_clitoralPressureRising = (m_clitoralPressure > m_previousClitoralPressure);
}

// ============================================================================
// Phase Priority
// ============================================================================

void AdaptiveSealMonitor::setPhasePriority(PhasePriority priority)
{
    if (m_phasePriority != priority) {
        m_phasePriority = priority;

        // Adjust monitoring rate based on priority
        switch (priority) {
        case NORMAL:
            m_monitoringRateHz = SafetyConstants::ANTI_DETACHMENT_MONITORING_RATE_HZ;
            break;
        case ELEVATED:
            m_monitoringRateHz = 150;
            break;
        case CRITICAL:
            m_monitoringRateHz = SafetyConstants::CRITICAL_PHASE_MONITORING_RATE_HZ;
            break;
        }

        if (m_monitoring && !m_paused) {
            m_monitoringTimer->setInterval(1000 / m_monitoringRateHz);
        }

        emit phasePriorityChanged(priority);
        qDebug() << "Phase priority changed to" << priority << "- monitoring at" << m_monitoringRateHz << "Hz";
    }
}

// ============================================================================
// Threshold Configuration
// ============================================================================

void AdaptiveSealMonitor::setBaselineThreshold(double thresholdMmHg)
{
    if (thresholdMmHg > SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG &&
        thresholdMmHg < SafetyConstants::MAX_VALID_PRESSURE) {
        m_baselineThreshold = thresholdMmHg;
        qDebug() << "Baseline threshold set to" << thresholdMmHg << "mmHg";
    }
}

void AdaptiveSealMonitor::setHysteresis(double hysteresisMmHg)
{
    if (hysteresisMmHg >= 0 && hysteresisMmHg < 20.0) {
        m_hysteresis = hysteresisMmHg;
        qDebug() << "Hysteresis set to" << hysteresisMmHg << "mmHg";
    }
}

double AdaptiveSealMonitor::getAdaptiveThreshold() const
{
    return calculateAdaptiveThreshold();
}

// ============================================================================
// Statistics
// ============================================================================

void AdaptiveSealMonitor::resetStatistics()
{
    m_totalLeakEvents = 0;
    m_successfulReseals = 0;
    m_failedReseals = 0;
}

// ============================================================================
// Integration Setters
// ============================================================================

void AdaptiveSealMonitor::setEmergencyStopCoordinator(EmergencyStopCoordinator* coordinator)
{
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->unregisterHandler("AdaptiveSealMonitor");
    }

    m_emergencyStopCoordinator = coordinator;

    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->registerHandler(
            "AdaptiveSealMonitor",
            EmergencyStopCoordinator::PRIORITY_HIGH,
            [this](const QString& reason) {
                onEmergencyStopTriggered(reason);
            }
        );
        qDebug() << "EmergencyStopCoordinator linked to AdaptiveSealMonitor";
    }
}

void AdaptiveSealMonitor::setSafetyLogger(ISafetyLogger* logger)
{
    m_safetyLogger = logger;
    qDebug() << "ISafetyLogger linked to AdaptiveSealMonitor";
}

// ============================================================================
// Core Monitoring Logic
// ============================================================================

void AdaptiveSealMonitor::performMonitoringCycle()
{
    if (!m_active || !m_monitoring || m_paused) return;

    try {
        double avlPressure = m_hardware->readAVLPressure();

        if (!SafetyConstants::isValidPressure(avlPressure)) {
            m_consecutiveErrors++;
            if (m_consecutiveErrors >= SafetyConstants::MAX_CONSECUTIVE_ERRORS) {
                emit systemError("Too many consecutive invalid pressure readings");
                setState(SYSTEM_ERROR);
            }
            return;
        }

        m_consecutiveErrors = 0;
        processAVLReading(avlPressure);

    } catch (const std::exception& e) {
        m_consecutiveErrors++;
        m_lastError = QString("Monitoring cycle error: %1").arg(e.what());

        if (m_consecutiveErrors >= SafetyConstants::MAX_CONSECUTIVE_ERRORS) {
            emit systemError(m_lastError);
            setState(SYSTEM_ERROR);
        }
    }
}

void AdaptiveSealMonitor::processAVLReading(double avlPressure)
{
    m_previousAVLPressure = m_currentAVLPressure;
    m_currentAVLPressure = avlPressure;
    m_lastReadingTime = QDateTime::currentMSecsSinceEpoch();

    // Update pressure history
    m_pressureHistory.enqueue(avlPressure);
    while (m_pressureHistory.size() > PRESSURE_HISTORY_SIZE) {
        m_pressureHistory.dequeue();
    }

    // Calculate rate of change
    m_pressureRateOfChange = calculatePressureRateOfChange();

    // Get current state
    SealState currentState = static_cast<SealState>(getState());

    // Skip leak detection during reseal attempts
    if (currentState == RESEALING) {
        return;
    }

    // Multi-path leak detection
    LeakPath detectedLeak = detectLeak(avlPressure);

    if (detectedLeak != NONE && currentState != LEAK_DETECTED && currentState != SEAL_LOST) {
        m_lastLeakPath = detectedLeak;
        m_totalLeakEvents++;

        logEvent(QString("Leak detected via path %1").arg(static_cast<int>(detectedLeak)), avlPressure);
        emit leakDetected(detectedLeak, avlPressure, getAdaptiveThreshold());

        // Emit specific signals based on leak path
        switch (detectedLeak) {
        case RAPID_MECHANICAL:
            emit rapidLeakDetected(avlPressure, m_pressureRateOfChange);
            break;
        case GRADUAL_LEAK:
            emit gradualLeakDetected(avlPressure, getAdaptiveThreshold());
            break;
        case CRITICAL_LOSS:
            emit criticalPressureLoss(avlPressure);
            break;
        default:
            break;
        }

        setState(LEAK_DETECTED);
        startResealSequence();
        return;
    }

    // Check for warning state
    double adaptiveThreshold = getAdaptiveThreshold();
    double warningThreshold = adaptiveThreshold + m_hysteresis * 2;

    if (currentState == SEALED && avlPressure < warningThreshold && avlPressure >= adaptiveThreshold) {
        setState(WARNING);
        emit sealWarning(avlPressure, adaptiveThreshold);
    } else if (currentState == WARNING && avlPressure >= warningThreshold + m_hysteresis) {
        setState(SEALED);
        emit sealIntact();
    }
}

double AdaptiveSealMonitor::calculateAdaptiveThreshold() const
{
    // Only apply arousal compensation above minimum arousal level
    if (m_arousalLevel < SafetyConstants::ADAPTIVE_THRESHOLD_MIN_AROUSAL) {
        return m_baselineThreshold;
    }

    // Formula: threshold = baseline * (1 - arousal * AROUSAL_COMPENSATION_FACTOR)
    // At max arousal (1.0), threshold is reduced by AROUSAL_COMPENSATION_FACTOR (50%)
    double compensationFactor = m_arousalLevel * SafetyConstants::AROUSAL_COMPENSATION_FACTOR;
    double adaptiveThreshold = m_baselineThreshold * (1.0 - compensationFactor);

    // Never go below critical threshold
    return std::max(adaptiveThreshold, SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG + m_hysteresis);
}

double AdaptiveSealMonitor::calculatePressureRateOfChange()
{
    if (m_pressureHistory.size() < 2) {
        return 0.0;
    }

    // Calculate rate over last few samples (100ms window at 100Hz = 10 samples)
    int sampleCount = std::min(10, m_pressureHistory.size());
    double oldest = m_pressureHistory.at(m_pressureHistory.size() - sampleCount);
    double newest = m_pressureHistory.last();

    // Rate in mmHg per 100ms
    double timeWindowMs = (sampleCount - 1) * (1000.0 / m_monitoringRateHz);
    return (oldest - newest) / (timeWindowMs / 100.0);  // Normalize to per 100ms
}

// ============================================================================
// Multi-Path Leak Detection
// ============================================================================

AdaptiveSealMonitor::LeakPath AdaptiveSealMonitor::detectLeak(double avlPressure)
{
    // Path 3: Critical loss - absolute minimum regardless of state
    if (isCriticalLoss(avlPressure)) {
        return CRITICAL_LOSS;
    }

    // Path 1: Rapid mechanical failure - rate of change exceeds threshold
    if (isRapidLeak(avlPressure, m_pressureRateOfChange)) {
        return RAPID_MECHANICAL;
    }

    // Path 2: Gradual leak - below adaptive threshold without physiological explanation
    double adaptiveThreshold = getAdaptiveThreshold();
    if (isGradualLeak(avlPressure, adaptiveThreshold)) {
        return GRADUAL_LEAK;
    }

    return NONE;
}

bool AdaptiveSealMonitor::isRapidLeak(double avlPressure, double rateOfChange)
{
    // Rapid pressure drop indicates mechanical failure, not physiology
    // Physiology changes slowly; mechanical failures are sudden
    return rateOfChange > SafetyConstants::RAPID_LEAK_RATE_THRESHOLD;
}

bool AdaptiveSealMonitor::isGradualLeak(double avlPressure, double adaptiveThreshold)
{
    // Below adaptive threshold AND clitoral pressure is not rising
    // (rising clitoral pressure indicates swelling which explains AVL pressure drop)
    if (avlPressure < adaptiveThreshold) {
        // If clitoral pressure is rising, tissue swelling may explain the drop
        if (m_clitoralPressureRising && m_arousalLevel > SafetyConstants::ADAPTIVE_THRESHOLD_MIN_AROUSAL) {
            // Swelling detected - this is physiological, not a leak
            return false;
        }
        return true;
    }
    return false;
}

bool AdaptiveSealMonitor::isCriticalLoss(double avlPressure)
{
    // Below absolute minimum - always a leak regardless of physiological state
    return avlPressure < SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG;
}

// ============================================================================
// Progressive Reseal Recovery
// ============================================================================

void AdaptiveSealMonitor::startResealSequence()
{
    m_resealAttemptCount = 0;
    m_resealElapsed.start();

    setState(RESEALING);
    executeResealAttempt();
}

void AdaptiveSealMonitor::executeResealAttempt()
{
    m_resealAttemptCount++;

    if (m_resealAttemptCount > SafetyConstants::MAX_RESEAL_ATTEMPTS) {
        handleResealFailure();
        return;
    }

    // Calculate pump power for this attempt
    // Attempt 1: 50%, Attempt 2: 70%, Attempt 3: 90%
    m_currentResealPower = SafetyConstants::RESEAL_BASE_PUMP_POWER +
                           (m_resealAttemptCount * SafetyConstants::RESEAL_POWER_INCREMENT);
    m_currentResealPower = std::min(m_currentResealPower, SafetyConstants::RESEAL_MAX_PUMP_POWER);

    logEvent(QString("Reseal attempt %1 at %2% power")
             .arg(m_resealAttemptCount)
             .arg(m_currentResealPower * 100), m_currentAVLPressure);

    emit resealAttemptStarted(m_resealAttemptCount, m_currentResealPower);

    // Close vent and apply pump power
    closeVent();
    setPumpPower(m_currentResealPower);

    // Start timer for this attempt
    m_resealTimer->start(SafetyConstants::RESEAL_ATTEMPT_DURATION_MS);
}

void AdaptiveSealMonitor::onResealTimeout()
{
    // Check if reseal was successful
    if (checkResealSuccess()) {
        handleResealSuccess();
    } else {
        // Try next attempt
        executeResealAttempt();
    }
}

bool AdaptiveSealMonitor::checkResealSuccess()
{
    double currentPressure = m_hardware->readAVLPressure();
    double adaptiveThreshold = getAdaptiveThreshold();

    // Success if pressure is above adaptive threshold with hysteresis margin
    return currentPressure >= (adaptiveThreshold + m_hysteresis);
}

void AdaptiveSealMonitor::handleResealSuccess()
{
    m_successfulReseals++;

    logEvent(QString("Reseal successful after %1 attempts").arg(m_resealAttemptCount), m_currentAVLPressure);
    emit resealSuccessful(m_resealAttemptCount);

    // Reduce pump power gradually
    setPumpPower(0.5);  // Return to normal operation power

    // Brief cooldown before resuming normal monitoring
    QTimer::singleShot(SafetyConstants::RESEAL_COOLDOWN_MS, this, [this]() {
        setState(SEALED);
        emit sealIntact();
    });
}

void AdaptiveSealMonitor::handleResealFailure()
{
    m_failedReseals++;

    logEvent("All reseal attempts failed", m_currentAVLPressure);
    emit resealFailed(m_resealAttemptCount);
    emit allResealsFailed();

    setState(SEAL_LOST);
    terminateSession("Seal integrity could not be restored after maximum reseal attempts");
}

void AdaptiveSealMonitor::terminateSession(const QString& reason)
{
    logEvent(QString("Session terminated: %1").arg(reason), m_currentAVLPressure);
    emit sessionTerminated(reason);

    // Trigger emergency stop via coordinator
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->triggerEmergencyStop(reason);
    }

    stopMonitoring();
}

// ============================================================================
// Hardware Control
// ============================================================================

void AdaptiveSealMonitor::closeVent()
{
    if (m_hardware) {
        m_hardware->setSOL2(false);  // SOL2 is vent valve
    }
}

void AdaptiveSealMonitor::setPumpPower(double power)
{
    if (m_hardware) {
        double clampedPower = std::clamp(power, 0.0, 1.0);
        m_hardware->setPumpEnabled(clampedPower > 0.0);
        m_hardware->setPumpSpeed(clampedPower * 100.0);  // Convert to percentage
    }
}

// ============================================================================
// State Management
// ============================================================================

void AdaptiveSealMonitor::setState(SealState newState)
{
    if (setStateInternal(static_cast<int>(newState))) {
        emit stateChanged(newState);
    }
}

QString AdaptiveSealMonitor::stateToString(int state) const
{
    switch (static_cast<SealState>(state)) {
        case SEALED:        return "SEALED";
        case WARNING:       return "WARNING";
        case LEAK_DETECTED: return "LEAK_DETECTED";
        case RESEALING:     return "RESEALING";
        case SEAL_LOST:     return "SEAL_LOST";
        case SYSTEM_ERROR:  return "SYSTEM_ERROR";
        default:            return "UNKNOWN";
    }
}

void AdaptiveSealMonitor::onStateTransition(int oldState, int newState)
{
    SealState newSealState = static_cast<SealState>(newState);

    if (newSealState == SYSTEM_ERROR) {
        if (m_emergencyStopCoordinator) {
            m_emergencyStopCoordinator->triggerEmergencyStop(
                "AdaptiveSealMonitor entered SYSTEM_ERROR state");
        }
        logEvent("SYSTEM_ERROR state entered", m_currentAVLPressure);
    } else if (newSealState == SEAL_LOST) {
        logEvent("Seal lost - session termination required", m_currentAVLPressure);
    }
}

// ============================================================================
// Logging
// ============================================================================

void AdaptiveSealMonitor::logEvent(const QString& event, double pressure)
{
    QString logMessage = QString("[%1] %2 - Pressure: %3 mmHg, Arousal: %4, Threshold: %5 mmHg")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                        .arg(event)
                        .arg(pressure, 0, 'f', 1)
                        .arg(m_arousalLevel, 0, 'f', 2)
                        .arg(getAdaptiveThreshold(), 0, 'f', 1);

    qInfo() << "ADAPTIVE_SEAL:" << logMessage;

    if (m_safetyLogger) {
        QJsonObject context;
        context["pressure_mmhg"] = pressure;
        context["arousal_level"] = m_arousalLevel;
        context["adaptive_threshold"] = getAdaptiveThreshold();
        context["state"] = stateToString(getState());
        context["reseal_attempts"] = m_resealAttemptCount;
        context["leak_path"] = static_cast<int>(m_lastLeakPath);
        m_safetyLogger->logEvent("AdaptiveSealMonitor", event, context);
    }
}

// ============================================================================
// Emergency Stop Handler
// ============================================================================

void AdaptiveSealMonitor::onEmergencyStopTriggered(const QString& reason)
{
    qWarning() << "AdaptiveSealMonitor handling emergency stop:" << reason;

    stopMonitoring();
    setPumpPower(0.0);

    logEvent(QString("Emergency stop: %1").arg(reason), m_currentAVLPressure);
}

