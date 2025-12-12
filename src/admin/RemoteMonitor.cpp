#include "RemoteMonitor.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

RemoteMonitor::RemoteMonitor(AccountManager* accountManager, QObject* parent)
    : QObject(parent)
    , m_accountManager(accountManager)
    , m_deviceRegistry(DeviceRegistry::instance())
{
    connectToDeviceRegistry();

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &RemoteMonitor::pollDevices);
}

RemoteMonitor::~RemoteMonitor()
{
    stopAllMonitoring();
}

void RemoteMonitor::connectToDeviceRegistry()
{
    connect(m_deviceRegistry, &DeviceRegistry::deviceStateUpdated,
            this, &RemoteMonitor::onDeviceStateUpdated);
    connect(m_deviceRegistry, &DeviceRegistry::deviceOffline,
            this, &RemoteMonitor::onDeviceOffline);
    connect(m_deviceRegistry, &DeviceRegistry::deviceOnline,
            this, &RemoteMonitor::onDeviceOnline);
}

bool RemoteMonitor::startMonitoring(const QString& deviceId, bool requestControl)
{
    // Check permissions
    if (!m_accountManager->isLoggedIn()) {
        emit errorOccurred(deviceId, "Not logged in");
        return false;
    }

    if (!canMonitor(m_accountManager->currentAccount().accountId, deviceId)) {
        emit errorOccurred(deviceId, "Permission denied");
        return false;
    }

    // Check if device exists
    DeviceInfo device = m_deviceRegistry->device(deviceId);
    if (device.deviceId.isEmpty()) {
        emit errorOccurred(deviceId, "Device not found");
        return false;
    }

    // Create session
    MonitorSession session;
    session.sessionId = generateSessionId();
    session.monitorAccountId = m_accountManager->currentAccount().accountId;
    session.targetAccountId = device.ownerAccountId;
    session.targetDeviceId = deviceId;
    session.startedAt = QDateTime::currentDateTime();
    session.isActive = true;
    session.hasControl = false;

    m_sessions[deviceId] = session;
    m_deviceRegistry->startMonitoring(deviceId);

    // Start polling if not already
    if (!m_pollTimer->isActive()) {
        m_pollTimer->start(m_pollInterval);
    }

    // Request control if needed
    if (requestControl) {
        this->requestControl(deviceId);
    }

    emit monitoringStarted(deviceId, session);

    // Log activity
    m_accountManager->logActivity(m_accountManager->currentAccount().accountId,
                                  "start_monitoring",
                                  QJsonObject{{"device_id", deviceId}, {"target_owner", device.ownerAccountId}});

    return true;
}

void RemoteMonitor::stopMonitoring(const QString& deviceId)
{
    if (!m_sessions.contains(deviceId)) return;

    // Release control if we have it
    if (m_sessions[deviceId].hasControl) {
        releaseControl(deviceId);
    }

    // Stop video stream if active
    if (m_streamSockets.contains(deviceId)) {
        stopVideoStream(deviceId);
    }

    m_sessions.remove(deviceId);
    m_latestData.remove(deviceId);
    m_deviceRegistry->stopMonitoring(deviceId);

    // Stop poll timer if no more sessions
    if (m_sessions.isEmpty()) {
        m_pollTimer->stop();
    }

    emit monitoringStopped(deviceId);
}

void RemoteMonitor::stopAllMonitoring()
{
    QStringList deviceIds = m_sessions.keys();
    for (const QString& deviceId : deviceIds) {
        stopMonitoring(deviceId);
    }
}

bool RemoteMonitor::isMonitoring(const QString& deviceId) const
{
    return m_sessions.contains(deviceId) && m_sessions[deviceId].isActive;
}

QList<MonitorSession> RemoteMonitor::activeSessions() const
{
    return m_sessions.values();
}

MonitorSession RemoteMonitor::session(const QString& deviceId) const
{
    return m_sessions.value(deviceId);
}

