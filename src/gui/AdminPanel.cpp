#include "AdminPanel.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QDateTime>

AdminPanel::AdminPanel(AccountManager* accountManager, QWidget* parent)
    : QWidget(parent)
    , m_accountManager(accountManager)
    , m_remoteMonitor(new RemoteMonitor(accountManager, this))
{
    setupUI();
    updateUIState();

    // Connect signals
    connect(m_accountManager, &AccountManager::loginSuccessful,
            this, &AdminPanel::onLoginSuccessful);
    connect(m_accountManager, &AccountManager::loggedOut,
            this, &AdminPanel::onLoggedOut);

    connect(m_remoteMonitor, &RemoteMonitor::monitoringStarted,
            this, &AdminPanel::onMonitoringStarted);
    connect(m_remoteMonitor, &RemoteMonitor::monitoringStopped,
            this, &AdminPanel::onMonitoringStopped);
    connect(m_remoteMonitor, &RemoteMonitor::stateChanged,
            this, &AdminPanel::onDeviceStateUpdated);

    // Auto-refresh timer
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &AdminPanel::refresh);
    m_refreshTimer->start(5000);
}

AdminPanel::~AdminPanel() = default;

void AdminPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Login/Account header
    setupLoginSection();
    mainLayout->addWidget(m_loginWidget);
    mainLayout->addWidget(m_adminWidget);

    // Tab widget for different admin sections
    m_tabWidget = new QTabWidget;
    setupAccountsTab();
    setupDevicesTab();
    setupMonitoringTab();
    setupActivityTab();
    mainLayout->addWidget(m_tabWidget, 1);
}

void AdminPanel::setupLoginSection()
{
    // Login widget
    m_loginWidget = new QWidget;
    QHBoxLayout* loginLayout = new QHBoxLayout(m_loginWidget);

    m_emailInput = new QLineEdit;
    m_emailInput->setPlaceholderText("Email");
    m_passwordInput = new QLineEdit;
    m_passwordInput->setPlaceholderText("Password");
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_loginButton = new QPushButton("Login");

    loginLayout->addWidget(new QLabel("Master Login:"));
    loginLayout->addWidget(m_emailInput);
    loginLayout->addWidget(m_passwordInput);
    loginLayout->addWidget(m_loginButton);
    loginLayout->addStretch();

    connect(m_loginButton, &QPushButton::clicked, this, &AdminPanel::onLoginClicked);

    // Admin widget (shown when logged in)
    m_adminWidget = new QWidget;
    QHBoxLayout* adminLayout = new QHBoxLayout(m_adminWidget);

    m_accountLabel = new QLabel;
    m_logoutButton = new QPushButton("Logout");

    adminLayout->addWidget(new QLabel("Logged in as:"));
    adminLayout->addWidget(m_accountLabel);
    adminLayout->addStretch();
    adminLayout->addWidget(m_logoutButton);

    connect(m_logoutButton, &QPushButton::clicked, this, &AdminPanel::onLogoutClicked);
}

void AdminPanel::setupAccountsTab()
{
    QWidget* accountsTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(accountsTab);

    // Accounts table
    m_accountsTable = new QTableWidget;
    m_accountsTable->setColumnCount(6);
    m_accountsTable->setHorizontalHeaderLabels({"ID", "Email", "Name", "Role", "Status", "Points"});
    m_accountsTable->horizontalHeader()->setStretchLastSection(true);
    m_accountsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_accountsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_accountsTable, 1);

    connect(m_accountsTable, &QTableWidget::cellClicked, this, &AdminPanel::onAccountSelected);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    m_createAccountBtn = new QPushButton("Create Sub-Account");
    m_suspendAccountBtn = new QPushButton("Suspend");
    m_unsuspendAccountBtn = new QPushButton("Unsuspend");
    m_deleteAccountBtn = new QPushButton("Delete");
    m_grantPointsBtn = new QPushButton("Grant Points");

    buttonLayout->addWidget(m_createAccountBtn);
    buttonLayout->addWidget(m_suspendAccountBtn);
    buttonLayout->addWidget(m_unsuspendAccountBtn);
    buttonLayout->addWidget(m_deleteAccountBtn);
    buttonLayout->addWidget(m_grantPointsBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(m_createAccountBtn, &QPushButton::clicked, this, &AdminPanel::onCreateSubAccountClicked);
    connect(m_suspendAccountBtn, &QPushButton::clicked, this, &AdminPanel::onSuspendAccountClicked);
    connect(m_unsuspendAccountBtn, &QPushButton::clicked, this, &AdminPanel::onUnsuspendAccountClicked);
    connect(m_deleteAccountBtn, &QPushButton::clicked, this, &AdminPanel::onDeleteAccountClicked);
    connect(m_grantPointsBtn, &QPushButton::clicked, this, &AdminPanel::onGrantPointsClicked);

    m_tabWidget->addTab(accountsTab, "Accounts");
}

