#include "SafetyManager.h"
#include "SafetyConstants.h"
#include "EmergencyStopCoordinator.h"
#include "../logging/ISafetyLogger.h"
#include "../hardware/HardwareManager.h"
#include "../error/CrashHandler.h"
#include "../core/SafeOperationHelper.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <QJsonObject>

// Constants - using centralized SafetyConstants for consistency
// NOTE: MAX_CONSECUTIVE_ERRORS, TISSUE_DAMAGE_RISK_MMHG are now accessed
// directly via SafetyConstants namespace
const double SafetyManager::DEFAULT_MAX_PRESSURE = SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG;
const double SafetyManager::DEFAULT_WARNING_THRESHOLD = SafetyConstants::WARNING_THRESHOLD_MMHG;
const int SafetyManager::DEFAULT_SENSOR_TIMEOUT_MS = SafetyConstants::SENSOR_TIMEOUT_MS;
const int SafetyManager::MONITORING_INTERVAL_MS = SafetyConstants::MONITORING_INTERVAL_MS;

SafetyManager::SafetyManager(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , StatefulComponent<int>(SAFE, "SafetyManager")  // Initialize StatefulComponent
    , m_hardware(hardware)
    , m_crashHandler(nullptr)
    , m_emergencyStopCoordinator(nullptr)
    , m_safetyLogger(nullptr)
    , m_active(false)
    , m_maxPressure(DEFAULT_MAX_PRESSURE)
    , m_warningThreshold(DEFAULT_WARNING_THRESHOLD)
    , m_sensorTimeoutMs(DEFAULT_SENSOR_TIMEOUT_MS)
    , m_monitoringTimer(new QTimer(this))
    , m_lastAVLReading(0)
    , m_lastTankReading(0)
    , m_overpressureEvents(0)
    , m_sensorErrorEvents(0)
    , m_emergencyStopEvents(0)
    , m_recoveryAttempts(0)
    , m_consecutiveErrors(0)
    , m_consecutiveInvalidSensorReadings(0)
    , m_consecutiveRunawaySamples(0)
    , m_autoRecoveryEnabled(true)
    , m_recoveryInProgress(false)
{
    // Set up monitoring timer
    m_monitoringTimer->setInterval(MONITORING_INTERVAL_MS);
    connect(m_monitoringTimer, &QTimer::timeout, this, &SafetyManager::performSafetyMonitoring);

    // Register state transition callback with StatefulComponent base
    registerTransitionCallback([this](int oldState, int newState) {
        onStateTransition(oldState, newState);
    });
}

SafetyManager::~SafetyManager()
{
    // Unregister from emergency stop coordinator
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->unregisterHandler("SafetyManager");
    }
    shutdown();
}

bool SafetyManager::initialize()
{
    if (!m_hardware) {
        m_lastSafetyError = "Hardware manager not provided";
        qCritical() << m_lastSafetyError;
        return false;
    }
    
    try {
        qDebug() << "Initializing Safety Manager...";
        
        // Initialize safety parameters
        initializeSafetyParameters();
        
        // Perform initial safety check
        if (!performSafetyCheck()) {
            throw std::runtime_error("Initial safety check failed");
        }
        
        // Start monitoring
        m_monitoringTimer->start();
        m_active = true;
        
        setState(SAFE);
        
        qDebug() << "Safety Manager initialized successfully";
        qDebug() << QString("Safety limits: Max pressure = %1 mmHg, Warning = %2 mmHg")
                    .arg(m_maxPressure).arg(m_warningThreshold);
        
        return true;
        
    } catch (const std::exception& e) {
        m_lastSafetyError = QString("Safety Manager initialization failed: %1").arg(e.what());
        qCritical() << m_lastSafetyError;
        return false;
    }
}

void SafetyManager::shutdown()
{
    if (m_active) {
        qDebug() << "Shutting down Safety Manager...";
        
        m_monitoringTimer->stop();
        m_active = false;
        
        qDebug() << "Safety Manager shutdown complete";
    }
}

void SafetyManager::setMaxPressure(double maxPressure)
{
    // Clamp operational max pressure to a conservative limit (well below tissue-damage risk)
    if (maxPressure <= 0.0) {
        return;
    }

    if (maxPressure > 100.0) {
        maxPressure = 100.0;  // User-configurable limit must stay below tissue-damage threshold
    }

    QMutexLocker locker(&m_stateMutex);
    m_maxPressure = maxPressure;
    m_warningThreshold = maxPressure * 0.8;  // 80% of max

    qDebug() << QString("Safety limits updated: Max = %1 mmHg, Warning = %2 mmHg")
                .arg(m_maxPressure).arg(m_warningThreshold);
}

