#include "GuiUpdateThread.h"
#include <QDebug>
#include <QApplication>
#include <QDateTime>

// Constants
const double GuiUpdateThread::DEFAULT_FILTER_ALPHA = 0.2;
const double GuiUpdateThread::DEFAULT_WARNING_THRESHOLD = 80.0;
const double GuiUpdateThread::DEFAULT_CRITICAL_THRESHOLD = 95.0;

GuiUpdateThread::GuiUpdateThread(QObject *parent)
    : QThread(parent)
    , m_dataThread(nullptr)
    , m_updating(false)
    , m_paused(false)
    , m_stopRequested(false)
    , m_updateRateFps(DEFAULT_UPDATE_RATE_FPS)
    , m_updateIntervalMs(1000 / DEFAULT_UPDATE_RATE_FPS)
    , m_filterAlpha(DEFAULT_FILTER_ALPHA)
    , m_warningThreshold(DEFAULT_WARNING_THRESHOLD)
    , m_criticalThreshold(DEFAULT_CRITICAL_THRESHOLD)
    , m_currentAlarmState(false)
    , m_actualUpdateRate(0.0)
    , m_averageFrameRate(0.0)
    , m_lastStatisticsUpdate(0)
    , m_lastFrameTime(0)
    , m_processedSamples(0)
    , m_updateCount(0)
    , m_frameCount(0)
    , m_updateTimer(new QTimer(this))
{
    // Set up update timer
    m_updateTimer->setSingleShot(false);
    m_updateTimer->setInterval(m_updateIntervalMs);
}

GuiUpdateThread::~GuiUpdateThread()
{
    stopThread();
    wait(5000); // Wait up to 5 seconds for thread to finish
}

void GuiUpdateThread::startThread()
{
    QMutexLocker locker(&m_controlMutex);
    if (!isRunning()) {
        m_stopRequested = false;
        m_updating = false;
        m_paused = false;
        m_lastStatisticsUpdate = QDateTime::currentMSecsSinceEpoch();
        QThread::start();
    }
}

void GuiUpdateThread::stopThread()
{
    {
        QMutexLocker locker(&m_controlMutex);
        m_stopRequested = true;
        m_updating = false;
        m_pauseCondition.wakeAll();
    }

    // Exit the event loop if running
    if (isRunning()) {
        quit();
    }
}





double GuiUpdateThread::getFrameRate() const
{
    return m_averageFrameRate;
}

int GuiUpdateThread::getFrameCount() const
{
    return m_frameCount;
}

void GuiUpdateThread::run()
{
    qDebug() << "GUI Update Thread running";
    emit updateThreadStarted();

    m_updating = true;
    qint64 frameStartTime = QDateTime::currentMSecsSinceEpoch();

    while (!m_stopRequested && m_updating) {
        QMutexLocker locker(&m_controlMutex);

        // Handle pause state
        if (m_paused) {
            m_pauseCondition.wait(&m_controlMutex);
            continue;
        }

        locker.unlock();

        // Process any new data
        processNewData();

        // Calculate frame rate
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        calculateFrameRate(currentTime);

        // Sleep for the remaining time to maintain target frame rate
        qint64 frameTime = currentTime - frameStartTime;

        if (frameTime < m_updateIntervalMs) {
            msleep(m_updateIntervalMs - frameTime);
        }

        // Check if we should emit performance metrics
        if (m_frameCount % 30 == 0) { // Every 30 frames (~1 second at 30 FPS)
            emit performanceUpdate(m_averageFrameRate, frameTime);
        }

        frameStartTime = currentTime;
    }

    m_updating = false;
    emit updateThreadStopped();
    qDebug() << "GUI Update Thread finished";
}

void GuiUpdateThread::processNewData()
{
    // This method would process new data from the data acquisition thread
    // For now, just update statistics
    updateStatistics();
}

void GuiUpdateThread::updateStatistics()
{
    m_updateCount++;
    m_processedSamples++;

    // Update actual update rate every second
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastStatisticsUpdate >= STATISTICS_UPDATE_INTERVAL_MS) {
        m_actualUpdateRate = m_updateCount * 1000.0 / (currentTime - m_lastStatisticsUpdate);
        m_updateCount = 0;
        m_lastStatisticsUpdate = currentTime;
    }
}

void GuiUpdateThread::startUpdates()
{
    startThread();
}

void GuiUpdateThread::stopUpdates()
{
    stopThread();
}

void GuiUpdateThread::pauseUpdates()
{
    QMutexLocker locker(&m_controlMutex);
    m_paused = true;
}

void GuiUpdateThread::resumeUpdates()
{
    QMutexLocker locker(&m_controlMutex);
    m_paused = false;
    m_pauseCondition.wakeAll();
}

void GuiUpdateThread::setUpdateRate(int fps)
{
    QMutexLocker locker(&m_controlMutex);
    m_updateRateFps = fps;
    m_updateIntervalMs = 1000 / fps;
}

void GuiUpdateThread::setFilterAlpha(double alpha)
{
    QMutexLocker locker(&m_controlMutex);
    m_filterAlpha = alpha;
}

void GuiUpdateThread::onSensorDataReady(const DataAcquisitionThread::SensorData& data)
{
    // Handle new sensor data
    QMutexLocker locker(&m_dataMutex);
    m_rawDataQueue.enqueue(data);

    // Limit queue size
    while (m_rawDataQueue.size() > 100) {
        m_rawDataQueue.dequeue();
    }
}

void GuiUpdateThread::calculateFrameRate(qint64 currentTime)
{
    m_frameCount++;
    
    if (m_lastFrameTime == 0) {
        m_lastFrameTime = currentTime;
        return;
    }
    
    qint64 timeDelta = currentTime - m_lastFrameTime;
    
    if (timeDelta > 0) {
        double instantFrameRate = 1000.0 / timeDelta;
        
        // Use exponential moving average for smooth frame rate calculation
        if (m_averageFrameRate == 0.0) {
            m_averageFrameRate = instantFrameRate;
        } else {
            m_averageFrameRate = 0.9 * m_averageFrameRate + 0.1 * instantFrameRate;
        }
    }
    
    m_lastFrameTime = currentTime;
}
