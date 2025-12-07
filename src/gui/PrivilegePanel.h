#ifndef PRIVILEGEPANEL_H
#define PRIVILEGEPANEL_H

#include "../game/GameTypes.h"
#include "../game/ProgressTracker.h"
#include "../network/MultiUserController.h"
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QTimer>

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
                            QWidget* parent = nullptr);
    ~PrivilegePanel();

public Q_SLOTS:
    void updateDisplay();
    void onPointsChanged(int newBalance, int change);
    void onTierChanged(PrivilegeTier newTier);
    void onPeerConnected(const QString& peerId, const QString& displayName);
    void onPeerDisconnected(const QString& peerId);
    void onConsentChanged(const QString& partnerId, ConsentStatus status);

private Q_SLOTS:
    void onTransferClicked();
    void onPairClicked();
    void onUnpairClicked();
    void onGrantConsentClicked();
    void onRevokeConsentClicked();
    void onCreateRoomClicked();
    void onEmergencyStopClicked();
    void onSafeWordChanged();

private:
    void setupUi();
    void setupConnections();
    void updateTierDisplay();
    void updatePairedUsersList();
    void updateRoomsList();
    QString tierColorStyle(PrivilegeTier tier) const;
    QString consentStatusText(ConsentStatus status) const;

    ProgressTracker* m_progressTracker;
    MultiUserController* m_multiUserController;

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

    QTimer* m_updateTimer;
};

#endif // PRIVILEGEPANEL_H

