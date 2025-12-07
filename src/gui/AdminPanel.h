#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QTimer>

#include "../admin/AccountManager.h"
#include "../admin/DeviceRegistry.h"
#include "../admin/RemoteMonitor.h"

/**
 * @brief Admin Panel for master account management and monitoring
 * 
 * Features:
 * - Account management (create/suspend sub-accounts)
 * - Device monitoring dashboard
 * - Real-time device viewing
 * - Activity log viewer
 * - Points management
 */
class AdminPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AdminPanel(AccountManager* accountManager, QWidget* parent = nullptr);
    ~AdminPanel();

    void refresh();

public Q_SLOTS:
    void onLoginSuccessful(const UserAccount& account);
    void onLoggedOut();

private Q_SLOTS:
    // Login/Logout
    void onLoginClicked();
    void onLogoutClicked();

    // Account management
    void onCreateSubAccountClicked();
    void onSuspendAccountClicked();
    void onUnsuspendAccountClicked();
    void onDeleteAccountClicked();
    void onGrantPointsClicked();
    void onAccountSelected(int row, int column);

    // Device monitoring
    void onDeviceSelected(int row, int column);
    void onMonitorDeviceClicked();
    void onStopMonitoringClicked();
    void onTakeControlClicked();
    void onReleaseControlClicked();
    void onEmergencyStopClicked();
    void onEmergencyStopAllClicked();

    // Data updates
    void onDeviceStateUpdated(const QString& deviceId, const QJsonObject& state);
    void onMonitoringStarted(const QString& deviceId, const MonitorSession& session);
    void onMonitoringStopped(const QString& deviceId);

    // Refresh
    void refreshAccounts();
    void refreshDevices();
    void refreshActivityLog();

private:
    void setupUI();
    void setupLoginSection();
    void setupAccountsTab();
    void setupDevicesTab();
    void setupMonitoringTab();
    void setupActivityTab();
    void updateUIState();
    void populateAccountsTable();
    void populateDevicesTable();
    void updateMonitoringView();

    AccountManager* m_accountManager;
    RemoteMonitor* m_remoteMonitor;
    QTimer* m_refreshTimer;

    // Login section
    QWidget* m_loginWidget;
    QLineEdit* m_emailInput;
    QLineEdit* m_passwordInput;
    QPushButton* m_loginButton;

    // Logged-in section
    QWidget* m_adminWidget;
    QLabel* m_accountLabel;
    QPushButton* m_logoutButton;

    // Tab widget
    QTabWidget* m_tabWidget;

    // Accounts tab
    QTableWidget* m_accountsTable;
    QPushButton* m_createAccountBtn;
    QPushButton* m_suspendAccountBtn;
    QPushButton* m_unsuspendAccountBtn;
    QPushButton* m_deleteAccountBtn;
    QPushButton* m_grantPointsBtn;

    // Devices tab
    QTableWidget* m_devicesTable;
    QLabel* m_onlineCountLabel;
    QLabel* m_totalCountLabel;

    // Monitoring tab
    QPushButton* m_monitorBtn;
    QPushButton* m_stopMonitorBtn;
    QPushButton* m_takeControlBtn;
    QPushButton* m_releaseControlBtn;
    QPushButton* m_emergencyStopBtn;
    QPushButton* m_emergencyStopAllBtn;
    QLabel* m_monitorStatusLabel;
    QTextEdit* m_deviceStateView;
    QProgressBar* m_batteryBar;
    QLabel* m_sessionInfoLabel;

    // Activity tab
    QTextEdit* m_activityLog;
    QComboBox* m_activityFilterCombo;
    QPushButton* m_refreshActivityBtn;

    QString m_selectedAccountId;
    QString m_selectedDeviceId;
};

#endif // ADMINPANEL_H

