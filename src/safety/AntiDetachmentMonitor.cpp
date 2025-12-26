#include "AntiDetachmentMonitor.h"
#include "SafetyConstants.h"
#include "EmergencyStopCoordinator.h"
#include "../logging/ISafetyLogger.h"
#include "../hardware/HardwareManager.h"
#include "../core/SafeOperationHelper.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <QThread>
#include <QJsonObject>
#include <cmath>
#include <algorithm>

// Constants - using centralized SafetyConstants for consistency
const double AntiDetachmentMonitor::DEFAULT_DETACHMENT_THRESHOLD = SafetyConstants::DEFAULT_DETACHMENT_THRESHOLD_MMHG;
const double AntiDetachmentMonitor::DEFAULT_WARNING_THRESHOLD = SafetyConstants::WARNING_THRESHOLD_MMHG;
const double AntiDetachmentMonitor::DEFAULT_HYSTERESIS = SafetyConstants::DEFAULT_HYSTERESIS_MMHG;
const int AntiDetachmentMonitor::DEFAULT_MONITORING_RATE_HZ = SafetyConstants::ANTI_DETACHMENT_MONITORING_RATE_HZ;
const int AntiDetachmentMonitor::DEFAULT_RESPONSE_DELAY_MS = SafetyConstants::ANTI_DETACHMENT_RESPONSE_DELAY_MS;
const double AntiDetachmentMonitor::DEFAULT_MAX_VACUUM_INCREASE = SafetyConstants::MAX_VACUUM_INCREASE_PERCENT;
const int AntiDetachmentMonitor::PRESSURE_HISTORY_SIZE = SafetyConstants::PRESSURE_HISTORY_SIZE;
// NOTE: MAX_CONSECUTIVE_ERRORS, MIN_VALID_PRESSURE, MAX_VALID_PRESSURE
// are now accessed directly via SafetyConstants namespace

AntiDetachmentMonitor::AntiDetachmentMonitor(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , StatefulComponent<int>(ATTACHED, "AntiDetachmentMonitor")  // Initialize StatefulComponent
    , m_hardware(hardware)
    , m_active(false)
    , m_monitoring(false)
    , m_paused(false)
    , m_detachmentThreshold(DEFAULT_DETACHMENT_THRESHOLD)
    , m_warningThreshold(DEFAULT_WARNING_THRESHOLD)
    , m_hysteresis(DEFAULT_HYSTERESIS)
    , m_monitoringRateHz(DEFAULT_MONITORING_RATE_HZ)
    , m_responseDelayMs(DEFAULT_RESPONSE_DELAY_MS)
    , m_maxVacuumIncrease(DEFAULT_MAX_VACUUM_INCREASE)
    , m_currentAVLPressure(0.0)
    , m_lastReadingTime(0)
    , m_sol1Active(false)
    , m_targetVacuumLevel(0.0)
    , m_responseTimer(new QTimer(this))
    , m_detectionTime(0)
    , m_detachmentEvents(0)
    , m_warningEvents(0)
    , m_lastDetachmentTime(0)
    , m_totalResponseTime(0.0)
    , m_responseCount(0)
    , m_averageResponseTime(0.0)
    , m_monitoringTimer(new QTimer(this))
    , m_consecutiveErrors(0)
    , m_emergencyStopCoordinator(nullptr)
    , m_safetyLogger(nullptr)
{
    // Set up monitoring timer for high-frequency monitoring
    m_monitoringTimer->setInterval(1000 / m_monitoringRateHz);  // Convert Hz to ms
    m_monitoringTimer->setTimerType(Qt::PreciseTimer);
    connect(m_monitoringTimer, &QTimer::timeout, this, &AntiDetachmentMonitor::performMonitoringCycle);

    // Set up response timer for delayed response
    m_responseTimer->setSingleShot(true);
    connect(m_responseTimer, &QTimer::timeout, this, &AntiDetachmentMonitor::onResponseTimer);

    // Register state transition callback with StatefulComponent base
    registerTransitionCallback([this](int oldState, int newState) {
        onStateTransition(oldState, newState);
    });

    qDebug() << "Anti-detachment monitor initialized with threshold:" << m_detachmentThreshold << "mmHg";
}

AntiDetachmentMonitor::~AntiDetachmentMonitor()
{
    // Unregister from emergency stop coordinator
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->unregisterHandler("AntiDetachmentMonitor");
    }
    shutdown();
}

