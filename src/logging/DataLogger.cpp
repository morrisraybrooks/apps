#include "DataLogger.h"
#include "../VacuumController.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QApplication>
#include <QProcess>

DataLogger::DataLogger(VacuumController* controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_loggingActive(false)
    , m_loggingPaused(false)
    , m_logFormat(CSV_FORMAT)
    , m_maxFileSizeMB(DEFAULT_MAX_FILE_SIZE_MB)
    , m_maxFiles(DEFAULT_MAX_FILES)
    , m_compressionEnabled(true)
    , m_loggingInterval(DEFAULT_LOGGING_INTERVAL)
    , m_maxBufferSize(DEFAULT_MAX_BUFFER_SIZE)
    , m_loggingTimer(new QTimer(this))
    , m_rotationCheckTimer(new QTimer(this))
    , m_totalLogEntries(0)
    , m_loggingStartTime(0)
{
    // Set default log directory
    m_logDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    
    // Initialize all log types as enabled by default
    m_enabledLogTypes[PRESSURE_DATA] = true;
    m_enabledLogTypes[PATTERN_EXECUTION] = true;
    m_enabledLogTypes[SAFETY_EVENTS] = true;
    m_enabledLogTypes[USER_ACTIONS] = true;
    m_enabledLogTypes[SYSTEM_PERFORMANCE] = true;
    m_enabledLogTypes[CALIBRATION_DATA] = true;
    m_enabledLogTypes[ERROR_EVENTS] = true;
    
    // Initialize log counts
    for (auto type : m_enabledLogTypes.keys()) {
        m_logCounts[type] = 0;
    }
    
    // Setup timers
    m_loggingTimer->setInterval(m_loggingInterval);
    m_rotationCheckTimer->setInterval(ROTATION_CHECK_INTERVAL);
    
    connect(m_loggingTimer, &QTimer::timeout, this, &DataLogger::performPeriodicLogging);
    connect(m_rotationCheckTimer, &QTimer::timeout, this, &DataLogger::checkLogRotation);
    
    initializeLogger();
    connectToController();
    
    qDebug() << "DataLogger initialized with directory:" << m_logDirectory;
}

DataLogger::~DataLogger()
{
    stopLogging();
    
    // Close all log files
    for (auto stream : m_logStreams.values()) {
        if (stream) {
            stream->flush();
            delete stream;
        }
    }
    
    for (auto file : m_logFiles.values()) {
        if (file) {
            file->close();
            delete file;
        }
    }
}

void DataLogger::initializeLogger()
{
    // Create log directory if it doesn't exist
    QDir logDir(m_logDirectory);
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) {
            qCritical() << "Failed to create log directory:" << m_logDirectory;
            return;
        }
    }
    
    setupLogFiles();
    
    qDebug() << "DataLogger initialized successfully";
}

void DataLogger::setupLogFiles()
{
    // Close existing files
    for (auto stream : m_logStreams.values()) {
        if (stream) {
            stream->flush();
            delete stream;
        }
    }
    m_logStreams.clear();
    
    for (auto file : m_logFiles.values()) {
        if (file) {
            file->close();
            delete file;
        }
    }
    m_logFiles.clear();
    
    // Create new log files for each enabled type
    for (auto it = m_enabledLogTypes.begin(); it != m_enabledLogTypes.end(); ++it) {
        if (it.value()) {
            openLogFile(it.key());
        }
    }
}

void DataLogger::connectToController()
{
    if (!m_controller) return;
    
    // Connect to controller signals for automatic logging
    connect(m_controller, &VacuumController::pressureUpdated,
            this, &DataLogger::onPressureUpdated);
    connect(m_controller, &VacuumController::patternStarted,
            this, &DataLogger::onPatternStarted);
    connect(m_controller, &VacuumController::patternStopped,
            this, &DataLogger::onPatternStopped);
    
    qDebug() << "Connected to VacuumController for automatic logging";
}

void DataLogger::startLogging()
{
    if (m_loggingActive) return;
    
    m_loggingActive = true;
    m_loggingPaused = false;
    m_loggingStartTime = QDateTime::currentMSecsSinceEpoch();
    
    // Start timers
    m_loggingTimer->start();
    m_rotationCheckTimer->start();
    
    // Log the start event
    logSystemPerformance(QJsonObject{
        {"event", "logging_started"},
        {"timestamp", m_loggingStartTime},
        {"log_directory", m_logDirectory},
        {"log_format", static_cast<int>(m_logFormat)}
    });
    
    emit loggingStarted();
    
    qDebug() << "Data logging started";
}

