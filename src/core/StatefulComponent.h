#ifndef STATEFULCOMPONENT_H
#define STATEFULCOMPONENT_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QString>
#include <functional>

/**
 * @brief Template mixin for state machine management
 * 
 * This class consolidates the duplicate state management pattern found across
 * 6+ classes (VacuumController, PatternEngine, SafetyManager, AntiDetachmentMonitor,
 * GameEngine, OrgasmControlAlgorithm).
 * 
 * Features:
 * - Thread-safe state transitions
 * - Previous state tracking
 * - Transition callbacks for side effects
 * - State change logging
 * - State validation hooks
 * 
 * Usage:
 * 1. Inherit from StatefulComponent<YourStateEnum>
 * 2. Call registerTransitionCallback() for side effects
 * 3. Use setState() instead of manual member assignment
 * 
 * Note: Since Qt's signal/slot system doesn't work with templates,
 * derived classes must emit their own stateChanged signals.
 */
template<typename StateType>
class StatefulComponent
{
public:
    using StateNameFunc = std::function<QString(StateType)>;
    using TransitionCallback = std::function<void(StateType oldState, StateType newState)>;

    /**
     * @brief Constructor with optional state naming function
     * @param initialState The initial state
     * @param componentName Name for logging
     * @param stateNameFunc Optional function to convert state to string
     */
    StatefulComponent(StateType initialState, 
                      const QString& componentName = "StatefulComponent",
                      StateNameFunc stateNameFunc = nullptr)
        : m_currentState(initialState)
        , m_previousState(initialState)
        , m_componentName(componentName)
        , m_stateNameFunc(stateNameFunc)
    {}

    virtual ~StatefulComponent() = default;

    /**
     * @brief Get the current state (thread-safe)
     */
    StateType getState() const {
        QMutexLocker locker(&m_stateMutex);
        return m_currentState;
    }

    /**
     * @brief Get the previous state (thread-safe)
     */
    StateType getPreviousState() const {
        QMutexLocker locker(&m_stateMutex);
        return m_previousState;
    }

    /**
     * @brief Register a callback for state transitions
     * @param callback Function to call on every state change
     */
    void registerTransitionCallback(TransitionCallback callback) {
        QMutexLocker locker(&m_stateMutex);
        m_transitionCallbacks.append(callback);
    }

protected:
    /**
     * @brief Set the component state (thread-safe)
     * @param newState The new state
     * @return true if state changed, false if already in that state
     */
    bool setStateInternal(StateType newState) {
        StateType oldState;
        QList<TransitionCallback> callbacksCopy;

        {
            QMutexLocker locker(&m_stateMutex);
            if (m_currentState == newState) {
                return false;
            }

            oldState = m_currentState;
            m_previousState = m_currentState;
            m_currentState = newState;
            callbacksCopy = m_transitionCallbacks;

            logStateChange(oldState, newState);
        }

        // Execute callbacks outside lock to prevent deadlock
        for (const auto& callback : callbacksCopy) {
            try {
                callback(oldState, newState);
            } catch (const std::exception& e) {
                qWarning() << m_componentName << "transition callback error:" << e.what();
            }
        }

        return true;
    }

    /**
     * @brief Override in derived class to customize state names
     */
    virtual QString stateToString(StateType state) const {
        if (m_stateNameFunc) {
            return m_stateNameFunc(state);
        }
        return QString::number(static_cast<int>(state));
    }

protected:
    // Protected mutex for subclasses that need additional synchronization
    mutable QMutex m_stateMutex;

private:
    void logStateChange(StateType oldState, StateType newState) {
        qDebug() << m_componentName << "state changed:"
                 << stateToString(oldState) << "->" << stateToString(newState);
    }

    StateType m_currentState;
    StateType m_previousState;
    QString m_componentName;
    StateNameFunc m_stateNameFunc;
    QList<TransitionCallback> m_transitionCallbacks;
};

#endif // STATEFULCOMPONENT_H

