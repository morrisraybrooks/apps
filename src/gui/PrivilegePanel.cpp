#include "PrivilegePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QFont>

PrivilegePanel::PrivilegePanel(ProgressTracker* progressTracker,
                               MultiUserController* multiUserController,
                               LicenseManager* licenseManager,
                               QWidget* parent)
    : QWidget(parent)
    , m_progressTracker(progressTracker)
    , m_multiUserController(multiUserController)
    , m_licenseManager(licenseManager)
    , m_updateTimer(new QTimer(this))
{
    setupUi();
    setupConnections();
    updateDisplay();

    m_updateTimer->start(5000);  // Update every 5 seconds
}

void PrivilegePanel::setLicenseManager(LicenseManager* manager)
{
    m_licenseManager = manager;
    if (m_licenseManager) {
        connect(m_licenseManager, &LicenseManager::licenseActivated,
                this, &PrivilegePanel::onLicenseChanged);
        connect(m_licenseManager, &LicenseManager::licenseValidated,
                this, [this](LicenseStatus) { updateLicenseDisplay(); });
        connect(m_licenseManager, &LicenseManager::pointsPurchased,
                this, &PrivilegePanel::onPurchaseComplete);
        updateLicenseDisplay();
    }
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

    // =========================================================================
    // License and Subscription Section
    // =========================================================================
    setupLicenseSection();
    mainLayout->addWidget(m_licenseGroup);

    mainLayout->addStretch();
}

void PrivilegePanel::setupLicenseSection()
{
    m_licenseGroup = new QGroupBox("Subscription & License", this);
    QGridLayout* licenseLayout = new QGridLayout(m_licenseGroup);

    // Subscription status
    m_subscriptionLabel = new QLabel("FREE", this);
    QFont subFont = m_subscriptionLabel->font();
    subFont.setPointSize(12);
    subFont.setBold(true);
    m_subscriptionLabel->setFont(subFont);
    m_subscriptionLabel->setStyleSheet("color: #888888;");

    m_licenseStatusLabel = new QLabel("Not Licensed", this);
    m_expirationLabel = new QLabel("", this);

    // License key input
    m_licenseKeyEdit = new QLineEdit(this);
    m_licenseKeyEdit->setPlaceholderText("XXXX-XXXX-XXXX-XXXX");
    m_licenseKeyEdit->setMaxLength(19);

    m_activateButton = new QPushButton("Activate", this);
    m_requestTrialButton = new QPushButton("Start 7-Day Trial", this);
    m_requestTrialButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");

    // Point bundles
    m_pointBundleCombo = new QComboBox(this);
    m_pointBundleCombo->addItem("Starter - 100 pts ($0.99)", "starter_100");
    m_pointBundleCombo->addItem("Basic - 550 pts ($3.99)", "basic_500");
    m_pointBundleCombo->addItem("Standard - 1,800 pts ($9.99)", "standard_1500");
    m_pointBundleCombo->addItem("Premium - 6,500 pts ($24.99)", "premium_5000");
    m_pointBundleCombo->addItem("Mega - 22,500 pts ($49.99)", "mega_15000");
    m_pointBundleCombo->addItem("Ultimate - 87,500 pts ($99.99)", "ultimate_50000");

    m_buyPointsButton = new QPushButton("Buy Points", this);
    m_buyPointsButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");

    // Subscription upgrades
    m_subscriptionCombo = new QComboBox(this);
    m_subscriptionCombo->addItem("Basic Monthly - $4.99/mo", "basic_monthly");
    m_subscriptionCombo->addItem("Standard Monthly - $9.99/mo", "standard_monthly");
    m_subscriptionCombo->addItem("Premium Monthly - $19.99/mo", "premium_monthly");
    m_subscriptionCombo->addItem("Standard Yearly - $95.88/yr (Save 20%)", "standard_yearly");
    m_subscriptionCombo->addItem("Premium Yearly - $179.88/yr (Save 25%)", "premium_yearly");
    m_subscriptionCombo->addItem("Lifetime Premium - $299.99", "lifetime");

    m_upgradeButton = new QPushButton("Upgrade", this);
    m_upgradeButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; }");

    // Layout
    licenseLayout->addWidget(new QLabel("Status:"), 0, 0);
    licenseLayout->addWidget(m_subscriptionLabel, 0, 1);
    licenseLayout->addWidget(m_licenseStatusLabel, 0, 2);
    licenseLayout->addWidget(m_expirationLabel, 1, 0, 1, 3);

    licenseLayout->addWidget(new QLabel("License Key:"), 2, 0);
    licenseLayout->addWidget(m_licenseKeyEdit, 2, 1);
    licenseLayout->addWidget(m_activateButton, 2, 2);
    licenseLayout->addWidget(m_requestTrialButton, 3, 1, 1, 2);

    licenseLayout->addWidget(new QLabel("Buy Points:"), 4, 0);
    licenseLayout->addWidget(m_pointBundleCombo, 4, 1);
    licenseLayout->addWidget(m_buyPointsButton, 4, 2);

    licenseLayout->addWidget(new QLabel("Subscribe:"), 5, 0);
    licenseLayout->addWidget(m_subscriptionCombo, 5, 1);
    licenseLayout->addWidget(m_upgradeButton, 5, 2);
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

    // License button connections
    connect(m_activateButton, &QPushButton::clicked,
            this, &PrivilegePanel::onActivateLicenseClicked);
    connect(m_requestTrialButton, &QPushButton::clicked,
            this, &PrivilegePanel::onRequestTrialClicked);
    connect(m_buyPointsButton, &QPushButton::clicked,
            this, &PrivilegePanel::onBuyPointsClicked);
    connect(m_upgradeButton, &QPushButton::clicked,
            this, &PrivilegePanel::onUpgradeSubscriptionClicked);

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

    // Update license
    updateLicenseDisplay();

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

