#include "PrivilegePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QFont>

PrivilegePanel::PrivilegePanel(ProgressTracker* progressTracker,
                               MultiUserController* multiUserController,
                               QWidget* parent)
    : QWidget(parent)
    , m_progressTracker(progressTracker)
    , m_multiUserController(multiUserController)
    , m_updateTimer(new QTimer(this))
{
    setupUi();
    setupConnections();
    updateDisplay();

    m_updateTimer->start(5000);  // Update every 5 seconds
}

PrivilegePanel::~PrivilegePanel()
{
}

void PrivilegePanel::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // =========================================================================
    // Points and Tier Display
    // =========================================================================
    QGroupBox* pointsGroup = new QGroupBox("Points Economy", this);
    QGridLayout* pointsLayout = new QGridLayout(pointsGroup);

    m_pointsLabel = new QLabel("0", this);
    QFont pointsFont = m_pointsLabel->font();
    pointsFont.setPointSize(24);
    pointsFont.setBold(true);
    m_pointsLabel->setFont(pointsFont);
    m_pointsLabel->setAlignment(Qt::AlignCenter);

    m_tierLabel = new QLabel("BEGINNER", this);
    QFont tierFont = m_tierLabel->font();
    tierFont.setPointSize(14);
    tierFont.setBold(true);
    m_tierLabel->setFont(tierFont);
    m_tierLabel->setAlignment(Qt::AlignCenter);

    m_tierProgress = new QProgressBar(this);
    m_tierProgress->setRange(0, 100);
    m_tierProgress->setTextVisible(true);
    m_tierProgress->setFormat("%v / %m points");

    m_nextTierLabel = new QLabel("Next: Intermediate (1000 pts)", this);
    m_nextTierLabel->setAlignment(Qt::AlignCenter);

    pointsLayout->addWidget(new QLabel("Points Balance:"), 0, 0);
    pointsLayout->addWidget(m_pointsLabel, 0, 1);
    pointsLayout->addWidget(new QLabel("Privilege Tier:"), 1, 0);
    pointsLayout->addWidget(m_tierLabel, 1, 1);
    pointsLayout->addWidget(m_tierProgress, 2, 0, 1, 2);
    pointsLayout->addWidget(m_nextTierLabel, 3, 0, 1, 2);

    mainLayout->addWidget(pointsGroup);

    // =========================================================================
    // Point Transfer (Advanced tier+)
    // =========================================================================
    m_transferGroup = new QGroupBox("Point Transfer", this);
    QHBoxLayout* transferLayout = new QHBoxLayout(m_transferGroup);

    m_transferRecipient = new QLineEdit(this);
    m_transferRecipient->setPlaceholderText("Recipient ID");

    m_transferAmount = new QSpinBox(this);
    m_transferAmount->setRange(1, 10000);
    m_transferAmount->setValue(100);

    m_transferButton = new QPushButton("Transfer", this);

    transferLayout->addWidget(new QLabel("To:"));
    transferLayout->addWidget(m_transferRecipient);
    transferLayout->addWidget(new QLabel("Amount:"));
    transferLayout->addWidget(m_transferAmount);
    transferLayout->addWidget(m_transferButton);

    mainLayout->addWidget(m_transferGroup);

    // =========================================================================
    // Paired Users
    // =========================================================================
    m_pairingGroup = new QGroupBox("Paired Users", this);
    QVBoxLayout* pairingLayout = new QVBoxLayout(m_pairingGroup);

    m_pairedUsersList = new QListWidget(this);
    m_pairedUsersList->setMaximumHeight(120);

    QHBoxLayout* pairInputLayout = new QHBoxLayout();
    m_pairAddress = new QLineEdit(this);
    m_pairAddress->setPlaceholderText("IP Address");
    m_pairPort = new QSpinBox(this);
    m_pairPort->setRange(1, 65535);
    m_pairPort->setValue(8765);
    m_pairButton = new QPushButton("Connect", this);

    pairInputLayout->addWidget(m_pairAddress);
    pairInputLayout->addWidget(m_pairPort);
    pairInputLayout->addWidget(m_pairButton);

    QHBoxLayout* pairButtonLayout = new QHBoxLayout();
    m_unpairButton = new QPushButton("Disconnect", this);
    m_grantConsentButton = new QPushButton("Grant Consent", this);
    m_revokeConsentButton = new QPushButton("Revoke Consent", this);

    pairButtonLayout->addWidget(m_unpairButton);
    pairButtonLayout->addWidget(m_grantConsentButton);
    pairButtonLayout->addWidget(m_revokeConsentButton);

    pairingLayout->addWidget(m_pairedUsersList);
    pairingLayout->addLayout(pairInputLayout);
    pairingLayout->addLayout(pairButtonLayout);

    mainLayout->addWidget(m_pairingGroup);

    // =========================================================================
    // Room Management (DOM Master)
    // =========================================================================
    m_roomGroup = new QGroupBox("Control Rooms (DOM Master)", this);
    QVBoxLayout* roomLayout = new QVBoxLayout(m_roomGroup);

    m_roomsList = new QListWidget(this);
    m_roomsList->setMaximumHeight(80);

    QHBoxLayout* roomInputLayout = new QHBoxLayout();
    m_roomName = new QLineEdit(this);
    m_roomName->setPlaceholderText("Room Name");
    m_createRoomButton = new QPushButton("Create Room", this);

    roomInputLayout->addWidget(m_roomName);
    roomInputLayout->addWidget(m_createRoomButton);

    roomLayout->addWidget(m_roomsList);
    roomLayout->addLayout(roomInputLayout);

    mainLayout->addWidget(m_roomGroup);

    // =========================================================================
    // Safety Controls
    // =========================================================================
    m_safetyGroup = new QGroupBox("Safety", this);
    QHBoxLayout* safetyLayout = new QHBoxLayout(m_safetyGroup);

    m_safeWordEdit = new QLineEdit(this);
    m_safeWordEdit->setPlaceholderText("Set Safe Word");
    m_safeWordEdit->setEchoMode(QLineEdit::Password);

    m_emergencyStopButton = new QPushButton("EMERGENCY STOP", this);
    m_emergencyStopButton->setStyleSheet(
        "QPushButton { background-color: #FF0000; color: white; font-weight: bold; "
        "padding: 10px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #CC0000; }");

    safetyLayout->addWidget(new QLabel("Safe Word:"));
    safetyLayout->addWidget(m_safeWordEdit);
    safetyLayout->addWidget(m_emergencyStopButton);

    mainLayout->addWidget(m_safetyGroup);
    mainLayout->addStretch();
}

