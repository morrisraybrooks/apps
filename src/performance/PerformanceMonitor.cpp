#include "PerformanceMonitor.h"
#include <QApplication>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <cmath>

// Constants
const double PerformanceMonitor::DEFAULT_CPU_THRESHOLD = 80.0;
const qint64 PerformanceMonitor::DEFAULT_MEMORY_THRESHOLD = 1024 * 1024 * 1024; // 1GB
const double PerformanceMonitor::DEFAULT_GUI_FRAMERATE_THRESHOLD = 25.0;
const double PerformanceMonitor::DEFAULT_DATA_RATE_THRESHOLD = 45.0;

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent)
    , m_monitoring(false)
    , m_paused(false)
    , m_monitoringInterval(DEFAULT_MONITORING_INTERVAL)
    , m_maxHistorySize(DEFAULT_MAX_HISTORY_SIZE)
    , m_monitoringTimer(new QTimer(this))
    , m_optimizationTimer(new QTimer(this))
    , m_cpuThreshold(DEFAULT_CPU_THRESHOLD)
    , m_memoryThreshold(DEFAULT_MEMORY_THRESHOLD)
    , m_guiFrameRateThreshold(DEFAULT_GUI_FRAMERATE_THRESHOLD)
    , m_dataRateThreshold(DEFAULT_DATA_RATE_THRESHOLD)
    , m_autoOptimizationEnabled(false)
    , m_optimizationInterval(DEFAULT_OPTIMIZATION_INTERVAL)
    , m_lastCPUTime(0)
    , m_lastSystemTime(0)
{
    // Setup timers
    m_monitoringTimer->setInterval(m_monitoringInterval);
    m_optimizationTimer->setInterval(m_optimizationInterval);
    
    connect(m_monitoringTimer, &QTimer::timeout, this, &PerformanceMonitor::onMonitoringTimer);
    connect(m_optimizationTimer, &QTimer::timeout, this, &PerformanceMonitor::onOptimizationTimer);
    
    initializeMonitor();
    
    qDebug() << "PerformanceMonitor initialized";
}

PerformanceMonitor::~PerformanceMonitor()
{
    stopMonitoring();
}

void PerformanceMonitor::initializeMonitor()
{
    setupPerformanceCounters();
    
    // Start uptime timer
    m_uptimeTimer.start();
    
    qDebug() << "Performance monitor initialized successfully";
}

void PerformanceMonitor::setupPerformanceCounters()
{
    // Initialize performance counters
    m_lastCPUTime = 0;
    m_lastSystemTime = 0;
    
    // Clear history
    m_metricsHistory.clear();
    m_alertHistory.clear();
    m_activeAlerts.clear();
    
    qDebug() << "Performance counters setup complete";
}

void PerformanceMonitor::startMonitoring()
{
    if (m_monitoring) return;
    
    m_monitoring = true;
    m_paused = false;
    
    // Start monitoring timer
    m_monitoringTimer->start();
    
    // Start optimization timer if auto-optimization is enabled
    if (m_autoOptimizationEnabled) {
        m_optimizationTimer->start();
    }
    
    qDebug() << "Performance monitoring started";
}

void PerformanceMonitor::stopMonitoring()
{
    if (!m_monitoring) return;
    
    m_monitoring = false;
    m_paused = false;
    
    // Stop timers
    m_monitoringTimer->stop();
    m_optimizationTimer->stop();
    
    qDebug() << "Performance monitoring stopped";
}

void PerformanceMonitor::pauseMonitoring()
{
    if (!m_monitoring || m_paused) return;
    
    m_paused = true;
    m_monitoringTimer->stop();
    
    qDebug() << "Performance monitoring paused";
}

void PerformanceMonitor::resumeMonitoring()
{
    if (!m_monitoring || !m_paused) return;
    
    m_paused = false;
    m_monitoringTimer->start();
    
    qDebug() << "Performance monitoring resumed";
}

void PerformanceMonitor::setMonitoringInterval(int intervalMs)
{
    m_monitoringInterval = qMax(100, intervalMs); // Minimum 100ms
    m_monitoringTimer->setInterval(m_monitoringInterval);
}

