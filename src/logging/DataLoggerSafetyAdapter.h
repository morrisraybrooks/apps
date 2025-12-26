#ifndef DATALOGGERSAFETYADAPTER_H
#define DATALOGGERSAFETYADAPTER_H

#include "ISafetyLogger.h"
#include "DataLogger.h"
#include <QMutex>
#include <memory>

/**
 * @brief Adapter that bridges ISafetyLogger to the existing DataLogger
 * 
 * This adapter eliminates the need for separate CSV logging implementations
 * in safety-critical components by delegating to DataLogger's unified system.
 * 
 * Benefits:
 * - Single point of log configuration
 * - Automatic log rotation handled by DataLogger
 * - Consistent log format across all components
 * - No duplicate file I/O code
 * 
 * Thread Safety:
 * - All methods are thread-safe via mutex protection
 * - Safe to use from high-frequency monitoring loops (100Hz)
 */
class DataLoggerSafetyAdapter : public ISafetyLogger
{
public:
    /**
     * @brief Construct adapter with existing DataLogger
     * @param dataLogger The DataLogger instance to delegate to (must not be null)
     */
    explicit DataLoggerSafetyAdapter(DataLogger* dataLogger);
    ~DataLoggerSafetyAdapter() override = default;

    /**
     * @brief Log a safety event through the DataLogger
     * @param level Log severity level
     * @param component Component name (e.g., "AntiDetachmentMonitor")
     * @param event Event description
     * @param context Additional JSON context data
     */
    void logSafety(LogLevel level, const QString& component,
                   const QString& event, const QJsonObject& context = QJsonObject()) override;

    /**
     * @brief Check if the adapter is properly configured
     * @return true if DataLogger is available and logging is active
     */
    bool isActive() const;

private:
    DataLogger* m_dataLogger;
    mutable QMutex m_mutex;

    QString levelToString(LogLevel level) const;
};

#endif // DATALOGGERSAFETYADAPTER_H