void PrivilegePanel::setupConnections()
{
    // Progress tracker signals
    connect(m_progressTracker, &ProgressTracker::pointsChanged,
            this, &PrivilegePanel::onPointsChanged);
    connect(m_progressTracker, &ProgressTracker::privilegeTierChanged,
            this, &PrivilegePanel::onTierChanged);

    // Multi-user controller signals
    connect(m_multiUserController, &MultiUserController::peerConnected,
            this, &PrivilegePanel::onPeerConnected);
    connect(m_multiUserController, &MultiUserController::peerDisconnected,
            this, &PrivilegePanel::onPeerDisconnected);

    // Button connections
    connect(m_transferButton, &QPushButton::clicked,
            this, &PrivilegePanel::onTransferClicked);
    connect(m_pairButton, &QPushButton::clicked,
            this, &PrivilegePanel::onPairClicked);
    connect(m_unpairButton, &QPushButton::clicked,
            this, &PrivilegePanel::onUnpairClicked);
    connect(m_grantConsentButton, &QPushButton::clicked,
            this, &PrivilegePanel::onGrantConsentClicked);
    connect(m_revokeConsentButton, &QPushButton::clicked,
            this, &PrivilegePanel::onRevokeConsentClicked);
    connect(m_createRoomButton, &QPushButton::clicked,
            this, &PrivilegePanel::onCreateRoomClicked);
    connect(m_emergencyStopButton, &QPushButton::clicked,
            this, &PrivilegePanel::onEmergencyStopClicked);
    connect(m_safeWordEdit, &QLineEdit::editingFinished,
            this, &PrivilegePanel::onSafeWordChanged);

    // Update timer
    connect(m_updateTimer, &QTimer::timeout, this, &PrivilegePanel::updateDisplay);
}

