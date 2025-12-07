#include "MultiUserController.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>

MultiUserController::MultiUserController(ProgressTracker* progressTracker, QObject* parent)
    : QObject(parent)
    , m_progressTracker(progressTracker)
    , m_server(nullptr)
    , m_heartbeatTimer(new QTimer(this))
{
    connect(m_heartbeatTimer, &QTimer::timeout, this, &MultiUserController::onHeartbeatTimer);
}

MultiUserController::~MultiUserController()
{
    stopServer();
    for (auto& peer : m_peers) {
        if (peer.socket) {
            peer.socket->close();
            peer.socket->deleteLater();
        }
    }
}

// ============================================================================
// Server Mode
// ============================================================================

bool MultiUserController::startServer(quint16 port)
{
    if (m_server) {
        qWarning() << "Server already running";
        return false;
    }

    m_server = new QWebSocketServer("V-Contour Control Server",
                                     QWebSocketServer::NonSecureMode, this);

    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to start server:" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection,
            this, &MultiUserController::onNewConnection);

    m_heartbeatTimer->start(HEARTBEAT_INTERVAL_MS);

    qDebug() << "Multi-user control server started on port" << port;
    emit serverStarted(port);
    return true;
}

void MultiUserController::stopServer()
{
    if (!m_server) return;

    m_heartbeatTimer->stop();

    // Disconnect all peers
    for (auto& peer : m_peers) {
        if (peer.socket) {
            peer.socket->close();
        }
    }
    m_peers.clear();

    m_server->close();
    delete m_server;
    m_server = nullptr;

    qDebug() << "Multi-user control server stopped";
    emit serverStopped();
}

quint16 MultiUserController::serverPort() const
{
    return m_server ? m_server->serverPort() : 0;
}

// ============================================================================
// Client Mode
// ============================================================================

bool MultiUserController::connectToPeer(const QString& address, quint16 port)
{
    QWebSocket* socket = new QWebSocket();

    connect(socket, &QWebSocket::connected, [this, socket]() {
        // Send handshake
        QJsonObject handshake;
        handshake["type"] = "handshake";
        handshake["userId"] = m_progressTracker->profile().id;
        handshake["displayName"] = m_progressTracker->profile().displayName;
        handshake["privilegeTier"] = static_cast<int>(m_progressTracker->privilegeTier());
        sendMessage(socket, handshake);
    });

    connect(socket, &QWebSocket::textMessageReceived,
            this, &MultiUserController::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected,
            this, &MultiUserController::onSocketDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &MultiUserController::onSocketError);

    QUrl url;
    url.setScheme("ws");
    url.setHost(address);
    url.setPort(port);

    socket->open(url);
    return true;
}

void MultiUserController::disconnectFromPeer(const QString& peerId)
{
    if (m_peers.contains(peerId)) {
        ConnectedPeer& peer = m_peers[peerId];
        if (peer.socket) {
            peer.socket->close();
        }
        m_peers.remove(peerId);
        emit peerDisconnected(peerId);
    }
}

bool MultiUserController::isConnectedTo(const QString& peerId) const
{
    return m_peers.contains(peerId);
}

// ============================================================================
// Command Sending
// ============================================================================

bool MultiUserController::sendCommand(const QString& targetId, ConsequenceAction action,
                                        double intensity, int durationMs)
{
    // Verify privilege tier
    PrivilegeTier tier = m_progressTracker->privilegeTier();
    if (tier < PrivilegeTier::INTERMEDIATE) {
        emit commandRejected("Insufficient privilege tier for DOM commands");
        return false;
    }

    // Check if targeting self or other
    bool targetingSelf = (targetId == m_progressTracker->profile().id);
    if (!targetingSelf && tier < PrivilegeTier::ADVANCED) {
        emit commandRejected("Advanced tier required to control other users");
        return false;
    }

    // Verify consent for remote control
    if (!targetingSelf && !verifyConsent(m_progressTracker->profile().id, targetId)) {
        emit commandRejected("No valid consent from target user");
        return false;
    }

    // Calculate and deduct points
    int cost = commandPointCost(action, intensity);
    if (!targetingSelf && !deductPoints(cost, targetId, action)) {
        emit commandRejected("Insufficient points for command");
        return false;
    }

    // Find peer and send command
    if (!m_peers.contains(targetId)) {
        emit commandRejected("Target user not connected");
        return false;
    }

    RemoteCommand cmd;
    cmd.commandId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cmd.senderId = m_progressTracker->profile().id;
    cmd.senderName = m_progressTracker->profile().displayName;
    cmd.targetId = targetId;
    cmd.action = action;
    cmd.intensity = intensity;
    cmd.durationMs = durationMs;
    cmd.pointCost = cost;
    cmd.timestamp = QDateTime::currentDateTime();

    QJsonObject msg;
    msg["type"] = "command";
    msg["commandId"] = cmd.commandId;
    msg["action"] = static_cast<int>(action);
    msg["intensity"] = intensity;
    msg["durationMs"] = durationMs;
    msg["pointCost"] = cost;

    sendMessage(m_peers[targetId].socket, msg);

    // Log the command
    m_progressTracker->logCommand(QString::number(static_cast<int>(action)),
                                   targetId, cost, true);

    emit commandSent(cmd);
    return true;
}