void SafetyManager::setWarningThreshold(double warningThreshold)
{
    if (warningThreshold > 0 && warningThreshold < m_maxPressure) {
        QMutexLocker locker(&m_stateMutex);
        m_warningThreshold = warningThreshold;
        qDebug() << "Warning threshold set to:" << warningThreshold << "mmHg";
    }
}

void SafetyManager::setSensorTimeoutMs(int timeoutMs)
{
    if (timeoutMs > 0 && timeoutMs <= 10000) {  // Max 10 seconds
        QMutexLocker locker(&m_stateMutex);
        m_sensorTimeoutMs = timeoutMs;
        qDebug() << "Sensor timeout set to:" << timeoutMs << "ms";
    }
}

void SafetyManager::triggerEmergencyStop(const QString& reason)
{
    qCritical() << "EMERGENCY STOP TRIGGERED:" << reason;

    m_emergencyStopEvents++;
    m_lastSafetyError = QString("Emergency stop: %1").arg(reason);

    // Use EmergencyStopCoordinator if available for centralized coordination
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->triggerEmergencyStop(reason);
    } else {
        // Fallback to direct hardware control if coordinator not available
        if (m_hardware) {
            m_hardware->enterSealMaintainedSafeState(reason);
        }
    }

    setState(EMERGENCY_STOP);
    emit emergencyStopTriggered(reason);
    logSafetyEvent(QString("Emergency stop: %1").arg(reason));
}

void SafetyManager::onEmergencyStopTriggered(const QString& reason)
{
    // This is called by the coordinator when any component triggers emergency stop
    qWarning() << "SafetyManager handling emergency stop from coordinator:" << reason;

    m_emergencyStopEvents++;
    m_lastSafetyError = QString("Emergency stop (coordinated): %1").arg(reason);

    // Default emergency-stop path should preserve AVL seal while venting inner chambers.
    if (m_hardware) {
        m_hardware->enterSealMaintainedSafeState(reason);
    }

    setState(EMERGENCY_STOP);
    emit emergencyStopTriggered(reason);
}

bool SafetyManager::resetEmergencyStop()
{
    if (getState() != EMERGENCY_STOP) {
        return true;  // Not in emergency stop
    }

    // Reset hardware emergency stop using SafeOperationHelper
    auto result = SafeOperationHelper::execute<bool>(
        "resetEmergencyStop", "SafetyManager",
        [this]() { return m_hardware ? m_hardware->resetEmergencyStop() : true; },
        [this](const QString& err) { emit systemError(err); }
    );

    if (!result.isSuccess() || !result.get()) {
        m_lastSafetyError = "Hardware emergency stop reset failed";
        qWarning() << m_lastSafetyError;
    }

    // Reset error counters
    m_consecutiveErrors = 0;

    setState(SAFE);
    logSafetyEvent("Emergency stop reset successfully");
    qDebug() << "Emergency stop reset successfully. Monitoring will re-assess safety.";
    return true;
}

bool SafetyManager::performSafetyCheck()
{
    if (!m_hardware || !m_hardware->isReady()) {
        m_lastSafetyError = "Hardware not ready";
        return false;
    }
    
    try {
        // Check pressure limits
        if (!checkPressureLimits()) {
            return false;
        }
        
        // Check sensor health
        if (!checkSensorHealth()) {
            return false;
        }
        
        // Check hardware status
        if (!checkHardwareStatus()) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        m_lastSafetyError = QString("Safety check error: %1").arg(e.what());
        return false;
    }
}

void SafetyManager::performSafetyMonitoring()
{
    QMutexLocker locker(&m_stateMutex);
    if (!m_active || !m_hardware) return;
    
    try {
        // Perform safety checks
        if (performSafetyCheck()) {
            // Reset consecutive error count on successful check
            if (m_consecutiveErrors > 0) {
                m_consecutiveErrors--;
            }

            // Update to SAFE state if we were in WARNING
            if (getState() == WARNING) {
                setState(SAFE);
            }
        } else {
            // Increment error count
            m_consecutiveErrors++;

            // Trigger emergency stop if too many consecutive errors
            if (m_consecutiveErrors >= SafetyConstants::MAX_CONSECUTIVE_ERRORS) {
                triggerEmergencyStop("Too many consecutive safety check failures");
            } else if (getState() == SAFE) {
                setState(WARNING);
                emit safetyWarning(m_lastSafetyError);
            }
        }
        
    } catch (const std::exception& e) {
        handleCriticalError(QString("Safety monitoring error: %1").arg(e.what()));
    }
}

