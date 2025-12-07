#ifndef DEVICEREGISTRY_H
#define DEVICEREGISTRY_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QWebSocket>
#include <QJsonObject>

/**
 * @brief Device connection status
 */
enum class DeviceStatus {
    ONLINE,
    OFFLINE,
    BUSY,           // In active session
    IDLE,           // Connected but inactive
    MAINTENANCE,
    ERROR
};
Q_DECLARE_METATYPE(DeviceStatus)

/**
 * @brief Device type enumeration
 */
enum class DeviceType {
    V_CONTOUR_BASIC,
    V_CONTOUR_PRO,
    V_CONTOUR_CLINICAL,
    UNKNOWN
};
Q_DECLARE_METATYPE(DeviceType)

/**
 * @brief Device information structure
 */
struct DeviceInfo {
    QString deviceId;
    QString deviceName;
    DeviceType type = DeviceType::UNKNOWN;
    DeviceStatus status = DeviceStatus::OFFLINE;
    QString ownerAccountId;
    QString currentUserId;      // Who is currently using the device
    QString ipAddress;
    QString firmwareVersion;
    QString softwareVersion;
    QDateTime firstSeenAt;
    QDateTime lastHeartbeatAt;
    QDateTime lastActivityAt;
    double batteryLevel = 100.0;
    bool isCharging = false;
    QJsonObject currentState;   // Current device state (intensity, mode, etc.)
    QJsonObject capabilities;   // What the device supports
    
    bool isOnline() const { return status == DeviceStatus::ONLINE || status == DeviceStatus::BUSY || status == DeviceStatus::IDLE; }
    int secondsSinceHeartbeat() const { return lastHeartbeatAt.secsTo(QDateTime::currentDateTime()); }
};

/**
 * @brief Device Registry for tracking all connected devices
 * 
 * Master accounts can:
 * - View all connected devices
 * - Monitor device status in real-time
 * - Connect to and view any device
 * - Send commands to any device (with permissions)
 */
class DeviceRegistry : public QObject
{
    Q_OBJECT

public:
    static DeviceRegistry* instance();
    
    // Device registration
    void registerDevice(const DeviceInfo& device);
    void unregisterDevice(const QString& deviceId);
    void updateDeviceStatus(const QString& deviceId, DeviceStatus status);
    void updateDeviceState(const QString& deviceId, const QJsonObject& state);
    void recordHeartbeat(const QString& deviceId, const QJsonObject& data);
    
    // Device queries
    DeviceInfo device(const QString& deviceId) const;
    QList<DeviceInfo> allDevices() const;
    QList<DeviceInfo> onlineDevices() const;
    QList<DeviceInfo> devicesByOwner(const QString& accountId) const;
    QList<DeviceInfo> devicesByStatus(DeviceStatus status) const;
    int onlineCount() const;
    int totalCount() const;
    
    // Real-time monitoring
    void startMonitoring(const QString& deviceId);
    void stopMonitoring(const QString& deviceId);
    bool isMonitoring(const QString& deviceId) const;
    QList<QString> monitoredDevices() const;
    
    // Remote control (master only)
    bool sendCommand(const QString& deviceId, const QString& command, const QJsonObject& params);
    bool forceDisconnect(const QString& deviceId, const QString& reason);
    bool lockDevice(const QString& deviceId, const QString& reason);
    bool unlockDevice(const QString& deviceId);
    
    // Configuration
    void setHeartbeatTimeout(int seconds) { m_heartbeatTimeout = seconds; }
    int heartbeatTimeout() const { return m_heartbeatTimeout; }

Q_SIGNALS:
    void deviceRegistered(const DeviceInfo& device);
    void deviceUnregistered(const QString& deviceId);
    void deviceStatusChanged(const QString& deviceId, DeviceStatus status);
    void deviceStateUpdated(const QString& deviceId, const QJsonObject& state);
    void deviceHeartbeat(const QString& deviceId);
    void deviceTimeout(const QString& deviceId);
    void deviceOnline(const QString& deviceId);
    void deviceOffline(const QString& deviceId);
    void commandSent(const QString& deviceId, const QString& command);
    void commandFailed(const QString& deviceId, const QString& error);
    void monitoringStarted(const QString& deviceId);
    void monitoringStopped(const QString& deviceId);

private Q_SLOTS:
    void checkHeartbeats();
    void processIncomingMessage(const QString& deviceId, const QJsonObject& message);

private:
    explicit DeviceRegistry(QObject* parent = nullptr);
    ~DeviceRegistry();
    
    void initWebSocketServer();
    void saveDeviceToDatabase(const DeviceInfo& device);
    void loadDevicesFromDatabase();

    QMap<QString, DeviceInfo> m_devices;
    QMap<QString, QWebSocket*> m_deviceSockets;
    QStringList m_monitoredDevices;
    QTimer* m_heartbeatTimer;
    int m_heartbeatTimeout = 30;  // seconds
    
    static DeviceRegistry* s_instance;
};

#endif // DEVICEREGISTRY_H

