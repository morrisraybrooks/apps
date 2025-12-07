#ifndef PRIVILEGEPANEL_H
#define PRIVILEGEPANEL_H

#include "../game/GameTypes.h"
#include "../game/ProgressTracker.h"
#include "../network/MultiUserController.h"
#include "../licensing/LicenseManager.h"
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QTimer>
#include <QComboBox>

/**
 * @brief GUI panel for points economy and multi-user control
 * 
 * Displays:
 * - Current points balance and privilege tier
 * - Progress to next tier
 * - Paired users with consent status
 * - Point transfer controls
 * - Room management (for DOM Master tier)
 */
class PrivilegePanel : public QWidget
{
    Q_OBJECT

public:
    explicit PrivilegePanel(ProgressTracker* progressTracker,
                            MultiUserController* multiUserController,
                            LicenseManager* licenseManager = nullptr,
                            QWidget* parent = nullptr);
    ~PrivilegePanel();

    void setLicenseManager(LicenseManager* manager);

public Q_SLOTS:
    void updateDisplay();
    void onPointsChanged(int newBalance, int change);
    void onTierChanged(PrivilegeTier newTier);
    void onPeerConnected(const QString& peerId, const QString& displayName);
    void onPeerDisconnected(const QString& peerId);
    void onConsentChanged(const QString& partnerId, ConsentStatus status);
    void onLicenseChanged(const LicenseInfo& info);
    void onPurchaseComplete(const QString& productId, int pointsAwarded);

private Q_SLOTS:
    void onTransferClicked();
    void onPairClicked();
    void onUnpairClicked();
    void onGrantConsentClicked();
    void onRevokeConsentClicked();
    void onCreateRoomClicked();
    void onEmergencyStopClicked();
    void onSafeWordChanged();
    void onActivateLicenseClicked();
    void onRequestTrialClicked();
    void onBuyPointsClicked();
    void onUpgradeSubscriptionClicked();

private:
    void setupUi();
    void setupConnections();
    void updateTierDisplay();
    void updatePairedUsersList();
    void updateRoomsList();
    void updateLicenseDisplay();
    void setupLicenseSection();
    QString tierColorStyle(PrivilegeTier tier) const;
    QString consentStatusText(ConsentStatus status) const;
    QString subscriptionTierText(SubscriptionTier tier) const;

    ProgressTracker* m_progressTracker;
    MultiUserController* m_multiUserController;
    LicenseManager* m_licenseManager;

    // Points display
    QLabel* m_pointsLabel;
    QLabel* m_tierLabel;
    QProgressBar* m_tierProgress;
    QLabel* m_nextTierLabel;

    // Transaction history
    QListWidget* m_transactionList;

    // Point transfer
    QGroupBox* m_transferGroup;
    QLineEdit* m_transferRecipient;
    QSpinBox* m_transferAmount;
    QPushButton* m_transferButton;

    // Paired users
    QGroupBox* m_pairingGroup;
    QListWidget* m_pairedUsersList;
    QLineEdit* m_pairAddress;
    QSpinBox* m_pairPort;
    QPushButton* m_pairButton;
    QPushButton* m_unpairButton;
    QPushButton* m_grantConsentButton;
    QPushButton* m_revokeConsentButton;

    // Room management (DOM Master)
    QGroupBox* m_roomGroup;
    QListWidget* m_roomsList;
    QLineEdit* m_roomName;
    QPushButton* m_createRoomButton;

    // Safety
    QGroupBox* m_safetyGroup;
    QLineEdit* m_safeWordEdit;
    QPushButton* m_emergencyStopButton;

    // License/Subscription
    QGroupBox* m_licenseGroup;
    QLabel* m_subscriptionLabel;
    QLabel* m_licenseStatusLabel;
    QLabel* m_expirationLabel;
    QLineEdit* m_licenseKeyEdit;
    QPushButton* m_activateButton;
    QPushButton* m_requestTrialButton;
    QComboBox* m_pointBundleCombo;
    QPushButton* m_buyPointsButton;
    QComboBox* m_subscriptionCombo;
    QPushButton* m_upgradeButton;

    QTimer* m_updateTimer;
};

#endif // PRIVILEGEPANEL_H
