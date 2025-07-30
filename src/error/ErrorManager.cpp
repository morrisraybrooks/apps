#include "ErrorManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QCoreApplication>

ErrorManager::ErrorManager(QObject *parent)
    : QObject(parent)
    , m_nextErrorId(1)
    , m_maxErrorHistory(DEFAULT_MAX_ERROR_HISTORY)
    , m_autoRecoveryEnabled(true)
    , m_maxRecoveryAttempts(DEFAULT_MAX_RECOVERY_ATTEMPTS)
    , m_recoveryTimer(new QTimer(this))
    , m_healthCheckTimer(new QTimer(this))
    , m_systemHealthy(true)
    , m_healthCheckInterval(DEFAULT_HEALTH_CHECK_INTERVAL)
    , m_logToFile(true)
    , m_logFilePath("/var/log/vacuum-controller-errors.log")
    , m_logFile(nullptr)
    , m_logStream(nullptr)
    , m_maxLogFileSize(DEFAULT_MAX_LOG_FILE_SIZE)
    , m_logRotationEnabled(true)
    , m_currentLogFileSize(0)
{
    initializeErrorManager();
}

ErrorManager::~ErrorManager()
{
    if (m_logStream) {
        delete m_logStream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

void ErrorManager::initializeErrorManager()
{
    // Setup timers
    m_recoveryTimer->setInterval(DEFAULT_RECOVERY_INTERVAL);
    m_recoveryTimer->setSingleShot(false);
    connect(m_recoveryTimer, &QTimer::timeout, this, &ErrorManager::onRecoveryTimer);
    
    m_healthCheckTimer->setInterval(m_healthCheckInterval);
    connect(m_healthCheckTimer, &QTimer::timeout, this, &ErrorManager::onHealthCheckTimer);
    m_healthCheckTimer->start();
    
    // Initialize logging
    if (m_logToFile) {
        setLogFilePath(m_logFilePath);
    }
    
    qDebug() << "ErrorManager initialized";
    reportInfo("ErrorManager", "Error management system initialized");
}

void ErrorManager::reportError(ErrorSeverity severity, ErrorCategory category, 
                              const QString& component, const QString& message, 
                              const QString& details, const QJsonObject& context)
{
    ErrorRecord error;
    error.timestamp = QDateTime::currentMSecsSinceEpoch();
    error.severity = severity;
    error.category = category;
    error.component = component;
    error.message = message;
    error.details = details;
    error.context = context;
    error.resolved = false;
    
    {
        QMutexLocker locker(&m_errorMutex);
        m_errorQueue.enqueue(error);
        
        // Maintain error history limit
        while (m_errorQueue.size() > m_maxErrorHistory) {
            m_errorQueue.dequeue();
        }
        
        // Track error timestamps for rate calculation
        m_errorTimestamps.enqueue(error.timestamp);
        qint64 cutoffTime = error.timestamp - (ERROR_RATE_WINDOW_MINUTES * 60 * 1000);
        while (!m_errorTimestamps.isEmpty() && m_errorTimestamps.first() < cutoffTime) {
            m_errorTimestamps.dequeue();
        }
    }
    
    // Log to file
    if (m_logToFile) {
        logErrorToFile(error);
    }
    
    // Process error
    processError(error);
    
    // Emit signals
    emit errorReported(error);
    
    if (severity == CRITICAL) {
        emit criticalErrorOccurred(error);
    } else if (severity == FATAL) {
        emit fatalErrorOccurred(error);
    }
    
    // Update system health
    updateSystemHealth();
    
    qDebug() << QString("[%1] %2: %3 - %4")
                .arg(severityToString(severity))
                .arg(component)
                .arg(message)
                .arg(details);
}

void ErrorManager::reportInfo(const QString& component, const QString& message, const QString& details)
{
    reportError(INFO, SYSTEM, component, message, details);
}

void ErrorManager::reportWarning(const QString& component, const QString& message, const QString& details)
{
    reportError(WARNING, SYSTEM, component, message, details);
}

void ErrorManager::reportError(const QString& component, const QString& message, const QString& details)
{
    reportError(ERROR, SYSTEM, component, message, details);
}

void ErrorManager::reportCritical(const QString& component, const QString& message, const QString& details)
{
    reportError(CRITICAL, SYSTEM, component, message, details);
}

void ErrorManager::reportFatal(const QString& component, const QString& message, const QString& details)
{
    reportError(FATAL, SYSTEM, component, message, details);
}

void ErrorManager::resolveError(int errorId)
{
    QMutexLocker locker(&m_errorMutex);
    
    for (auto& error : m_errorQueue) {
        if (!error.resolved) {
            error.resolved = true;
            error.resolvedTimestamp = QDateTime::currentMSecsSinceEpoch();
            
            reportInfo("ErrorManager", QString("Error resolved: %1").arg(error.message));
            break;
        }
    }
    
    updateSystemHealth();
}

QList<ErrorManager::ErrorRecord> ErrorManager::getErrors(ErrorSeverity minSeverity) const
{
    QMutexLocker locker(&m_errorMutex);
    
    QList<ErrorRecord> result;
    for (const auto& error : m_errorQueue) {
        if (error.severity >= minSeverity) {
            result.append(error);
        }
    }
    
    return result;
}

QList<ErrorManager::ErrorRecord> ErrorManager::getUnresolvedErrors() const
{
    QMutexLocker locker(&m_errorMutex);
    
    QList<ErrorRecord> result;
    for (const auto& error : m_errorQueue) {
        if (!error.resolved) {
            result.append(error);
        }
    }
    
    return result;
}

QList<ErrorManager::ErrorRecord> ErrorManager::getErrorsByCategory(ErrorCategory category) const
{
    QMutexLocker locker(&m_errorMutex);
    
    QList<ErrorRecord> result;
    for (const auto& error : m_errorQueue) {
        if (error.category == category) {
            result.append(error);
        }
    }
    
    return result;
}

QList<ErrorManager::ErrorRecord> ErrorManager::getRecentErrors(int minutes) const
{
    QMutexLocker locker(&m_errorMutex);
    
    qint64 cutoffTime = QDateTime::currentMSecsSinceEpoch() - (minutes * 60 * 1000);
    
    QList<ErrorRecord> result;
    for (const auto& error : m_errorQueue) {
        if (error.timestamp >= cutoffTime) {
            result.append(error);
        }
    }
    
    return result;
}

int ErrorManager::getErrorCount(ErrorSeverity severity) const
{
    QMutexLocker locker(&m_errorMutex);
    
    int count = 0;
    for (const auto& error : m_errorQueue) {
        if (error.severity >= severity) {
            count++;
        }
    }
    
    return count;
}

int ErrorManager::getUnresolvedErrorCount() const
{
    QMutexLocker locker(&m_errorMutex);
    
    int count = 0;
    for (const auto& error : m_errorQueue) {
        if (!error.resolved) {
            count++;
        }
    }
    
    return count;
}

double ErrorManager::getErrorRate() const
{
    QMutexLocker locker(&m_errorMutex);
    
    if (m_errorTimestamps.isEmpty()) {
        return 0.0;
    }
    
    qint64 timeSpan = QDateTime::currentMSecsSinceEpoch() - m_errorTimestamps.first();
    double minutes = timeSpan / (60.0 * 1000.0);
    
    if (minutes < 1.0) {
        minutes = 1.0; // Minimum 1 minute window
    }
    
    return m_errorTimestamps.size() / minutes;
}

bool ErrorManager::isSystemHealthy() const
{
    return m_systemHealthy;
}

QString ErrorManager::getSystemHealthReport() const
{
    QStringList report;
    
    int criticalErrors = getErrorCount(CRITICAL);
    int unresolvedErrors = getUnresolvedErrorCount();
    double errorRate = getErrorRate();
    
    report << QString("System Health: %1").arg(m_systemHealthy ? "HEALTHY" : "UNHEALTHY");
    report << QString("Critical Errors: %1").arg(criticalErrors);
    report << QString("Unresolved Errors: %1").arg(unresolvedErrors);
    report << QString("Error Rate: %1 errors/minute").arg(errorRate, 0, 'f', 2);
    
    if (!m_systemHealthy) {
        report << "\nRecent Critical Issues:";
        auto recentErrors = getRecentErrors(30); // Last 30 minutes
        for (const auto& error : recentErrors) {
            if (error.severity >= CRITICAL && !error.resolved) {
                report << QString("- %1: %2").arg(error.component, error.message);
            }
        }
    }
    
    return report.join("\n");
}

void ErrorManager::setLogToFile(bool enabled)
{
    m_logToFile = enabled;
    
    if (enabled && !m_logFile) {
        setLogFilePath(m_logFilePath);
    } else if (!enabled && m_logFile) {
        if (m_logStream) {
            delete m_logStream;
            m_logStream = nullptr;
        }
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
}

void ErrorManager::setLogFilePath(const QString& path)
{
    m_logFilePath = path;
    
    if (!m_logToFile) return;
    
    // Close existing file
    if (m_logStream) {
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
    
    // Create directory if needed
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Open new file
    m_logFile = new QFile(path);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        m_currentLogFileSize = m_logFile->size();
        
        // Write header
        *m_logStream << QString("\n=== Error Log Started: %1 ===\n")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        m_logStream->flush();
    } else {
        qWarning() << "Failed to open error log file:" << path;
        delete m_logFile;
        m_logFile = nullptr;
    }
}

void ErrorManager::logErrorToFile(const ErrorRecord& error)
{
    if (!m_logStream) return;
    
    QString logEntry = QString("[%1] [%2] [%3] %4: %5")
                      .arg(QDateTime::fromMSecsSinceEpoch(error.timestamp).toString("yyyy-MM-dd hh:mm:ss.zzz"))
                      .arg(severityToString(error.severity))
                      .arg(categoryToString(error.category))
                      .arg(error.component)
                      .arg(error.message);
    
    if (!error.details.isEmpty()) {
        logEntry += QString(" - %1").arg(error.details);
    }
    
    *m_logStream << logEntry << "\n";
    m_logStream->flush();
    
    m_currentLogFileSize += logEntry.length() + 1;
    
    // Check for log rotation
    if (m_logRotationEnabled && m_currentLogFileSize > (m_maxLogFileSize * 1024 * 1024)) {
        rotateLogFile();
    }
}

void ErrorManager::rotateLogFile()
{
    if (!m_logFile) return;
    
    QString basePath = m_logFilePath;
    QString rotatedPath = basePath + ".1";
    
    // Close current file
    if (m_logStream) {
        delete m_logStream;
        m_logStream = nullptr;
    }
    m_logFile->close();
    
    // Rotate existing files
    for (int i = 9; i >= 1; --i) {
        QString oldPath = basePath + QString(".%1").arg(i);
        QString newPath = basePath + QString(".%1").arg(i + 1);
        
        if (QFile::exists(oldPath)) {
            QFile::remove(newPath);
            QFile::rename(oldPath, newPath);
        }
    }
    
    // Move current file to .1
    QFile::rename(basePath, rotatedPath);
    
    // Reopen log file
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        m_currentLogFileSize = 0;
        
        *m_logStream << QString("=== Log Rotated: %1 ===\n")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        m_logStream->flush();
    }
}

void ErrorManager::processError(const ErrorRecord& error)
{
    // Handle critical and fatal errors
    if (error.severity >= CRITICAL) {
        if (m_autoRecoveryEnabled && canAttemptRecovery(error.component)) {
            // Start recovery timer if not already running
            if (!m_recoveryTimer->isActive()) {
                m_recoveryTimer->start();
            }
        }
    }
}

void ErrorManager::attemptComponentRecovery(const QString& component)
{
    if (!canAttemptRecovery(component)) {
        return;
    }
    
    m_recoveryAttempts[component]++;
    m_lastRecoveryAttempt[component] = QDateTime::currentMSecsSinceEpoch();
    
    reportInfo("ErrorManager", QString("Attempting recovery for component: %1 (attempt %2)")
               .arg(component).arg(m_recoveryAttempts[component]));
    
    // Emit recovery signal - actual recovery logic should be implemented by components
    bool success = false; // This would be determined by the actual recovery attempt
    
    emit recoveryAttempted(component, success);
    
    if (!success && m_recoveryAttempts[component] >= m_maxRecoveryAttempts) {
        reportCritical("ErrorManager", QString("Recovery failed for component: %1 after %2 attempts")
                      .arg(component).arg(m_maxRecoveryAttempts));
        emit recoveryFailed(component, m_recoveryAttempts[component]);
    }
}

bool ErrorManager::canAttemptRecovery(const QString& component)
{
    // Check if we've exceeded max attempts
    if (m_recoveryAttempts.value(component, 0) >= m_maxRecoveryAttempts) {
        return false;
    }
    
    // Check cooldown period
    qint64 lastAttempt = m_lastRecoveryAttempt.value(component, 0);
    qint64 cooldownPeriod = RECOVERY_COOLDOWN_MINUTES * 60 * 1000;
    
    if (lastAttempt > 0 && (QDateTime::currentMSecsSinceEpoch() - lastAttempt) < cooldownPeriod) {
        return false;
    }
    
    return true;
}

void ErrorManager::updateSystemHealth()
{
    bool wasHealthy = m_systemHealthy;
    
    // Determine system health based on error conditions
    int criticalErrors = getErrorCount(CRITICAL);
    int unresolvedErrors = getUnresolvedErrorCount();
    double errorRate = getErrorRate();
    
    m_systemHealthy = (criticalErrors == 0) && 
                     (unresolvedErrors < 10) && 
                     (errorRate < 5.0); // Less than 5 errors per minute
    
    if (wasHealthy != m_systemHealthy) {
        emit systemHealthChanged(m_systemHealthy);
        
        if (m_systemHealthy) {
            reportInfo("ErrorManager", "System health restored");
        } else {
            reportWarning("ErrorManager", "System health degraded");
        }
    }
}

QString ErrorManager::severityToString(ErrorSeverity severity) const
{
    switch (severity) {
    case INFO: return "INFO";
    case WARNING: return "WARNING";
    case ERROR: return "ERROR";
    case CRITICAL: return "CRITICAL";
    case FATAL: return "FATAL";
    default: return "UNKNOWN";
    }
}

QString ErrorManager::categoryToString(ErrorCategory category) const
{
    switch (category) {
    case HARDWARE: return "HARDWARE";
    case SENSOR: return "SENSOR";
    case SAFETY: return "SAFETY";
    case PATTERN: return "PATTERN";
    case GUI: return "GUI";
    case SYSTEM: return "SYSTEM";
    case COMMUNICATION: return "COMMUNICATION";
    case CALIBRATION: return "CALIBRATION";
    default: return "UNKNOWN";
    }
}

void ErrorManager::onHealthCheckTimer()
{
    performHealthCheck();
}

void ErrorManager::onRecoveryTimer()
{
    attemptRecovery();
}

void ErrorManager::performHealthCheck()
{
    // This would perform various system health checks
    // For now, just update based on current error state
    updateSystemHealth();
}

void ErrorManager::attemptRecovery()
{
    // Get components that need recovery
    QStringList componentsNeedingRecovery;
    
    auto unresolvedErrors = getUnresolvedErrors();
    for (const auto& error : unresolvedErrors) {
        if (error.severity >= CRITICAL && !componentsNeedingRecovery.contains(error.component)) {
            componentsNeedingRecovery.append(error.component);
        }
    }
    
    // Attempt recovery for each component
    for (const QString& component : componentsNeedingRecovery) {
        attemptComponentRecovery(component);
    }
    
    // Stop recovery timer if no more components need recovery
    if (componentsNeedingRecovery.isEmpty()) {
        m_recoveryTimer->stop();
    }
}