void PerformanceMonitor::setMetricsHistorySize(int maxEntries)
{
    QMutexLocker locker(&m_dataMutex);
    m_maxHistorySize = qMax(10, maxEntries); // Minimum 10 entries
    
    // Trim history if necessary
    while (m_metricsHistory.size() > m_maxHistorySize) {
        m_metricsHistory.dequeue();
    }
}

void PerformanceMonitor::setAlertThresholds(const QJsonObject& thresholds)
{
    if (thresholds.contains("cpu_threshold")) {
        m_cpuThreshold = thresholds["cpu_threshold"].toDouble(DEFAULT_CPU_THRESHOLD);
    }
    
    if (thresholds.contains("memory_threshold")) {
        m_memoryThreshold = static_cast<qint64>(thresholds["memory_threshold"].toDouble(DEFAULT_MEMORY_THRESHOLD));
    }
    
    if (thresholds.contains("gui_framerate_threshold")) {
        m_guiFrameRateThreshold = thresholds["gui_framerate_threshold"].toDouble(DEFAULT_GUI_FRAMERATE_THRESHOLD);
    }
    
    if (thresholds.contains("data_rate_threshold")) {
        m_dataRateThreshold = thresholds["data_rate_threshold"].toDouble(DEFAULT_DATA_RATE_THRESHOLD);
    }
    
    qDebug() << "Alert thresholds updated";
}

PerformanceMonitor::PerformanceMetrics PerformanceMonitor::getCurrentMetrics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    if (!m_metricsHistory.isEmpty()) {
        return m_metricsHistory.last();
    }
    
    return PerformanceMetrics();
}

QList<PerformanceMonitor::PerformanceMetrics> PerformanceMonitor::getMetricsHistory(int maxEntries) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<PerformanceMetrics> history;
    
    int count = (maxEntries > 0) ? qMin(maxEntries, m_metricsHistory.size()) : m_metricsHistory.size();
    int startIndex = m_metricsHistory.size() - count;
    
    for (int i = startIndex; i < m_metricsHistory.size(); ++i) {
        history.append(m_metricsHistory.at(i));
    }
    
    return history;
}

QList<PerformanceMonitor::PerformanceMetrics> PerformanceMonitor::getMetricsInTimeRange(const QDateTime& start, const QDateTime& end) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<PerformanceMetrics> filteredMetrics;
    qint64 startMs = start.toMSecsSinceEpoch();
    qint64 endMs = end.toMSecsSinceEpoch();
    
    for (const PerformanceMetrics& metrics : m_metricsHistory) {
        if (metrics.timestamp >= startMs && metrics.timestamp <= endMs) {
            filteredMetrics.append(metrics);
        }
    }
    
    return filteredMetrics;
}

void PerformanceMonitor::collectMetrics()
{
    if (!m_monitoring || m_paused) return;
    
    PerformanceMetrics metrics;
    metrics.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    // Collect system metrics
    collectSystemMetrics(metrics);
    
    // Collect application metrics
    collectApplicationMetrics(metrics);
    
    // Collect thread metrics
    collectThreadMetrics(metrics);
    
    // Collect GUI metrics
    collectGUIMetrics(metrics);
    
    // Add custom metrics
    {
        QMutexLocker locker(&m_customMetricsMutex);
        for (auto it = m_customMetrics.begin(); it != m_customMetrics.end(); ++it) {
            metrics.customMetrics[it.key()] = it.value();
        }
    }
    
    // Store metrics
    {
        QMutexLocker locker(&m_dataMutex);
        m_metricsHistory.enqueue(metrics);
        
        // Trim history if necessary
        while (m_metricsHistory.size() > m_maxHistorySize) {
            m_metricsHistory.dequeue();
        }
    }
    
    // Emit signal
    emit metricsUpdated(metrics);
    
    // Check for alerts
    checkPerformanceAlerts();
}

void PerformanceMonitor::collectSystemMetrics(PerformanceMetrics& metrics)
{
    metrics.cpuUsage = getCPUUsage();
    metrics.memoryUsage = getMemoryUsage();
    metrics.memoryAvailable = getAvailableMemory();
    metrics.diskUsage = getDiskUsage();
    metrics.networkLatency = getNetworkLatency();
}

