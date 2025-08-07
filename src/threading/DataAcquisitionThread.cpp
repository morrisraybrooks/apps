#include "DataAcquisitionThread.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QCoreApplication>
#include <cmath>

DataAcquisitionThread::DataAcquisitionThread(HardwareManager* hardware, QObject *parent)
    : QThread(parent)
    , m_hardware(hardware)
    , m_acquiring(false)
    , m_paused(false)
    , m_stopRequested(false)
    , m_maxBufferSize(DEFAULT_BUFFER_SIZE)
    , m_samplingRateHz(DEFAULT_SAMPLING_RATE_HZ)
    , m_samplingIntervalMs(1000 / DEFAULT_SAMPLING_RATE_HZ)
    , m_actualSamplingRate(0.0)
    , m_lastUpdateTime(0)
    , m_lastStatisticsUpdate(0)
    , m_sampleCount(0)
    , m_errorCount(0)
    , m_acquisitionTimer(nullptr)
    , m_safetyEnabled(true)
    , m_safetyCheckCounter(0)
    , m_safetyCheckInterval(1)  // Check safety every sample by default
    , m_maxPressure(100.0)
    , m_warningThreshold(80.0)
    , m_consecutiveSafetyErrors(0)
{
    // Set thread priority for real-time performance (but not highest to avoid GUI conflicts)
    setPriority(QThread::HighPriority);
}

DataAcquisitionThread::~DataAcquisitionThread()
{
    stopAcquisition();
    wait(3000);  // Wait up to 3 seconds for thread to finish
}

void DataAcquisitionThread::startAcquisition()
{
    QMutexLocker locker(&m_controlMutex);
    
    if (m_acquiring) {
        qWarning() << "Data acquisition already running";
        return;
    }
    
    if (!m_hardware || !m_hardware->isReady()) {
        qCritical() << "Hardware not ready for data acquisition";
        emit samplingError("Hardware not ready");
        return;
    }
    
    m_acquiring = true;
    m_paused = false;
    m_stopRequested = false;
    m_errorCount = 0;
    m_sampleCount = 0;
    
    // Clear existing buffer
    {
        QMutexLocker bufferLocker(&m_bufferMutex);
        m_dataBuffer.clear();
    }
    
    // Start the thread
    start();
    
    qDebug() << QString("Data acquisition started at %1 Hz").arg(m_samplingRateHz);
}

void DataAcquisitionThread::stopAcquisition()
{
    {
        QMutexLocker locker(&m_controlMutex);
        if (!m_acquiring) return;

        m_stopRequested = true;
        m_acquiring = false;
        m_pauseCondition.wakeAll();
    }

    // Stop the timer first to prevent new acquisitions
    if (m_acquisitionTimer) {
        m_acquisitionTimer->stop();
    }

    // Exit the event loop to stop the thread
    quit();

    // Wait for thread to finish
    if (!wait(3000)) {
        qWarning() << "Data acquisition thread did not stop gracefully, terminating";
        terminate();
        wait(1000);
    }

    qDebug() << "Data acquisition stopped";
    emit threadStopped();
}

void DataAcquisitionThread::pauseAcquisition()
{
    QMutexLocker locker(&m_controlMutex);
    if (m_acquiring && !m_paused) {
        m_paused = true;
        qDebug() << "Data acquisition paused";
    }
}

void DataAcquisitionThread::resumeAcquisition()
{
    QMutexLocker locker(&m_controlMutex);
    if (m_acquiring && m_paused) {
        m_paused = false;
        m_pauseCondition.wakeAll();
        qDebug() << "Data acquisition resumed";
    }
}

void DataAcquisitionThread::setSamplingRate(int hz)
{
    if (hz > 0 && hz <= 1000) {  // Reasonable limits
        QMutexLocker locker(&m_controlMutex);
        m_samplingRateHz = hz;
        m_samplingIntervalMs = 1000 / hz;
        qDebug() << QString("Sampling rate set to %1 Hz").arg(hz);
    }
}

void DataAcquisitionThread::setBufferSize(int maxSamples)
{
    if (maxSamples > 0) {
        QMutexLocker locker(&m_bufferMutex);
        m_maxBufferSize = maxSamples;
        
        // Trim buffer if necessary
        while (m_dataBuffer.size() > m_maxBufferSize) {
            m_dataBuffer.dequeue();
        }
        
        qDebug() << QString("Buffer size set to %1 samples").arg(maxSamples);
    }
}

