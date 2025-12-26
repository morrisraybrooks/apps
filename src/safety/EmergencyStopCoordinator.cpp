#include "EmergencyStopCoordinator.h"
#include <QMutexLocker>
#include <QDebug>
#include <algorithm>

EmergencyStopCoordinator::EmergencyStopCoordinator(QObject *parent)
    : QObject(parent)
    , m_emergencyStop(false)
{
    qDebug() << "EmergencyStopCoordinator initialized";
}

EmergencyStopCoordinator::~EmergencyStopCoordinator()
{
    QMutexLocker locker(&m_mutex);
    m_handlers.clear();
}

void EmergencyStopCoordinator::registerHandler(const QString& componentName,
                                               Priority priority,
                                               EmergencyStopCallback callback)
{
    QMutexLocker locker(&m_mutex);

    // Remove existing handler if present
    m_handlers.erase(
        std::remove_if(m_handlers.begin(), m_handlers.end(),
            [&componentName](const RegisteredHandler& h) {
                return h.componentName == componentName;
            }),
        m_handlers.end()
    );

    // Add new handler
    m_handlers.append({componentName, priority, callback});

    // Sort by priority (descending)
    std::sort(m_handlers.begin(), m_handlers.end(),
        [](const RegisteredHandler& a, const RegisteredHandler& b) {
            return a.priority > b.priority;
        });

    qDebug() << "Emergency stop handler registered:" << componentName
             << "priority:" << priority
             << "total handlers:" << m_handlers.size();
}

void EmergencyStopCoordinator::unregisterHandler(const QString& componentName)
{
    QMutexLocker locker(&m_mutex);

    m_handlers.erase(
        std::remove_if(m_handlers.begin(), m_handlers.end(),
            [&componentName](const RegisteredHandler& h) {
                return h.componentName == componentName;
            }),
        m_handlers.end()
    );

    qDebug() << "Emergency stop handler unregistered:" << componentName;
}

void EmergencyStopCoordinator::triggerEmergencyStop(const QString& reason)
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_emergencyStop) {
            qWarning() << "Emergency stop already active, ignoring new trigger:" << reason;
            return;
        }
        m_emergencyStop = true;
        m_lastReason = reason;
    }

    qCritical() << "EMERGENCY STOP TRIGGERED:" << reason;

    // Execute handlers outside the lock to prevent deadlock
    executeHandlers(reason);

    emit emergencyStopTriggered(reason);
}

void EmergencyStopCoordinator::executeHandlers(const QString& reason)
{
    // Take a snapshot of handlers to avoid holding lock during execution
    QList<RegisteredHandler> handlersSnapshot;
    {
        QMutexLocker locker(&m_mutex);
        handlersSnapshot = m_handlers;
    }

    for (const auto& handler : handlersSnapshot) {
        bool success = true;
        try {
            qDebug() << "Executing emergency stop handler:" << handler.componentName;
            handler.callback(reason);
            qDebug() << "Emergency stop handler completed:" << handler.componentName;
        } catch (const std::exception& e) {
            success = false;
            qCritical() << "Emergency stop handler failed:" << handler.componentName
                       << "error:" << e.what();
        } catch (...) {
            success = false;
            qCritical() << "Emergency stop handler failed with unknown exception:"
                       << handler.componentName;
        }
        emit handlerExecuted(handler.componentName, success);
    }
}

bool EmergencyStopCoordinator::isEmergencyStop() const
{
    QMutexLocker locker(&m_mutex);
    return m_emergencyStop;
}

bool EmergencyStopCoordinator::resetEmergencyStop()
{
    QMutexLocker locker(&m_mutex);
    if (!m_emergencyStop) {
        return true;
    }

    qDebug() << "Resetting emergency stop state";
    m_emergencyStop = false;
    m_lastReason.clear();

    locker.unlock();
    emit emergencyStopReset();
    return true;
}

QString EmergencyStopCoordinator::getLastReason() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastReason;
}

QStringList EmergencyStopCoordinator::getRegisteredComponents() const
{
    QMutexLocker locker(&m_mutex);
    QStringList names;
    for (const auto& h : m_handlers) {
        names.append(h.componentName);
    }
    return names;
}

