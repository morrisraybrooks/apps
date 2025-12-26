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
    , m_monitorTimer(nullptr)  // Will be created in run() on the worker thread
    , m_monitoring(false)
    , m_stopRequested(false)
    , m_monitoringRateHz(DEFAULT_MONITORING_RATE_HZ)
{
    // Note: Timer is created inside run() to ensure it lives on the worker thread.
    // This is the correct Qt threading pattern for QTimer with QThread::exec().
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

        // Start the thread - timer will be created and started inside run()
        if (!isRunning()) {
            start();  // Start thread first
            // Set priority after thread is running
            QTimer::singleShot(100, [this]() {
                if (isRunning()) {
                    setPriority(QThread::LowPriority);  // Set priority after thread starts
                }
            });
        }

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

        // Timer will be stopped and deleted in run() when event loop exits
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
        // Only update interval if timer exists (thread is running)
        if (m_monitorTimer) {
            m_monitorTimer->setInterval(1000 / rateHz);
        }
        qDebug() << "Safety monitoring rate set to" << rateHz << "Hz";
    }
}

void SafetyMonitorThread::run()
{
    qDebug() << "Safety Monitor Thread running with EGLFS compatibility";

    // Set thread name for debugging
    QThread::currentThread()->setObjectName("SafetyMonitor");

    // Create timer on the worker thread - this is the correct Qt pattern
    // Timer must be created here (inside run()) to be on the worker thread's event loop
    m_monitorTimer = new QTimer();  // No parent - will be deleted manually
    m_monitorTimer->setSingleShot(false);
    m_monitorTimer->setInterval(1000 / m_monitoringRateHz);
    m_monitorTimer->setTimerType(Qt::PreciseTimer);  // Ensure precise timing for safety

    // Connect timer to safety check - DirectConnection ensures execution on this thread
    connect(m_monitorTimer, &QTimer::timeout, this, &SafetyMonitorThread::performSafetyCheck, Qt::DirectConnection);

    // Signal that the thread is now running
    emit threadStarted();

    // Delay timer start to allow GUI to stabilize
    QTimer::singleShot(1000, m_monitorTimer, [this]() {
        if (m_monitoring && !m_stopRequested && m_monitorTimer) {
            m_monitorTimer->start();
            qDebug() << "Safety monitoring timer started (delayed for GUI stability)";
        }
    });

    // Run the event loop for timer-based monitoring
    exec();

    // Cleanup timer on worker thread before exiting
    if (m_monitorTimer) {
        m_monitorTimer->stop();
        delete m_monitorTimer;
        m_monitorTimer = nullptr;
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