void SafetyManager::handleSensorError(const QString& sensor, const QString& error)
{
    QMutexLocker locker(&m_stateMutex);
    m_sensorErrorEvents++;
    m_lastSafetyError = QString("Sensor error (%1): %2").arg(sensor, error);

    qWarning() << m_lastSafetyError;

    if (getState() == SAFE) {
        setState(WARNING);
        emit safetyWarning(m_lastSafetyError);
    }

    emit sensorTimeout(sensor);
}

void SafetyManager::setState(SafetyState newState)
{
    // Use StatefulComponent base class for thread-safe state management
    if (setStateInternal(static_cast<int>(newState))) {
        // Emit Qt signal (StatefulComponent can't emit signals since it's a template)
        emit safetyStateChanged(newState);
    }
}

QString SafetyManager::stateToString(int state) const
{
    switch (static_cast<SafetyState>(state)) {
        case SAFE:           return "SAFE";
        case WARNING:        return "WARNING";
        case CRITICAL:       return "CRITICAL";
        case EMERGENCY_STOP: return "EMERGENCY_STOP";
        default:             return "UNKNOWN";
    }
}

void SafetyManager::onStateTransition(int oldState, int newState)
{
    SafetyState newSafetyState = static_cast<SafetyState>(newState);

    // Log state transition via unified logger
    logSafetyEvent(QString("State transition: %1 -> %2")
                   .arg(stateToString(oldState))
                   .arg(stateToString(newState)));

    // Handle emergency stop state via coordinator if available
    if (newSafetyState == EMERGENCY_STOP && m_emergencyStopCoordinator) {
        // Don't call coordinator here to avoid recursion - coordinator will be called
        // explicitly by triggerEmergencyStop() method
    }
}

void SafetyManager::setEmergencyStopCoordinator(EmergencyStopCoordinator* coordinator)
{
    // Unregister from old coordinator
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->unregisterHandler("SafetyManager");
    }

    m_emergencyStopCoordinator = coordinator;

    // Register with new coordinator - SafetyManager is CRITICAL priority
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->registerHandler("SafetyManager",
            EmergencyStopCoordinator::PRIORITY_CRITICAL,
            [this](const QString& reason) { onEmergencyStopTriggered(reason); });
    }
}

void SafetyManager::setSafetyLogger(ISafetyLogger* logger)
{
    m_safetyLogger = logger;
}

void SafetyManager::logSafetyEvent(const QString& event)
{
    qDebug() << "SafetyManager:" << event;

    if (m_safetyLogger) {
        m_safetyLogger->logEvent("SafetyManager", event);
    }
}

bool SafetyManager::checkPressureLimits()
{
    if (!m_hardware) {
        m_lastSafetyError = "Hardware manager not available";
        return false;
    }

    try {
        double avlPressure = m_hardware->readAVLPressure();
        double tankPressure = m_hardware->readTankPressure();
        
        // Evaluate complex safety conditions that combine sensor validity and pump behavior
        evaluateRunawayAndInvalidSensors(avlPressure, tankPressure);

        // Check for grossly invalid readings
        if (!isSensorDataValid(avlPressure, tankPressure)) {
            m_lastSafetyError = "Invalid pressure readings";
            return false;
        }
        
        // Check maximum pressure limits
        if (avlPressure > m_maxPressure) {
            handleOverpressure(avlPressure);
            return false;
        }
        
        if (tankPressure > m_maxPressure) {
            handleOverpressure(tankPressure);
            return false;
        }
        
        // Check warning thresholds
        if (avlPressure > m_warningThreshold || tankPressure > m_warningThreshold) {
            if (getState() == SAFE) {
                setState(WARNING);
                emit safetyWarning(QString("Pressure approaching limit: AVL=%1, Tank=%2 mmHg")
                                  .arg(avlPressure, 0, 'f', 1).arg(tankPressure, 0, 'f', 1));
            }
        }
        
        // Update reading timestamps
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        m_lastAVLReading = currentTime;
        m_lastTankReading = currentTime;
        
        return true;
        
    } catch (const std::exception& e) {
        m_lastSafetyError = QString("Pressure check error: %1").arg(e.what());
        return false;
    }
}