void PrivilegePanel::updateDisplay()
{
    // Update points
    m_pointsLabel->setText(QString::number(m_progressTracker->pointsBalance()));

    // Update tier display
    updateTierDisplay();

    // Update paired users
    updatePairedUsersList();

    // Update rooms
    updateRoomsList();

    // Enable/disable features based on tier
    PrivilegeTier tier = m_progressTracker->privilegeTier();
    m_transferGroup->setEnabled(tier >= PrivilegeTier::ADVANCED);
    m_roomGroup->setEnabled(tier >= PrivilegeTier::DOM_MASTER);
    m_pairingGroup->setEnabled(tier >= PrivilegeTier::INTERMEDIATE);
}

void PrivilegePanel::updateTierDisplay()
{
    PrivilegeTier tier = m_progressTracker->privilegeTier();
    int points = m_progressTracker->pointsBalance();

    m_tierLabel->setText(ProgressTracker::tierName(tier).toUpper());
    m_tierLabel->setStyleSheet(tierColorStyle(tier));

    // Calculate progress to next tier
    int currentTierPoints = ProgressTracker::pointsForTier(tier);
    int nextTierPoints = 0;
    QString nextTierName;

    switch (tier) {
        case PrivilegeTier::BEGINNER:
            nextTierPoints = 1000;
            nextTierName = "Intermediate";
            break;
        case PrivilegeTier::INTERMEDIATE:
            nextTierPoints = 5000;
            nextTierName = "Advanced";
            break;
        case PrivilegeTier::ADVANCED:
            nextTierPoints = 15000;
            nextTierName = "DOM Master";
            break;
        case PrivilegeTier::DOM_MASTER:
            nextTierPoints = points;  // Already at max
            nextTierName = "MAX";
            break;
    }

    m_tierProgress->setMaximum(nextTierPoints);
    m_tierProgress->setValue(points);
    m_tierProgress->setFormat(QString("%1 / %2 points").arg(points).arg(nextTierPoints));

    if (tier == PrivilegeTier::DOM_MASTER) {
        m_nextTierLabel->setText("Maximum tier reached!");
    } else {
        m_nextTierLabel->setText(QString("Next: %1 (%2 pts)")
                                     .arg(nextTierName)
                                     .arg(nextTierPoints));
    }
}

void PrivilegePanel::updatePairedUsersList()
{
    m_pairedUsersList->clear();

    for (const auto& peer : m_multiUserController->connectedPeers()) {
        QString status = consentStatusText(peer.consentStatus);
        QString item = QString("%1 (%2) - %3")
                           .arg(peer.displayName)
                           .arg(peer.peerId.left(8))
                           .arg(status);
        m_pairedUsersList->addItem(item);
    }
}

void PrivilegePanel::updateRoomsList()
{
    m_roomsList->clear();

    for (const auto& room : m_multiUserController->availableRooms()) {
        QString item = QString("%1 (%2/%3 members)")
                           .arg(room.roomName)
                           .arg(room.memberIds.size())
                           .arg(room.maxMembers);
        m_roomsList->addItem(item);
    }
}

QString PrivilegePanel::tierColorStyle(PrivilegeTier tier) const
{
    switch (tier) {
        case PrivilegeTier::BEGINNER:
            return "color: #808080;";  // Gray
        case PrivilegeTier::INTERMEDIATE:
            return "color: #00AA00;";  // Green
        case PrivilegeTier::ADVANCED:
            return "color: #0066CC;";  // Blue
        case PrivilegeTier::DOM_MASTER:
            return "color: #CC00CC;";  // Purple
        default:
            return "";
    }
}

QString PrivilegePanel::consentStatusText(ConsentStatus status) const
{
    switch (status) {
        case ConsentStatus::NONE: return "No Consent";
        case ConsentStatus::PENDING: return "Pending";
        case ConsentStatus::GRANTED: return "Granted";
        case ConsentStatus::REVOKED: return "Revoked";
        case ConsentStatus::EXPIRED: return "Expired";
        default: return "Unknown";
    }
}



// ============================================================================
// Slot Implementations
// ============================================================================

void PrivilegePanel::onPointsChanged(int newBalance, int change)
{
    m_pointsLabel->setText(QString::number(newBalance));

    // Flash effect for point changes
    QString color = change > 0 ? "#00FF00" : "#FF0000";
    m_pointsLabel->setStyleSheet(QString("color: %1;").arg(color));

    QTimer::singleShot(500, this, [this]() {
        m_pointsLabel->setStyleSheet("");
    });

    updateTierDisplay();
}

