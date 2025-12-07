#include "SafetyManager.h"
#include "../hardware/HardwareManager.h"
#include "../error/CrashHandler.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>

// Constants
const double SafetyManager::DEFAULT_MAX_PRESSURE = 75.0;       // 75 mmHg (MPX5010DP FS ~75 mmHg)
const double SafetyManager::DEFAULT_WARNING_THRESHOLD = 60.0;  // 80% of max (60 mmHg)
const int SafetyManager::DEFAULT_SENSOR_TIMEOUT_MS = 1000;     // 1 second timeout
const int SafetyManager::MONITORING_INTERVAL_MS = 100;         // 10Hz monitoring
const int SafetyManager::MAX_CONSECUTIVE_ERRORS = 3;           // 3 errors trigger emergency stop

SafetyManager::SafetyManager(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_crashHandler(nullptr)
    , m_safetyState(SAFE)
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
    , m_autoRecoveryEnabled(true)
    , m_recoveryInProgress(false)
{
    // Set up monitoring timer
    m_monitoringTimer->setInterval(MONITORING_INTERVAL_MS);
    connect(m_monitoringTimer, &QTimer::timeout, this, &SafetyManager::performSafetyMonitoring);
}

SafetyManager::~SafetyManager()
{
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
    if (maxPressure > 0 && maxPressure <= 150.0) {  // Reasonable upper limit
        QMutexLocker locker(&m_stateMutex);
        m_maxPressure = maxPressure;
        m_warningThreshold = maxPressure * 0.8;  // 80% of max
        
        qDebug() << QString("Safety limits updated: Max = %1 mmHg, Warning = %2 mmHg")
                    .arg(m_maxPressure).arg(m_warningThreshold);
    }
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
        m_sensorTimeoutMs = timeoutMs;
        qDebug() << "Sensor timeout set to:" << timeoutMs << "ms";
    }
}

void SafetyManager::triggerEmergencyStop(const QString& reason)
{
    QMutexLocker locker(&m_stateMutex);
    
    qCritical() << "EMERGENCY STOP TRIGGERED:" << reason;
    
    m_emergencyStopEvents++;
    m_lastSafetyError = QString("Emergency stop: %1").arg(reason);
    
    // Immediately stop hardware
    if (m_hardware) {
        m_hardware->emergencyStop();
    }
    
    setState(EMERGENCY_STOP);
    emit emergencyStopTriggered(reason);
}

bool SafetyManager::resetEmergencyStop()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_safetyState != EMERGENCY_STOP) {
        return true;  // Not in emergency stop
    }
    
    // Perform comprehensive safety check before reset
    if (!performSafetyCheck()) {
        m_lastSafetyError = "Safety check failed during emergency stop reset";
        return false;
    }
    
    // Reset hardware emergency stop
    if (m_hardware && !m_hardware->resetEmergencyStop()) {
        m_lastSafetyError = "Hardware emergency stop reset failed";
        return false;
    }
    
    // Reset error counters
    m_consecutiveErrors = 0;
    
    setState(SAFE);
    qDebug() << "Emergency stop reset successfully";
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
    if (!m_active || !m_hardware) return;
    
    try {
        // Perform safety checks
        if (performSafetyCheck()) {
            // Reset consecutive error count on successful check
            if (m_consecutiveErrors > 0) {
                m_consecutiveErrors--;
            }
            
            // Update to SAFE state if we were in WARNING
            if (m_safetyState == WARNING) {
                setState(SAFE);
            }
        } else {
            // Increment error count
            m_consecutiveErrors++;
            
            // Trigger emergency stop if too many consecutive errors
            if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
                triggerEmergencyStop("Too many consecutive safety check failures");
            } else if (m_safetyState == SAFE) {
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
    m_sensorErrorEvents++;
    m_lastSafetyError = QString("Sensor error (%1): %2").arg(sensor, error);
    
    qWarning() << m_lastSafetyError;
    
    if (m_safetyState == SAFE) {
        setState(WARNING);
        emit safetyWarning(m_lastSafetyError);
    }
    
    emit sensorTimeout(sensor);
}

void SafetyManager::setState(SafetyState newState)
{
    if (m_safetyState != newState) {
        SafetyState oldState = m_safetyState;
        m_safetyState = newState;
        
        qDebug() << QString("Safety state changed: %1 -> %2")
                    .arg(static_cast<int>(oldState))
                    .arg(static_cast<int>(newState));
        
        emit safetyStateChanged(newState);
    }
}

bool SafetyManager::checkPressureLimits()
{
    try {
        double avlPressure = m_hardware->readAVLPressure();
        double tankPressure = m_hardware->readTankPressure();
        
        // Check for invalid readings
        if (avlPressure < 0 || tankPressure < 0) {
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
            if (m_safetyState == SAFE) {
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
    
    qCritical() << m_lastSafetyError;
    
    emit overpressureDetected(pressure);
    triggerEmergencyStop(m_lastSafetyError);
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

        // Step 2: Reset hardware to safe state
        if (m_hardware) {
            // Stop all actuators
            m_hardware->setPumpEnabled(false);
            m_hardware->setPumpSpeed(0.0);

            // Open vent valves for safety
            m_hardware->setSOL2(true);  // AVL vent valve
            m_hardware->setSOL3(true);  // Tank vent valve
            m_hardware->setSOL1(false); // Close AVL valve

            // Clear emergency stop if set
            m_hardware->resetEmergencyStop();

            qDebug() << "Hardware reset to safe state";
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
    if (m_safetyState == EMERGENCY_STOP && m_autoRecoveryEnabled) {
        // Delay recovery to allow system to fully initialize
        QTimer::singleShot(3000, this, &SafetyManager::performSystemRecovery);
    }
}