bool RemoteMonitor::requestControl(const QString& deviceId)
{
    if (!m_sessions.contains(deviceId)) return false;

    if (!canControl(m_accountManager->currentAccount().accountId, deviceId)) {
        emit controlDenied(deviceId, "Insufficient privileges");
        return false;
    }

    // Master accounts get immediate control
    if (m_accountManager->isMasterAccount()) {
        m_sessions[deviceId].hasControl = true;
        emit controlGranted(deviceId);

        m_accountManager->logActivity(m_accountManager->currentAccount().accountId,
                                      "take_control",
                                      QJsonObject{{"device_id", deviceId}});
        return true;
    }

    // Non-master would need consent flow (not implemented here)
    emit controlDenied(deviceId, "Control requires master privileges");
    return false;
}

void RemoteMonitor::releaseControl(const QString& deviceId)
{
    if (!m_sessions.contains(deviceId)) return;

    m_sessions[deviceId].hasControl = false;

    m_accountManager->logActivity(m_accountManager->currentAccount().accountId,
                                  "release_control",
                                  QJsonObject{{"device_id", deviceId}});

    emit controlReleased(deviceId);
}

bool RemoteMonitor::sendRemoteCommand(const QString& deviceId, const QString& command, const QJsonObject& params)
{
    if (!m_sessions.contains(deviceId)) return false;
    if (!m_sessions[deviceId].hasControl) {
        emit errorOccurred(deviceId, "No control over device");
        return false;
    }

    bool success = m_deviceRegistry->sendCommand(deviceId, command, params);

    if (success) {
        m_accountManager->logActivity(m_accountManager->currentAccount().accountId,
                                      "remote_command",
                                      QJsonObject{{"device_id", deviceId}, {"command", command}});
    }

    return success;
}

bool RemoteMonitor::emergencyStop(const QString& deviceId)
{
    // Emergency stop works even without control
    QJsonObject params;
    params["reason"] = "remote_emergency_stop";
    params["triggered_by"] = m_accountManager->currentAccount().accountId;

    bool success = m_deviceRegistry->sendCommand(deviceId, "emergency_stop", params);

    if (success) {
        emit emergencyStopTriggered(deviceId);
        m_accountManager->logActivity(m_accountManager->currentAccount().accountId,
                                      "emergency_stop",
                                      QJsonObject{{"device_id", deviceId}});
    }

    return success;
}