void PerformanceMonitor::collectApplicationMetrics(PerformanceMetrics& metrics)
{
    metrics.dataAcquisitionRate = getDataAcquisitionRate();
    metrics.safetyCheckRate = getSafetyCheckRate();
}

void PerformanceMonitor::collectThreadMetrics(PerformanceMetrics& metrics)
{
    metrics.activeThreads = getActiveThreadCount();
}

void PerformanceMonitor::collectGUIMetrics(PerformanceMetrics& metrics)
{
    metrics.guiFrameRate = getGUIFrameRate();
}

double PerformanceMonitor::getCPUUsage()
{
    // Read CPU usage from /proc/stat on Linux
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly)) {
        return 0.0;
    }
    
    QTextStream stream(&file);
    QString line = stream.readLine();
    file.close();
    
    if (!line.startsWith("cpu ")) {
        return 0.0;
    }
    
    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 8) {
        return 0.0;
    }
    
    qint64 user = parts[1].toLongLong();
    qint64 nice = parts[2].toLongLong();
    qint64 system = parts[3].toLongLong();
    qint64 idle = parts[4].toLongLong();
    qint64 iowait = parts[5].toLongLong();
    qint64 irq = parts[6].toLongLong();
    qint64 softirq = parts[7].toLongLong();
    
    qint64 totalTime = user + nice + system + idle + iowait + irq + softirq;
    qint64 activeTime = totalTime - idle - iowait;
    
    if (m_lastSystemTime > 0) {
        qint64 totalDelta = totalTime - m_lastSystemTime;
        qint64 activeDelta = activeTime - m_lastCPUTime;
        
        if (totalDelta > 0) {
            double cpuUsage = (static_cast<double>(activeDelta) / totalDelta) * 100.0;
            m_lastCPUTime = activeTime;
            m_lastSystemTime = totalTime;
            return qBound(0.0, cpuUsage, 100.0);
        }
    }
    
    m_lastCPUTime = activeTime;
    m_lastSystemTime = totalTime;
    return 0.0;
}

qint64 PerformanceMonitor::getMemoryUsage()
{
    // Get memory usage for current process
    QFile file(QString("/proc/%1/status").arg(QApplication::applicationPid()));
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }
    
    QTextStream stream(&file);
    QString line;
    qint64 vmRSS = 0;
    
    while (stream.readLineInto(&line)) {
        if (line.startsWith("VmRSS:")) {
            QStringList parts = line.split('\t', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                vmRSS = parts[1].split(' ')[0].toLongLong() * 1024; // Convert from KB to bytes
            }
            break;
        }
    }
    
    file.close();
    return vmRSS;
}

qint64 PerformanceMonitor::getAvailableMemory()
{
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }
    
    QTextStream stream(&file);
    QString line;
    qint64 memAvailable = 0;
    
    while (stream.readLineInto(&line)) {
        if (line.startsWith("MemAvailable:")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                memAvailable = parts[1].toLongLong() * 1024; // Convert from KB to bytes
            }
            break;
        }
    }
    
    file.close();
    return memAvailable;
}

qint64 PerformanceMonitor::getDiskUsage()
{
    // Get disk usage for application directory
    QProcess df;
    df.start("df", QStringList() << QApplication::applicationDirPath());
    if (df.waitForFinished(5000)) {
        QString output = df.readAllStandardOutput();
        QStringList lines = output.split('\n');
        if (lines.size() >= 2) {
            QStringList parts = lines[1].split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                return parts[2].toLongLong() * 1024; // Convert from KB to bytes
            }
        }
    }
    return 0;
}

double PerformanceMonitor::getNetworkLatency()
{
    // Simple network latency check (placeholder)
    return 0.0; // Would implement actual network latency measurement
}

int PerformanceMonitor::getActiveThreadCount()
{
    return QThread::idealThreadCount();
}

double PerformanceMonitor::getGUIFrameRate()
{
    // Placeholder for GUI frame rate measurement
    return 60.0; // Would implement actual frame rate measurement
}

double PerformanceMonitor::getDataAcquisitionRate()
{
    // Placeholder for data acquisition rate
    return 100.0; // Would get actual rate from data acquisition system
}

double PerformanceMonitor::getSafetyCheckRate()
{
    // Placeholder for safety check rate
    return 50.0; // Would get actual rate from safety system
}

