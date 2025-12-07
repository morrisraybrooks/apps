#include "AccountManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

const char* AccountManager::DB_CONNECTION_NAME = "AccountManagerDB";

AccountManager::AccountManager(QObject* parent)
    : QObject(parent)
{
    initDatabase();
}

AccountManager::~AccountManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
    QSqlDatabase::removeDatabase(DB_CONNECTION_NAME);
}

void AccountManager::initDatabase()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString dbPath = dataPath + "/accounts.db";

    m_database = QSqlDatabase::addDatabase("QSQLITE", DB_CONNECTION_NAME);
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qCritical() << "Failed to open accounts database:" << m_database.lastError().text();
        return;
    }

    QSqlQuery query(m_database);

    // Create accounts table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS accounts (
            account_id TEXT PRIMARY KEY,
            email TEXT UNIQUE NOT NULL,
            display_name TEXT,
            password_hash TEXT NOT NULL,
            role INTEGER DEFAULT 0,
            status INTEGER DEFAULT 2,
            master_account_id TEXT,
            created_at TEXT,
            last_login_at TEXT,
            last_activity_at TEXT,
            current_device_id TEXT,
            subscription_tier INTEGER DEFAULT 0,
            points_balance INTEGER DEFAULT 0,
            preferences TEXT,
            permissions TEXT,
            FOREIGN KEY (master_account_id) REFERENCES accounts(account_id)
        )
    )");

    // Create linked devices table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS linked_devices (
            account_id TEXT NOT NULL,
            device_id TEXT NOT NULL,
            linked_at TEXT,
            last_seen_at TEXT,
            device_name TEXT,
            PRIMARY KEY (account_id, device_id),
            FOREIGN KEY (account_id) REFERENCES accounts(account_id)
        )
    )");

    // Create activity log table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS activity_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account_id TEXT NOT NULL,
            activity TEXT NOT NULL,
            data TEXT,
            timestamp TEXT,
            ip_address TEXT,
            device_id TEXT,
            FOREIGN KEY (account_id) REFERENCES accounts(account_id)
        )
    )");

    // Create master keys table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS master_keys (
            key_hash TEXT PRIMARY KEY,
            created_at TEXT,
            created_by TEXT,
            description TEXT
        )
    )");

    // Check if master account exists, create default if not
    query.exec("SELECT COUNT(*) FROM accounts WHERE role = 3");
    if (query.next() && query.value(0).toInt() == 0) {
        // Create default master account
        QString masterId = generateAccountId();
        QString defaultPassword = hashPassword("master_admin_2024");

        query.prepare(R"(
            INSERT INTO accounts (account_id, email, display_name, password_hash, role, status, created_at)
            VALUES (?, ?, ?, ?, 3, 0, ?)
        )");
        query.addBindValue(masterId);
        query.addBindValue("master@vcontour.local");
        query.addBindValue("Master Admin");
        query.addBindValue(defaultPassword);
        query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
        query.exec();

        qInfo() << "Created default master account: master@vcontour.local";
    }
}

bool AccountManager::login(const QString& email, const QString& password)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM accounts WHERE email = ? AND status != 4");
    query.addBindValue(email.toLower());

    if (!query.exec() || !query.next()) {
        emit loginFailed("Account not found");
        return false;
    }

    QString storedHash = query.value("password_hash").toString();
    if (!verifyPassword(password, storedHash)) {
        emit loginFailed("Invalid password");
        return false;
    }

    AccountStatus status = static_cast<AccountStatus>(query.value("status").toInt());
    if (status == AccountStatus::SUSPENDED) {
        emit loginFailed("Account is suspended");
        return false;
    }

    if (status == AccountStatus::LOCKED) {
        emit loginFailed("Account is locked");
        return false;
    }

    // Load account data
    loadCurrentAccount(query.value("account_id").toString());

    // Update last login
    QSqlQuery updateQuery(m_database);
    updateQuery.prepare("UPDATE accounts SET last_login_at = ? WHERE account_id = ?");
    updateQuery.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    updateQuery.addBindValue(m_currentAccount.accountId);
    updateQuery.exec();

    logActivity(m_currentAccount.accountId, "login", QJsonObject());
    emit loginSuccessful(m_currentAccount);
    return true;
}

