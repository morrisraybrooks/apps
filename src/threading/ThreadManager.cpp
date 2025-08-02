#include "ThreadManager.h"
#include "DataAcquisitionThread.h"
#include "GuiUpdateThread.h"
#include "SafetyMonitorThread.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>
#include <QDateTime>
#include <QCoreApplication>

ThreadManager::ThreadManager(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_overallState(STOPPED)
    , m_dataThreadRunning(false)
    , m_guiThreadRunning(false)
    , m_safetyThreadRunning(false)
    , m_errorCount(0)
    , m_emergencyStop(false)
    , m_dataAcquisitionRateHz(DEFAULT_DATA_RATE_HZ)
    , m_guiUpdateRateFps(DEFAULT_GUI_RATE_FPS)
    , m_safetyMonitorRateHz(DEFAULT_SAFETY_RATE_HZ)
{
    initializeThreads();
}

ThreadManager::~ThreadManager()
{
    stopAllThreads();
    cleanupThreads();
}

bool ThreadManager::startAllThreads()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_overallState == RUNNING) {
        qWarning() << "Threads already running";
        return true;
    }
    
    if (m_emergencyStop) {
        qWarning() << "Cannot start threads: Emergency stop active";
        return false;
    }
    
    qDebug() << "Starting all threads...";
    m_overallState = STARTING;
    
    try {
        // Start threads in order of dependency
        if (!startSafetyMonitoring()) {
            throw std::runtime_error("Failed to start safety monitoring thread");
        }
        
        if (!startDataAcquisition()) {
            throw std::runtime_error("Failed to start data acquisition thread");
        }
        
        if (!startGuiUpdates()) {
            throw std::runtime_error("Failed to start GUI update thread");
        }
        
        m_overallState = RUNNING;
        qDebug() << "All threads started successfully";
        emit allThreadsStarted();
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Thread startup failed: %1").arg(e.what());
        qCritical() << m_lastError;
        
        // Stop any threads that were started
        stopAllThreads();
        m_overallState = ERROR;
        emit threadError("ThreadManager", m_lastError);
        return false;
    }
}

void ThreadManager::stopAllThreads()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_overallState == STOPPED) {
        return;
    }
    
    qDebug() << "Stopping all threads...";
    m_overallState = STOPPING;
    
    // Stop threads in reverse order
    stopGuiUpdates();
    stopDataAcquisition();
    stopSafetyMonitoring();
    
    // Wait for all threads to stop
    if (waitForThreadsToStop()) {
        m_overallState = STOPPED;
        qDebug() << "All threads stopped successfully";
        emit allThreadsStopped();
    } else {
        qWarning() << "Some threads did not stop gracefully";
        m_overallState = ERROR;
    }
}

void ThreadManager::pauseAllThreads()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_overallState != RUNNING) {
        return;
    }
    
    qDebug() << "Pausing all threads...";
    m_overallState = PAUSING;
    
    if (m_dataThread) m_dataThread->pauseAcquisition();
    if (m_guiThread) m_guiThread->pauseUpdates();
    // Note: Safety thread continues running during pause
    
    m_overallState = PAUSED;
    qDebug() << "All threads paused";
}

void ThreadManager::resumeAllThreads()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_overallState != PAUSED) {
        return;
    }
    
    qDebug() << "Resuming all threads...";
    
    if (m_dataThread) m_dataThread->resumeAcquisition();
    if (m_guiThread) m_guiThread->resumeUpdates();
    
    m_overallState = RUNNING;
    qDebug() << "All threads resumed";
}

bool ThreadManager::startDataAcquisition()
{
    if (!m_dataThread || !m_hardware) {
        return false;
    }
    
    try {
        m_dataThread->setSamplingRate(m_dataAcquisitionRateHz);
        m_dataThread->startAcquisition();
        return true;
    } catch (const std::exception& e) {
        qCritical() << "Failed to start data acquisition:" << e.what();
        return false;
    }
}

bool ThreadManager::startGuiUpdates()
{
    if (!m_guiThread || !m_dataThread) {
        return false;
    }
    
    try {
        m_guiThread->setUpdateRate(m_guiUpdateRateFps);
        m_guiThread->startUpdates();
        return true;
    } catch (const std::exception& e) {
        qCritical() << "Failed to start GUI updates:" << e.what();
        return false;
    }
}