// ============================================================================
// License and Purchase Slots
// ============================================================================

void PrivilegePanel::onActivateLicenseClicked()
{
    if (!m_licenseManager) return;

    QString key = m_licenseKeyEdit->text().trimmed();
    if (key.isEmpty()) {
        QMessageBox::warning(this, "Activation Error", "Please enter a license key.");
        return;
    }

    if (m_licenseManager->activateLicense(key)) {
        m_activateButton->setEnabled(false);
        m_activateButton->setText("Activating...");
    }
}

void PrivilegePanel::onRequestTrialClicked()
{
    if (!m_licenseManager) return;

    // For trial, we'll need an email - show simple dialog
    QString email = QInputDialog::getText(this, "Start Trial",
        "Enter your email address to start a 7-day free trial:");

    if (!email.isEmpty() && email.contains("@")) {
        // Request trial through license server
        m_licenseManager->activateLicense("TRIAL-" + email.left(4).toUpper());
        m_requestTrialButton->setEnabled(false);
        m_requestTrialButton->setText("Trial Requested...");
    }
}

void PrivilegePanel::onBuyPointsClicked()
{
    if (!m_licenseManager) return;

    QString bundleId = m_pointBundleCombo->currentData().toString();
    if (bundleId.isEmpty()) return;

    // This would normally open a payment dialog or redirect to payment processor
    int ret = QMessageBox::question(this, "Purchase Points",
        QString("Purchase %1?\n\nThis will open a payment window.")
            .arg(m_pointBundleCombo->currentText()),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_licenseManager->purchasePointBundle(bundleId);
    }
}

void PrivilegePanel::onUpgradeSubscriptionClicked()
{
    if (!m_licenseManager) return;

    QString planId = m_subscriptionCombo->currentData().toString();
    if (planId.isEmpty()) return;

    int ret = QMessageBox::question(this, "Upgrade Subscription",
        QString("Upgrade to %1?\n\nThis will open a payment window.")
            .arg(m_subscriptionCombo->currentText()),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_licenseManager->upgradePlan(planId);
    }
}