void AccountManager::logout()
{
    if (!m_currentAccount.accountId.isEmpty()) {
        logActivity(m_currentAccount.accountId, "logout", QJsonObject());
    }
    m_currentAccount = UserAccount();
    emit loggedOut();
}

void AccountManager::loadCurrentAccount(const QString& accountId)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM accounts WHERE account_id = ?");
    query.addBindValue(accountId);

    if (!query.exec() || !query.next()) return;

    m_currentAccount.accountId = query.value("account_id").toString();
    m_currentAccount.email = query.value("email").toString();
    m_currentAccount.displayName = query.value("display_name").toString();
    m_currentAccount.passwordHash = query.value("password_hash").toString();
    m_currentAccount.role = static_cast<AccountRole>(query.value("role").toInt());
    m_currentAccount.status = static_cast<AccountStatus>(query.value("status").toInt());
    m_currentAccount.masterAccountId = query.value("master_account_id").toString();
    m_currentAccount.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    m_currentAccount.lastLoginAt = QDateTime::fromString(query.value("last_login_at").toString(), Qt::ISODate);
    m_currentAccount.currentDeviceId = query.value("current_device_id").toString();
    m_currentAccount.subscriptionTier = static_cast<SubscriptionTier>(query.value("subscription_tier").toInt());
    m_currentAccount.pointsBalance = query.value("points_balance").toInt();

    // Load preferences and permissions
    QJsonDocument prefsDoc = QJsonDocument::fromJson(query.value("preferences").toString().toUtf8());
    m_currentAccount.preferences = prefsDoc.object();

    QJsonDocument permsDoc = QJsonDocument::fromJson(query.value("permissions").toString().toUtf8());
    m_currentAccount.permissions = permsDoc.object();

    // Load linked devices
    QSqlQuery devQuery(m_database);
    devQuery.prepare("SELECT device_id FROM linked_devices WHERE account_id = ?");
    devQuery.addBindValue(accountId);
    if (devQuery.exec()) {
        while (devQuery.next()) {
            m_currentAccount.linkedDeviceIds.append(devQuery.value(0).toString());
        }
    }
}

bool AccountManager::verifyMasterCredentials(const QString& masterKey)
{
    QString keyHash = hashPassword(masterKey);

    QSqlQuery query(m_database);
    query.prepare("SELECT key_hash FROM master_keys WHERE key_hash = ?");
    query.addBindValue(keyHash);

    return query.exec() && query.next();
}

bool AccountManager::createSubAccount(const SubAccountRequest& request)
{
    if (!m_currentAccount.isAdmin()) {
        qWarning() << "Permission denied: requires ADMIN or MASTER role";
        return false;
    }

    // Check if email already exists
    QSqlQuery checkQuery(m_database);
    checkQuery.prepare("SELECT COUNT(*) FROM accounts WHERE email = ?");
    checkQuery.addBindValue(request.email.toLower());
    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        qWarning() << "Email already exists:" << request.email;
        return false;
    }

    QString accountId = generateAccountId();
    QString tempPassword = QUuid::createUuid().toString(QUuid::Id128).left(12);
    QString passwordHash = hashPassword(tempPassword);

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO accounts (account_id, email, display_name, password_hash, role, status,
                              master_account_id, created_at, subscription_tier, points_balance, permissions)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(accountId);
    query.addBindValue(request.email.toLower());
    query.addBindValue(request.displayName);
    query.addBindValue(passwordHash);
    query.addBindValue(static_cast<int>(request.role));
    query.addBindValue(static_cast<int>(AccountStatus::PENDING_VERIFICATION));
    query.addBindValue(m_currentAccount.accountId);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(static_cast<int>(request.tier));
    query.addBindValue(request.initialPoints);
    query.addBindValue(QString::fromUtf8(QJsonDocument(request.permissions).toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qWarning() << "Failed to create sub-account:" << query.lastError().text();
        return false;
    }

    logActivity(m_currentAccount.accountId, "create_sub_account",
                QJsonObject{{"sub_account_id", accountId}, {"email", request.email}});

    emit subAccountCreated(accountId);
    qInfo() << "Created sub-account:" << request.email << "Temp password:" << tempPassword;
    return true;
}

