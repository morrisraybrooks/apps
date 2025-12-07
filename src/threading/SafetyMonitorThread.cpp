#include "SafetyMonitorThread.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>
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
    // Set up timer with precise timing for safety monitoring
    m_monitorTimer->setSingleShot(false);
    m_monitorTimer->setInterval(1000 / m_monitoringRateHz);
    m_monitorTimer->setTimerType(Qt::PreciseTimer);  // Ensure precise timing

    // Connect timer to safety check
    connect(m_monitorTimer, &QTimer::timeout, this, &SafetyMonitorThread::performSafetyCheck, Qt::DirectConnection);
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

        // Start the thread with lower priority to avoid GUI conflicts
        if (!isRunning()) {
            start();  // Start thread first
            // Set priority after thread is running
            QTimer::singleShot(100, [this]() {
                if (isRunning()) {
                    setPriority(QThread::LowPriority);  // Set priority after thread starts
                }
            });
        }

        // Delay timer start to allow GUI to stabilize
        QTimer::singleShot(1000, [this]() {
            if (m_monitoring && !m_stopRequested) {
                m_monitorTimer->start();
                qDebug() << "Safety monitoring timer started (delayed for GUI stability)";
            }
        });

        emit monitoringStarted();
        qDebug() << "Safety monitoring started with EGLFS compatibility";
    }
}

void SafetyMonitorThread::stopMonitoring()
{
    QMutexLocker locker(&m_mutex);
    if (m_monitoring) {
        m_monitoring = false;
        m_stopRequested = true;
        m_monitorTimer->stop();

        // Exit the event loop to stop the thread
        if (isRunning()) {
            quit();
        }

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
    qDebug() << "Safety Monitor Thread running with EGLFS compatibility";

    // Set thread name for debugging
    QThread::currentThread()->setObjectName("SafetyMonitor");

    // Move timer to this thread's event loop
    m_monitorTimer->moveToThread(this);

    // Signal that the thread is now running
    emit threadStarted();

    // Run the event loop for timer-based monitoring
    exec();

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
