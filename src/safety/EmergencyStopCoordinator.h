#ifndef EMERGENCYSTOPCOORDINATOR_H
#define EMERGENCYSTOPCOORDINATOR_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <functional>

/**
 * @brief Centralized emergency stop coordination
 * 
 * This class eliminates the duplicate emergency stop patterns found across
 * 12+ files by providing a single registration point for shutdown callbacks.
 * 
 * Instead of each component implementing its own emergencyStop() method
 * that calls hardware directly, components register callbacks with this
 * coordinator. When an emergency stop is triggered, all callbacks execute
 * in priority order.
 * 
 * Benefits:
 * - Single point of emergency stop logic
 * - Guaranteed execution order
 * - No duplicate shutdown code
 * - Easier testing (mock the coordinator, not 12 components)
 * - Comprehensive logging of all shutdown actions
 * 
 * Thread Safety:
 * - All methods are thread-safe
 * - Callbacks are invoked with mutex released to prevent deadlock
 */
class EmergencyStopCoordinator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Priority levels for shutdown callbacks (higher = executed first)
     */
    enum Priority {
        PRIORITY_CRITICAL = 100,  // Hardware-level safety (pumps, valves)
        PRIORITY_HIGH = 75,       // Core subsystems (patterns, control)
        PRIORITY_NORMAL = 50,     // Standard components
        PRIORITY_LOW = 25,        // GUI, logging
        PRIORITY_CLEANUP = 0      // Final cleanup operations
    };

    /**
     * @brief Callback type for emergency stop handlers
     * @param reason The reason for the emergency stop
     */
    using EmergencyStopCallback = std::function<void(const QString& reason)>;

    explicit EmergencyStopCoordinator(QObject *parent = nullptr);
    ~EmergencyStopCoordinator() override;

    /**
     * @brief Register a component's emergency stop handler
     * @param componentName Name for logging/debugging
     * @param priority Execution priority (higher = earlier)
     * @param callback Function to call during emergency stop
     */
    void registerHandler(const QString& componentName, Priority priority,
                        EmergencyStopCallback callback);

    /**
     * @brief Unregister a component's handler
     * @param componentName Name of the component to unregister
     */
    void unregisterHandler(const QString& componentName);

    /**
     * @brief Trigger emergency stop across all registered components
     * @param reason Description of why the stop was triggered
     */
    void triggerEmergencyStop(const QString& reason);

    /**
     * @brief Check if emergency stop is currently active
     */
    bool isEmergencyStop() const;

    /**
     * @brief Reset emergency stop state (requires all handlers to acknowledge)
     * @return true if reset was successful
     */
    bool resetEmergencyStop();

    /**
     * @brief Get the reason for the current emergency stop
     */
    QString getLastReason() const;

    /**
     * @brief Get list of registered component names
     */
    QStringList getRegisteredComponents() const;

Q_SIGNALS:
    void emergencyStopTriggered(const QString& reason);
    void emergencyStopReset();
    void handlerExecuted(const QString& componentName, bool success);

private:
    struct RegisteredHandler {
        QString componentName;
        Priority priority;
        EmergencyStopCallback callback;
    };

    QList<RegisteredHandler> m_handlers;
    bool m_emergencyStop;
    QString m_lastReason;
    mutable QMutex m_mutex;

    void executeHandlers(const QString& reason);
};

#endif // EMERGENCYSTOPCOORDINATOR_H

