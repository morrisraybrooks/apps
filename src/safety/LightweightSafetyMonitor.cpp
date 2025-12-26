#include "LightweightSafetyMonitor.h"
#include "SafetyConstants.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>

// Constants - using centralized SafetyConstants for consistency
// NOTE: MAX_CONSECUTIVE_ERRORS, MIN_VALID_PRESSURE, MAX_VALID_PRESSURE
// are now accessed directly via SafetyConstants namespace
const double LightweightSafetyMonitor::DEFAULT_MAX_PRESSURE = SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG;
const double LightweightSafetyMonitor::DEFAULT_WARNING_THRESHOLD = SafetyConstants::WARNING_THRESHOLD_MMHG;
const int LightweightSafetyMonitor::DEFAULT_MONITORING_RATE_HZ = SafetyConstants::DEFAULT_MONITORING_RATE_HZ;

LightweightSafetyMonitor::LightweightSafetyMonitor(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_monitorTimer(new QTimer(this))
    , m_active(false)
    , m_monitoringRateHz(DEFAULT_MONITORING_RATE_HZ)
    , m_maxPressure(DEFAULT_MAX_PRESSURE)
    , m_warningThreshold(DEFAULT_WARNING_THRESHOLD)
    , m_checkCount(0)
    , m_averageCheckTime(0.0)
    , m_consecutiveErrors(0)
{
    // Configure timer for lightweight monitoring
    m_monitorTimer->setSingleShot(false);
    m_monitorTimer->setInterval(1000 / m_monitoringRateHz);
    m_monitorTimer->setTimerType(Qt::CoarseTimer);  // Use coarse timer to reduce CPU load
    
    // Connect timer to safety check
    connect(m_monitorTimer, &QTimer::timeout, this, &LightweightSafetyMonitor::performSafetyCheck);
    
    qDebug() << "Lightweight Safety Monitor initialized for EGLFS compatibility";
    qDebug() << QString("Safety thresholds: Max = %1 mmHg, Warning = %2 mmHg")
                .arg(m_maxPressure).arg(m_warningThreshold);
}

LightweightSafetyMonitor::~LightweightSafetyMonitor()
{
    stopMonitoring();
}

void LightweightSafetyMonitor::startMonitoring()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_active) {
        qWarning() << "Safety monitoring already active";
        return;
    }
    
    if (!m_hardware) {
        qCritical() << "Cannot start safety monitoring: Hardware manager not available";
        return;
    }
    
    m_active = true;
    m_consecutiveErrors = 0;
    m_checkCount = 0;
    m_performanceTimer.start();
    
    // Start monitoring with a slight delay to allow GUI to stabilize
    QTimer::singleShot(500, [this]() {
        if (m_active) {
            m_monitorTimer->start();
            qDebug() << "Lightweight safety monitoring started at" << m_monitoringRateHz << "Hz";
            emit monitoringStarted();
        }
    });
}

void LightweightSafetyMonitor::stopMonitoring()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_active) {
        return;
    }
    
    m_active = false;
    m_monitorTimer->stop();
    
    // Log performance statistics
    if (m_checkCount > 0) {
        qDebug() << QString("Safety monitoring stopped. Performed %1 checks, average time: %2 ms")
                    .arg(m_checkCount).arg(m_averageCheckTime, 0, 'f', 2);
    }
    
    emit monitoringStopped();
    qDebug() << "Lightweight safety monitoring stopped";
}

void LightweightSafetyMonitor::setMonitoringRate(int hz)
{
    QMutexLocker locker(&m_mutex);
    
    if (hz < 1 || hz > 50) {  // Limit to 50Hz max for GUI compatibility
        qWarning() << "Invalid monitoring rate:" << hz << "Hz. Must be 1-50 Hz";
        return;
    }
    
    m_monitoringRateHz = hz;
    m_monitorTimer->setInterval(1000 / hz);
    
    qDebug() << "Safety monitoring rate set to" << hz << "Hz (lightweight mode)";
}

void LightweightSafetyMonitor::performSafetyCheck()
{
    if (!m_active || !m_hardware) {
        return;
    }
    
    QElapsedTimer checkTimer;
    checkTimer.start();
    
    try {
        // Perform lightweight safety checks
        checkPressureLimits();
        checkSystemHealth();
        checkHardwareStatus();
        
        // Reset error count on successful check
        m_consecutiveErrors = 0;
        
    } catch (const std::exception& e) {
        m_consecutiveErrors++;
        m_lastError = QString("Safety check error: %1").arg(e.what());
        
        if (m_consecutiveErrors >= SafetyConstants::MAX_CONSECUTIVE_ERRORS) {
            emit emergencyStopRequired(QString("Too many consecutive safety errors: %1").arg(m_lastError));
        }
        
        qWarning() << m_lastError;
    }
    
    // Update performance statistics
    double checkTime = checkTimer.nsecsElapsed() / 1000000.0;  // Convert to milliseconds
    m_checkCount++;
    m_averageCheckTime = ((m_averageCheckTime * (m_checkCount - 1)) + checkTime) / m_checkCount;
}

void LightweightSafetyMonitor::checkPressureLimits()
{
    if (!m_hardware) return;
    
    // Read pressures (these calls should be fast)
    double avlPressure = m_hardware->readAVLPressure();
    double tankPressure = m_hardware->readTankPressure();
    
    // Validate readings using centralized SafetyConstants
    if (!SafetyConstants::isValidPressure(avlPressure) || !SafetyConstants::isValidPressure(tankPressure)) {
        throw std::runtime_error("Invalid pressure readings");
    }
    
    // Check AVL pressure
    if (avlPressure > m_maxPressure) {
        emit pressureAlarm(avlPressure, "AVL");
        emit safetyViolation(QString("AVL pressure alarm: %1 mmHg (max: %2)")
                            .arg(avlPressure, 0, 'f', 1).arg(m_maxPressure, 0, 'f', 1));
    } else if (avlPressure > m_warningThreshold) {
        emit pressureWarning(avlPressure, "AVL");
    }
    
    // Check tank pressure
    if (tankPressure > m_maxPressure) {
        emit pressureAlarm(tankPressure, "Tank");
        emit safetyViolation(QString("Tank pressure alarm: %1 mmHg (max: %2)")
                            .arg(tankPressure, 0, 'f', 1).arg(m_maxPressure, 0, 'f', 1));
    } else if (tankPressure > m_warningThreshold) {
        emit pressureWarning(tankPressure, "Tank");
    }
}

void LightweightSafetyMonitor::checkSystemHealth()
{
    if (!m_hardware) return;
    
    if (!m_hardware->isReady()) {
        emit systemHealthWarning("Hardware system not ready");
    }
}

void LightweightSafetyMonitor::checkHardwareStatus()
{
    // Lightweight hardware status check
    // This could be expanded to check specific hardware components
    if (!m_hardware) {
        throw std::runtime_error("Hardware manager unavailable");
    }
}

// NOTE: isValidPressure() removed - use SafetyConstants::isValidPressure() instead

void LightweightSafetyMonitor::logSafetyEvent(const QString& event)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    qDebug() << QString("[%1] Safety Event: %2").arg(timestamp, event);
}