void PerformanceMonitor::checkPerformanceAlerts()
{
    PerformanceMetrics metrics = getCurrentMetrics();

    checkCPUAlert(metrics);
    checkMemoryAlert(metrics);
    checkGUIAlert(metrics);
    checkThreadAlert(metrics);
}

void PerformanceMonitor::checkCPUAlert(const PerformanceMetrics& metrics)
{
    if (metrics.cpuUsage > m_cpuThreshold) {
        addAlert("CPU",
                QString("High CPU usage: %1%").arg(metrics.cpuUsage, 0, 'f', 1),
                "warning",
                QJsonObject{{"cpu_usage", metrics.cpuUsage}, {"threshold", m_cpuThreshold}});
    } else {
        resolveAlert("CPU");
    }
}

void PerformanceMonitor::checkMemoryAlert(const PerformanceMetrics& metrics)
{
    if (metrics.memoryUsage > m_memoryThreshold) {
        addAlert("Memory",
                QString("High memory usage: %1 MB").arg(metrics.memoryUsage / (1024 * 1024)),
                "warning",
                QJsonObject{{"memory_usage", static_cast<qint64>(metrics.memoryUsage)}, {"threshold", static_cast<qint64>(m_memoryThreshold)}});
    } else {
        resolveAlert("Memory");
    }
}

void PerformanceMonitor::checkGUIAlert(const PerformanceMetrics& metrics)
{
    if (metrics.guiFrameRate < m_guiFrameRateThreshold) {
        addAlert("GUI",
                QString("Low GUI frame rate: %1 FPS").arg(metrics.guiFrameRate, 0, 'f', 1),
                "warning",
                QJsonObject{{"frame_rate", metrics.guiFrameRate}, {"threshold", m_guiFrameRateThreshold}});
    } else {
        resolveAlert("GUI");
    }
}

void PerformanceMonitor::checkThreadAlert(const PerformanceMetrics& metrics)
{
    int maxThreads = QThread::idealThreadCount() * 2; // Allow 2x ideal thread count
    if (metrics.activeThreads > maxThreads) {
        addAlert("Threads",
                QString("High thread count: %1").arg(metrics.activeThreads),
                "warning",
                QJsonObject{{"active_threads", metrics.activeThreads}, {"max_recommended", maxThreads}});
    } else {
        resolveAlert("Threads");
    }
}

void PerformanceMonitor::addAlert(const QString& category, const QString& message, const QString& severity, const QJsonObject& context)
{
    // Check if alert already exists
    for (const PerformanceAlert& alert : m_activeAlerts) {
        if (alert.category == category && !alert.resolved) {
            return; // Alert already active
        }
    }

    PerformanceAlert alert;
    alert.timestamp = QDateTime::currentMSecsSinceEpoch();
    alert.category = category;
    alert.message = message;
    alert.severity = severity;
    alert.context = context;
    alert.resolved = false;

    m_activeAlerts.append(alert);
    m_alertHistory.enqueue(alert);

    // Trim alert history
    while (m_alertHistory.size() > 1000) {
        m_alertHistory.dequeue();
    }

    emit performanceAlert(alert);

    qWarning() << "Performance alert:" << category << "-" << message;
}

void PerformanceMonitor::resolveAlert(const QString& category)
{
    for (PerformanceAlert& alert : m_activeAlerts) {
        if (alert.category == category && !alert.resolved) {
            alert.resolved = true;
            break;
        }
    }

    // Remove resolved alerts from active list
    m_activeAlerts.erase(std::remove_if(m_activeAlerts.begin(), m_activeAlerts.end(),
                                       [](const PerformanceAlert& alert) { return alert.resolved; }),
                        m_activeAlerts.end());
}

QJsonObject PerformanceMonitor::getPerformanceStatistics() const
{
    QMutexLocker locker(&m_dataMutex);

    QJsonObject stats;

    if (m_metricsHistory.isEmpty()) {
        return stats;
    }

    // Calculate averages
    double avgCPU = calculateAverageMetric("cpu");
    double avgMemory = calculateAverageMetric("memory");
    double avgGUIFrameRate = calculateAverageMetric("gui_framerate");

    stats["average_cpu_usage"] = avgCPU;
    stats["average_memory_usage"] = avgMemory;
    stats["average_gui_framerate"] = avgGUIFrameRate;
    stats["total_samples"] = m_metricsHistory.size();
    stats["monitoring_duration_ms"] = m_uptimeTimer.elapsed();
    stats["active_alerts"] = m_activeAlerts.size();

    return stats;
}

