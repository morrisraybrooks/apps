#include "DataLoggerSafetyAdapter.h"
#include <QMutexLocker>
#include <QDebug>

DataLoggerSafetyAdapter::DataLoggerSafetyAdapter(DataLogger* dataLogger)
    : m_dataLogger(dataLogger)
{
    if (!m_dataLogger) {
        qWarning() << "DataLoggerSafetyAdapter created with null DataLogger - logging will be disabled";
    }
}

void DataLoggerSafetyAdapter::logSafety(LogLevel level, const QString& component,
                                        const QString& event, const QJsonObject& context)
{
    QMutexLocker locker(&m_mutex);

    if (!m_dataLogger) {
        // Fallback to qDebug when DataLogger unavailable
        qDebug() << QString("[%1] %2: %3").arg(levelToString(level), component, event);
        return;
    }

    // Build context with level info
    QJsonObject enrichedContext = context;
    enrichedContext["level"] = levelToString(level);

    // Delegate to DataLogger's safety event logging
    // This uses the existing SAFETY_EVENTS log type
    m_dataLogger->logSafetyEvent(event, component, enrichedContext);
}

bool DataLoggerSafetyAdapter::isActive() const
{
    QMutexLocker locker(&m_mutex);
    return m_dataLogger && m_dataLogger->isLogging();
}

QString DataLoggerSafetyAdapter::levelToString(LogLevel level) const
{
    switch (level) {
    case INFO:     return "INFO";
    case WARNING:  return "WARNING";
    case CRITICAL: return "CRITICAL";
    case EVENT:    return "EVENT";
    default:       return "UNKNOWN";
    }
}