void AdminPanel::setupDevicesTab()
{
    QWidget* devicesTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(devicesTab);

    // Status bar
    QHBoxLayout* statusLayout = new QHBoxLayout;
    m_onlineCountLabel = new QLabel("Online: 0");
    m_totalCountLabel = new QLabel("Total: 0");
    statusLayout->addWidget(m_onlineCountLabel);
    statusLayout->addWidget(m_totalCountLabel);
    statusLayout->addStretch();
    layout->addLayout(statusLayout);

    // Devices table
    m_devicesTable = new QTableWidget;
    m_devicesTable->setColumnCount(7);
    m_devicesTable->setHorizontalHeaderLabels({"ID", "Name", "Owner", "Status", "Battery", "Last Seen", "IP"});
    m_devicesTable->horizontalHeader()->setStretchLastSection(true);
    m_devicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_devicesTable, 1);

    connect(m_devicesTable, &QTableWidget::cellClicked, this, &AdminPanel::onDeviceSelected);

    m_tabWidget->addTab(devicesTab, "Devices");
}

void AdminPanel::setupMonitoringTab()
{
    QWidget* monitorTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(monitorTab);

    // Control buttons
    QHBoxLayout* controlLayout = new QHBoxLayout;
    m_monitorBtn = new QPushButton("Start Monitoring");
    m_stopMonitorBtn = new QPushButton("Stop Monitoring");
    m_takeControlBtn = new QPushButton("Take Control");
    m_releaseControlBtn = new QPushButton("Release Control");
    m_emergencyStopBtn = new QPushButton("EMERGENCY STOP");
    m_emergencyStopAllBtn = new QPushButton("STOP ALL DEVICES");

    m_emergencyStopBtn->setStyleSheet("background-color: #ff4444; color: white; font-weight: bold;");
    m_emergencyStopAllBtn->setStyleSheet("background-color: #ff0000; color: white; font-weight: bold;");

    controlLayout->addWidget(m_monitorBtn);
    controlLayout->addWidget(m_stopMonitorBtn);
    controlLayout->addWidget(m_takeControlBtn);
    controlLayout->addWidget(m_releaseControlBtn);
    controlLayout->addStretch();
    controlLayout->addWidget(m_emergencyStopBtn);
    controlLayout->addWidget(m_emergencyStopAllBtn);
    layout->addLayout(controlLayout);

    connect(m_monitorBtn, &QPushButton::clicked, this, &AdminPanel::onMonitorDeviceClicked);
    connect(m_stopMonitorBtn, &QPushButton::clicked, this, &AdminPanel::onStopMonitoringClicked);
    connect(m_takeControlBtn, &QPushButton::clicked, this, &AdminPanel::onTakeControlClicked);
    connect(m_releaseControlBtn, &QPushButton::clicked, this, &AdminPanel::onReleaseControlClicked);
    connect(m_emergencyStopBtn, &QPushButton::clicked, this, &AdminPanel::onEmergencyStopClicked);
    connect(m_emergencyStopAllBtn, &QPushButton::clicked, this, &AdminPanel::onEmergencyStopAllClicked);

    // Status section
    QGroupBox* statusGroup = new QGroupBox("Monitor Status");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);

    m_monitorStatusLabel = new QLabel("Not monitoring");
    m_sessionInfoLabel = new QLabel;
    m_batteryBar = new QProgressBar;
    m_batteryBar->setRange(0, 100);
    m_batteryBar->setTextVisible(true);
    m_batteryBar->setFormat("Battery: %p%");

    statusLayout->addWidget(m_monitorStatusLabel);
    statusLayout->addWidget(m_sessionInfoLabel);
    statusLayout->addWidget(m_batteryBar);
    layout->addWidget(statusGroup);

    // Device state view
    QGroupBox* stateGroup = new QGroupBox("Device State");
    QVBoxLayout* stateLayout = new QVBoxLayout(stateGroup);
    m_deviceStateView = new QTextEdit;
    m_deviceStateView->setReadOnly(true);
    m_deviceStateView->setFont(QFont("Courier", 10));
    stateLayout->addWidget(m_deviceStateView);
    layout->addWidget(stateGroup, 1);

    m_tabWidget->addTab(monitorTab, "Monitoring");
}