bool AccountManager::suspendAccount(const QString& accountId, const QString& reason)
{
    if (!m_currentAccount.isAdmin()) return false;

    // Cannot suspend master accounts
    UserAccount target = getAccount(accountId);
    if (target.isMaster()) {
        qWarning() << "Cannot suspend master account";
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET status = ? WHERE account_id = ?");
    query.addBindValue(static_cast<int>(AccountStatus::SUSPENDED));
    query.addBindValue(accountId);

    if (!query.exec()) return false;

    logActivity(m_currentAccount.accountId, "suspend_account",
                QJsonObject{{"target_id", accountId}, {"reason", reason}});

    emit accountSuspended(accountId);
    return true;
}

bool AccountManager::unsuspendAccount(const QString& accountId)
{
    if (!m_currentAccount.isAdmin()) return false;

    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET status = ? WHERE account_id = ?");
    query.addBindValue(static_cast<int>(AccountStatus::ACTIVE));
    query.addBindValue(accountId);

    if (!query.exec()) return false;

    logActivity(m_currentAccount.accountId, "unsuspend_account",
                QJsonObject{{"target_id", accountId}});

    emit accountUpdated(accountId);
    return true;
}

bool AccountManager::deleteAccount(const QString& accountId)
{
    if (!m_currentAccount.isMaster()) return false;

    UserAccount target = getAccount(accountId);
    if (target.isMaster()) {
        qWarning() << "Cannot delete master account";
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET status = ? WHERE account_id = ?");
    query.addBindValue(static_cast<int>(AccountStatus::DELETED));
    query.addBindValue(accountId);

    if (!query.exec()) return false;

    logActivity(m_currentAccount.accountId, "delete_account",
                QJsonObject{{"target_id", accountId}});

    return true;
}

bool AccountManager::updateAccountRole(const QString& accountId, AccountRole newRole)
{
    if (!m_currentAccount.isMaster()) return false;

    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET role = ? WHERE account_id = ?");
    query.addBindValue(static_cast<int>(newRole));
    query.addBindValue(accountId);

    if (!query.exec()) return false;

    logActivity(m_currentAccount.accountId, "update_role",
                QJsonObject{{"target_id", accountId}, {"new_role", static_cast<int>(newRole)}});

    emit accountUpdated(accountId);
    return true;
}

bool AccountManager::updateAccountPermissions(const QString& accountId, const QJsonObject& perms)
{
    if (!m_currentAccount.isAdmin()) return false;

    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET permissions = ? WHERE account_id = ?");
    query.addBindValue(QString::fromUtf8(QJsonDocument(perms).toJson(QJsonDocument::Compact)));
    query.addBindValue(accountId);

    if (!query.exec()) return false;

    emit accountUpdated(accountId);
    return true;
}

QVector<UserAccount> AccountManager::subAccounts() const
{
    QVector<UserAccount> accounts;
    QSqlQuery query(m_database);
    query.prepare("SELECT account_id FROM accounts WHERE master_account_id = ?");
    query.addBindValue(m_currentAccount.accountId);

    if (query.exec()) {
        while (query.next()) {
            UserAccount acc;
            // Would need to load full account here
            acc.accountId = query.value(0).toString();
            accounts.append(acc);
        }
    }
    return accounts;
}

QVector<UserAccount> AccountManager::allAccounts() const
{
    QVector<UserAccount> accounts;
    if (!m_currentAccount.isMaster()) return accounts;

    QSqlQuery query(m_database);
    query.exec("SELECT account_id, email, display_name, role, status FROM accounts WHERE status != 4");

    while (query.next()) {
        UserAccount acc;
        acc.accountId = query.value(0).toString();
        acc.email = query.value(1).toString();
        acc.displayName = query.value(2).toString();
        acc.role = static_cast<AccountRole>(query.value(3).toInt());
        acc.status = static_cast<AccountStatus>(query.value(4).toInt());
        accounts.append(acc);
    }
    return accounts;
}

UserAccount AccountManager::getAccount(const QString& accountId) const
{
    UserAccount acc;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM accounts WHERE account_id = ?");
    query.addBindValue(accountId);

    if (query.exec() && query.next()) {
        acc.accountId = query.value("account_id").toString();
        acc.email = query.value("email").toString();
        acc.displayName = query.value("display_name").toString();
        acc.role = static_cast<AccountRole>(query.value("role").toInt());
        acc.status = static_cast<AccountStatus>(query.value("status").toInt());
        acc.masterAccountId = query.value("master_account_id").toString();
        acc.subscriptionTier = static_cast<SubscriptionTier>(query.value("subscription_tier").toInt());
        acc.pointsBalance = query.value("points_balance").toInt();
    }
    return acc;
}

UserAccount AccountManager::getAccountByEmail(const QString& email) const
{
    UserAccount acc;
    QSqlQuery query(m_database);
    query.prepare("SELECT account_id FROM accounts WHERE email = ?");
    query.addBindValue(email.toLower());

    if (query.exec() && query.next()) {
        return getAccount(query.value(0).toString());
    }
    return acc;
}

bool AccountManager::linkDevice(const QString& accountId, const QString& deviceId)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO linked_devices (account_id, device_id, linked_at, last_seen_at)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(accountId);
    query.addBindValue(deviceId);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) return false;

    emit deviceLinked(accountId, deviceId);
    return true;
}