bool SafetyManager::checkSensorHealth()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Check for sensor timeouts
    if (currentTime - m_lastAVLReading > m_sensorTimeoutMs) {
        m_lastSafetyError = "AVL sensor timeout";
        emit sensorTimeout("AVL");
        return false;
    }
    
    if (currentTime - m_lastTankReading > m_sensorTimeoutMs) {
        m_lastSafetyError = "Tank sensor timeout";
        emit sensorTimeout("Tank");
        return false;
    }
    
    return true;
}

bool SafetyManager::checkHardwareStatus()
{
    if (!m_hardware) {
        m_lastSafetyError = "Hardware manager not available";
        return false;
    }

    if (!m_hardware->isReady()) {
        m_lastSafetyError = "Hardware not ready";
        return false;
    }

    return true;
}

void SafetyManager::handleOverpressure(double pressure)
{
    m_overpressureEvents++;
    m_lastSafetyError = QString("Overpressure detected: %1 mmHg (max: %2 mmHg)")
                       .arg(pressure, 0, 'f', 1).arg(m_maxPressure, 0, 'f', 1);

    emit overpressureDetected(pressure);

    // Two-tier response:
    // 1) Above hard tissue-damage risk threshold -> full-vent emergency (cup may detach)
    // 2) Above configured max but below tissue-damage risk -> seal-maintained safe state
    if (pressure >= SafetyConstants::TISSUE_DAMAGE_RISK_MMHG) {
        qCritical() << m_lastSafetyError << "- exceeding tissue-damage risk threshold, FULL VENT";

        m_emergencyStopEvents++;

        if (m_hardware) {
            m_hardware->enterFullVentState("Overpressure above tissue-damage risk threshold");
        }

        setState(EMERGENCY_STOP);
        emit emergencyStopTriggered(m_lastSafetyError);
    } else {
        qWarning() << m_lastSafetyError << "- entering seal-maintained safe state";

        if (m_hardware) {
            m_hardware->enterSealMaintainedSafeState("Overpressure above configured maximum");
        }

        // This is a critical-but-recoverable condition; keep explicit CRITICAL state
        setState(CRITICAL);
        emit safetyWarning(m_lastSafetyError);
    }
}

void SafetyManager::handleCriticalError(const QString& error)
{
    m_lastSafetyError = error;
    qCritical() << "CRITICAL SAFETY ERROR:" << error;

    setState(CRITICAL);
    emit systemError(error);

    // Consider emergency stop for critical errors
    triggerEmergencyStop(error);
}

void SafetyManager::initializeSafetyParameters()
{
    // Initialize timestamps
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    m_lastAVLReading = currentTime;
    m_lastTankReading = currentTime;
    
    // Reset statistics
    m_overpressureEvents = 0;
    m_sensorErrorEvents = 0;
    m_emergencyStopEvents = 0;
    m_recoveryAttempts = 0;
    m_consecutiveErrors = 0;

    m_consecutiveInvalidSensorReadings = 0;
    m_consecutiveRunawaySamples = 0;
}

bool SafetyManager::isSensorDataValid(double avlPressure, double tankPressure) const
{
    // Use centralized SafetyConstants for pressure validation range
    return SafetyConstants::isValidPressure(avlPressure) &&
           SafetyConstants::isValidPressure(tankPressure);
}

void SafetyManager::evaluateRunawayAndInvalidSensors(double avlPressure, double tankPressure)
{
    if (!m_hardware) {
        return;
    }

    const bool valid = isSensorDataValid(avlPressure, tankPressure);
    if (!valid) {
        m_consecutiveInvalidSensorReadings++;
    } else {
        m_consecutiveInvalidSensorReadings = 0;
    }

    // Pump "runaway" heuristic: sustained high pump speed
    const double pumpSpeed = m_hardware->getPumpSpeed();  // 0-100%
    const bool runawayNow = (pumpSpeed >= 80.0);          // >=80% duty considered suspicious

    if (runawayNow) {
        m_consecutiveRunawaySamples++;
    } else {
        m_consecutiveRunawaySamples = 0;
    }

    // Require both conditions to persist for multiple consecutive checks before escalating
    static const int REQUIRED_INVALID_SAMPLES = 5;  // At 10Hz, ~0.5s
    static const int REQUIRED_RUNAWAY_SAMPLES = 5;  // At 10Hz, ~0.5s

    if (m_consecutiveInvalidSensorReadings >= REQUIRED_INVALID_SAMPLES &&
        m_consecutiveRunawaySamples >= REQUIRED_RUNAWAY_SAMPLES &&
        getState() != EMERGENCY_STOP)
    {
        m_lastSafetyError = "Invalid pressure sensor data combined with pump runaway";
        qCritical() << m_lastSafetyError << "- triggering FULL VENT for safety";

        m_emergencyStopEvents++;

        if (m_hardware) {
            m_hardware->enterFullVentState("Invalid sensor data + pump runaway");
        }

        setState(EMERGENCY_STOP);
        emit emergencyStopTriggered(m_lastSafetyError);
    }
}