bool AntiDetachmentMonitor::initialize()
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
        // Perform self-test
        if (!performSelfTest()) {
            throw std::runtime_error("Self-test failed");
        }

        // Initialize state
        setState(ATTACHED);
        m_active = true;

        qDebug() << "Anti-detachment monitor initialized successfully";
        return true;

    } catch (const std::exception& e) {
        m_lastError = QString("Initialization failed: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void AntiDetachmentMonitor::shutdown()
{
    if (!m_active) return;

    qDebug() << "Shutting down anti-detachment monitor...";

    // Stop monitoring
    stopMonitoring();

    // Deactivate any active anti-detachment response
    deactivateAntiDetachment();

    m_active = false;
    qDebug() << "Anti-detachment monitor shutdown complete";
}

void AntiDetachmentMonitor::startMonitoring()
{
    if (!m_active) {
        qWarning() << "Cannot start monitoring: System not initialized";
        return;
    }

    QMutexLocker locker(&m_stateMutex);

    if (m_monitoring) {
        qWarning() << "Monitoring already active";
        return;
    }

    m_monitoring = true;
    m_paused = false;
    m_consecutiveErrors = 0;

    // Start monitoring timer
    m_monitoringTimer->start();

    qDebug() << QString("Anti-detachment monitoring started at %1 Hz").arg(m_monitoringRateHz);
}

void AntiDetachmentMonitor::stopMonitoring()
{
    QMutexLocker locker(&m_stateMutex);

    if (!m_monitoring) return;

    // Stop timers
    m_monitoringTimer->stop();
    m_responseTimer->stop();

    // Deactivate anti-detachment response
    deactivateAntiDetachment();

    m_monitoring = false;
    m_paused = false;

    setState(ATTACHED);  // Reset to safe state

    qDebug() << "Anti-detachment monitoring stopped";
}

void AntiDetachmentMonitor::pauseMonitoring()
{
    QMutexLocker locker(&m_stateMutex);

    if (m_monitoring && !m_paused) {
        m_paused = true;
        m_monitoringTimer->stop();
        qDebug() << "Anti-detachment monitoring paused";
    }
}

void AntiDetachmentMonitor::resumeMonitoring()
{
    QMutexLocker locker(&m_stateMutex);

    if (m_monitoring && m_paused) {
        m_paused = false;
        m_monitoringTimer->start();
        qDebug() << "Anti-detachment monitoring resumed";
    }
}

void AntiDetachmentMonitor::setThreshold(double thresholdMmHg)
{
    if (thresholdMmHg > 0 && thresholdMmHg < SafetyConstants::MAX_VALID_PRESSURE) {
        m_detachmentThreshold = thresholdMmHg;

        // Automatically adjust warning threshold if needed
        if (m_warningThreshold <= thresholdMmHg) {
            m_warningThreshold = thresholdMmHg + 10.0;  // 10 mmHg above detachment threshold
        }

        qDebug() << QString("Anti-detachment threshold set to %1 mmHg").arg(thresholdMmHg);
    }
}

void AntiDetachmentMonitor::setWarningThreshold(double thresholdMmHg)
{
    if (thresholdMmHg > m_detachmentThreshold && thresholdMmHg < SafetyConstants::MAX_VALID_PRESSURE) {
        m_warningThreshold = thresholdMmHg;
        qDebug() << QString("Warning threshold set to %1 mmHg").arg(thresholdMmHg);
    }
}

void AntiDetachmentMonitor::setHysteresis(double hysteresisMmHg)
{
    if (hysteresisMmHg >= 0 && hysteresisMmHg < 20.0) {  // Reasonable limit
        m_hysteresis = hysteresisMmHg;
        qDebug() << QString("Hysteresis set to %1 mmHg").arg(hysteresisMmHg);
    }
}

void AntiDetachmentMonitor::setResponseDelay(int delayMs)
{
    if (delayMs >= 0 && delayMs <= 1000) {  // Max 1 second delay
        m_responseDelayMs = delayMs;
        qDebug() << QString("Response delay set to %1 ms").arg(delayMs);
    }
}

void AntiDetachmentMonitor::setMaxVacuumIncrease(double maxIncrease)
{
    if (maxIncrease > 0 && maxIncrease <= 50.0) {  // Max 50% increase
        m_maxVacuumIncrease = maxIncrease;
        qDebug() << QString("Max vacuum increase set to %1%").arg(maxIncrease);
    }
}

bool AntiDetachmentMonitor::performSelfTest()
{
    if (!m_hardware) {
        m_lastError = "Hardware not available for self-test";
        return false;
    }

    try {
        // Test AVL pressure reading
        double testPressure = m_hardware->readAVLPressure();
        if (!SafetyConstants::isValidPressure(testPressure)) {
            throw std::runtime_error("Invalid AVL pressure reading during self-test");
        }

        // Test SOL1 valve control
        bool originalSOL1State = m_hardware->getSOL1State();

        // Test valve activation
        m_hardware->setSOL1(true);
        QThread::msleep(50);  // Brief delay
        if (!m_hardware->getSOL1State()) {
            throw std::runtime_error("SOL1 valve activation test failed");
        }

        // Test valve deactivation
        m_hardware->setSOL1(false);
        QThread::msleep(50);  // Brief delay
        if (m_hardware->getSOL1State()) {
            throw std::runtime_error("SOL1 valve deactivation test failed");
        }

        // Restore original state
        m_hardware->setSOL1(originalSOL1State);

        qDebug() << "Anti-detachment monitor self-test passed";
        emit selfTestCompleted(true);
        return true;

    } catch (const std::exception& e) {
        m_lastError = QString("Self-test failed: %1").arg(e.what());
        qCritical() << m_lastError;
        emit selfTestCompleted(false);
        return false;
    }
}

void AntiDetachmentMonitor::performMonitoringCycle()
{
    if (!m_active || !m_monitoring || m_paused) return;

    try {
        // Read AVL pressure
        double avlPressure = m_hardware->readAVLPressure();

        if (!SafetyConstants::isValidPressure(avlPressure)) {
            m_consecutiveErrors++;
            if (m_consecutiveErrors >= SafetyConstants::MAX_CONSECUTIVE_ERRORS) {
                emit systemError("Too many consecutive invalid pressure readings");
                setState(SYSTEM_ERROR);
            }
            return;
        }

        // Reset error count on successful reading
        m_consecutiveErrors = 0;

        // Process the reading
        processAVLReading(avlPressure);

        // Update statistics
        updateStatistics();

    } catch (const std::exception& e) {
        m_consecutiveErrors++;
        m_lastError = QString("Monitoring cycle error: %1").arg(e.what());

        if (m_consecutiveErrors >= SafetyConstants::MAX_CONSECUTIVE_ERRORS) {
            emit systemError(m_lastError);
            setState(SYSTEM_ERROR);
        }
    }
}

void AntiDetachmentMonitor::processAVLReading(double avlPressure)
{
    m_currentAVLPressure = avlPressure;
    m_lastReadingTime = QDateTime::currentMSecsSinceEpoch();

    // Add to pressure history
    m_pressureHistory.enqueue(avlPressure);
    while (m_pressureHistory.size() > PRESSURE_HISTORY_SIZE) {
        m_pressureHistory.dequeue();
    }

    // Determine new state based on pressure and hysteresis
    // Use getCurrentState() from StatefulComponent base class
    DetachmentState currentState = getCurrentState();
    DetachmentState newState = currentState;

    switch (currentState) {
    case ATTACHED:
        if (avlPressure < m_warningThreshold) {
            newState = WARNING;
        }
        if (avlPressure < m_detachmentThreshold) {
            newState = DETACHMENT_RISK;
        }
        break;

    case WARNING:
        if (avlPressure > m_warningThreshold + m_hysteresis) {
            newState = ATTACHED;
        } else if (avlPressure < m_detachmentThreshold) {
            newState = DETACHMENT_RISK;
        }
        break;

    case DETACHMENT_RISK:
        if (avlPressure > m_warningThreshold + m_hysteresis) {
            newState = ATTACHED;
        } else if (avlPressure > m_detachmentThreshold + m_hysteresis) {
            newState = WARNING;
        } else if (avlPressure < m_detachmentThreshold - m_hysteresis) {
            newState = DETACHED;
        }
        break;

    case DETACHED:
        if (avlPressure > m_detachmentThreshold + m_hysteresis) {
            newState = DETACHMENT_RISK;
        }
        break;

    case SYSTEM_ERROR:
        // Can only exit error state through manual reset
        break;
    }

    // Handle state changes
    if (newState != currentState) {
        setState(newState);

        switch (newState) {
        case WARNING:
            handleWarningEvent();
            break;
        case DETACHMENT_RISK:
        case DETACHED:
            handleDetachmentEvent();
            break;
        case ATTACHED:
            if (m_sol1Active) {
                deactivateAntiDetachment();
                emit detachmentResolved();
            }
            break;
        default:
            break;
        }
    }
}

void AntiDetachmentMonitor::handleDetachmentEvent()
{
    m_detachmentEvents++;
    m_lastDetachmentTime = QDateTime::currentMSecsSinceEpoch();
    m_detectionTime = m_lastDetachmentTime;

    logEvent("Detachment detected", m_currentAVLPressure);

    emit detachmentDetected(m_currentAVLPressure);

    // Start response timer if not already active
    if (!m_responseTimer->isActive()) {
        m_responseTimer->start(m_responseDelayMs);
    }
}

void AntiDetachmentMonitor::handleWarningEvent()
{
    m_warningEvents++;

    logEvent("Detachment warning", m_currentAVLPressure);

    emit detachmentWarning(m_currentAVLPressure);
}

void AntiDetachmentMonitor::onResponseTimer()
{
    // Activate anti-detachment response
    activateAntiDetachment();
}

void AntiDetachmentMonitor::activateAntiDetachment()
{
    if (!m_hardware || m_sol1Active) return;

    try {
        // Calculate target vacuum level
        m_targetVacuumLevel = calculateTargetVacuum(m_currentAVLPressure);

        // Activate SOL1 valve to increase vacuum
        m_hardware->setSOL1(true);
        m_sol1Active = true;

        // Apply vacuum correction
        applyVacuumCorrection(m_targetVacuumLevel);

        logEvent("Anti-detachment activated", m_targetVacuumLevel);

        emit sol1Activated(m_targetVacuumLevel);

        qWarning() << QString("ANTI-DETACHMENT ACTIVATED - Target vacuum: %1 mmHg")
                      .arg(m_targetVacuumLevel, 0, 'f', 1);

    } catch (const std::exception& e) {
        m_lastError = QString("Failed to activate anti-detachment: %1").arg(e.what());
        emit systemError(m_lastError);
    }
}

void AntiDetachmentMonitor::deactivateAntiDetachment()
{
    if (!m_hardware || !m_sol1Active) return;

    try {
        // Deactivate SOL1 valve
        m_hardware->setSOL1(false);
        m_sol1Active = false;
        m_targetVacuumLevel = 0.0;

        logEvent("Anti-detachment deactivated", m_currentAVLPressure);

        emit sol1Deactivated();

        qDebug() << "Anti-detachment deactivated";

        // Update response time statistics
        if (m_detectionTime > 0) {
            qint64 responseTime = QDateTime::currentMSecsSinceEpoch() - m_detectionTime;
            m_totalResponseTime += responseTime;
            m_responseCount++;
            m_averageResponseTime = m_totalResponseTime / m_responseCount;
            m_detectionTime = 0;
        }

    } catch (const std::exception& e) {
        m_lastError = QString("Failed to deactivate anti-detachment: %1").arg(e.what());
        emit systemError(m_lastError);
    }
}

void AntiDetachmentMonitor::setState(DetachmentState newState)
{
    // Use StatefulComponent base class for thread-safe state management
    // This handles mutex locking, previous state tracking, and logging automatically
    if (setStateInternal(static_cast<int>(newState))) {
        // Emit Qt signal (StatefulComponent can't emit signals since it's a template)
        emit stateChanged(newState);
    }
}

QString AntiDetachmentMonitor::stateToString(int state) const
{
    switch (static_cast<DetachmentState>(state)) {
        case ATTACHED:        return "ATTACHED";
        case WARNING:         return "WARNING";
        case DETACHMENT_RISK: return "DETACHMENT_RISK";
        case DETACHED:        return "DETACHED";
        case SYSTEM_ERROR:    return "SYSTEM_ERROR";
        default:              return "UNKNOWN";
    }
}

void AntiDetachmentMonitor::onStateTransition(int oldState, int newState)
{
    // This callback is invoked by StatefulComponent after state change
    DetachmentState newDetachmentState = static_cast<DetachmentState>(newState);

    // Handle critical state transitions via centralized coordinator
    if (newDetachmentState == SYSTEM_ERROR) {
        if (m_emergencyStopCoordinator) {
            m_emergencyStopCoordinator->triggerEmergencyStop(
                "AntiDetachmentMonitor entered SYSTEM_ERROR state");
        }
        logEvent("SYSTEM_ERROR state entered", m_currentAVLPressure);
    } else if (newDetachmentState == DETACHED) {
        // Log critical event but don't trigger emergency stop yet
        // (anti-detachment response will try to recover first)
        logEvent("Cup detachment detected", m_currentAVLPressure);
    }
}

// NOTE: validatePressureReading() removed - use SafetyConstants::isValidPressure() instead

double AntiDetachmentMonitor::calculateTargetVacuum(double currentPressure)
{
    // Calculate how much additional vacuum is needed
    double pressureDeficit = m_detachmentThreshold - currentPressure;

    // Apply maximum increase limit
    double maxIncrease = m_detachmentThreshold * (m_maxVacuumIncrease / 100.0);
    double targetIncrease = std::min(pressureDeficit * 1.2, maxIncrease);  // 20% safety margin

    return currentPressure + targetIncrease;
}

void AntiDetachmentMonitor::applyVacuumCorrection(double targetPressure)
{
    if (!m_hardware) {
        qWarning() << "Cannot apply vacuum correction: Hardware not available";
        return;
    }

    // Calculate pump speed based on target pressure
    // The relationship between pump speed and pressure is approximately linear
    // in the operating range. We use a proportional control approach.

    double currentPressure = m_currentAVLPressure;
    double pressureDelta = targetPressure - currentPressure;

    // Clamp target to safe operating limits
    double maxSafePressure = SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG;
    if (targetPressure > maxSafePressure) {
        targetPressure = maxSafePressure;
        qWarning() << "Target pressure clamped to max safe pressure:" << maxSafePressure << "mmHg";
    }

    // Convert pressure target to pump speed percentage
    // Assuming linear relationship: 0% = 0 mmHg, 100% = max pressure
    double pumpSpeedPercent = (targetPressure / maxSafePressure) * 100.0;

    // Apply minimum and maximum constraints
    pumpSpeedPercent = std::max(0.0, std::min(100.0, pumpSpeedPercent));

    // Enable pump and set speed
    m_hardware->setPumpEnabled(true);
    m_hardware->setPumpSpeed(pumpSpeedPercent);

    // Ensure SOL1 is open for vacuum application (we control it during anti-detachment)
    m_hardware->setSOL1(true);

    // Close vent valve to build pressure
    m_hardware->setSOL2(false);

    // Use unified logging instead of deprecated writeToSafetyLog
    logEvent(QString("Vacuum correction applied: target=%1 mmHg, pump=%2%")
             .arg(targetPressure, 0, 'f', 1)
             .arg(pumpSpeedPercent, 0, 'f', 1), targetPressure);

    qDebug() << QString("Vacuum correction applied: target=%1 mmHg, pump speed=%2%")
                .arg(targetPressure, 0, 'f', 1)
                .arg(pumpSpeedPercent, 0, 'f', 1);
}

void AntiDetachmentMonitor::updateStatistics()
{
    // Calculate rolling average of pressure readings
    if (m_pressureHistory.isEmpty()) return;

    double sum = 0.0;
    double minPressure = SafetyConstants::MAX_VALID_PRESSURE;
    double maxPressure = SafetyConstants::MIN_VALID_PRESSURE;

    for (double p : m_pressureHistory) {
        sum += p;
        minPressure = std::min(minPressure, p);
        maxPressure = std::max(maxPressure, p);
    }

    double avgPressure = sum / m_pressureHistory.size();
    double pressureRange = maxPressure - minPressure;

    // Detect pressure instability (high variance could indicate seal issues)
    // Use getCurrentState() from StatefulComponent base class
    if (pressureRange > 20.0 && getCurrentState() == ATTACHED) {
        qWarning() << "High pressure variance detected:" << pressureRange << "mmHg - possible seal instability";
    }
}

void AntiDetachmentMonitor::logEvent(const QString& event, double pressure)
{
    QString logMessage = QString("[%1] %2 - Pressure: %3 mmHg")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                        .arg(event)
                        .arg(pressure, 0, 'f', 1);

    qInfo() << "ANTI-DETACHMENT:" << logMessage;

    // Use unified safety logger (delegates to DataLogger)
    if (m_safetyLogger) {
        QJsonObject context;
        context["pressure_mmhg"] = pressure;
        context["state"] = stateToString(getState());
        context["sol1_active"] = m_sol1Active;
        m_safetyLogger->logEvent("AntiDetachmentMonitor", event, context);
    }
}


// ============================================================================
// Emergency Stop Coordinator Integration (replaces direct SafetyManager calls)
// ============================================================================

void AntiDetachmentMonitor::setEmergencyStopCoordinator(EmergencyStopCoordinator* coordinator)
{
    // Unregister from previous coordinator if any
    if (m_emergencyStopCoordinator) {
        m_emergencyStopCoordinator->unregisterHandler("AntiDetachmentMonitor");
    }

    m_emergencyStopCoordinator = coordinator;

    if (m_emergencyStopCoordinator) {
        // Register our emergency stop handler with HIGH priority (safety-critical component)
        m_emergencyStopCoordinator->registerHandler(
            "AntiDetachmentMonitor",
            EmergencyStopCoordinator::PRIORITY_HIGH,
            [this](const QString& reason) {
                onEmergencyStopTriggered(reason);
            }
        );
        qDebug() << "EmergencyStopCoordinator linked to AntiDetachmentMonitor";
    }
}

void AntiDetachmentMonitor::onEmergencyStopTriggered(const QString& reason)
{
    // This is called by the coordinator when any component triggers emergency stop
    qWarning() << "AntiDetachmentMonitor handling emergency stop:" << reason;

    // Stop monitoring and deactivate
    stopMonitoring();
    deactivateAntiDetachment();

    // Log the event
    logEvent(QString("Emergency stop: %1").arg(reason), m_currentAVLPressure);
}

// ============================================================================
// Unified Safety Logging (replaces custom CSV logging)
// ============================================================================

void AntiDetachmentMonitor::setSafetyLogger(ISafetyLogger* logger)
{
    m_safetyLogger = logger;
    qDebug() << "ISafetyLogger linked to AntiDetachmentMonitor";
}

// ============================================================================
// Error Recovery (refactored to use StatefulComponent and SafeOperationHelper)
// ============================================================================

bool AntiDetachmentMonitor::resetSystemError()
{
    // Use StatefulComponent's getState() instead of direct m_currentState access
    if (getCurrentState() != SYSTEM_ERROR) {
        qDebug() << "No system error to reset";
        return true;
    }

    // Validate hardware is working before allowing reset
    if (!m_hardware || !m_hardware->isReady()) {
        m_lastError = "Cannot reset: Hardware not ready";
        qWarning() << m_lastError;
        return false;
    }

    // Use SafeOperationHelper for consistent error handling
    auto result = SafeOperationHelper::execute<double>(
        "resetSystemError", "AntiDetachmentMonitor",
        [this]() { return m_hardware->readAVLPressure(); },
        [this](const QString& err) { emit systemError(err); }
    );

    if (!result.isSuccess()) {
        m_lastError = result.error;
        return false;
    }

    if (!SafetyConstants::isValidPressure(result.get())) {
        m_lastError = "Cannot reset: Invalid pressure reading during validation";
        qWarning() << m_lastError;
        return false;
    }

    // Reset error counters
    m_consecutiveErrors = 0;
    m_lastError.clear();

    // Transition back to safe ATTACHED state
    setState(ATTACHED);

    // Log via unified logger
    logEvent("System error reset - monitoring resumed", m_currentAVLPressure);
    qDebug() << "System error reset successfully";

    return true;
}

// ============================================================================
// DEPRECATED: Legacy Safety Log Path (for backward compatibility)
// New code should use setSafetyLogger() with a DataLoggerSafetyAdapter
// ============================================================================

void AntiDetachmentMonitor::setSafetyLogPath(const QString& logPath)
{
    qWarning() << "setSafetyLogPath() is deprecated. Use setSafetyLogger() instead.";
    m_safetyLogPath = logPath;
    // Legacy path stored but not used - logging now goes through ISafetyLogger
}