void PrivilegePanel::onLicenseChanged(const LicenseInfo& info)
{
    Q_UNUSED(info)
    updateLicenseDisplay();

    // Re-enable buttons
    m_activateButton->setEnabled(true);
    m_activateButton->setText("Activate");
    m_requestTrialButton->setEnabled(true);
    m_requestTrialButton->setText("Start 7-Day Trial");

    // Notify user
    if (info.isValid()) {
        QMessageBox::information(this, "License Activated",
            QString("Your %1 subscription is now active!")
                .arg(subscriptionTierText(info.tier)));
    }
}

void PrivilegePanel::onPurchaseComplete(int pointsAwarded, const QString& productId)
{
    QMessageBox::information(this, "Purchase Complete",
        QString("Purchase successful! %1 points have been added to your account.")
            .arg(pointsAwarded));

    // Add points to progress tracker
    m_progressTracker->addPoints(pointsAwarded, PointTransactionType::PURCHASE,
                                 QString("Purchased: %1").arg(productId));

    updateDisplay();
}

void PrivilegePanel::updateLicenseDisplay()
{
    if (!m_licenseManager) {
        m_subscriptionLabel->setText("FREE");
        m_licenseStatusLabel->setText("(No license manager)");
        return;
    }

    LicenseInfo info = m_licenseManager->licenseInfo();

    // Update subscription tier display
    m_subscriptionLabel->setText(subscriptionTierText(info.tier).toUpper());

    // Color code by tier
    QString tierColor;
    switch (info.tier) {
        case SubscriptionTier::LIFETIME:
            tierColor = "#9C27B0";  // Purple
            break;
        case SubscriptionTier::PREMIUM:
            tierColor = "#FF9800";  // Orange
            break;
        case SubscriptionTier::STANDARD:
            tierColor = "#2196F3";  // Blue
            break;
        case SubscriptionTier::BASIC:
            tierColor = "#4CAF50";  // Green
            break;
        default:
            tierColor = "#888888";  // Gray
    }
    m_subscriptionLabel->setStyleSheet(QString("color: %1;").arg(tierColor));

    // Status label
    switch (info.status) {
        case LicenseStatus::VALID:
            m_licenseStatusLabel->setText("✓ Active");
            m_licenseStatusLabel->setStyleSheet("color: #4CAF50;");
            break;
        case LicenseStatus::EXPIRED:
            m_licenseStatusLabel->setText("✗ Expired");
            m_licenseStatusLabel->setStyleSheet("color: #F44336;");
            break;
        case LicenseStatus::PENDING:
            m_licenseStatusLabel->setText("⋯ Validating");
            m_licenseStatusLabel->setStyleSheet("color: #FF9800;");
            break;
        default:
            m_licenseStatusLabel->setText("Not Licensed");
            m_licenseStatusLabel->setStyleSheet("color: #888888;");
    }

    // Expiration
    int days = info.daysRemaining();
    if (days < 0) {
        m_expirationLabel->setText("Lifetime license - never expires");
    } else if (days == 0) {
        m_expirationLabel->setText("Expires today!");
        m_expirationLabel->setStyleSheet("color: #F44336;");
    } else if (days <= 7) {
        m_expirationLabel->setText(QString("Expires in %1 days").arg(days));
        m_expirationLabel->setStyleSheet("color: #FF9800;");
    } else if (info.expiresAt.isValid()) {
        m_expirationLabel->setText(QString("Expires: %1")
            .arg(info.expiresAt.toString("MMM dd, yyyy")));
        m_expirationLabel->setStyleSheet("");
    } else {
        m_expirationLabel->setText("");
    }

    // Hide/show trial button based on status
    m_requestTrialButton->setVisible(!info.isValid());
    m_licenseKeyEdit->setVisible(!info.isValid());
    m_activateButton->setVisible(!info.isValid());
}

QString PrivilegePanel::subscriptionTierText(SubscriptionTier tier) const
{
    switch (tier) {
        case SubscriptionTier::LIFETIME: return "Lifetime";
        case SubscriptionTier::PREMIUM: return "Premium";
        case SubscriptionTier::STANDARD: return "Standard";
        case SubscriptionTier::BASIC: return "Basic";
        default: return "Free";
    }
}