bool MultiUserController::sendCommandToRoom(const QString& roomId, ConsequenceAction action,
                                             double intensity, int durationMs)
{
    // DOM Master tier required for room control
    if (m_progressTracker->privilegeTier() < PrivilegeTier::DOM_MASTER) {
        emit commandRejected("DOM Master tier required for room control");
        return false;
    }

    // Find room
    ControlRoom* room = nullptr;
    for (auto& r : m_rooms) {
        if (r.roomId == roomId) {
            room = &r;
            break;
        }
    }

    if (!room) {
        emit commandRejected("Room not found");
        return false;
    }

    // Calculate total cost (per member)
    int costPerMember = commandPointCost(action, intensity);
    int totalCost = costPerMember * room->memberIds.size();

    if (!m_progressTracker->canAfford(totalCost)) {
        emit commandRejected("Insufficient points for room command");
        return false;
    }

    // Send to all room members
    int successCount = 0;
    for (const QString& memberId : room->memberIds) {
        if (sendCommand(memberId, action, intensity, durationMs)) {
            successCount++;
        }
    }

    return successCount > 0;
}

// ============================================================================
// Point Cost Calculation
// ============================================================================

int MultiUserController::commandPointCost(ConsequenceAction action, double intensity)
{
    int baseCost = 0;

    switch (action) {
        // High-cost punishments
        case ConsequenceAction::TENS_SHOCK:
            baseCost = 50;
            break;
        case ConsequenceAction::TENS_BURST_SERIES:
            baseCost = 100;
            break;
        case ConsequenceAction::COMBINED_ASSAULT:
            baseCost = 150;
            break;

        // Medium-cost actions
        case ConsequenceAction::MAX_VACUUM_PULSE:
            baseCost = 40;
            break;
        case ConsequenceAction::INTENSITY_INCREASE:
            baseCost = 20;
            break;
        case ConsequenceAction::DENIAL_EXTENSION:
            baseCost = 30;
            break;
        case ConsequenceAction::FORCED_EDGE:
            baseCost = 35;
            break;

        // Low-cost actions
        case ConsequenceAction::PATTERN_SWITCH:
            baseCost = 10;
            break;
        case ConsequenceAction::AUDIO_WARNING:
            baseCost = 5;
            break;
        case ConsequenceAction::HAPTIC_PULSE:
            baseCost = 10;
            break;

        // Rewards (no cost)
        case ConsequenceAction::PLEASURE_BURST:
        case ConsequenceAction::INTENSITY_DECREASE:
            baseCost = 0;
            break;

        default:
            baseCost = 15;
            break;
    }

    // Scale by intensity (0.5-1.5x)
    double intensityMultiplier = 0.5 + intensity;
    return static_cast<int>(baseCost * intensityMultiplier);
}



// ============================================================================
// Room Management
// ============================================================================

bool MultiUserController::createRoom(const QString& roomName, int maxMembers, bool isPrivate)
{
    if (m_progressTracker->privilegeTier() < PrivilegeTier::DOM_MASTER) {
        qWarning() << "DOM Master tier required to create rooms";
        return false;
    }

    ControlRoom room;
    room.roomId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    room.roomName = roomName;
    room.ownerId = m_progressTracker->profile().id;
    room.maxMembers = maxMembers;
    room.isPrivate = isPrivate;
    room.createdAt = QDateTime::currentDateTime();

    m_rooms.append(room);
    emit roomCreated(room);
    return true;
}

bool MultiUserController::joinRoom(const QString& roomId)
{
    for (auto& room : m_rooms) {
        if (room.roomId == roomId) {
            if (room.memberIds.size() >= room.maxMembers) {
                return false;
            }
            room.memberIds.append(m_progressTracker->profile().id);
            emit roomJoined(roomId);
            return true;
        }
    }
    return false;
}