DataAcquisitionThread::SensorData DataAcquisitionThread::getLatestData()
{
    QMutexLocker locker(&m_bufferMutex);
    
    if (m_dataBuffer.isEmpty()) {
        return SensorData();  // Invalid data
    }
    
    return m_dataBuffer.last();
}

QList<DataAcquisitionThread::SensorData> DataAcquisitionThread::getBufferedData(int maxSamples)
{
    QMutexLocker locker(&m_bufferMutex);
    
    QList<SensorData> result;
    
    int count = (maxSamples > 0) ? std::min(maxSamples, m_dataBuffer.size()) : m_dataBuffer.size();
    
    // Get the most recent samples
    auto it = m_dataBuffer.end() - count;
    for (; it != m_dataBuffer.end(); ++it) {
        result.append(*it);
    }
    
    return result;
}

void DataAcquisitionThread::clearBuffer()
{
    QMutexLocker locker(&m_bufferMutex);
    m_dataBuffer.clear();
    qDebug() << "Data buffer cleared";
}

int DataAcquisitionThread::getBufferCount() const
{
    QMutexLocker locker(&m_bufferMutex);
    return m_dataBuffer.size();
}

void DataAcquisitionThread::run()
{
    qDebug() << "Data acquisition thread started";
    emit threadStarted();
    
    initializeThread();
    
    // Create timer for precise timing
    m_acquisitionTimer = new QTimer();
    m_acquisitionTimer->setInterval(m_samplingIntervalMs);
    m_acquisitionTimer->setTimerType(Qt::PreciseTimer);
    
    connect(m_acquisitionTimer, &QTimer::timeout, this, &DataAcquisitionThread::performDataAcquisition);
    
    m_acquisitionTimer->start();
    
    // Run event loop for timer
    exec();
    
    // Cleanup
    if (m_acquisitionTimer) {
        m_acquisitionTimer->stop();
        delete m_acquisitionTimer;
        m_acquisitionTimer = nullptr;
    }
    
    cleanupThread();
    qDebug() << "Data acquisition thread finished";
}

void DataAcquisitionThread::performDataAcquisition()
{
    // Check if we should stop
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_stopRequested) {
            quit();
            return;
        }
        
        // Handle pause
        if (m_paused) {
            m_pauseCondition.wait(&m_controlMutex);
            if (m_stopRequested) {
                quit();
                return;
            }
        }
    }
    
    // Acquire sensor data
    SensorData data = acquireSensorData();

    if (data.valid) {
        // Add to buffer
        addToBuffer(data);

        // Perform integrated safety monitoring
        if (m_safetyEnabled) {
            performIntegratedSafetyCheck(data);
        }

        // Emit signal for real-time updates
        emit dataReady(data);

        m_sampleCount++;
        m_lastUpdateTime = data.timestamp;

        // Update statistics periodically
        updateStatistics();

    } else {
        m_errorCount++;

        // Stop acquisition if too many consecutive errors
        if (m_errorCount >= MAX_CONSECUTIVE_ERRORS) {
            emit samplingError(QString("Too many consecutive errors (%1), stopping acquisition").arg(m_errorCount));
            stopAcquisition();
        }
    }
}

void DataAcquisitionThread::initializeThread()
{
    m_lastStatisticsUpdate = QDateTime::currentMSecsSinceEpoch();
    m_sampleCount = 0;
    m_errorCount = 0;
}

void DataAcquisitionThread::cleanupThread()
{
    // Thread cleanup if needed
}

DataAcquisitionThread::SensorData DataAcquisitionThread::acquireSensorData()
{
    if (!m_hardware) {
        return SensorData();  // Invalid
    }
    
    try {
        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        double avlPressure = m_hardware->readAVLPressure();
        double tankPressure = m_hardware->readTankPressure();
        
        // Validate readings
        bool valid = (avlPressure >= 0 && tankPressure >= 0);
        
        return SensorData(timestamp, avlPressure, tankPressure, valid);
        
    } catch (const std::exception& e) {
        emit samplingError(QString("Sensor acquisition error: %1").arg(e.what()));
        return SensorData();  // Invalid
    }
}

