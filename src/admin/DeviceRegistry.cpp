#include "DeviceRegistry.h"
#include <QWebSocketServer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

// Helper function to check if a status represents an online state
static bool isOnlineStatus(DeviceStatus status) {
    return status == DeviceStatus::ONLINE ||
           status == DeviceStatus::BUSY ||
           status == DeviceStatus::IDLE;
}

DeviceRegistry* DeviceRegistry::s_instance = nullptr;

DeviceRegistry* DeviceRegistry::instance()
{
    if (!s_instance) {
        s_instance = new DeviceRegistry();
    }
    return s_instance;
}

DeviceRegistry::DeviceRegistry(QObject* parent)
    : QObject(parent)
{
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &DeviceRegistry::checkHeartbeats);
    m_heartbeatTimer->start(5000);  // Check every 5 seconds

    loadDevicesFromDatabase();
}

DeviceRegistry::~DeviceRegistry()
{
    m_heartbeatTimer->stop();

    // Close all device connections
    for (auto* socket : m_deviceSockets) {
        socket->close();
        socket->deleteLater();
    }
    m_deviceSockets.clear();
}

void DeviceRegistry::registerDevice(const DeviceInfo& device)
{
    m_devices[device.deviceId] = device;
    saveDeviceToDatabase(device);

    emit deviceRegistered(device);

    if (device.isOnline()) {
        emit deviceOnline(device.deviceId);
    }
}

void DeviceRegistry::unregisterDevice(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return;

    m_devices.remove(deviceId);
    m_monitoredDevices.removeAll(deviceId);

    if (m_deviceSockets.contains(deviceId)) {
        m_deviceSockets[deviceId]->close();
        m_deviceSockets[deviceId]->deleteLater();
        m_deviceSockets.remove(deviceId);
    }

    emit deviceUnregistered(deviceId);
}

void DeviceRegistry::updateDeviceStatus(const QString& deviceId, DeviceStatus status)
{
    if (!m_devices.contains(deviceId)) return;

    DeviceStatus oldStatus = m_devices[deviceId].status;
    m_devices[deviceId].status = status;

    emit deviceStatusChanged(deviceId, status);

    // Emit online/offline signals
    if (!isOnlineStatus(oldStatus) && isOnlineStatus(status)) {
        emit deviceOnline(deviceId);
    } else if (isOnlineStatus(oldStatus) && !isOnlineStatus(status)) {
        emit deviceOffline(deviceId);
    }
}

void DeviceRegistry::updateDeviceState(const QString& deviceId, const QJsonObject& state)
{
    if (!m_devices.contains(deviceId)) return;

    m_devices[deviceId].currentState = state;
    m_devices[deviceId].lastActivityAt = QDateTime::currentDateTime();

    emit deviceStateUpdated(deviceId, state);
}

void DeviceRegistry::recordHeartbeat(const QString& deviceId, const QJsonObject& data)
{
    if (!m_devices.contains(deviceId)) return;

    DeviceInfo& device = m_devices[deviceId];
    device.lastHeartbeatAt = QDateTime::currentDateTime();

    // Update device info from heartbeat data
    if (data.contains("battery")) {
        device.batteryLevel = data["battery"].toDouble();
    }
    if (data.contains("charging")) {
        device.isCharging = data["charging"].toBool();
    }
    if (data.contains("firmware")) {
        device.firmwareVersion = data["firmware"].toString();
    }
    if (data.contains("state")) {
        device.currentState = data["state"].toObject();
    }

    // Mark as online if was offline
    if (device.status == DeviceStatus::OFFLINE) {
        updateDeviceStatus(deviceId, DeviceStatus::IDLE);
    }

    emit deviceHeartbeat(deviceId);
}

DeviceInfo DeviceRegistry::device(const QString& deviceId) const
{
    return m_devices.value(deviceId);
}

QList<DeviceInfo> DeviceRegistry::allDevices() const
{
    return m_devices.values();
}

QList<DeviceInfo> DeviceRegistry::onlineDevices() const
{
    QList<DeviceInfo> online;
    for (const auto& device : m_devices) {
        if (device.isOnline()) {
            online.append(device);
        }
    }
    return online;
}

QList<DeviceInfo> DeviceRegistry::devicesByOwner(const QString& accountId) const
{
    QList<DeviceInfo> devices;
    for (const auto& device : m_devices) {
        if (device.ownerAccountId == accountId) {
            devices.append(device);
        }
    }
    return devices;
}

int DeviceRegistry::onlineCount() const
{
    return onlineDevices().count();
}

int DeviceRegistry::totalCount() const
{
    return m_devices.count();
}

void DeviceRegistry::startMonitoring(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return;
    if (m_monitoredDevices.contains(deviceId)) return;

    m_monitoredDevices.append(deviceId);
    emit monitoringStarted(deviceId);

    // Request immediate state update
    sendCommand(deviceId, "request_state", QJsonObject());
}

void DeviceRegistry::stopMonitoring(const QString& deviceId)
{
    if (!m_monitoredDevices.contains(deviceId)) return;

    m_monitoredDevices.removeAll(deviceId);
    emit monitoringStopped(deviceId);
}

bool DeviceRegistry::isMonitoring(const QString& deviceId) const
{
    return m_monitoredDevices.contains(deviceId);
}

QList<QString> DeviceRegistry::monitoredDevices() const
{
    return m_monitoredDevices;
}

