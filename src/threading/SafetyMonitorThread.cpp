#include "SafetyMonitorThread.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <stdexcept>

// Constants
const double SafetyMonitorThread::MAX_SAFE_PRESSURE = 100.0;
const double SafetyMonitorThread::MIN_SAFE_PRESSURE = 0.0;

SafetyMonitorThread::SafetyMonitorThread(HardwareManager* hardware, QObject *parent)
    : QThread(parent)
    , m_hardware(hardware)
    , m_monitorTimer(new QTimer(this))
    , m_monitoring(false)
    , m_stopRequested(false)
    , m_monitoringRateHz(DEFAULT_MONITORING_RATE_HZ)
{
    // Set up timer
    m_monitorTimer->setSingleShot(false);
    m_monitorTimer->setInterval(1000 / m_monitoringRateHz);
    
    // Connect timer to safety check
    connect(m_monitorTimer, &QTimer::timeout, this, &SafetyMonitorThread::performSafetyCheck);
}

SafetyMonitorThread::~SafetyMonitorThread()
{
    stopMonitoring();
    wait(5000);
}

void SafetyMonitorThread::startMonitoring()
{
    QMutexLocker locker(&m_mutex);
    if (!m_monitoring) {
        m_monitoring = true;
        m_stopRequested = false;
        if (!isRunning()) {
            start();
        }
        m_monitorTimer->start();
        emit monitoringStarted();
        qDebug() << "Safety monitoring started";
    }
}

void SafetyMonitorThread::stopMonitoring()
{
    QMutexLocker locker(&m_mutex);
    if (m_monitoring) {
        m_monitoring = false;
        m_stopRequested = true;
        m_monitorTimer->stop();
        emit monitoringStopped();
        qDebug() << "Safety monitoring stopped";
    }
}

void SafetyMonitorThread::setMonitoringRate(int rateHz)
{
    QMutexLocker locker(&m_mutex);
    if (rateHz > 0 && rateHz <= 100) {
        m_monitoringRateHz = rateHz;
        m_monitorTimer->setInterval(1000 / rateHz);
        qDebug() << "Safety monitoring rate set to" << rateHz << "Hz";
    }
}

void SafetyMonitorThread::run()
{
    qDebug() << "Safety Monitor Thread running";
    
    while (!m_stopRequested) {
        msleep(100); // Sleep for 100ms
        
        QMutexLocker locker(&m_mutex);
        if (m_stopRequested) {
            break;
        }
    }
    
    qDebug() << "Safety Monitor Thread stopped";
}

void SafetyMonitorThread::performSafetyCheck()
{
    if (!m_hardware || !m_monitoring) {
        return;
    }
    
    try {
        checkPressureLimits();
        checkEmergencyStop();
        checkSystemHealth();
    } catch (const std::exception& e) {
        emit safetyViolation(QString("Safety check error: %1").arg(e.what()));
    }
}

void SafetyMonitorThread::checkPressureLimits()
{
    if (!m_hardware) return;
    
    double avlPressure = m_hardware->readAVLPressure();
    double tankPressure = m_hardware->readTankPressure();
    
    if (avlPressure > MAX_SAFE_PRESSURE) {
        emit pressureAlarm(avlPressure, "AVL");
        emit safetyViolation(QString("AVL pressure too high: %1 mmHg").arg(avlPressure));
    }
    
    if (tankPressure > MAX_SAFE_PRESSURE) {
        emit pressureAlarm(tankPressure, "Tank");
        emit safetyViolation(QString("Tank pressure too high: %1 mmHg").arg(tankPressure));
    }
}

void SafetyMonitorThread::checkEmergencyStop()
{
    // This would check emergency stop button state
    // For now, just a placeholder
}

void SafetyMonitorThread::checkSystemHealth()
{
    if (!m_hardware) return;
    
    if (!m_hardware->isReady()) {
        emit safetyViolation("Hardware system not ready");
    }
}