void AdminPanel::setupActivityTab()
{
    QWidget* activityTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(activityTab);

    // Filter controls
    QHBoxLayout* filterLayout = new QHBoxLayout;
    m_activityFilterCombo = new QComboBox;
    m_activityFilterCombo->addItems({"All Activity", "Logins", "Commands", "Control Actions", "Emergency Stops"});
    m_refreshActivityBtn = new QPushButton("Refresh");

    filterLayout->addWidget(new QLabel("Filter:"));
    filterLayout->addWidget(m_activityFilterCombo);
    filterLayout->addStretch();
    filterLayout->addWidget(m_refreshActivityBtn);
    layout->addLayout(filterLayout);

    connect(m_refreshActivityBtn, &QPushButton::clicked, this, &AdminPanel::refreshActivityLog);

    // Activity log view
    m_activityLog = new QTextEdit;
    m_activityLog->setReadOnly(true);
    layout->addWidget(m_activityLog, 1);

    m_tabWidget->addTab(activityTab, "Activity Log");
}

void AdminPanel::updateUIState()
{
    bool loggedIn = m_accountManager->isLoggedIn();
    bool isMaster = m_accountManager->isMasterAccount();

    m_loginWidget->setVisible(!loggedIn);
    m_adminWidget->setVisible(loggedIn);
    m_tabWidget->setEnabled(loggedIn && isMaster);

    if (loggedIn) {
        UserAccount acc = m_accountManager->currentAccount();
        QString roleStr = acc.isMaster() ? "MASTER" : (acc.isAdmin() ? "ADMIN" : "USER");
        m_accountLabel->setText(QString("%1 (%2) - %3").arg(acc.displayName, acc.email, roleStr));
    }

    // Update button states based on selection
    bool hasAccountSelected = !m_selectedAccountId.isEmpty();
    bool hasDeviceSelected = !m_selectedDeviceId.isEmpty();
    bool isMonitoring = hasDeviceSelected && m_remoteMonitor->isMonitoring(m_selectedDeviceId);
    bool hasControl = isMonitoring && m_remoteMonitor->session(m_selectedDeviceId).hasControl;

    m_suspendAccountBtn->setEnabled(hasAccountSelected);
    m_unsuspendAccountBtn->setEnabled(hasAccountSelected);
    m_deleteAccountBtn->setEnabled(hasAccountSelected);
    m_grantPointsBtn->setEnabled(hasAccountSelected);

    m_monitorBtn->setEnabled(hasDeviceSelected && !isMonitoring);
    m_stopMonitorBtn->setEnabled(isMonitoring);
    m_takeControlBtn->setEnabled(isMonitoring && !hasControl);
    m_releaseControlBtn->setEnabled(hasControl);
    m_emergencyStopBtn->setEnabled(isMonitoring);
}

void AdminPanel::refresh()
{
    if (!m_accountManager->isLoggedIn()) return;

    refreshAccounts();
    refreshDevices();
    updateMonitoringView();
}

void AdminPanel::onLoginSuccessful(const UserAccount& account)
{
    Q_UNUSED(account)
    updateUIState();
    refresh();
}

void AdminPanel::onLoggedOut()
{
    m_selectedAccountId.clear();
    m_selectedDeviceId.clear();
    m_accountsTable->setRowCount(0);
    m_devicesTable->setRowCount(0);
    updateUIState();
}

void AdminPanel::onLoginClicked()
{
    QString email = m_emailInput->text().trimmed();
    QString password = m_passwordInput->text();

    if (email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login", "Please enter email and password");
        return;
    }

    if (!m_accountManager->login(email, password)) {
        QMessageBox::warning(this, "Login Failed", "Invalid credentials or access denied");
    }
}

void AdminPanel::onLogoutClicked()
{
    m_accountManager->logout();
}

void AdminPanel::onCreateSubAccountClicked()
{
    QString email = QInputDialog::getText(this, "Create Sub-Account", "Email:");
    if (email.isEmpty()) return;

    QString name = QInputDialog::getText(this, "Create Sub-Account", "Display Name:");
    if (name.isEmpty()) return;

    SubAccountRequest request;
    request.email = email;
    request.displayName = name;
    request.role = AccountRole::USER;
    request.tier = SubscriptionTier::BASIC;

    if (m_accountManager->createSubAccount(request)) {
        QMessageBox::information(this, "Success", "Sub-account created. Temporary password has been logged.");
        refreshAccounts();
    } else {
        QMessageBox::warning(this, "Error", "Failed to create sub-account");
    }
}