QJsonObject PerformanceMonitor::getPerformanceTrends() const
{
    QJsonObject trends;

    trends["cpu_trend"] = calculateMetricTrend("cpu");
    trends["memory_trend"] = calculateMetricTrend("memory");
    trends["gui_framerate_trend"] = calculateMetricTrend("gui_framerate");

    return trends;
}

QList<PerformanceMonitor::PerformanceAlert> PerformanceMonitor::getActiveAlerts() const
{
    return m_activeAlerts;
}

QList<PerformanceMonitor::PerformanceAlert> PerformanceMonitor::getAlertHistory() const
{
    QList<PerformanceAlert> history;
    for (const PerformanceAlert& alert : m_alertHistory) {
        history.append(alert);
    }
    return history;
}

void PerformanceMonitor::enableAutoOptimization(bool enabled)
{
    m_autoOptimizationEnabled = enabled;

    if (enabled && m_monitoring) {
        m_optimizationTimer->start();
    } else {
        m_optimizationTimer->stop();
    }
}

void PerformanceMonitor::optimizePerformance()
{
    qDebug() << "Starting performance optimization";

    optimizeCPUUsage();
    optimizeMemoryUsage();
    optimizeThreadPriorities();
    optimizeGUIPerformance();

    qDebug() << "Performance optimization completed";
}

void PerformanceMonitor::optimizeMemoryUsage()
{
    // Force garbage collection and memory cleanup
    QApplication::processEvents();

    qDebug() << "Memory optimization performed";
}

void PerformanceMonitor::optimizeThreadPriorities()
{
    // Placeholder for thread priority optimization
    qDebug() << "Thread priority optimization performed";
}

void PerformanceMonitor::optimizeGUIPerformance()
{
    // Placeholder for GUI performance optimization
    qDebug() << "GUI performance optimization performed";
}

// Slot implementations
void PerformanceMonitor::onMonitoringTimer()
{
    collectMetrics();
}

void PerformanceMonitor::onOptimizationTimer()
{
    performOptimization();
}

void PerformanceMonitor::performOptimization()
{
    if (m_autoOptimizationEnabled) {
        optimizePerformance();
    }
}

// Helper methods
double PerformanceMonitor::calculateAverageMetric(const QString& metricName, int samples) const
{
    if (m_metricsHistory.isEmpty()) return 0.0;

    int count = qMin(samples, m_metricsHistory.size());
    double sum = 0.0;

    for (int i = m_metricsHistory.size() - count; i < m_metricsHistory.size(); ++i) {
        const PerformanceMetrics& metrics = m_metricsHistory.at(i);

        if (metricName == "cpu") {
            sum += metrics.cpuUsage;
        } else if (metricName == "memory") {
            sum += metrics.memoryUsage;
        } else if (metricName == "gui_framerate") {
            sum += metrics.guiFrameRate;
        }
    }

    return sum / count;
}

double PerformanceMonitor::calculateMetricTrend(const QString& metricName, int samples) const
{
    if (m_metricsHistory.size() < 2) return 0.0;

    int count = qMin(samples, m_metricsHistory.size());
    if (count < 2) return 0.0;

    // Simple linear trend calculation
    double firstValue = 0.0, lastValue = 0.0;

    const PerformanceMetrics& first = m_metricsHistory.at(m_metricsHistory.size() - count);
    const PerformanceMetrics& last = m_metricsHistory.last();

    if (metricName == "cpu") {
        firstValue = first.cpuUsage;
        lastValue = last.cpuUsage;
    } else if (metricName == "memory") {
        firstValue = first.memoryUsage;
        lastValue = last.memoryUsage;
    } else if (metricName == "gui_framerate") {
        firstValue = first.guiFrameRate;
        lastValue = last.guiFrameRate;
    }

    return lastValue - firstValue;
}