bool MultiUserController::leaveRoom(const QString& roomId)
{
    for (auto& room : m_rooms) {
        if (room.roomId == roomId) {
            room.memberIds.removeAll(m_progressTracker->profile().id);
            emit roomLeft(roomId);
            return true;
        }
    }
    return false;
}

// ============================================================================
// Consent Management
// ============================================================================

void MultiUserController::grantControlTo(const QString& peerId, int expirationMinutes)
{
    if (!m_peers.contains(peerId)) return;

    m_progressTracker->grantConsent(peerId, expirationMinutes);

    QJsonObject msg;
    msg["type"] = "consent_granted";
    msg["expirationMinutes"] = expirationMinutes;

    sendMessage(m_peers[peerId].socket, msg);
    emit consentGranted(peerId);
}

void MultiUserController::revokeControlFrom(const QString& peerId)
{
    m_progressTracker->revokeConsent(peerId);

    if (m_peers.contains(peerId)) {
        QJsonObject msg;
        msg["type"] = "consent_revoked";
        sendMessage(m_peers[peerId].socket, msg);
    }

    emit consentRevoked(peerId);
}

void MultiUserController::revokeAllControl()
{
    // Emergency stop - revoke all consent immediately
    for (const auto& peer : m_peers) {
        m_progressTracker->revokeConsent(peer.peerId);

        QJsonObject msg;
        msg["type"] = "emergency_stop";
        sendMessage(peer.socket, msg);

        emit consentRevoked(peer.peerId);
    }
}

QVector<ConnectedPeer> MultiUserController::connectedPeers() const
{
    QVector<ConnectedPeer> peers;
    for (const auto& peer : m_peers) {
        peers.append(peer);
    }
    return peers;
}

// ============================================================================
// WebSocket Event Handlers
// ============================================================================

void MultiUserController::onNewConnection()
{
    QWebSocket* socket = m_server->nextPendingConnection();

    connect(socket, &QWebSocket::textMessageReceived,
            this, &MultiUserController::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected,
            this, &MultiUserController::onSocketDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &MultiUserController::onSocketError);

    qDebug() << "New connection from" << socket->peerAddress().toString();
}

void MultiUserController::onTextMessageReceived(const QString& message)
{
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON message received";
        return;
    }

    processMessage(socket, doc.object());
}

void MultiUserController::onBinaryMessageReceived(const QByteArray& message)
{
    Q_UNUSED(message);
    // Binary messages not used in this protocol
}

void MultiUserController::onSocketDisconnected()
{
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    // Find and remove peer
    for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
        if (it->socket == socket) {
            QString peerId = it->peerId;
            m_peers.erase(it);
            emit peerDisconnected(peerId);
            break;
        }
    }

    socket->deleteLater();
}


void MultiUserController::onSocketError(QAbstractSocket::SocketError error)
{
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        emit connectionError(socket->errorString());
    }
}

void MultiUserController::onHeartbeatTimer()
{
    QDateTime now = QDateTime::currentDateTime();

    // Check for timed-out peers
    QVector<QString> timedOutPeers;
    for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
        if (it->lastHeartbeat.msecsTo(now) > PEER_TIMEOUT_MS) {
            timedOutPeers.append(it->peerId);
        }
    }

    for (const QString& peerId : timedOutPeers) {
        disconnectFromPeer(peerId);
    }

    // Send heartbeat to all peers
    QJsonObject heartbeat;
    heartbeat["type"] = "heartbeat";
    heartbeat["timestamp"] = now.toString(Qt::ISODate);

    for (auto& peer : m_peers) {
        sendMessage(peer.socket, heartbeat);
    }
}

// ============================================================================
// Message Processing
// ============================================================================

void MultiUserController::processMessage(QWebSocket* socket, const QJsonObject& msg)
{
    QString type = msg["type"].toString();

    if (type == "handshake") {
        handleHandshake(socket, msg);
    } else if (type == "command") {
        handleCommand(socket, msg);
    } else if (type == "consent_request") {
        handleConsentRequest(socket, msg);
    } else if (type == "consent_response") {
        handleConsentResponse(socket, msg);
    } else if (type == "emergency_stop") {
        handleEmergencyStop(socket, msg);
    } else if (type == "safe_word") {
        handleSafeWord(socket, msg);
    } else if (type == "heartbeat") {
        // Update last heartbeat time
        for (auto& peer : m_peers) {
            if (peer.socket == socket) {
                peer.lastHeartbeat = QDateTime::currentDateTime();
                break;
            }
        }
    }
}