void DataLogger::stopLogging()
{
    if (!m_loggingActive) return;
    
    // Stop timers
    m_loggingTimer->stop();
    m_rotationCheckTimer->stop();
    
    // Flush all buffers
    flushBuffers();
    
    // Log the stop event
    logSystemPerformance(QJsonObject{
        {"event", "logging_stopped"},
        {"timestamp", QDateTime::currentMSecsSinceEpoch()},
        {"total_entries", static_cast<qint64>(m_totalLogEntries)},
        {"duration_ms", QDateTime::currentMSecsSinceEpoch() - m_loggingStartTime}
    });
    
    // Flush streams
    for (auto stream : m_logStreams.values()) {
        if (stream) {
            stream->flush();
        }
    }
    
    m_loggingActive = false;
    m_loggingPaused = false;
    
    emit loggingStopped();
    
    qDebug() << "Data logging stopped";
}

void DataLogger::pauseLogging()
{
    if (!m_loggingActive || m_loggingPaused) return;
    
    m_loggingPaused = true;
    m_loggingTimer->stop();
    
    logSystemPerformance(QJsonObject{
        {"event", "logging_paused"},
        {"timestamp", QDateTime::currentMSecsSinceEpoch()}
    });
    
    qDebug() << "Data logging paused";
}

void DataLogger::resumeLogging()
{
    if (!m_loggingActive || !m_loggingPaused) return;
    
    m_loggingPaused = false;
    m_loggingTimer->start();
    
    logSystemPerformance(QJsonObject{
        {"event", "logging_resumed"},
        {"timestamp", QDateTime::currentMSecsSinceEpoch()}
    });
    
    qDebug() << "Data logging resumed";
}

void DataLogger::setLogType(LogType type, bool enabled)
{
    bool wasEnabled = m_enabledLogTypes.value(type, false);
    m_enabledLogTypes[type] = enabled;

    if (enabled && !wasEnabled) {
        // Create log file for this type if logging is active
        if (m_loggingActive) {
            openLogFile(type);
        }
    } else if (!enabled && wasEnabled) {
        // Close log file for this type
        if (m_logStreams.contains(type)) {
            m_logStreams[type]->flush();
            delete m_logStreams[type];
            m_logStreams.remove(type);
        }

        if (m_logFiles.contains(type)) {
            m_logFiles[type]->close();
            delete m_logFiles[type];
            m_logFiles.remove(type);
        }

        m_currentLogFiles.remove(type);
    }
}

bool DataLogger::isLogTypeEnabled(LogType type) const
{
    return m_enabledLogTypes.value(type, false);
}

void DataLogger::setLogFormat(LogFormat format)
{
    if (m_logFormat != format) {
        m_logFormat = format;
        
        // Recreate log files with new format if logging is active
        if (m_loggingActive) {
            setupLogFiles();
        }
    }
}

void DataLogger::setLogDirectory(const QString& directory)
{
    if (m_logDirectory != directory) {
        bool wasLogging = m_loggingActive;
        
        if (wasLogging) {
            stopLogging();
        }
        
        m_logDirectory = directory;
        
        // Create directory if it doesn't exist
        QDir logDir(m_logDirectory);
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }
        
        setupLogFiles();
        
        if (wasLogging) {
            startLogging();
        }
    }
}

void DataLogger::setMaxFileSize(int sizeMB)
{
    m_maxFileSizeMB = qMax(1, sizeMB);
}

void DataLogger::setMaxFiles(int maxFiles)
{
    m_maxFiles = qMax(1, maxFiles);
}

void DataLogger::setCompressionEnabled(bool enabled)
{
    m_compressionEnabled = enabled;
}

// Manual logging methods
void DataLogger::logPressureData(double avlPressure, double tankPressure)
{
    if (!isLogTypeEnabled(PRESSURE_DATA)) return;

    QJsonObject data;
    data["avl_pressure"] = avlPressure;
    data["tank_pressure"] = tankPressure;

    LogEntry entry(PRESSURE_DATA, "SensorInterface", "pressure_reading", data);
    writeLogEntry(entry);
}

void DataLogger::logPatternEvent(const QString& patternName, const QString& event, const QJsonObject& parameters)
{
    if (!isLogTypeEnabled(PATTERN_EXECUTION)) return;

    QJsonObject data = parameters;
    data["pattern_name"] = patternName;

    LogEntry entry(PATTERN_EXECUTION, "PatternEngine", event, data);
    writeLogEntry(entry);
}