bool DeviceRegistry::sendCommand(const QString& deviceId, const QString& command, const QJsonObject& params)
{
    if (!m_deviceSockets.contains(deviceId)) {
        emit commandFailed(deviceId, "Device not connected");
        return false;
    }

    QJsonObject message;
    message["type"] = "command";
    message["command"] = command;
    message["params"] = params;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_deviceSockets[deviceId]->sendTextMessage(
        QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));

    emit commandSent(deviceId, command);
    return true;
}

bool DeviceRegistry::forceDisconnect(const QString& deviceId, const QString& reason)
{
    if (!m_deviceSockets.contains(deviceId)) return false;

    // Send disconnect message
    QJsonObject message;
    message["type"] = "force_disconnect";
    message["reason"] = reason;
    m_deviceSockets[deviceId]->sendTextMessage(
        QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));

    // Close socket
    m_deviceSockets[deviceId]->close();
    m_deviceSockets[deviceId]->deleteLater();
    m_deviceSockets.remove(deviceId);

    updateDeviceStatus(deviceId, DeviceStatus::OFFLINE);
    return true;
}

bool DeviceRegistry::lockDevice(const QString& deviceId, const QString& reason)
{
    if (!m_devices.contains(deviceId)) return false;

    updateDeviceStatus(deviceId, DeviceStatus::MAINTENANCE);

    if (m_deviceSockets.contains(deviceId)) {
        QJsonObject message;
        message["type"] = "lock";
        message["reason"] = reason;
        m_deviceSockets[deviceId]->sendTextMessage(
            QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));
    }

    return true;
}

bool DeviceRegistry::unlockDevice(const QString& deviceId)
{
    if (!m_devices.contains(deviceId)) return false;
    if (m_devices[deviceId].status != DeviceStatus::MAINTENANCE) return false;

    updateDeviceStatus(deviceId, DeviceStatus::IDLE);

    if (m_deviceSockets.contains(deviceId)) {
        QJsonObject message;
        message["type"] = "unlock";
        m_deviceSockets[deviceId]->sendTextMessage(
            QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));
    }

    return true;
}

void DeviceRegistry::checkHeartbeats()
{
    QDateTime now = QDateTime::currentDateTime();

    for (auto& device : m_devices) {
        if (!device.isOnline()) continue;

        int secondsSince = device.lastHeartbeatAt.secsTo(now);
        if (secondsSince > m_heartbeatTimeout) {
            emit deviceTimeout(device.deviceId);
            updateDeviceStatus(device.deviceId, DeviceStatus::OFFLINE);
        }
    }
}

void DeviceRegistry::processIncomingMessage(const QString& deviceId, const QJsonObject& message)
{
    QString type = message["type"].toString();

    if (type == "heartbeat") {
        recordHeartbeat(deviceId, message);
    } else if (type == "state_update") {
        updateDeviceState(deviceId, message["state"].toObject());
    } else if (type == "status_change") {
        DeviceStatus status = static_cast<DeviceStatus>(message["status"].toInt());
        updateDeviceStatus(deviceId, status);
    }
}

void DeviceRegistry::saveDeviceToDatabase(const DeviceInfo& device)
{
    QSqlDatabase db = QSqlDatabase::database("AccountManagerDB");
    if (!db.isValid()) return;

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT OR REPLACE INTO devices (device_id, device_name, type, owner_account_id,
            firmware_version, software_version, first_seen_at, capabilities)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(device.deviceId);
    query.addBindValue(device.deviceName);
    query.addBindValue(static_cast<int>(device.type));
    query.addBindValue(device.ownerAccountId);
    query.addBindValue(device.firmwareVersion);
    query.addBindValue(device.softwareVersion);
    query.addBindValue(device.firstSeenAt.toString(Qt::ISODate));
    query.addBindValue(QString::fromUtf8(QJsonDocument(device.capabilities).toJson(QJsonDocument::Compact)));
    query.exec();
}

void DeviceRegistry::loadDevicesFromDatabase()
{
    QSqlDatabase db = QSqlDatabase::database("AccountManagerDB");
    if (!db.isValid()) return;

    // Create devices table if not exists
    QSqlQuery createQuery(db);
    createQuery.exec(R"(
        CREATE TABLE IF NOT EXISTS devices (
            device_id TEXT PRIMARY KEY,
            device_name TEXT,
            type INTEGER,
            owner_account_id TEXT,
            firmware_version TEXT,
            software_version TEXT,
            first_seen_at TEXT,
            capabilities TEXT
        )
    )");

    QSqlQuery query(db);
    query.exec("SELECT * FROM devices");

    while (query.next()) {
        DeviceInfo device;
        device.deviceId = query.value("device_id").toString();
        device.deviceName = query.value("device_name").toString();
        device.type = static_cast<DeviceType>(query.value("type").toInt());
        device.ownerAccountId = query.value("owner_account_id").toString();
        device.firmwareVersion = query.value("firmware_version").toString();
        device.softwareVersion = query.value("software_version").toString();
        device.firstSeenAt = QDateTime::fromString(query.value("first_seen_at").toString(), Qt::ISODate);
        device.status = DeviceStatus::OFFLINE;  // All start offline until heartbeat
        device.lastHeartbeatAt = QDateTime();

        QJsonDocument caps = QJsonDocument::fromJson(query.value("capabilities").toString().toUtf8());
        device.capabilities = caps.object();

        m_devices[device.deviceId] = device;
    }
}