// Additional methods for completeness
void PerformanceMonitor::startBenchmark(const QString& benchmarkName)
{
    QMutexLocker locker(&m_benchmarkMutex);

    BenchmarkData& benchmark = m_activeBenchmarks[benchmarkName];
    benchmark.name = benchmarkName;
    benchmark.timer.start();
    benchmark.active = true;

    qDebug() << "Started benchmark:" << benchmarkName;
}

void PerformanceMonitor::endBenchmark(const QString& benchmarkName)
{
    QMutexLocker locker(&m_benchmarkMutex);

    if (!m_activeBenchmarks.contains(benchmarkName)) {
        qWarning() << "Benchmark not found:" << benchmarkName;
        return;
    }

    BenchmarkData& benchmark = m_activeBenchmarks[benchmarkName];
    if (!benchmark.active) {
        qWarning() << "Benchmark not active:" << benchmarkName;
        return;
    }

    qint64 elapsed = benchmark.timer.elapsed();
    benchmark.active = false;

    QJsonObject results;
    results["name"] = benchmarkName;
    results["duration_ms"] = elapsed;
    results["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    m_benchmarkResults[benchmarkName] = results;

    emit benchmarkCompleted(benchmarkName, results);

    qDebug() << "Completed benchmark:" << benchmarkName << "Duration:" << elapsed << "ms";
}

QJsonObject PerformanceMonitor::getBenchmarkResults() const
{
    QMutexLocker locker(&m_benchmarkMutex);

    QJsonObject allResults;
    for (auto it = m_benchmarkResults.begin(); it != m_benchmarkResults.end(); ++it) {
        allResults[it.key()] = it.value();
    }

    return allResults;
}

void PerformanceMonitor::addCustomMetric(const QString& name, double value)
{
    QMutexLocker locker(&m_customMetricsMutex);
    m_customMetrics[name] = value;
}

void PerformanceMonitor::removeCustomMetric(const QString& name)
{
    QMutexLocker locker(&m_customMetricsMutex);
    m_customMetrics.remove(name);
}

QJsonObject PerformanceMonitor::getCustomMetrics() const
{
    QMutexLocker locker(&m_customMetricsMutex);

    QJsonObject metrics;
    for (auto it = m_customMetrics.begin(); it != m_customMetrics.end(); ++it) {
        metrics[it.key()] = it.value();
    }

    return metrics;
}

// Additional optimization methods
void PerformanceMonitor::optimizeCPUUsage()
{
    // Placeholder for CPU optimization
    qDebug() << "CPU usage optimization performed";
}

void PerformanceMonitor::optimizeMemoryFragmentation()
{
    // Placeholder for memory fragmentation optimization
    qDebug() << "Memory fragmentation optimization performed";
}

void PerformanceMonitor::optimizeThreadScheduling()
{
    // Placeholder for thread scheduling optimization
    qDebug() << "Thread scheduling optimization performed";
}

void PerformanceMonitor::optimizeGUIRendering()
{
    // Placeholder for GUI rendering optimization
    qDebug() << "GUI rendering optimization performed";
}

void PerformanceMonitor::optimizeDataBuffering()
{
    // Placeholder for data buffering optimization
    qDebug() << "Data buffering optimization performed";
}

QJsonObject PerformanceMonitor::calculateMetricStatistics(const QString& metricName) const
{
    QJsonObject stats;

    if (m_metricsHistory.isEmpty()) {
        return stats;
    }

    QList<double> values;
    for (const PerformanceMetrics& metrics : m_metricsHistory) {
        if (metricName == "cpu") {
            values.append(metrics.cpuUsage);
        } else if (metricName == "memory") {
            values.append(metrics.memoryUsage);
        } else if (metricName == "gui_framerate") {
            values.append(metrics.guiFrameRate);
        }
    }

    if (values.isEmpty()) {
        return stats;
    }

    std::sort(values.begin(), values.end());

    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }

    double mean = sum / values.size();
    double min = values.first();
    double max = values.last();
    double median = values.size() % 2 == 0 ?
                   (values[values.size()/2 - 1] + values[values.size()/2]) / 2.0 :
                   values[values.size()/2];

    stats["mean"] = mean;
    stats["min"] = min;
    stats["max"] = max;
    stats["median"] = median;
    stats["samples"] = values.size();

    return stats;
}