void DataLogger::logSafetyEvent(const QString& event, const QString& details, const QJsonObject& context)
{
    if (!isLogTypeEnabled(SAFETY_EVENTS)) return;

    QJsonObject data = context;
    data["details"] = details;
    data["severity"] = "safety";

    LogEntry entry(SAFETY_EVENTS, "SafetyManager", event, data);
    writeLogEntry(entry);
}

void DataLogger::logUserAction(const QString& action, const QString& details, const QJsonObject& context)
{
    if (!isLogTypeEnabled(USER_ACTIONS)) return;

    QJsonObject data = context;
    data["details"] = details;

    LogEntry entry(USER_ACTIONS, "GUI", action, data);
    writeLogEntry(entry);
}

void DataLogger::logSystemPerformance(const QJsonObject& metrics)
{
    if (!isLogTypeEnabled(SYSTEM_PERFORMANCE)) return;

    LogEntry entry(SYSTEM_PERFORMANCE, "System", "performance_metrics", metrics);
    writeLogEntry(entry);
}

void DataLogger::logCalibrationEvent(const QString& sensor, const QString& event, const QJsonObject& results)
{
    if (!isLogTypeEnabled(CALIBRATION_DATA)) return;

    QJsonObject data = results;
    data["sensor"] = sensor;

    LogEntry entry(CALIBRATION_DATA, "CalibrationManager", event, data);
    writeLogEntry(entry);
}

void DataLogger::logErrorEvent(const QString& component, const QString& error, const QString& severity)
{
    if (!isLogTypeEnabled(ERROR_EVENTS)) return;

    QJsonObject data;
    data["error"] = error;
    data["severity"] = severity;

    LogEntry entry(ERROR_EVENTS, component, "error", data);
    writeLogEntry(entry);
}

// Slot implementations
void DataLogger::onPressureUpdated(double avlPressure, double tankPressure)
{
    logPressureData(avlPressure, tankPressure);
}

void DataLogger::onPatternStarted(const QString& patternName)
{
    logPatternEvent(patternName, "pattern_started");
}

void DataLogger::onPatternStopped()
{
    logPatternEvent("", "pattern_stopped");
}

void DataLogger::onSafetyEvent(const QString& event)
{
    logSafetyEvent(event, "Safety system event");
}

void DataLogger::onUserAction(const QString& action)
{
    logUserAction(action, "User interface action");
}

void DataLogger::performPeriodicLogging()
{
    if (!m_loggingActive || m_loggingPaused) return;

    // Log system performance metrics
    QJsonObject metrics;
    metrics["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    metrics["total_log_entries"] = static_cast<qint64>(m_totalLogEntries);
    metrics["buffer_size"] = m_logBuffer.size();
    metrics["memory_usage"] = QApplication::applicationPid(); // Placeholder for actual memory usage

    logSystemPerformance(metrics);

    // Process buffered entries
    flushBuffers();
}

void DataLogger::checkLogRotation()
{
    for (auto it = m_logFiles.begin(); it != m_logFiles.end(); ++it) {
        QFile* file = it.value();
        if (file && file->size() > m_maxFileSizeMB * 1024 * 1024) {
            LogType type = it.key();
            rotateLogFile();
            break; // Rotate one file at a time
        }
    }
}

void DataLogger::writeLogEntry(const LogEntry& entry)
{
    if (!m_loggingActive || m_loggingPaused) return;

    // Add to buffer for thread safety
    QMutexLocker locker(&m_bufferMutex);
    m_logBuffer.enqueue(entry);

    // Flush buffer if it's getting full
    if (m_logBuffer.size() >= m_maxBufferSize) {
        flushBuffers();
    }

    m_totalLogEntries++;
    m_logCounts[entry.type]++;
}

void DataLogger::flushBuffers()
{
    QMutexLocker locker(&m_bufferMutex);

    while (!m_logBuffer.isEmpty()) {
        LogEntry entry = m_logBuffer.dequeue();

        if (m_logStreams.contains(entry.type)) {
            QTextStream* stream = m_logStreams[entry.type];
            if (stream) {
                QString formattedEntry = formatLogEntry(entry);
                *stream << formattedEntry << "\n";
                stream->flush();
            }
        }
    }
}

bool DataLogger::openLogFile(LogType type)
{
    QString fileName = generateLogFileName(type);
    QString filePath = m_logDirectory + "/" + fileName;

    QFile* logFile = new QFile(filePath);
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream* stream = new QTextStream(logFile);
        stream->setCodec("UTF-8");

        m_logFiles[type] = logFile;
        m_logStreams[type] = stream;
        m_currentLogFiles[type] = fileName;

        // Write header for CSV format
        if (m_logFormat == CSV_FORMAT) {
            *stream << "Timestamp,Component,Event,Data\n";
            stream->flush();
        }

        qDebug() << "Created log file:" << filePath;
        return true;
    } else {
        qCritical() << "Failed to create log file:" << filePath;
        delete logFile;
        return false;
    }
}