void PrivilegePanel::onTierChanged(PrivilegeTier newTier)
{
    updateTierDisplay();

    // Show tier upgrade notification
    QMessageBox::information(this, "Tier Upgrade!",
        QString("Congratulations! You've reached %1 tier!")
            .arg(ProgressTracker::tierName(newTier)));
}

void PrivilegePanel::onPeerConnected(const QString& peerId, const QString& displayName)
{
    Q_UNUSED(peerId);
    Q_UNUSED(displayName);
    updatePairedUsersList();
}

void PrivilegePanel::onPeerDisconnected(const QString& peerId)
{
    Q_UNUSED(peerId);
    updatePairedUsersList();
}

void PrivilegePanel::onConsentChanged(const QString& partnerId, ConsentStatus status)
{
    Q_UNUSED(partnerId);
    Q_UNUSED(status);
    updatePairedUsersList();
}

void PrivilegePanel::onTransferClicked()
{
    QString recipient = m_transferRecipient->text().trimmed();
    int amount = m_transferAmount->value();

    if (recipient.isEmpty()) {
        QMessageBox::warning(this, "Transfer Error", "Please enter a recipient ID.");
        return;
    }

    if (m_progressTracker->transferPoints(recipient, amount)) {
        QMessageBox::information(this, "Transfer Complete",
            QString("Successfully transferred %1 points to %2")
                .arg(amount).arg(recipient));
        m_transferRecipient->clear();
    } else {
        QMessageBox::warning(this, "Transfer Failed",
            "Could not complete transfer. Check your balance and recipient ID.");
    }
}

void PrivilegePanel::onPairClicked()
{
    QString address = m_pairAddress->text().trimmed();
    int port = m_pairPort->value();

    if (address.isEmpty()) {
        QMessageBox::warning(this, "Connection Error", "Please enter an IP address.");
        return;
    }

    m_multiUserController->connectToPeer(address, port);
}

void PrivilegePanel::onUnpairClicked()
{
    QListWidgetItem* item = m_pairedUsersList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Selection Error", "Please select a user to disconnect.");
        return;
    }

    // Extract peer ID from item text
    QString text = item->text();
    int start = text.indexOf('(') + 1;
    int end = text.indexOf(')');
    QString peerId = text.mid(start, end - start);

    m_multiUserController->disconnectFromPeer(peerId);
}

void PrivilegePanel::onGrantConsentClicked()
{
    QListWidgetItem* item = m_pairedUsersList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Selection Error", "Please select a user to grant consent.");
        return;
    }

    QString text = item->text();
    int start = text.indexOf('(') + 1;
    int end = text.indexOf(')');
    QString peerId = text.mid(start, end - start);

    // Default 60 minute consent
    m_multiUserController->grantControlTo(peerId, 60);
}

void PrivilegePanel::onRevokeConsentClicked()
{
    QListWidgetItem* item = m_pairedUsersList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Selection Error", "Please select a user to revoke consent.");
        return;
    }

    QString text = item->text();
    int start = text.indexOf('(') + 1;
    int end = text.indexOf(')');
    QString peerId = text.mid(start, end - start);

    m_multiUserController->revokeControlFrom(peerId);
}

void PrivilegePanel::onCreateRoomClicked()
{
    QString name = m_roomName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Room Error", "Please enter a room name.");
        return;
    }

    if (m_multiUserController->createRoom(name, 10, false)) {
        m_roomName->clear();
        updateRoomsList();
    } else {
        QMessageBox::warning(this, "Room Error", "Could not create room. DOM Master tier required.");
    }
}

void PrivilegePanel::onEmergencyStopClicked()
{
    // Immediate emergency stop - revoke all control
    m_multiUserController->revokeAllControl();

    QMessageBox::warning(this, "Emergency Stop",
        "All remote control has been revoked. All consent has been cancelled.");
}

void PrivilegePanel::onSafeWordChanged()
{
    QString safeWord = m_safeWordEdit->text();
    if (!safeWord.isEmpty()) {
        m_progressTracker->setSafeWord(safeWord);
        m_safeWordEdit->clear();
        m_safeWordEdit->setPlaceholderText("Safe word set ✓");
    }
}