void DataAcquisitionThread::addToBuffer(const SensorData& data)
{
    QMutexLocker locker(&m_bufferMutex);
    
    // Add new data
    m_dataBuffer.enqueue(data);
    
    // Remove old data if buffer is full
    while (m_dataBuffer.size() > m_maxBufferSize) {
        m_dataBuffer.dequeue();
    }
    
    // Emit buffer full signal if at capacity
    if (m_dataBuffer.size() >= m_maxBufferSize) {
        emit bufferFull();
    }
}

void DataAcquisitionThread::updateStatistics()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (currentTime - m_lastStatisticsUpdate >= STATISTICS_UPDATE_INTERVAL_MS) {
        // Calculate actual sampling rate
        qint64 timeDiff = currentTime - m_lastStatisticsUpdate;
        if (timeDiff > 0) {
            m_actualSamplingRate = (m_sampleCount * 1000.0) / timeDiff;
        }

        m_lastStatisticsUpdate = currentTime;
        m_sampleCount = 0;  // Reset for next interval
    }
}

void DataAcquisitionThread::performIntegratedSafetyCheck(const SensorData& data)
{
    // Only perform safety check at specified intervals to reduce overhead
    m_safetyCheckCounter++;
    if (m_safetyCheckCounter < m_safetyCheckInterval) {
        return;
    }
    m_safetyCheckCounter = 0;

    try {
        // Check pressure limits using the fresh sensor data
        bool safetyViolation = false;

        // Check AVL pressure
        if (data.avlPressure > m_maxPressure) {
            emit safetyAlarm(QString("AVL pressure alarm: %1 mmHg (max: %2)")
                                .arg(data.avlPressure, 0, 'f', 1).arg(m_maxPressure, 0, 'f', 1));
            safetyViolation = true;
        } else if (data.avlPressure > m_warningThreshold) {
            emit safetyWarning(QString("AVL pressure warning: %1 mmHg")
                              .arg(data.avlPressure, 0, 'f', 1));
        }

        // Check tank pressure
        if (data.tankPressure > m_maxPressure) {
            emit safetyAlarm(QString("Tank pressure alarm: %1 mmHg (max: %2)")
                                .arg(data.tankPressure, 0, 'f', 1).arg(m_maxPressure, 0, 'f', 1));
            safetyViolation = true;
        } else if (data.tankPressure > m_warningThreshold) {
            emit safetyWarning(QString("Tank pressure warning: %1 mmHg")
                              .arg(data.tankPressure, 0, 'f', 1));
        }

        // Check for invalid readings
        if (data.avlPressure < 0 || data.tankPressure < 0 ||
            data.avlPressure > 200.0 || data.tankPressure > 200.0) {
            emit safetyWarning("Invalid pressure readings detected");
            m_consecutiveSafetyErrors++;
        } else {
            m_consecutiveSafetyErrors = 0;  // Reset on valid readings
        }

        // Check hardware status
        if (m_hardware && !m_hardware->isReady()) {
            emit safetyWarning("Hardware system not ready");
            m_consecutiveSafetyErrors++;
        }

        // Trigger emergency stop if too many consecutive safety errors
        if (m_consecutiveSafetyErrors >= MAX_CONSECUTIVE_SAFETY_ERRORS) {
            emit emergencyStopRequired("Too many consecutive safety errors");
        }

    } catch (const std::exception& e) {
        emit safetyWarning(QString("Safety check error: %1").arg(e.what()));
        m_consecutiveSafetyErrors++;
    }
}

void DataAcquisitionThread::setSafetyEnabled(bool enabled)
{
    QMutexLocker locker(&m_controlMutex);
    m_safetyEnabled = enabled;
    if (enabled) {
        qDebug() << "Integrated safety monitoring enabled in data acquisition thread";
    } else {
        qDebug() << "Integrated safety monitoring disabled";
    }
}

void DataAcquisitionThread::setSafetyThresholds(double maxPressure, double warningThreshold)
{
    QMutexLocker locker(&m_controlMutex);
    m_maxPressure = maxPressure;
    m_warningThreshold = warningThreshold;
    qDebug() << QString("Safety thresholds updated: Max = %1 mmHg, Warning = %2 mmHg")
                .arg(m_maxPressure).arg(m_warningThreshold);
}

void DataAcquisitionThread::setSafetyCheckInterval(int interval)
{
    QMutexLocker locker(&m_controlMutex);
    m_safetyCheckInterval = qMax(1, interval);  // Minimum interval of 1
    qDebug() << "Safety check interval set to every" << m_safetyCheckInterval << "samples";
}
