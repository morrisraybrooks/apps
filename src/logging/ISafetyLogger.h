#ifndef ISAFETYLOGGER_H
#define ISAFETYLOGGER_H

#include <QString>
#include <QJsonObject>

/**
 * @brief Interface for safety-critical logging
 * 
 * This interface consolidates the safety logging pattern that was duplicated
 * across AntiDetachmentMonitor, SafetyManager, and other safety-critical components.
 * 
 * Implementations can delegate to DataLogger for unified log management including:
 * - Automatic log rotation
 * - Multiple output formats (CSV, JSON)
 * - Compression and archival
 * - Centralized log analysis
 */
class ISafetyLogger
{
public:
    virtual ~ISafetyLogger() = default;

    /**
     * @brief Log levels matching ErrorManager::ErrorSeverity for consistency
     */
    enum LogLevel {
        INFO,
        WARNING,
        CRITICAL,
        EVENT
    };

    /**
     * @brief Log a safety-related event
     * @param level The log level (INFO, WARNING, CRITICAL, EVENT)
     * @param component The component name (e.g., "AntiDetachmentMonitor")
     * @param event Description of the event
     * @param context Additional context data as JSON
     */
    virtual void logSafety(LogLevel level, const QString& component, 
                          const QString& event, const QJsonObject& context = QJsonObject()) = 0;

    /**
     * @brief Convenience method for INFO level logs
     */
    virtual void logInfo(const QString& component, const QString& event, 
                        const QJsonObject& context = QJsonObject()) {
        logSafety(INFO, component, event, context);
    }

    /**
     * @brief Convenience method for WARNING level logs
     */
    virtual void logWarning(const QString& component, const QString& event,
                           const QJsonObject& context = QJsonObject()) {
        logSafety(WARNING, component, event, context);
    }

    /**
     * @brief Convenience method for CRITICAL level logs
     */
    virtual void logCritical(const QString& component, const QString& event,
                            const QJsonObject& context = QJsonObject()) {
        logSafety(CRITICAL, component, event, context);
    }

    /**
     * @brief Convenience method for EVENT level logs
     */
    virtual void logEvent(const QString& component, const QString& event,
                         const QJsonObject& context = QJsonObject()) {
        logSafety(EVENT, component, event, context);
    }
};

#endif // ISAFETYLOGGER_H