void AdminPanel::onSuspendAccountClicked()
{
    if (m_selectedAccountId.isEmpty()) return;

    QString reason = QInputDialog::getText(this, "Suspend Account", "Reason for suspension:");
    if (m_accountManager->suspendAccount(m_selectedAccountId, reason)) {
        refreshAccounts();
    }
}

void AdminPanel::onUnsuspendAccountClicked()
{
    if (m_selectedAccountId.isEmpty()) return;

    if (m_accountManager->unsuspendAccount(m_selectedAccountId)) {
        refreshAccounts();
    }
}

void AdminPanel::onDeleteAccountClicked()
{
    if (m_selectedAccountId.isEmpty()) return;

    if (QMessageBox::question(this, "Confirm Delete",
            "Are you sure you want to delete this account?") == QMessageBox::Yes) {
        if (m_accountManager->deleteAccount(m_selectedAccountId)) {
            m_selectedAccountId.clear();
            refreshAccounts();
        }
    }
}

void AdminPanel::onGrantPointsClicked()
{
    if (m_selectedAccountId.isEmpty()) return;

    bool ok;
    int amount = QInputDialog::getInt(this, "Grant Points", "Amount:", 100, 1, 1000000, 1, &ok);
    if (!ok) return;

    QString reason = QInputDialog::getText(this, "Grant Points", "Reason:");
    if (m_accountManager->grantPoints(m_selectedAccountId, amount, reason)) {
        refreshAccounts();
    }
}

void AdminPanel::onAccountSelected(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0) {
        m_selectedAccountId = m_accountsTable->item(row, 0)->text();
    }
    updateUIState();
}

void AdminPanel::onDeviceSelected(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0) {
        m_selectedDeviceId = m_devicesTable->item(row, 0)->text();
    }
    updateUIState();
}

void AdminPanel::onMonitorDeviceClicked()
{
    if (m_selectedDeviceId.isEmpty()) return;
    m_remoteMonitor->startMonitoring(m_selectedDeviceId, false);
}

void AdminPanel::onStopMonitoringClicked()
{
    if (m_selectedDeviceId.isEmpty()) return;
    m_remoteMonitor->stopMonitoring(m_selectedDeviceId);
}

void AdminPanel::onTakeControlClicked()
{
    if (m_selectedDeviceId.isEmpty()) return;
    m_remoteMonitor->requestControl(m_selectedDeviceId);
    updateUIState();
}

void AdminPanel::onReleaseControlClicked()
{
    if (m_selectedDeviceId.isEmpty()) return;
    m_remoteMonitor->releaseControl(m_selectedDeviceId);
    updateUIState();
}

void AdminPanel::onEmergencyStopClicked()
{
    if (m_selectedDeviceId.isEmpty()) return;
    m_remoteMonitor->emergencyStop(m_selectedDeviceId);
    QMessageBox::warning(this, "Emergency Stop", "Emergency stop triggered for device");
}

void AdminPanel::onEmergencyStopAllClicked()
{
    if (QMessageBox::question(this, "Emergency Stop All",
            "Are you sure you want to emergency stop ALL monitored devices?") == QMessageBox::Yes) {
        m_remoteMonitor->emergencyStopAll();
        QMessageBox::warning(this, "Emergency Stop", "Emergency stop triggered for all devices");
    }
}

void AdminPanel::onDeviceStateUpdated(const QString& deviceId, const QJsonObject& state)
{
    if (deviceId != m_selectedDeviceId) return;

    m_deviceStateView->setPlainText(
        QString::fromUtf8(QJsonDocument(state).toJson(QJsonDocument::Indented)));

    DeviceInfo device = DeviceRegistry::instance()->device(deviceId);
    m_batteryBar->setValue(static_cast<int>(device.batteryLevel));
}

void AdminPanel::onMonitoringStarted(const QString& deviceId, const MonitorSession& session)
{
    Q_UNUSED(deviceId)
    m_monitorStatusLabel->setText(QString("Monitoring: %1").arg(session.targetDeviceId));
    m_sessionInfoLabel->setText(QString("Session ID: %1\nStarted: %2")
        .arg(session.sessionId)
        .arg(session.startedAt.toString()));
    updateUIState();
}

void AdminPanel::onMonitoringStopped(const QString& deviceId)
{
    Q_UNUSED(deviceId)
    m_monitorStatusLabel->setText("Not monitoring");
    m_sessionInfoLabel->clear();
    m_deviceStateView->clear();
    updateUIState();
}