QString DataLogger::formatLogEntry(const LogEntry& entry)
{
    QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(entry.timestamp);
    QString timeStr = timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");

    if (m_logFormat == CSV_FORMAT) {
        QString dataStr = QJsonDocument(entry.data).toJson(QJsonDocument::Compact);
        dataStr.replace("\"", "\"\""); // Escape quotes for CSV
        return QString("%1,%2,%3,\"%4\"")
               .arg(timeStr)
               .arg(entry.component)
               .arg(entry.event)
               .arg(dataStr);
    } else if (m_logFormat == JSON_FORMAT) {
        QJsonObject jsonEntry;
        jsonEntry["timestamp"] = timeStr;
        jsonEntry["type"] = logTypeToString(entry.type);
        jsonEntry["component"] = entry.component;
        jsonEntry["event"] = entry.event;
        jsonEntry["data"] = entry.data;

        return QJsonDocument(jsonEntry).toJson(QJsonDocument::Compact);
    }

    return QString(); // Binary format would be handled differently
}

QString DataLogger::logTypeToString(LogType type)
{
    switch (type) {
    case PRESSURE_DATA: return "pressure_data";
    case PATTERN_EXECUTION: return "pattern_execution";
    case SAFETY_EVENTS: return "safety_events";
    case USER_ACTIONS: return "user_actions";
    case SYSTEM_PERFORMANCE: return "system_performance";
    case CALIBRATION_DATA: return "calibration_data";
    case ERROR_EVENTS: return "error_events";
    default: return "unknown";
    }
}

QString DataLogger::generateLogFileName(LogType type)
{
    QString typeStr = logTypeToString(type);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString extension = (m_logFormat == JSON_FORMAT) ? "json" : "csv";

    return QString("%1_%2.%3").arg(typeStr).arg(timestamp).arg(extension);
}

void DataLogger::rotateLogFile()
{
    // Implementation for log rotation
    emit logFileRotated("New log file created");
    setupLogFiles(); // Recreate log files
}

// Data export methods
bool DataLogger::exportLogs(const QString& filePath, const QDateTime& startTime,
                           const QDateTime& endTime, LogType typeFilter)
{
    QList<LogEntry> entries = getLogEntries(startTime, endTime, typeFilter);

    if (entries.isEmpty()) {
        qWarning() << "No log entries found for export";
        return false;
    }

    QString extension = QFileInfo(filePath).suffix().toLower();

    if (extension == "csv") {
        return exportToCSV(filePath, entries);
    } else if (extension == "json") {
        return exportToJSON(filePath, entries);
    } else {
        qWarning() << "Unsupported export format:" << extension;
        return false;
    }
}

bool DataLogger::exportToCSV(const QString& filePath, const QList<LogEntry>& entries)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Failed to open export file:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    // Write header
    stream << "Timestamp,Type,Component,Event,Data\n";

    // Write entries
    for (const LogEntry& entry : entries) {
        QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(entry.timestamp);
        QString timeStr = timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString dataStr = QJsonDocument(entry.data).toJson(QJsonDocument::Compact);
        dataStr.replace("\"", "\"\""); // Escape quotes for CSV

        stream << QString("%1,%2,%3,%4,\"%5\"\n")
                  .arg(timeStr)
                  .arg(logTypeToString(entry.type))
                  .arg(entry.component)
                  .arg(entry.event)
                  .arg(dataStr);
    }

    file.close();
    qDebug() << "Exported" << entries.size() << "log entries to CSV:" << filePath;
    return true;
}

bool DataLogger::exportToJSON(const QString& filePath, const QList<LogEntry>& entries)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Failed to open export file:" << filePath;
        return false;
    }

    QJsonArray jsonArray;
    for (const LogEntry& entry : entries) {
        QJsonObject jsonEntry;
        jsonEntry["timestamp"] = QDateTime::fromMSecsSinceEpoch(entry.timestamp).toString(Qt::ISODate);
        jsonEntry["type"] = logTypeToString(entry.type);
        jsonEntry["component"] = entry.component;
        jsonEntry["event"] = entry.event;
        jsonEntry["data"] = entry.data;
        jsonArray.append(jsonEntry);
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson());
    file.close();

    qDebug() << "Exported" << entries.size() << "log entries to JSON:" << filePath;
    return true;
}