bool ThreadManager::startSafetyMonitoring()
{
    if (!m_safetyThread || !m_hardware) {
        return false;
    }
    
    try {
        m_safetyThread->setMonitoringRate(m_safetyMonitorRateHz);
        m_safetyThread->startMonitoring();
        return true;
    } catch (const std::exception& e) {
        qCritical() << "Failed to start safety monitoring:" << e.what();
        return false;
    }
}

void ThreadManager::stopDataAcquisition()
{
    if (m_dataThread) {
        m_dataThread->stopAcquisition();
    }
}

void ThreadManager::stopGuiUpdates()
{
    if (m_guiThread) {
        m_guiThread->stopUpdates();
    }
}

void ThreadManager::stopSafetyMonitoring()
{
    if (m_safetyThread) {
        m_safetyThread->stopMonitoring();
    }
}

bool ThreadManager::areAllThreadsRunning() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_dataThreadRunning && m_guiThreadRunning && m_safetyThreadRunning;
}

bool ThreadManager::areAllThreadsStopped() const
{
    QMutexLocker locker(&m_stateMutex);
    return !m_dataThreadRunning && !m_guiThreadRunning && !m_safetyThreadRunning;
}

void ThreadManager::setDataAcquisitionRate(int hz)
{
    m_dataAcquisitionRateHz = hz;
    if (m_dataThread) {
        m_dataThread->setSamplingRate(hz);
    }
}

void ThreadManager::setGuiUpdateRate(int fps)
{
    m_guiUpdateRateFps = fps;
    if (m_guiThread) {
        m_guiThread->setUpdateRate(fps);
    }
}

void ThreadManager::setSafetyMonitorRate(int hz)
{
    m_safetyMonitorRateHz = hz;
    if (m_safetyThread) {
        m_safetyThread->setMonitoringRate(hz);
    }
}

QString ThreadManager::getThreadStatistics() const
{
    QString stats;
    
    if (m_dataThread) {
        stats += QString("Data Acquisition: %1 Hz (actual: %2 Hz), Buffer: %3 samples\n")
                .arg(m_dataThread->getSamplingRate())
                .arg(m_dataThread->getActualSamplingRate(), 0, 'f', 1)
                .arg(m_dataThread->getBufferCount());
    }
    
    if (m_guiThread) {
        stats += QString("GUI Updates: %1 FPS (actual: %2 FPS), Processed: %3 samples\n")
                .arg(m_guiThread->getUpdateRate())
                .arg(m_guiThread->getActualUpdateRate(), 0, 'f', 1)
                .arg(m_guiThread->getProcessedSampleCount());
    }
    
    if (m_safetyThread) {
        stats += QString("Safety Monitoring: %1 Hz\n")
                .arg(m_safetyThread->getMonitoringRate());
    }
    
    stats += QString("Overall State: %1\n").arg(static_cast<int>(m_overallState));
    stats += QString("Error Count: %1").arg(m_errorCount);
    
    return stats;
}

int ThreadManager::getActiveThreadCount() const
{
    int count = 0;
    if (m_dataThreadRunning) count++;
    if (m_guiThreadRunning) count++;
    if (m_safetyThreadRunning) count++;
    return count;
}

void ThreadManager::emergencyStopAllThreads()
{
    qCritical() << "EMERGENCY STOP - Stopping all threads immediately";
    
    m_emergencyStop = true;
    
    // Force stop all threads
    if (m_dataThread) {
        m_dataThread->stopAcquisition();
    }
    if (m_guiThread) {
        m_guiThread->stopUpdates();
    }
    if (m_safetyThread) {
        m_safetyThread->stopMonitoring();
    }
    
    m_overallState = ERROR;
    emit emergencyStopTriggered();
}

bool ThreadManager::resetAfterEmergencyStop()
{
    if (!m_emergencyStop) {
        return true;  // Not in emergency stop
    }
    
    qDebug() << "Resetting after emergency stop...";
    
    // Ensure all threads are stopped
    stopAllThreads();
    
    // Reset error state
    m_emergencyStop = false;
    m_errorCount = 0;
    m_lastError.clear();
    
    // Reinitialize threads
    cleanupThreads();
    initializeThreads();
    
    m_overallState = STOPPED;
    qDebug() << "Emergency stop reset complete";
    return true;
}

void ThreadManager::initializeThreads()
{
    // Create data acquisition thread
    m_dataThread = std::make_unique<DataAcquisitionThread>(m_hardware);
    
    // Create GUI update thread
    m_guiThread = std::make_unique<GuiUpdateThread>(m_dataThread.get());
    
    // Create safety monitoring thread
    m_safetyThread = std::make_unique<SafetyMonitorThread>(m_hardware);
    
    // Connect signals
    connectThreadSignals();
    
    qDebug() << "Threads initialized";
}

