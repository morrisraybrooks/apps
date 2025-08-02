#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QJsonObject>
#include <QThread>
#include <QElapsedTimer>

/**
 * @brief System performance monitoring and optimization
 * 
 * This system provides:
 * - Real-time performance monitoring
 * - CPU and memory usage tracking
 * - Thread performance analysis
 * - GUI responsiveness monitoring
 * - I/O performance tracking
 * - Automatic performance optimization
 * - Performance bottleneck detection
 * - Resource usage alerts
 */
class PerformanceMonitor : public QObject
{
    Q_OBJECT

public:
    struct PerformanceMetrics {
        qint64 timestamp;
        double cpuUsage;            // CPU usage percentage
        qint64 memoryUsage;         // Memory usage in bytes
        qint64 memoryAvailable;     // Available memory in bytes
        double guiFrameRate;        // GUI frame rate (FPS)
        double dataAcquisitionRate; // Data acquisition rate (Hz)
        double safetyCheckRate;     // Safety check rate (Hz)
        int activeThreads;          // Number of active threads
        qint64 diskUsage;          // Disk usage in bytes
        double networkLatency;      // Network latency in ms
        QJsonObject customMetrics;  // Custom application metrics
        
        PerformanceMetrics() : timestamp(0), cpuUsage(0.0), memoryUsage(0), memoryAvailable(0),
                              guiFrameRate(0.0), dataAcquisitionRate(0.0), safetyCheckRate(0.0),
                              activeThreads(0), diskUsage(0), networkLatency(0.0) {}
    };

    struct PerformanceAlert {
        qint64 timestamp;
        QString category;
        QString message;
        QString severity;
        QJsonObject context;
        bool resolved;
        
        PerformanceAlert() : timestamp(0), resolved(false) {}
    };

    explicit PerformanceMonitor(QObject *parent = nullptr);
    ~PerformanceMonitor();

    // Monitoring control
    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();
    bool isMonitoring() const { return m_monitoring; }
    
    // Configuration
    void setMonitoringInterval(int intervalMs);
    void setMetricsHistorySize(int maxEntries);
    void setAlertThresholds(const QJsonObject& thresholds);
    
    // Performance data access
    PerformanceMetrics getCurrentMetrics() const;
    QList<PerformanceMetrics> getMetricsHistory(int maxEntries = -1) const;
    QList<PerformanceMetrics> getMetricsInTimeRange(const QDateTime& start, const QDateTime& end) const;
    
    // Performance analysis
    QJsonObject getPerformanceStatistics() const;
    QJsonObject getPerformanceTrends() const;
    QList<PerformanceAlert> getActiveAlerts() const;
    QList<PerformanceAlert> getAlertHistory() const;
    
    // Optimization
    void enableAutoOptimization(bool enabled);
    void optimizePerformance();
    void optimizeMemoryUsage();
    void optimizeThreadPriorities();
    void optimizeGUIPerformance();
    
    // Benchmarking
    void startBenchmark(const QString& benchmarkName);
    void endBenchmark(const QString& benchmarkName);
    QJsonObject getBenchmarkResults() const;
    
    // Custom metrics
    void addCustomMetric(const QString& name, double value);
    void removeCustomMetric(const QString& name);
    QJsonObject getCustomMetrics() const;

public Q_SLOTS:
    void collectMetrics();
    void checkPerformanceAlerts();
    void performOptimization();

Q_SIGNALS:
    void metricsUpdated(const PerformanceMetrics& metrics);
    void performanceAlert(const PerformanceAlert& alert);
    void performanceImproved(const QString& optimization, double improvement);
    void benchmarkCompleted(const QString& benchmarkName, const QJsonObject& results);

private Q_SLOTS:
    void onMonitoringTimer();
    void onOptimizationTimer();

private:
    void initializeMonitor();
    void setupPerformanceCounters();
    void collectSystemMetrics(PerformanceMetrics& metrics);
    void collectApplicationMetrics(PerformanceMetrics& metrics);
    void collectThreadMetrics(PerformanceMetrics& metrics);
    void collectGUIMetrics(PerformanceMetrics& metrics);
    
    // System monitoring
    double getCPUUsage();
    qint64 getMemoryUsage();
    qint64 getAvailableMemory();
    qint64 getDiskUsage();
    double getNetworkLatency();
    int getActiveThreadCount();
    
    // Application monitoring
    double getGUIFrameRate();
    double getDataAcquisitionRate();
    double getSafetyCheckRate();
    
    // Alert management
    void checkCPUAlert(const PerformanceMetrics& metrics);
    void checkMemoryAlert(const PerformanceMetrics& metrics);
    void checkGUIAlert(const PerformanceMetrics& metrics);
    void checkThreadAlert(const PerformanceMetrics& metrics);
    void addAlert(const QString& category, const QString& message, const QString& severity, const QJsonObject& context = QJsonObject());
    void resolveAlert(const QString& category);
    
    // Optimization strategies
    void optimizeCPUUsage();
    void optimizeMemoryFragmentation();
    void optimizeThreadScheduling();
    void optimizeGUIRendering();
    void optimizeDataBuffering();
    
    // Statistics calculation
    double calculateAverageMetric(const QString& metricName, int samples = 10) const;
    double calculateMetricTrend(const QString& metricName, int samples = 20) const;
    QJsonObject calculateMetricStatistics(const QString& metricName) const;
    
    // Benchmarking
    struct BenchmarkData {
        QString name;
        QElapsedTimer timer;
        QJsonObject results;
        bool active;
        
        BenchmarkData() : active(false) {}
    };
    
    // Monitoring state
    bool m_monitoring;
    bool m_paused;
    int m_monitoringInterval;
    int m_maxHistorySize;
    
    // Performance data
    QQueue<PerformanceMetrics> m_metricsHistory;
    QQueue<PerformanceAlert> m_alertHistory;
    QList<PerformanceAlert> m_activeAlerts;
    mutable QMutex m_dataMutex;
    
    // Timers
    QTimer* m_monitoringTimer;
    QTimer* m_optimizationTimer;
    
    // Alert thresholds
    double m_cpuThreshold;
    qint64 m_memoryThreshold;
    double m_guiFrameRateThreshold;
    double m_dataRateThreshold;
    
    // Optimization settings
    bool m_autoOptimizationEnabled;
    int m_optimizationInterval;
    QJsonObject m_optimizationSettings;
    
    // Custom metrics
    QMap<QString, double> m_customMetrics;
    mutable QMutex m_customMetricsMutex;

    // Benchmarking
    QMap<QString, BenchmarkData> m_activeBenchmarks;
    QMap<QString, QJsonObject> m_benchmarkResults;
    mutable QMutex m_benchmarkMutex;
    
    // Performance counters
    QElapsedTimer m_uptimeTimer;
    qint64 m_lastCPUTime;
    qint64 m_lastSystemTime;
    
    // Constants
    static const int DEFAULT_MONITORING_INTERVAL = 1000;    // 1 second
    static const int DEFAULT_MAX_HISTORY_SIZE = 3600;       // 1 hour of data
    static const int DEFAULT_OPTIMIZATION_INTERVAL = 60000; // 1 minute
    static const double DEFAULT_CPU_THRESHOLD;              // 80%
    static const qint64 DEFAULT_MEMORY_THRESHOLD;           // 1GB
    static const double DEFAULT_GUI_FRAMERATE_THRESHOLD;    // 25 FPS
    static const double DEFAULT_DATA_RATE_THRESHOLD;        // 45 Hz
};

#endif // PERFORMANCEMONITOR_H
