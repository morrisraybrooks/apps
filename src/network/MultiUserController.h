#ifndef MULTIUSERCONTROLLER_H
#define MULTIUSERCONTROLLER_H

#include "../game/GameTypes.h"
#include "../game/ProgressTracker.h"
#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QMap>
#include <QTimer>

/**
 * @brief Remote command structure for multi-user control
 */
struct RemoteCommand {
    QString commandId;
    QString senderId;
    QString senderName;
    QString targetId;
    ConsequenceAction action;
    double intensity;
    int durationMs;
    int pointCost;
    QDateTime timestamp;
};

/**
 * @brief Connected peer information
 */
struct ConnectedPeer {
    QString peerId;
    QString displayName;
    QWebSocket* socket;
    ConsentStatus consentStatus;
    bool isController;        // Can this peer control us?
    bool isControlled;        // Can we control this peer?
    QDateTime connectedAt;
    QDateTime lastHeartbeat;
};

/**
 * @brief Room for group control (DOM Master feature)
 */
struct ControlRoom {
    QString roomId;
    QString roomName;
    QString ownerId;
    QVector<QString> memberIds;
    int maxMembers;
    bool isPrivate;
    QDateTime createdAt;
};

/**
 * @brief Multi-user controller for networked device control
 * 
 * Enables paired users to send commands to each other's devices
 * with consent verification and point cost deduction.
 */
class MultiUserController : public QObject
{
    Q_OBJECT

public:
    explicit MultiUserController(ProgressTracker* progressTracker,
                                  QObject* parent = nullptr);
    ~MultiUserController();

    // Server mode (receiving commands)
    bool startServer(quint16 port = 8765);
    void stopServer();
    bool isServerRunning() const { return m_server && m_server->isListening(); }
    quint16 serverPort() const;

    // Client mode (sending commands)
    bool connectToPeer(const QString& address, quint16 port);
    void disconnectFromPeer(const QString& peerId);
    bool isConnectedTo(const QString& peerId) const;

    // Command sending (requires consent and points)
    bool sendCommand(const QString& targetId, ConsequenceAction action,
                     double intensity = 0.5, int durationMs = 500);
    bool sendCommandToRoom(const QString& roomId, ConsequenceAction action,
                           double intensity = 0.5, int durationMs = 500);

    // Room management (DOM Master tier)
    bool createRoom(const QString& roomName, int maxMembers = 10, bool isPrivate = true);
    bool joinRoom(const QString& roomId);
    bool leaveRoom(const QString& roomId);
    QVector<ControlRoom> availableRooms() const { return m_rooms; }

    // Consent management
    void grantControlTo(const QString& peerId, int expirationMinutes = 60);
    void revokeControlFrom(const QString& peerId);
    void revokeAllControl();  // Emergency stop

    // Point costs for commands
    static int commandPointCost(ConsequenceAction action, double intensity);

    // Connected peers
    QVector<ConnectedPeer> connectedPeers() const;
    int peerCount() const { return m_peers.size(); }

Q_SIGNALS:
    // Connection events
    void serverStarted(quint16 port);
    void serverStopped();
    void peerConnected(const QString& peerId, const QString& displayName);
    void peerDisconnected(const QString& peerId);
    void connectionError(const QString& error);

    // Command events
    void commandReceived(const RemoteCommand& command);
    void commandSent(const RemoteCommand& command);
    void commandRejected(const QString& reason);
    void commandExecuted(const RemoteCommand& command);

    // Consent events
    void consentRequested(const QString& peerId, const QString& displayName);
    void consentGranted(const QString& peerId);
    void consentRevoked(const QString& peerId);

    // Room events
    void roomCreated(const ControlRoom& room);
    void roomJoined(const QString& roomId);
    void roomLeft(const QString& roomId);
    void roomMemberJoined(const QString& roomId, const QString& peerId);
    void roomMemberLeft(const QString& roomId, const QString& peerId);

    // Safety events
    void emergencyStopReceived(const QString& fromPeerId);
    void safeWordActivated(const QString& peerId);

private Q_SLOTS:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onHeartbeatTimer();

private:
    void processMessage(QWebSocket* socket, const QJsonObject& msg);
    void handleHandshake(QWebSocket* socket, const QJsonObject& msg);
    void handleCommand(QWebSocket* socket, const QJsonObject& msg);
    void handleConsentRequest(QWebSocket* socket, const QJsonObject& msg);
    void handleConsentResponse(QWebSocket* socket, const QJsonObject& msg);
    void handleEmergencyStop(QWebSocket* socket, const QJsonObject& msg);
    void handleSafeWord(QWebSocket* socket, const QJsonObject& msg);
    void sendMessage(QWebSocket* socket, const QJsonObject& msg);
    bool verifyConsent(const QString& senderId, const QString& targetId);
    bool deductPoints(int amount, const QString& targetId, ConsequenceAction action);

    ProgressTracker* m_progressTracker;
    QWebSocketServer* m_server;
    QMap<QString, ConnectedPeer> m_peers;
    QVector<ControlRoom> m_rooms;
    QTimer* m_heartbeatTimer;

    static const int HEARTBEAT_INTERVAL_MS = 30000;
    static const int PEER_TIMEOUT_MS = 90000;
};

#endif // MULTIUSERCONTROLLER_H

