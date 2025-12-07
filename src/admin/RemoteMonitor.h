#ifndef REMOTEMONITOR_H
#define REMOTEMONITOR_H

#include "DeviceRegistry.h"
#include "AccountManager.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QWebSocket>
#include <QJsonObject>
#include <QImage>

/**
 * @brief Remote monitoring session information
 */
struct MonitorSession {
    QString sessionId;
    QString monitorAccountId;    // Who is monitoring
    QString targetAccountId;     // Whose device is being monitored
    QString targetDeviceId;
    QDateTime startedAt;
    bool isActive = false;
    bool hasControl = false;     // Can send commands
    bool hasVideoFeed = false;   // Receiving camera feed
    bool hasAudioFeed = false;   // Receiving audio
    QJsonObject lastState;
};

/**
 * @brief Remote view data from device
 */
struct RemoteViewData {
    QString deviceId;
    QDateTime timestamp;
    QJsonObject deviceState;     // Current intensity, modes, etc.
    QJsonObject sensorData;      // Motion, pressure, temperature
    QJsonObject gameState;       // Active game info
    double batteryLevel = 0;
    bool isEmergencyStopped = false;
    QImage cameraFrame;          // Optional camera feed
};

/**
 * @brief Remote Monitor for master account surveillance
 * 
 * Allows master accounts to:
 * - View any device in real-time
 * - See current session state and settings
 * - Monitor sensor data and activity
 * - Take control of devices (with permission)
 * - View activity logs and history
 */
class RemoteMonitor : public QObject
{
    Q_OBJECT

public:
    explicit RemoteMonitor(AccountManager* accountManager, QObject* parent = nullptr);
    ~RemoteMonitor();

    // Session management
    bool startMonitoring(const QString& deviceId, bool requestControl = false);
    void stopMonitoring(const QString& deviceId);
    void stopAllMonitoring();
    bool isMonitoring(const QString& deviceId) const;
    QList<MonitorSession> activeSessions() const;
    MonitorSession session(const QString& deviceId) const;

    // Control operations (master only)
    bool requestControl(const QString& deviceId);
    void releaseControl(const QString& deviceId);
    bool sendRemoteCommand(const QString& deviceId, const QString& command, const QJsonObject& params);
    bool emergencyStop(const QString& deviceId);
    bool emergencyStopAll();

    // View data
    RemoteViewData latestData(const QString& deviceId) const;
    QList<QJsonObject> sessionHistory(const QString& deviceId, int hours = 24) const;
    QList<QJsonObject> activityFeed(const QString& deviceId, int limit = 100) const;

    // Video/Audio streaming
    bool startVideoStream(const QString& deviceId);
    void stopVideoStream(const QString& deviceId);
    bool isStreamingVideo(const QString& deviceId) const;

    // Bulk operations (monitor multiple devices)
    void startBulkMonitoring(const QStringList& deviceIds);
    void stopBulkMonitoring(const QStringList& deviceIds);
    QList<RemoteViewData> bulkData() const;

    // Access control
    bool canMonitor(const QString& accountId, const QString& deviceId) const;
    bool canControl(const QString& accountId, const QString& deviceId) const;

Q_SIGNALS:
    void monitoringStarted(const QString& deviceId, const MonitorSession& session);
    void monitoringStopped(const QString& deviceId);
    void controlGranted(const QString& deviceId);
    void controlDenied(const QString& deviceId, const QString& reason);
    void controlReleased(const QString& deviceId);
    void dataReceived(const QString& deviceId, const RemoteViewData& data);
    void stateChanged(const QString& deviceId, const QJsonObject& state);
    void emergencyStopTriggered(const QString& deviceId);
    void videoFrameReceived(const QString& deviceId, const QImage& frame);
    void connectionLost(const QString& deviceId);
    void connectionRestored(const QString& deviceId);
    void errorOccurred(const QString& deviceId, const QString& error);

private Q_SLOTS:
    void onDeviceStateUpdated(const QString& deviceId, const QJsonObject& state);
    void onDeviceOffline(const QString& deviceId);
    void onDeviceOnline(const QString& deviceId);
    void pollDevices();

private:
    void connectToDeviceRegistry();
    QString generateSessionId() const;
    void sendMonitorRequest(const QString& deviceId, bool requestControl);
    void processRemoteData(const QString& deviceId, const QJsonObject& data);

    AccountManager* m_accountManager;
    DeviceRegistry* m_deviceRegistry;
    QMap<QString, MonitorSession> m_sessions;
    QMap<QString, RemoteViewData> m_latestData;
    QMap<QString, QWebSocket*> m_streamSockets;
    QTimer* m_pollTimer;
    int m_pollInterval = 500;  // ms
};

#endif // REMOTEMONITOR_H