void ThreadManager::connectThreadSignals()
{
    // Data acquisition thread signals
    if (m_dataThread) {
        connect(m_dataThread.get(), &DataAcquisitionThread::threadStarted,
                this, &ThreadManager::onDataThreadStarted);
        connect(m_dataThread.get(), &DataAcquisitionThread::threadStopped,
                this, &ThreadManager::onDataThreadStopped);
        connect(m_dataThread.get(), &DataAcquisitionThread::samplingError,
                this, &ThreadManager::onDataThreadError);
    }
    
    // GUI update thread signals
    if (m_guiThread) {
        connect(m_guiThread.get(), &GuiUpdateThread::updateThreadStarted,
                this, &ThreadManager::onGuiThreadStarted);
        connect(m_guiThread.get(), &GuiUpdateThread::updateThreadStopped,
                this, &ThreadManager::onGuiThreadStopped);
    }
    
    // Safety monitoring thread signals
    if (m_safetyThread) {
        connect(m_safetyThread.get(), &SafetyMonitorThread::monitoringStarted,
                this, &ThreadManager::onSafetyThreadStarted);
        connect(m_safetyThread.get(), &SafetyMonitorThread::monitoringStopped,
                this, &ThreadManager::onSafetyThreadStopped);
        connect(m_safetyThread.get(), &SafetyMonitorThread::monitoringError,
                this, &ThreadManager::onSafetyThreadError);
        connect(m_safetyThread.get(), &SafetyMonitorThread::emergencyStopRequired,
                this, &ThreadManager::emergencyStopAllThreads);
    }
}

void ThreadManager::cleanupThreads()
{
    m_safetyThread.reset();
    m_guiThread.reset();
    m_dataThread.reset();
}

bool ThreadManager::waitForThreadsToStop(int timeoutMs)
{
    QDateTime startTime = QDateTime::currentDateTime();
    
    while (getActiveThreadCount() > 0) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
        
        if (startTime.msecsTo(QDateTime::currentDateTime()) > timeoutMs) {
            qWarning() << "Timeout waiting for threads to stop";
            return false;
        }
    }
    
    return true;
}

// Slot implementations
void ThreadManager::onDataThreadStarted()
{
    m_dataThreadRunning = true;
    updateOverallState();
    emit threadStateChanged("DataAcquisition", RUNNING);
}

void ThreadManager::onDataThreadStopped()
{
    m_dataThreadRunning = false;
    updateOverallState();
    emit threadStateChanged("DataAcquisition", STOPPED);
}

void ThreadManager::onDataThreadError(const QString& error)
{
    m_errorCount++;
    emit threadError("DataAcquisition", error);
    
    if (m_errorCount >= MAX_THREAD_ERRORS) {
        emergencyStopAllThreads();
    }
}

void ThreadManager::onGuiThreadStarted()
{
    m_guiThreadRunning = true;
    updateOverallState();
    emit threadStateChanged("GuiUpdate", RUNNING);
}

void ThreadManager::onGuiThreadStopped()
{
    m_guiThreadRunning = false;
    updateOverallState();
    emit threadStateChanged("GuiUpdate", STOPPED);
}

void ThreadManager::onGuiThreadError(const QString& error)
{
    m_errorCount++;
    emit threadError("GuiUpdate", error);
}

void ThreadManager::onSafetyThreadStarted()
{
    m_safetyThreadRunning = true;
    updateOverallState();
    emit threadStateChanged("SafetyMonitor", RUNNING);
}

void ThreadManager::onSafetyThreadStopped()
{
    m_safetyThreadRunning = false;
    updateOverallState();
    emit threadStateChanged("SafetyMonitor", STOPPED);
}

void ThreadManager::onSafetyThreadError(const QString& error)
{
    m_errorCount++;
    emit threadError("SafetyMonitor", error);
    
    // Safety thread errors are critical
    emergencyStopAllThreads();
}

void ThreadManager::updateOverallState()
{
    // Update overall state based on individual thread states
    if (areAllThreadsRunning()) {
        if (m_overallState == STARTING) {
            m_overallState = RUNNING;
        }
    } else if (areAllThreadsStopped()) {
        if (m_overallState == STOPPING) {
            m_overallState = STOPPED;
        }
    }
}
