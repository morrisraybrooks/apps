#ifndef DATAACQUISITIONTHREAD_H
#define DATAACQUISITIONTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QQueue>
#include <QDateTime>

// Forward declarations
class HardwareManager;

/**
 * @brief High-priority thread for real-time sensor data acquisition
 * 
 * This thread runs at high priority to ensure consistent sensor sampling
 * for the safety-critical vacuum controller system. It provides:
 * - Consistent 50Hz sensor sampling rate
 * - Thread-safe data buffering
 * - Automatic error detection and recovery
 * - Minimal latency for safety systems
 */
class DataAcquisitionThread : public QThread
{
    Q_OBJECT

public:
    struct SensorData {
        qint64 timestamp;
        double avlPressure;
        double tankPressure;
        bool valid;
        
        SensorData() : timestamp(0), avlPressure(0.0), tankPressure(0.0), valid(false) {}
        SensorData(qint64 ts, double avl, double tank, bool v) 
            : timestamp(ts), avlPressure(avl), tankPressure(tank), valid(v) {}
    };

    explicit DataAcquisitionThread(HardwareManager* hardware, QObject *parent = nullptr);
    ~DataAcquisitionThread();

    // Thread control
    void startAcquisition();
    void stopAcquisition();
    void pauseAcquisition();
    void resumeAcquisition();
    
    // Configuration
    void setSamplingRate(int hz);
    int getSamplingRate() const { return m_samplingRateHz; }
    
    void setBufferSize(int maxSamples);
    int getBufferSize() const { return m_maxBufferSize; }
    
    // Data access (thread-safe)
    SensorData getLatestData();
    QList<SensorData> getBufferedData(int maxSamples = -1);
    void clearBuffer();
    
    // Statistics
    int getBufferCount() const;
    double getActualSamplingRate() const { return m_actualSamplingRate; }
    int getErrorCount() const { return m_errorCount; }
    qint64 getLastUpdateTime() const { return m_lastUpdateTime; }
    
    // Thread status
    bool isAcquiring() const { return m_acquiring; }
    bool isPaused() const { return m_paused; }

signals:
    void dataReady(const SensorData& data);
    void bufferFull();
    void samplingError(const QString& error);
    void threadStarted();
    void threadStopped();

protected:
    void run() override;

private slots:
    void performDataAcquisition();

private:
    void initializeThread();
    void cleanupThread();
    SensorData acquireSensorData();
    void addToBuffer(const SensorData& data);
    void updateStatistics();

    // Hardware interface
    HardwareManager* m_hardware;
    
    // Thread control
    bool m_acquiring;
    bool m_paused;
    bool m_stopRequested;
    mutable QMutex m_controlMutex;
    QWaitCondition m_pauseCondition;
    
    // Data buffer (thread-safe)
    QQueue<SensorData> m_dataBuffer;
    mutable QMutex m_bufferMutex;
    int m_maxBufferSize;
    
    // Timing and statistics
    int m_samplingRateHz;
    int m_samplingIntervalMs;
    double m_actualSamplingRate;
    qint64 m_lastUpdateTime;
    qint64 m_lastStatisticsUpdate;
    int m_sampleCount;
    int m_errorCount;
    
    // High-resolution timer for precise timing
    QTimer* m_acquisitionTimer;
    
    // Constants
    static const int DEFAULT_SAMPLING_RATE_HZ = 50;    // 50Hz for smooth real-time updates
    static const int DEFAULT_BUFFER_SIZE = 1000;       // 20 seconds at 50Hz
    static const int STATISTICS_UPDATE_INTERVAL_MS = 1000;  // Update stats every second
    static const int MAX_CONSECUTIVE_ERRORS = 10;      // Max errors before stopping
};

#endif // DATAACQUISITIONTHREAD_H