bool RemoteMonitor::emergencyStopAll()
{
    bool allSuccess = true;

    for (const QString& deviceId : m_sessions.keys()) {
        if (!emergencyStop(deviceId)) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

RemoteViewData RemoteMonitor::latestData(const QString& deviceId) const
{
    return m_latestData.value(deviceId);
}

QList<QJsonObject> RemoteMonitor::sessionHistory(const QString& deviceId, int hours) const
{
    Q_UNUSED(hours)
    // Would query database for historical session data
    return QList<QJsonObject>();
}

QList<QJsonObject> RemoteMonitor::activityFeed(const QString& deviceId, int limit) const
{
    DeviceInfo device = m_deviceRegistry->device(deviceId);
    if (device.ownerAccountId.isEmpty()) return QList<QJsonObject>();

    QVector<QJsonObject> vec = m_accountManager->activityLog(device.ownerAccountId, limit);
    return QList<QJsonObject>(vec.begin(), vec.end());
}

bool RemoteMonitor::startVideoStream(const QString& deviceId)
{
    if (!m_sessions.contains(deviceId)) return false;
    if (m_streamSockets.contains(deviceId)) return true;  // Already streaming

    // Send request to device to start streaming
    QJsonObject params;
    params["quality"] = "medium";
    params["fps"] = 15;

    bool success = m_deviceRegistry->sendCommand(deviceId, "start_video_stream", params);

    if (success) {
        m_sessions[deviceId].hasVideoFeed = true;
    }

    return success;
}

void RemoteMonitor::stopVideoStream(const QString& deviceId)
{
    if (!m_streamSockets.contains(deviceId)) return;

    m_deviceRegistry->sendCommand(deviceId, "stop_video_stream", QJsonObject());

    m_streamSockets[deviceId]->close();
    m_streamSockets[deviceId]->deleteLater();
    m_streamSockets.remove(deviceId);

    if (m_sessions.contains(deviceId)) {
        m_sessions[deviceId].hasVideoFeed = false;
    }
}

bool RemoteMonitor::isStreamingVideo(const QString& deviceId) const
{
    return m_streamSockets.contains(deviceId);
}

void RemoteMonitor::startBulkMonitoring(const QStringList& deviceIds)
{
    for (const QString& deviceId : deviceIds) {
        startMonitoring(deviceId, false);
    }
}

void RemoteMonitor::stopBulkMonitoring(const QStringList& deviceIds)
{
    for (const QString& deviceId : deviceIds) {
        stopMonitoring(deviceId);
    }
}

QList<RemoteViewData> RemoteMonitor::bulkData() const
{
    return m_latestData.values();
}

bool RemoteMonitor::canMonitor(const QString& accountId, const QString& deviceId) const
{
    UserAccount account = m_accountManager->getAccount(accountId);

    // Master can monitor everything
    if (account.isMaster()) return true;

    // Admin can monitor their sub-accounts' devices
    if (account.isAdmin()) {
        DeviceInfo device = m_deviceRegistry->device(deviceId);
        UserAccount owner = m_accountManager->getAccount(device.ownerAccountId);
        return owner.masterAccountId == accountId;
    }

    // Moderator can view (not control)
    if (account.canViewAll()) {
        return true;
    }

    // Users can only monitor their own devices
    DeviceInfo device = m_deviceRegistry->device(deviceId);
    return device.ownerAccountId == accountId;
}

bool RemoteMonitor::canControl(const QString& accountId, const QString& deviceId) const
{
    UserAccount account = m_accountManager->getAccount(accountId);

    // Master can control everything
    if (account.isMaster()) return true;

    // Admin can control their sub-accounts' devices
    if (account.isAdmin()) {
        DeviceInfo device = m_deviceRegistry->device(deviceId);
        UserAccount owner = m_accountManager->getAccount(device.ownerAccountId);
        return owner.masterAccountId == accountId;
    }

    // Users can only control their own devices
    DeviceInfo device = m_deviceRegistry->device(deviceId);
    return device.ownerAccountId == accountId;
}

void RemoteMonitor::onDeviceStateUpdated(const QString& deviceId, const QJsonObject& state)
{
    if (!m_sessions.contains(deviceId)) return;

    m_sessions[deviceId].lastState = state;

    RemoteViewData data;
    data.deviceId = deviceId;
    data.timestamp = QDateTime::currentDateTime();
    data.deviceState = state;

    DeviceInfo device = m_deviceRegistry->device(deviceId);
    data.batteryLevel = device.batteryLevel;
    data.isEmergencyStopped = state.value("emergency_stopped").toBool(false);

    if (state.contains("sensors")) {
        data.sensorData = state["sensors"].toObject();
    }
    if (state.contains("game")) {
        data.gameState = state["game"].toObject();
    }

    m_latestData[deviceId] = data;
    emit dataReceived(deviceId, data);
    emit stateChanged(deviceId, state);
}

void RemoteMonitor::onDeviceOffline(const QString& deviceId)
{
    if (!m_sessions.contains(deviceId)) return;

    m_sessions[deviceId].isActive = false;
    emit connectionLost(deviceId);
}

void RemoteMonitor::onDeviceOnline(const QString& deviceId)
{
    if (!m_sessions.contains(deviceId)) return;

    m_sessions[deviceId].isActive = true;
    emit connectionRestored(deviceId);
}

void RemoteMonitor::pollDevices()
{
    // Request state updates from all monitored devices
    for (const QString& deviceId : m_sessions.keys()) {
        if (m_sessions[deviceId].isActive) {
            m_deviceRegistry->sendCommand(deviceId, "request_state", QJsonObject());
        }
    }
}

QString RemoteMonitor::generateSessionId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void RemoteMonitor::sendMonitorRequest(const QString& deviceId, bool requestControl)
{
    QJsonObject params;
    params["session_id"] = m_sessions[deviceId].sessionId;
    params["request_control"] = requestControl;
    params["monitor_account"] = m_accountManager->currentAccount().accountId;

    m_deviceRegistry->sendCommand(deviceId, "start_monitoring", params);
}

void RemoteMonitor::processRemoteData(const QString& deviceId, const QJsonObject& data)
{
    onDeviceStateUpdated(deviceId, data);
}
