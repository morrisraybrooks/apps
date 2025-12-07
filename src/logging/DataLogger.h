#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QDateTime>

// Forward declarations
class VacuumController;

/**
 * @brief Comprehensive data logging system
 * 
 * This system provides:
 * - Real-time pressure data logging
 * - Pattern execution logging
 * - Safety event logging
 * - System performance logging
 * - User action logging
 * - Configurable log formats (CSV, JSON)
 * - Automatic log rotation
 * - Data export capabilities
 */
class DataLogger : public QObject
{
    Q_OBJECT

public:
    enum LogType {
        PRESSURE_DATA,      // Pressure sensor readings
        PATTERN_EXECUTION,  // Pattern start/stop/parameters
        SAFETY_EVENTS,      // Safety system activations
        USER_ACTIONS,       // User interface interactions
        SYSTEM_PERFORMANCE, // System metrics and performance
        CALIBRATION_DATA,   // Calibration events and results
        ERROR_EVENTS        // Error and warning events
    };
    Q_ENUM(LogType)

    enum LogFormat {
        CSV_FORMAT,         // Comma-separated values
        JSON_FORMAT,        // JSON format
        BINARY_FORMAT       // Binary format for high-frequency data
    };
    Q_ENUM(LogFormat)

    struct LogEntry {
        qint64 timestamp;
        LogType type;
        QString component;
        QString event;
        QJsonObject data;
        
        LogEntry() : timestamp(0), type(PRESSURE_DATA) {}
        LogEntry(LogType t, const QString& comp, const QString& evt, const QJsonObject& d = QJsonObject())
            : timestamp(QDateTime::currentMSecsSinceEpoch()), type(t), component(comp), event(evt), data(d) {}
    };

    explicit DataLogger(VacuumController* controller, QObject *parent = nullptr);
    ~DataLogger();

    // Logging control
    void startLogging();
    void stopLogging();
    void pauseLogging();
    void resumeLogging();
    bool isLogging() const { return m_loggingActive; }
    
    // Log configuration
    void setLogType(LogType type, bool enabled);
    bool isLogTypeEnabled(LogType type) const;
    void setLogFormat(LogFormat format);
    LogFormat getLogFormat() const { return m_logFormat; }
    
    void setLogDirectory(const QString& directory);
    QString getLogDirectory() const { return m_logDirectory; }
    
    void setMaxFileSize(int sizeMB);
    void setMaxFiles(int maxFiles);
    void setCompressionEnabled(bool enabled);
    
    // Manual logging
    void logPressureData(double avlPressure, double tankPressure);
    void logPatternEvent(const QString& patternName, const QString& event, const QJsonObject& parameters = QJsonObject());
    void logSafetyEvent(const QString& event, const QString& details, const QJsonObject& context = QJsonObject());
    void logUserAction(const QString& action, const QString& details, const QJsonObject& context = QJsonObject());
    void logSystemPerformance(const QJsonObject& metrics);
    void logCalibrationEvent(const QString& sensor, const QString& event, const QJsonObject& results = QJsonObject());
    void logErrorEvent(const QString& component, const QString& error, const QString& severity);
    
    // Data export
    bool exportLogs(const QString& filePath, const QDateTime& startTime = QDateTime(), 
                   const QDateTime& endTime = QDateTime(), LogType typeFilter = PRESSURE_DATA);
    bool exportToCSV(const QString& filePath, const QList<LogEntry>& entries);
    bool exportToJSON(const QString& filePath, const QList<LogEntry>& entries);
    
    // Log analysis
    QList<LogEntry> getLogEntries(const QDateTime& startTime, const QDateTime& endTime, LogType type = PRESSURE_DATA);
    QJsonObject getLogStatistics();
    QStringList getAvailableLogFiles();
    
    // Maintenance
    void rotateLogs();
    void compressOldLogs();
    void cleanupOldLogs(int daysToKeep = 30);

public Q_SLOTS:
    void onPressureUpdated(double avlPressure, double tankPressure);
    void onPatternStarted(const QString& patternName);
    void onPatternStopped();
    void onSafetyEvent(const QString& event);
    void onUserAction(const QString& action);

Q_SIGNALS:
    void loggingStarted();
    void loggingStopped();
    void logFileRotated(const QString& newFileName);
    void logError(const QString& error);

private Q_SLOTS:
    void performPeriodicLogging();
    void checkLogRotation();

private:
    void initializeLogger();
    void setupLogFiles();
    void writeLogEntry(const LogEntry& entry);
    void rotateLogFile();
    QString generateLogFileName(LogType type);
    QString formatLogEntry(const LogEntry& entry);
    QString logTypeToString(LogType type);
    void connectToController();
    void flushBuffers();
    
    // Controller interface
    VacuumController* m_controller;
    
    // Logging state
    bool m_loggingActive;
    bool m_loggingPaused;
    QMap<LogType, bool> m_enabledLogTypes;
    LogFormat m_logFormat;
    
    // File management
    QString m_logDirectory;
    QMap<LogType, QFile*> m_logFiles;
    QMap<LogType, QTextStream*> m_logStreams;
    QMap<LogType, QString> m_currentLogFiles;
    
    // Configuration
    int m_maxFileSizeMB;
    int m_maxFiles;
    bool m_compressionEnabled;
    int m_loggingInterval;
    
    // Buffering
    QQueue<LogEntry> m_logBuffer;
    QMutex m_bufferMutex;
    int m_maxBufferSize;
    
    // Timers
    QTimer* m_loggingTimer;
    QTimer* m_rotationCheckTimer;
    
    // Statistics
    QMap<LogType, int> m_logCounts;
    qint64 m_totalLogEntries;
    qint64 m_loggingStartTime;
    
    // Constants
    static const int DEFAULT_MAX_FILE_SIZE_MB = 100;
    static const int DEFAULT_MAX_FILES = 10;
    static const int DEFAULT_LOGGING_INTERVAL = 1000;  // 1 second
    static const int DEFAULT_MAX_BUFFER_SIZE = 1000;
    static const int ROTATION_CHECK_INTERVAL = 60000;  // 1 minute
};

#endif // DATALOGGER_H