// Log analysis methods
QList<DataLogger::LogEntry> DataLogger::getLogEntries(const QDateTime& startTime,
                                                      const QDateTime& endTime, LogType type)
{
    QList<LogEntry> entries;

    // For now, return empty list - full implementation would read from log files
    // This would involve parsing existing log files and filtering by time/type
    qDebug() << "getLogEntries called - full implementation needed";

    return entries;
}

QJsonObject DataLogger::getLogStatistics()
{
    QJsonObject stats;

    stats["total_entries"] = static_cast<qint64>(m_totalLogEntries);
    stats["logging_active"] = m_loggingActive;
    stats["logging_paused"] = m_loggingPaused;
    stats["log_directory"] = m_logDirectory;
    stats["log_format"] = static_cast<int>(m_logFormat);

    // Add counts for each log type
    QJsonObject typeCounts;
    for (auto it = m_logCounts.begin(); it != m_logCounts.end(); ++it) {
        typeCounts[logTypeToString(it.key())] = it.value();
    }
    stats["type_counts"] = typeCounts;

    // Add file information
    QJsonObject fileInfo;
    for (auto it = m_currentLogFiles.begin(); it != m_currentLogFiles.end(); ++it) {
        QString filePath = m_logDirectory + "/" + it.value();
        QFileInfo info(filePath);
        if (info.exists()) {
            QJsonObject fileData;
            fileData["size_bytes"] = info.size();
            fileData["created"] = info.birthTime().toString(Qt::ISODate);
            fileData["modified"] = info.lastModified().toString(Qt::ISODate);
            fileInfo[logTypeToString(it.key())] = fileData;
        }
    }
    stats["files"] = fileInfo;

    if (m_loggingStartTime > 0) {
        stats["session_duration_ms"] = QDateTime::currentMSecsSinceEpoch() - m_loggingStartTime;
    }

    return stats;
}

QStringList DataLogger::getAvailableLogFiles()
{
    QStringList logFiles;

    QDir logDir(m_logDirectory);
    if (logDir.exists()) {
        QStringList filters;
        filters << "*.csv" << "*.json" << "*.log";

        QFileInfoList fileList = logDir.entryInfoList(filters, QDir::Files, QDir::Time);
        for (const QFileInfo& fileInfo : fileList) {
            logFiles.append(fileInfo.fileName());
        }
    }

    return logFiles;
}

// Maintenance methods
void DataLogger::rotateLogs()
{
    qDebug() << "Manual log rotation requested";

    // Stop logging temporarily
    bool wasLogging = m_loggingActive;
    if (wasLogging) {
        stopLogging();
    }

    // Compress old logs if enabled
    if (m_compressionEnabled) {
        compressOldLogs();
    }

    // Setup new log files
    setupLogFiles();

    // Restart logging if it was active
    if (wasLogging) {
        startLogging();
    }

    emit logFileRotated("Manual log rotation completed");
}

void DataLogger::compressOldLogs()
{
    if (!m_compressionEnabled) return;

    QDir logDir(m_logDirectory);
    QStringList filters;
    filters << "*.csv" << "*.json";

    QFileInfoList fileList = logDir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo& fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QString compressedPath = filePath + ".gz";

        // Use gzip to compress the file
        QProcess gzip;
        gzip.start("gzip", QStringList() << filePath);
        if (gzip.waitForFinished(30000)) { // 30 second timeout
            qDebug() << "Compressed log file:" << compressedPath;
        } else {
            qWarning() << "Failed to compress log file:" << filePath;
        }
    }
}

void DataLogger::cleanupOldLogs(int daysToKeep)
{
    QDir logDir(m_logDirectory);
    QStringList filters;
    filters << "*.csv" << "*.json" << "*.gz" << "*.log";

    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-daysToKeep);

    QFileInfoList fileList = logDir.entryInfoList(filters, QDir::Files);
    int deletedCount = 0;

    for (const QFileInfo& fileInfo : fileList) {
        if (fileInfo.lastModified() < cutoffTime) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                deletedCount++;
                qDebug() << "Deleted old log file:" << fileInfo.fileName();
            } else {
                qWarning() << "Failed to delete old log file:" << fileInfo.fileName();
            }
        }
    }

    qDebug() << "Cleanup completed:" << deletedCount << "old log files deleted";
}