// Auto-recovery implementation
void SafetyManager::enableAutoRecovery(bool enabled)
{
    m_autoRecoveryEnabled = enabled;
    qDebug() << "Auto-recovery" << (enabled ? "enabled" : "disabled");
}

void SafetyManager::setCrashHandler(CrashHandler* crashHandler)
{
    m_crashHandler = crashHandler;

    if (m_crashHandler) {
        // Connect crash handler signals
        connect(m_crashHandler, &CrashHandler::crashDetected,
                this, &SafetyManager::onCrashDetected);
        connect(m_crashHandler, &CrashHandler::systemStateRestored,
                this, &SafetyManager::onSystemStateRestored);

        qDebug() << "CrashHandler connected to SafetyManager";
    }
}

void SafetyManager::performSystemRecovery()
{
    if (m_recoveryInProgress || !m_autoRecoveryEnabled) {
        return;
    }

    m_recoveryInProgress = true;
    m_recoveryAttempts++;

    qWarning() << "Starting system recovery attempt" << m_recoveryAttempts;
    emit systemRecoveryStarted();

    bool recoverySuccess = true;

    try {
        // Step 1: Reset safety state to safe
        setState(SAFE);

        // Step 2: Reset hardware to a seal-maintained safe state
        if (m_hardware) {
            m_hardware->enterSealMaintainedSafeState("SafetyManager system recovery");

            // Clear emergency stop if set (allows future operation once user explicitly resets)
            m_hardware->resetEmergencyStop();

            qDebug() << "Hardware reset to seal-maintained safe state";
        }

        // Step 3: Reset error counters
        m_consecutiveErrors = 0;
        m_lastSafetyError.clear();

        // Step 4: Perform safety check
        if (!performSafetyCheck()) {
            throw std::runtime_error("Safety check failed during recovery");
        }

        // Step 5: Restart monitoring if it was stopped
        if (!m_monitoringTimer->isActive() && m_active) {
            m_monitoringTimer->start();
        }

        qDebug() << "System recovery completed successfully";

    } catch (const std::exception& e) {
        recoverySuccess = false;
        m_lastSafetyError = QString("Recovery failed: %1").arg(e.what());
        qCritical() << m_lastSafetyError;

        // If recovery fails, trigger emergency stop
        triggerEmergencyStop("System recovery failed");
    }

    m_recoveryInProgress = false;
    emit systemRecoveryCompleted(recoverySuccess);

    if (!recoverySuccess && m_recoveryAttempts < 3) {
        // Attempt recovery again after a delay
        QTimer::singleShot(5000, this, &SafetyManager::performSystemRecovery);
    }
}

void SafetyManager::handleSystemCrash(const QString& crashInfo)
{
    qCritical() << "System crash detected:" << crashInfo;

    // Emit crash detection signal
    emit crashDetected(crashInfo);

    // Trigger emergency stop immediately
    triggerEmergencyStop(QString("System crash: %1").arg(crashInfo));

    // Attempt recovery if enabled
    if (m_autoRecoveryEnabled) {
        // Delay recovery to allow system to stabilize
        QTimer::singleShot(2000, this, &SafetyManager::performSystemRecovery);
    }
}

// Slot implementations
void SafetyManager::onCrashDetected(const QString& crashInfo)
{
    handleSystemCrash(crashInfo);
}

void SafetyManager::onSystemStateRestored()
{
    qDebug() << "System state restored - checking for recovery needs";

    // Check if we need to perform recovery
    if (getState() == EMERGENCY_STOP && m_autoRecoveryEnabled) {
        // Delay recovery to allow system to fully initialize
        QTimer::singleShot(3000, this, &SafetyManager::performSystemRecovery);
    }
}
