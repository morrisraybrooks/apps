#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "../game/GameTypes.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QSqlDatabase>

/**
 * @brief Account role enumeration
 */
enum class AccountRole {
    USER,           // Regular user account
    MODERATOR,      // Can view but not control
    ADMIN,          // Can manage sub-accounts
    MASTER          // Full access to all accounts and devices
};
Q_DECLARE_METATYPE(AccountRole)

/**
 * @brief Account status enumeration
 */
enum class AccountStatus {
    ACTIVE,
    SUSPENDED,
    PENDING_VERIFICATION,
    LOCKED,
    DELETED
};
Q_DECLARE_METATYPE(AccountStatus)

/**
 * @brief User account information
 */
struct UserAccount {
    QString accountId;
    QString email;
    QString displayName;
    QString passwordHash;
    AccountRole role = AccountRole::USER;
    AccountStatus status = AccountStatus::PENDING_VERIFICATION;
    QString masterAccountId;  // Empty for master accounts
    QDateTime createdAt;
    QDateTime lastLoginAt;
    QDateTime lastActivityAt;
    QString currentDeviceId;
    QStringList linkedDeviceIds;
    SubscriptionTier subscriptionTier = SubscriptionTier::FREE;
    int pointsBalance = 0;
    QJsonObject preferences;
    QJsonObject permissions;
    
    bool isMaster() const { return role == AccountRole::MASTER; }
    bool isAdmin() const { return role >= AccountRole::ADMIN; }
    bool canViewAll() const { return role >= AccountRole::MODERATOR; }
    bool canControl() const { return role >= AccountRole::ADMIN; }
};

/**
 * @brief Sub-account creation request
 */
struct SubAccountRequest {
    QString email;
    QString displayName;
    AccountRole role = AccountRole::USER;
    SubscriptionTier tier = SubscriptionTier::BASIC;
    QJsonObject permissions;
    int initialPoints = 0;
};

/**
 * @brief Account Manager for master/sub-account hierarchy
 * 
 * Master accounts can:
 * - Create and manage sub-accounts
 * - View any account's activity and statistics
 * - Monitor any linked device in real-time
 * - Control devices remotely (with permissions)
 * - Suspend/unsuspend sub-accounts
 * - Transfer points between accounts
 */
class AccountManager : public QObject
{
    Q_OBJECT

public:
    explicit AccountManager(QObject* parent = nullptr);
    ~AccountManager();

    // Authentication
    bool login(const QString& email, const QString& password);
    void logout();
    bool isLoggedIn() const { return !m_currentAccount.accountId.isEmpty(); }
    UserAccount currentAccount() const { return m_currentAccount; }

    // Master account verification
    bool isMasterAccount() const { return m_currentAccount.isMaster(); }
    bool verifyMasterCredentials(const QString& masterKey);

    // Sub-account management (requires ADMIN or MASTER role)
    bool createSubAccount(const SubAccountRequest& request);
    bool suspendAccount(const QString& accountId, const QString& reason);
    bool unsuspendAccount(const QString& accountId);
    bool deleteAccount(const QString& accountId);
    bool updateAccountRole(const QString& accountId, AccountRole newRole);
    bool updateAccountPermissions(const QString& accountId, const QJsonObject& perms);

    // Account queries
    QVector<UserAccount> subAccounts() const;
    QVector<UserAccount> allAccounts() const;  // Master only
    UserAccount getAccount(const QString& accountId) const;
    UserAccount getAccountByEmail(const QString& email) const;

    // Device linking
    bool linkDevice(const QString& accountId, const QString& deviceId);
    bool unlinkDevice(const QString& accountId, const QString& deviceId);
    QStringList linkedDevices(const QString& accountId) const;

    // Points management
    bool transferPoints(const QString& fromAccountId, const QString& toAccountId, int amount);
    bool grantPoints(const QString& accountId, int amount, const QString& reason);

    // Activity tracking
    void logActivity(const QString& accountId, const QString& activity, const QJsonObject& data);
    QVector<QJsonObject> activityLog(const QString& accountId, int limit = 100) const;

Q_SIGNALS:
    void loginSuccessful(const UserAccount& account);
    void loginFailed(const QString& error);
    void loggedOut();
    void subAccountCreated(const QString& accountId);
    void accountUpdated(const QString& accountId);
    void accountSuspended(const QString& accountId);
    void deviceLinked(const QString& accountId, const QString& deviceId);
    void pointsTransferred(const QString& from, const QString& to, int amount);
    void activityLogged(const QString& accountId, const QString& activity);

private:
    void initDatabase();
    QString hashPassword(const QString& password) const;
    bool verifyPassword(const QString& password, const QString& hash) const;
    QString generateAccountId() const;
    void loadCurrentAccount(const QString& accountId);

    QSqlDatabase m_database;
    UserAccount m_currentAccount;
    QString m_masterKey;
    
    static const char* DB_CONNECTION_NAME;
};

#endif // ACCOUNTMANAGER_H