bool AccountManager::unlinkDevice(const QString& accountId, const QString& deviceId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM linked_devices WHERE account_id = ? AND device_id = ?");
    query.addBindValue(accountId);
    query.addBindValue(deviceId);
    return query.exec();
}

QStringList AccountManager::linkedDevices(const QString& accountId) const
{
    QStringList devices;
    QSqlQuery query(m_database);
    query.prepare("SELECT device_id FROM linked_devices WHERE account_id = ?");
    query.addBindValue(accountId);

    if (query.exec()) {
        while (query.next()) {
            devices.append(query.value(0).toString());
        }
    }
    return devices;
}

bool AccountManager::transferPoints(const QString& fromAccountId, const QString& toAccountId, int amount)
{
    if (amount <= 0) return false;

    // Check balance
    UserAccount from = getAccount(fromAccountId);
    if (from.pointsBalance < amount) return false;

    m_database.transaction();

    QSqlQuery deduct(m_database);
    deduct.prepare("UPDATE accounts SET points_balance = points_balance - ? WHERE account_id = ?");
    deduct.addBindValue(amount);
    deduct.addBindValue(fromAccountId);

    QSqlQuery add(m_database);
    add.prepare("UPDATE accounts SET points_balance = points_balance + ? WHERE account_id = ?");
    add.addBindValue(amount);
    add.addBindValue(toAccountId);

    if (deduct.exec() && add.exec()) {
        m_database.commit();
        emit pointsTransferred(fromAccountId, toAccountId, amount);
        return true;
    }

    m_database.rollback();
    return false;
}

bool AccountManager::grantPoints(const QString& accountId, int amount, const QString& reason)
{
    if (!m_currentAccount.isAdmin()) return false;

    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET points_balance = points_balance + ? WHERE account_id = ?");
    query.addBindValue(amount);
    query.addBindValue(accountId);

    if (!query.exec()) return false;

    logActivity(m_currentAccount.accountId, "grant_points",
                QJsonObject{{"target_id", accountId}, {"amount", amount}, {"reason", reason}});

    return true;
}

void AccountManager::logActivity(const QString& accountId, const QString& activity, const QJsonObject& data)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO activity_log (account_id, activity, data, timestamp)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(accountId);
    query.addBindValue(activity);
    query.addBindValue(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.exec();

    emit activityLogged(accountId, activity);
}

QVector<QJsonObject> AccountManager::activityLog(const QString& accountId, int limit) const
{
    QVector<QJsonObject> log;

    // Only master/admin can view any account, others can only view their own
    if (!m_currentAccount.isAdmin() && accountId != m_currentAccount.accountId) {
        return log;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT activity, data, timestamp FROM activity_log
        WHERE account_id = ? ORDER BY timestamp DESC LIMIT ?
    )");
    query.addBindValue(accountId);
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            QJsonObject entry;
            entry["activity"] = query.value(0).toString();
            entry["data"] = QJsonDocument::fromJson(query.value(1).toString().toUtf8()).object();
            entry["timestamp"] = query.value(2).toString();
            log.append(entry);
        }
    }
    return log;
}

QString AccountManager::hashPassword(const QString& password) const
{
    QByteArray salt = "VContour_Salt_2024";
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8() + salt, QCryptographicHash::Sha256);
    return hash.toHex();
}

bool AccountManager::verifyPassword(const QString& password, const QString& hash) const
{
    return hashPassword(password) == hash;
}

QString AccountManager::generateAccountId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