void MultiUserController::handleHandshake(QWebSocket* socket, const QJsonObject& msg)
{
    QString peerId = msg["userId"].toString();
    QString displayName = msg["displayName"].toString();

    ConnectedPeer peer;
    peer.peerId = peerId;
    peer.displayName = displayName;
    peer.socket = socket;
    peer.consentStatus = ConsentStatus::NONE;
    peer.isController = false;
    peer.isControlled = false;
    peer.connectedAt = QDateTime::currentDateTime();
    peer.lastHeartbeat = QDateTime::currentDateTime();

    m_peers[peerId] = peer;

    // Send our handshake response
    QJsonObject response;
    response["type"] = "handshake_ack";
    response["userId"] = m_progressTracker->profile().id;
    response["displayName"] = m_progressTracker->profile().displayName;
    response["privilegeTier"] = static_cast<int>(m_progressTracker->privilegeTier());
    sendMessage(socket, response);

    emit peerConnected(peerId, displayName);
}

void MultiUserController::handleCommand(QWebSocket* socket, const QJsonObject& msg)
{
    // Find sender
    QString senderId;
    for (const auto& peer : m_peers) {
        if (peer.socket == socket) {
            senderId = peer.peerId;
            break;
        }
    }

    if (senderId.isEmpty()) return;

    // Verify consent
    if (!m_progressTracker->hasValidConsent(senderId)) {
        QJsonObject reject;
        reject["type"] = "command_rejected";
        reject["reason"] = "No valid consent";
        sendMessage(socket, reject);
        return;
    }

    RemoteCommand cmd;
    cmd.commandId = msg["commandId"].toString();
    cmd.senderId = senderId;
    cmd.targetId = m_progressTracker->profile().id;
    cmd.action = static_cast<ConsequenceAction>(msg["action"].toInt());
    cmd.intensity = msg["intensity"].toDouble();
    cmd.durationMs = msg["durationMs"].toInt();
    cmd.pointCost = msg["pointCost"].toInt();
    cmd.timestamp = QDateTime::currentDateTime();

    emit commandReceived(cmd);
}

void MultiUserController::handleConsentRequest(QWebSocket* socket, const QJsonObject& msg)
{
    QString senderId;
    QString senderName;
    for (const auto& peer : m_peers) {
        if (peer.socket == socket) {
            senderId = peer.peerId;
            senderName = peer.displayName;
            break;
        }
    }

    emit consentRequested(senderId, senderName);
}

void MultiUserController::handleConsentResponse(QWebSocket* socket, const QJsonObject& msg)
{
    bool granted = msg["granted"].toBool();
    int expirationMinutes = msg["expirationMinutes"].toInt(60);

    for (auto& peer : m_peers) {
        if (peer.socket == socket) {
            if (granted) {
                peer.consentStatus = ConsentStatus::GRANTED;
                peer.isControlled = true;
                emit consentGranted(peer.peerId);
            } else {
                peer.consentStatus = ConsentStatus::REVOKED;
                peer.isControlled = false;
                emit consentRevoked(peer.peerId);
            }
            break;
        }
    }
}


void MultiUserController::handleEmergencyStop(QWebSocket* socket, const QJsonObject& msg)
{
    Q_UNUSED(msg);

    QString senderId;
    for (const auto& peer : m_peers) {
        if (peer.socket == socket) {
            senderId = peer.peerId;
            break;
        }
    }

    // Revoke all consent from this peer
    m_progressTracker->revokeConsent(senderId);

    emit emergencyStopReceived(senderId);
}

void MultiUserController::handleSafeWord(QWebSocket* socket, const QJsonObject& msg)
{
    QString safeWord = msg["safeWord"].toString();

    QString senderId;
    for (const auto& peer : m_peers) {
        if (peer.socket == socket) {
            senderId = peer.peerId;
            break;
        }
    }

    // Verify safe word and revoke all control
    if (m_progressTracker->verifySafeWord(safeWord)) {
        revokeAllControl();
        emit safeWordActivated(senderId);
    }
}

void MultiUserController::sendMessage(QWebSocket* socket, const QJsonObject& msg)
{
    if (!socket) return;

    QJsonDocument doc(msg);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

bool MultiUserController::verifyConsent(const QString& senderId, const QString& targetId)
{
    Q_UNUSED(senderId);
    return m_progressTracker->hasValidConsent(targetId);
}

bool MultiUserController::deductPoints(int amount, const QString& targetId, ConsequenceAction action)
{
    QString desc = QString("Command %1 to %2")
                       .arg(static_cast<int>(action))
                       .arg(targetId);

    return m_progressTracker->spendPoints(amount, PointTransactionType::COMMAND_COST,
                                           desc, targetId);
}