void AdminPanel::refreshAccounts()
{
    QVector<UserAccount> accounts = m_accountManager->allAccounts();
    m_accountsTable->setRowCount(accounts.size());

    for (int i = 0; i < accounts.size(); ++i) {
        const UserAccount& acc = accounts[i];
        m_accountsTable->setItem(i, 0, new QTableWidgetItem(acc.accountId));
        m_accountsTable->setItem(i, 1, new QTableWidgetItem(acc.email));
        m_accountsTable->setItem(i, 2, new QTableWidgetItem(acc.displayName));

        QString roleStr;
        switch (acc.role) {
            case AccountRole::MASTER: roleStr = "MASTER"; break;
            case AccountRole::ADMIN: roleStr = "ADMIN"; break;
            case AccountRole::MODERATOR: roleStr = "MODERATOR"; break;
            default: roleStr = "USER"; break;
        }
        m_accountsTable->setItem(i, 3, new QTableWidgetItem(roleStr));

        QString statusStr;
        switch (acc.status) {
            case AccountStatus::ACTIVE: statusStr = "Active"; break;
            case AccountStatus::SUSPENDED: statusStr = "Suspended"; break;
            case AccountStatus::PENDING_VERIFICATION: statusStr = "Pending"; break;
            case AccountStatus::LOCKED: statusStr = "Locked"; break;
            default: statusStr = "Deleted"; break;
        }
        m_accountsTable->setItem(i, 4, new QTableWidgetItem(statusStr));
        m_accountsTable->setItem(i, 5, new QTableWidgetItem(QString::number(acc.pointsBalance)));
    }
}

void AdminPanel::refreshDevices()
{
    DeviceRegistry* registry = DeviceRegistry::instance();
    QList<DeviceInfo> devices = registry->allDevices();

    m_onlineCountLabel->setText(QString("Online: %1").arg(registry->onlineCount()));
    m_totalCountLabel->setText(QString("Total: %1").arg(registry->totalCount()));

    m_devicesTable->setRowCount(devices.size());

    for (int i = 0; i < devices.size(); ++i) {
        const DeviceInfo& dev = devices[i];
        m_devicesTable->setItem(i, 0, new QTableWidgetItem(dev.deviceId));
        m_devicesTable->setItem(i, 1, new QTableWidgetItem(dev.deviceName));
        m_devicesTable->setItem(i, 2, new QTableWidgetItem(dev.ownerAccountId));

        QString statusStr;
        switch (dev.status) {
            case DeviceStatus::ONLINE: statusStr = "Online"; break;
            case DeviceStatus::OFFLINE: statusStr = "Offline"; break;
            case DeviceStatus::BUSY: statusStr = "Busy"; break;
            case DeviceStatus::IDLE: statusStr = "Idle"; break;
            case DeviceStatus::MAINTENANCE: statusStr = "Maintenance"; break;
            default: statusStr = "Error"; break;
        }
        m_devicesTable->setItem(i, 3, new QTableWidgetItem(statusStr));
        m_devicesTable->setItem(i, 4, new QTableWidgetItem(QString::number(dev.batteryLevel, 'f', 0) + "%"));
        m_devicesTable->setItem(i, 5, new QTableWidgetItem(dev.lastHeartbeatAt.toString("hh:mm:ss")));
        m_devicesTable->setItem(i, 6, new QTableWidgetItem(dev.ipAddress));
    }
}

void AdminPanel::refreshActivityLog()
{
    m_activityLog->clear();

    // Get activity from all accounts if master
    QVector<UserAccount> accounts = m_accountManager->allAccounts();
    for (const UserAccount& acc : accounts) {
        QVector<QJsonObject> log = m_accountManager->activityLog(acc.accountId, 50);
        for (const QJsonObject& entry : log) {
            QString line = QString("[%1] %2: %3")
                .arg(entry["timestamp"].toString())
                .arg(acc.email)
                .arg(entry["activity"].toString());
            m_activityLog->append(line);
        }
    }
}

void AdminPanel::updateMonitoringView()
{
    if (m_selectedDeviceId.isEmpty()) return;
    if (!m_remoteMonitor->isMonitoring(m_selectedDeviceId)) return;

    RemoteViewData data = m_remoteMonitor->latestData(m_selectedDeviceId);
    if (!data.deviceId.isEmpty()) {
        onDeviceStateUpdated(data.deviceId, data.deviceState);
    }
}

void AdminPanel::populateAccountsTable()
{
    refreshAccounts();
}

void AdminPanel::populateDevicesTable()
{
    refreshDevices();
}