#ifndef GUIUPDATETHREAD_H
#define GUIUPDATETHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <QQueue>
#include <QWaitCondition>
#include "DataAcquisitionThread.h"

// Forward declarations

/**
 * @brief Dedicated thread for GUI updates and data processing
 * 
 * This thread handles GUI updates at a consistent frame rate while
 * processing high-frequency sensor data from the acquisition thread.
 * It provides:
 * - Smooth 30 FPS GUI updates
 * - Data filtering and processing
 * - Chart data preparation
 * - Thread-safe communication with GUI components
 */
class GuiUpdateThread : public QThread
{
    Q_OBJECT

public:
    struct ProcessedData {
        qint64 timestamp;
        double avlPressure;
        double tankPressure;
        double avlFiltered;
        double tankFiltered;
        bool alarmState;
        QString statusMessage;
        
        ProcessedData() : timestamp(0), avlPressure(0.0), tankPressure(0.0), 
                         avlFiltered(0.0), tankFiltered(0.0), alarmState(false) {}
    };

    explicit GuiUpdateThread(QObject *parent = nullptr);
    ~GuiUpdateThread();

    // Thread control
    void startThread();
    void stopThread();
    void startUpdates();
    void stopUpdates();
    void pauseUpdates();
    void resumeUpdates();
    
    // Configuration
    void setUpdateRate(int fps);
    int getUpdateRate() const { return m_updateRateFps; }
    
    void setFilterAlpha(double alpha);
    double getFilterAlpha() const { return m_filterAlpha; }
    
    // Data access
    ProcessedData getLatestProcessedData();
    QList<ProcessedData> getChartData(int maxPoints = -1);
    
    // Statistics
    double getActualUpdateRate() const { return m_actualUpdateRate; }
    double getFrameRate() const;
    int getFrameCount() const;
    int getProcessedSampleCount() const { return m_processedSamples; }
    
    // Status
    bool isUpdating() const { return m_updating; }
    bool isPaused() const { return m_paused; }

signals:
    void guiDataReady(const ProcessedData& data);
    void chartDataReady(const QList<ProcessedData>& data);
    void alarmStateChanged(bool alarmActive, const QString& message);
    void updateThreadStarted();
    void updateThreadStopped();
    void performanceUpdate(double frameRate, qint64 frameTime);

protected:
    void run() override;

private slots:
    void processNewData();
    void onSensorDataReady(const DataAcquisitionThread::SensorData& data);

private:
    void initializeThread();
    void cleanupThread();
    ProcessedData processRawData(const DataAcquisitionThread::SensorData& rawData);
    void applyFiltering(ProcessedData& data);
    void checkAlarmConditions(ProcessedData& data);
    void updateStatistics();
    void addToChartBuffer(const ProcessedData& data);
    void calculateFrameRate(qint64 currentTime);

    // Data source
    DataAcquisitionThread* m_dataThread;
    
    // Thread control
    bool m_updating;
    bool m_paused;
    bool m_stopRequested;
    mutable QMutex m_controlMutex;
    QWaitCondition m_pauseCondition;
    
    // Data processing
    QQueue<DataAcquisitionThread::SensorData> m_rawDataQueue;
    QQueue<ProcessedData> m_chartDataBuffer;
    mutable QMutex m_dataMutex;
    
    ProcessedData m_latestProcessedData;
    ProcessedData m_previousData;  // For filtering
    
    // Configuration
    int m_updateRateFps;
    int m_updateIntervalMs;
    double m_filterAlpha;  // Exponential moving average factor
    int m_maxChartPoints;
    
    // Alarm thresholds
    double m_warningThreshold;
    double m_criticalThreshold;
    bool m_currentAlarmState;
    
    // Statistics
    double m_actualUpdateRate;
    double m_averageFrameRate;
    qint64 m_lastStatisticsUpdate;
    qint64 m_lastFrameTime;
    int m_processedSamples;
    int m_updateCount;
    int m_frameCount;
    
    // Timer for GUI updates
    QTimer* m_updateTimer;
    
    // Constants
    static const int DEFAULT_UPDATE_RATE_FPS = 30;     // 30 FPS for smooth GUI
    static const double DEFAULT_FILTER_ALPHA;          // 0.2 for moderate filtering
    static const int DEFAULT_MAX_CHART_POINTS = 600;   // 20 seconds at 30 FPS
    static const int STATISTICS_UPDATE_INTERVAL_MS = 1000;
    static const double DEFAULT_WARNING_THRESHOLD;     // 80.0 mmHg
    static const double DEFAULT_CRITICAL_THRESHOLD;    // 95.0 mmHg
};

#endif // GUIUPDATETHREAD_H
