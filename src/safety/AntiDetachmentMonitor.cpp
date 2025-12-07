#include "AntiDetachmentMonitor.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <QThread>
#include <cmath>
#include <algorithm>

// Constants
const double AntiDetachmentMonitor::DEFAULT_DETACHMENT_THRESHOLD = 50.0;  // 50 mmHg
const double AntiDetachmentMonitor::DEFAULT_WARNING_THRESHOLD = 60.0;     // 60 mmHg
const double AntiDetachmentMonitor::DEFAULT_HYSTERESIS = 5.0;             // 5 mmHg
const int AntiDetachmentMonitor::DEFAULT_MONITORING_RATE_HZ = 100;        // 100 Hz
const int AntiDetachmentMonitor::DEFAULT_RESPONSE_DELAY_MS = 100;         // 100 ms
const double AntiDetachmentMonitor::DEFAULT_MAX_VACUUM_INCREASE = 20.0;   // 20%
const int AntiDetachmentMonitor::PRESSURE_HISTORY_SIZE = 10;              // 10 samples
const int AntiDetachmentMonitor::MAX_CONSECUTIVE_ERRORS = 3;              // 3 errors
const double AntiDetachmentMonitor::MIN_VALID_PRESSURE = 0.0;             // 0 mmHg
const double AntiDetachmentMonitor::MAX_VALID_PRESSURE = 200.0;           // 200 mmHg

AntiDetachmentMonitor::AntiDetachmentMonitor(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_active(false)
    , m_monitoring(false)
    , m_paused(false)
    , m_currentState(ATTACHED)
    , m_previousState(ATTACHED)
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
{
    // Set up monitoring timer for high-frequency monitoring
    m_monitoringTimer->setInterval(1000 / m_monitoringRateHz);  // Convert Hz to ms
    m_monitoringTimer->setTimerType(Qt::PreciseTimer);
    connect(m_monitoringTimer, &QTimer::timeout, this, &AntiDetachmentMonitor::performMonitoringCycle);
    
    // Set up response timer for delayed response
    m_responseTimer->setSingleShot(true);
    connect(m_responseTimer, &QTimer::timeout, this, &AntiDetachmentMonitor::onResponseTimer);
    
    qDebug() << "Anti-detachment monitor initialized with threshold:" << m_detachmentThreshold << "mmHg";
}

AntiDetachmentMonitor::~AntiDetachmentMonitor()
{
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
    if (thresholdMmHg > 0 && thresholdMmHg < MAX_VALID_PRESSURE) {
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
    if (thresholdMmHg > m_detachmentThreshold && thresholdMmHg < MAX_VALID_PRESSURE) {
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
        if (!validatePressureReading(testPressure)) {
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
        
        if (!validatePressureReading(avlPressure)) {
            m_consecutiveErrors++;
            if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
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
        
        if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
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
    DetachmentState newState = m_currentState;
    
    switch (m_currentState) {
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
    if (newState != m_currentState) {
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
    if (m_currentState != newState) {
        m_previousState = m_currentState;
        m_currentState = newState;
        
        qDebug() << QString("Anti-detachment state changed: %1 -> %2")
                    .arg(static_cast<int>(m_previousState))
                    .arg(static_cast<int>(newState));
        
        emit stateChanged(newState);
    }
}

bool AntiDetachmentMonitor::validatePressureReading(double pressure)
{
    return (pressure >= MIN_VALID_PRESSURE && pressure <= MAX_VALID_PRESSURE);
}

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
    // This would interface with the pump control system
    // For now, just log the action
    qDebug() << QString("Applying vacuum correction to reach %1 mmHg").arg(targetPressure, 0, 'f', 1);
}

void AntiDetachmentMonitor::updateStatistics()
{
    // Update statistics periodically
    // This could include trend analysis, performance metrics, etc.
}

void AntiDetachmentMonitor::logEvent(const QString& event, double pressure)
{
    QString logMessage = QString("[%1] %2 - Pressure: %3 mmHg")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                        .arg(event)
                        .arg(pressure, 0, 'f', 1);
    
    qInfo() << "ANTI-DETACHMENT:" << logMessage;
    
    // In a production system, this would write to a dedicated safety log file
}
