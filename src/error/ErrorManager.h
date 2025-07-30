#ifndef ERRORMANAGER_H
#define ERRORMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>

/**
 * @brief Comprehensive error handling and recovery system
 * 
 * This system provides:
 * - Centralized error logging and management
 * - Automatic recovery mechanisms
 * - Error classification and prioritization
 * - System health monitoring
 * - Crash detection and recovery
 * - Error reporting and notifications
 */
class ErrorManager : public QObject
{
    Q_OBJECT

public:
    enum ErrorSeverity {
        INFO,           // Informational messages
        WARNING,        // Warning conditions
        ERROR,          // Error conditions
        CRITICAL,       // Critical errors requiring immediate attention
        FATAL           // Fatal errors requiring system shutdown
    };
    Q_ENUM(ErrorSeverity)

    enum ErrorCategory {
        HARDWARE,       // Hardware-related errors
        SENSOR,         // Sensor errors
        SAFETY,         // Safety system errors
        PATTERN,        // Pattern execution errors
        GUI,            // GUI-related errors
        SYSTEM,         // General system errors
        COMMUNICATION,  // Communication errors
        CALIBRATION     // Calibration errors
    };
    Q_ENUM(ErrorCategory)

    struct ErrorRecord {
        qint64 timestamp;
        ErrorSeverity severity;
        ErrorCategory category;
        QString component;
        QString message;
        QString details;
        QJsonObject context;
        bool resolved;
        qint64 resolvedTimestamp;
        
        ErrorRecord() : timestamp(0), severity(INFO), category(SYSTEM), 
                       resolved(false), resolvedTimestamp(0) {}
    };

    explicit ErrorManager(QObject *parent = nullptr);
    ~ErrorManager();

    // Error reporting
    void reportError(ErrorSeverity severity, ErrorCategory category, 
                    const QString& component, const QString& message, 
                    const QString& details = QString(), 
                    const QJsonObject& context = QJsonObject());
    
    void reportInfo(const QString& component, const QString& message, const QString& details = QString());
    void reportWarning(const QString& component, const QString& message, const QString& details = QString());
    void reportError(const QString& component, const QString& message, const QString& details = QString());
    void reportCritical(const QString& component, const QString& message, const QString& details = QString());
    void reportFatal(const QString& component, const QString& message, const QString& details = QString());
    
    // Error management
    void resolveError(int errorId);
    void clearResolvedErrors();
    void clearAllErrors();
    
    // Error retrieval
    QList<ErrorRecord> getErrors(ErrorSeverity minSeverity = INFO) const;
    QList<ErrorRecord> getUnresolvedErrors() const;
    QList<ErrorRecord> getErrorsByCategory(ErrorCategory category) const;
    QList<ErrorRecord> getRecentErrors(int minutes = 60) const;
    
    // Statistics
    int getErrorCount(ErrorSeverity severity = INFO) const;
    int getUnresolvedErrorCount() const;
    double getErrorRate() const; // Errors per minute
    
    // Recovery system
    void enableAutoRecovery(bool enabled);
    bool isAutoRecoveryEnabled() const { return m_autoRecoveryEnabled; }
    void setMaxRecoveryAttempts(int maxAttempts);
    
    // System health
    bool isSystemHealthy() const;
    QString getSystemHealthReport() const;
    
    // Logging
    void setLogToFile(bool enabled);
    void setLogFilePath(const QString& path);
    void setMaxLogFileSize(int sizeMB);
    void setLogRotationEnabled(bool enabled);
    
    // Configuration
    void loadConfiguration(const QString& configPath);
    void saveConfiguration(const QString& configPath);

public slots:
    void performHealthCheck();
    void attemptRecovery();

signals:
    void errorReported(const ErrorRecord& error);
    void criticalErrorOccurred(const ErrorRecord& error);
    void fatalErrorOccurred(const ErrorRecord& error);
    void systemHealthChanged(bool healthy);
    void recoveryAttempted(const QString& component, bool success);
    void recoveryFailed(const QString& component, int attempts);

private slots:
    void onHealthCheckTimer();
    void onRecoveryTimer();

private:
    void initializeErrorManager();
    void logErrorToFile(const ErrorRecord& error);
    void rotateLogFile();
    void processError(const ErrorRecord& error);
    void attemptComponentRecovery(const QString& component);
    bool canAttemptRecovery(const QString& component);
    void updateSystemHealth();
    QString formatErrorMessage(const ErrorRecord& error) const;
    QString severityToString(ErrorSeverity severity) const;
    QString categoryToString(ErrorCategory category) const;
    
    // Error storage
    QQueue<ErrorRecord> m_errorQueue;
    mutable QMutex m_errorMutex;
    int m_nextErrorId;
    int m_maxErrorHistory;
    
    // Recovery system
    bool m_autoRecoveryEnabled;
    int m_maxRecoveryAttempts;
    QMap<QString, int> m_recoveryAttempts;
    QMap<QString, qint64> m_lastRecoveryAttempt;
    QTimer* m_recoveryTimer;
    
    // Health monitoring
    QTimer* m_healthCheckTimer;
    bool m_systemHealthy;
    int m_healthCheckInterval;
    
    // Logging
    bool m_logToFile;
    QString m_logFilePath;
    QFile* m_logFile;
    QTextStream* m_logStream;
    int m_maxLogFileSize;
    bool m_logRotationEnabled;
    int m_currentLogFileSize;
    
    // Error rate tracking
    QQueue<qint64> m_errorTimestamps;
    
    // Constants
    static const int DEFAULT_MAX_ERROR_HISTORY = 1000;
    static const int DEFAULT_MAX_RECOVERY_ATTEMPTS = 3;
    static const int DEFAULT_HEALTH_CHECK_INTERVAL = 30000; // 30 seconds
    static const int DEFAULT_RECOVERY_INTERVAL = 5000;      // 5 seconds
    static const int DEFAULT_MAX_LOG_FILE_SIZE = 100;       // 100 MB
    static const int ERROR_RATE_WINDOW_MINUTES = 10;        // 10 minutes
    static const int RECOVERY_COOLDOWN_MINUTES = 5;         // 5 minutes
};

#endif // ERRORMANAGER_H
