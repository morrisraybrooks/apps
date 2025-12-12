#include "LicenseManager.h"
#include "LicenseServer.h"
#include <QTimer>
#include <QUuid>
#include <QSysInfo>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QDir>

const char* LicenseManager::LICENSE_STORAGE_KEY = "license/data";
const char* LicenseManager::DEVICE_ID_KEY = "device/id";

LicenseManager::LicenseManager(QObject* parent)
    : QObject(parent)
    , m_settings(new QSettings("VContour", "LicenseManager", this))
    , m_expirationTimer(new QTimer(this))
    , m_offlineMode(false)
    , m_server(new LicenseServer(this))
{
    // Generate or load device ID
    m_deviceId = m_settings->value(DEVICE_ID_KEY).toString();
    if (m_deviceId.isEmpty()) {
        m_deviceId = generateDeviceId();
        m_settings->setValue(DEVICE_ID_KEY, m_deviceId);
    }

    // Initialize available products
    initializePointBundles();
    initializeSubscriptionPlans();

    // Load existing license
    loadLicense();

    // Check expiration daily
    connect(m_expirationTimer, &QTimer::timeout, this, &LicenseManager::checkExpiration);
    m_expirationTimer->start(24 * 60 * 60 * 1000);  // 24 hours

    // Connect to server signals
    connect(m_server, &LicenseServer::validationComplete,
            this, &LicenseManager::onValidationComplete);
    connect(m_server, &LicenseServer::error,
            this, &LicenseManager::validationError);
}

LicenseManager::~LicenseManager()
{
    saveLicense();
}

void LicenseManager::initializePointBundles()
{
    m_pointBundles = {
        {"starter_100", "Starter Pack", 100, 0.99, 0},
        {"basic_500", "Basic Bundle", 500, 3.99, 10},
        {"standard_1500", "Standard Bundle", 1500, 9.99, 20},
        {"premium_5000", "Premium Bundle", 5000, 24.99, 30},
        {"mega_15000", "Mega Bundle", 15000, 49.99, 50},
        {"ultimate_50000", "Ultimate Bundle", 50000, 99.99, 75}
    };
}

void LicenseManager::initializeSubscriptionPlans()
{
    m_subscriptionPlans = {
        {
            "basic_monthly", "Basic Monthly", "Essential features for beginners",
            SubscriptionTier::BASIC, LicenseType::MONTHLY, 4.99, 30,
            {"basic_patterns", "basic_games", "progress_tracking"},
            50
        },
        {
            "standard_monthly", "Standard Monthly", "Most popular - all standard features",
            SubscriptionTier::STANDARD, LicenseType::MONTHLY, 9.99, 30,
            {"all_patterns", "all_games", "multi_user", "custom_games", "statistics"},
            150
        },
        {
            "premium_monthly", "Premium Monthly", "Full experience with TENS & advanced",
            SubscriptionTier::PREMIUM, LicenseType::MONTHLY, 19.99, 30,
            {"tens_control", "intense_modes", "dom_features", "priority_support", "beta_access"},
            500
        },
        {
            "standard_yearly", "Standard Yearly", "Save 20% with yearly billing",
            SubscriptionTier::STANDARD, LicenseType::YEARLY, 95.88, 365,
            {"all_patterns", "all_games", "multi_user", "custom_games", "statistics"},
            200
        },
        {
            "premium_yearly", "Premium Yearly", "Best value - save 25%",
            SubscriptionTier::PREMIUM, LicenseType::YEARLY, 179.88, 365,
            {"tens_control", "intense_modes", "dom_features", "priority_support", "beta_access"},
            750
        },
        {
            "lifetime", "Lifetime Premium", "One-time purchase, forever premium",
            SubscriptionTier::LIFETIME, LicenseType::LIFETIME, 299.99, -1,
            {"all_features", "lifetime_updates", "founder_badge"},
            5000
        }
    };
}

bool LicenseManager::activateLicense(const QString& licenseKey)
{
    if (!validateKeyFormat(licenseKey)) {
        emit validationError("Invalid license key format");
        return false;
    }

    if (!validateKeyChecksum(licenseKey)) {
        emit validationError("License key checksum failed");
        return false;
    }

    // Send to server for validation
    m_server->activateLicense(licenseKey, m_deviceId);
    m_license.licenseKey = licenseKey;
    m_license.status = LicenseStatus::PENDING;

    return true;
}

bool LicenseManager::deactivateLicense()
{
    if (m_license.licenseKey.isEmpty()) {
        return false;
    }

    m_server->deactivateLicense(m_license.licenseKey, m_deviceId);

    m_license = LicenseInfo();
    m_license.status = LicenseStatus::INVALID;
    saveLicense();

    emit licenseDeactivated();
    return true;
}

bool LicenseManager::validateLicense()
{
    if (m_license.licenseKey.isEmpty()) {
        m_license.status = LicenseStatus::INVALID;
        return false;
    }

    if (m_offlineMode) {
        // Check offline grace period
        QDateTime lastValidation = m_settings->value("license/lastValidation").toDateTime();
        if (lastValidation.daysTo(QDateTime::currentDateTime()) > OFFLINE_GRACE_DAYS) {
            m_license.status = LicenseStatus::EXPIRED;
            return false;
        }
        return m_license.isValid();
    }

    m_server->validateLicense(m_license.licenseKey, m_deviceId);
    return true;
}

void LicenseManager::onValidationComplete()
{
    LicenseInfo serverInfo = m_server->lastLicenseInfo();

    if (serverInfo.status == LicenseStatus::VALID) {
        m_license = serverInfo;
        m_license.deviceId = m_deviceId;
        m_settings->setValue("license/lastValidation", QDateTime::currentDateTime());
        saveLicense();

        emit licenseActivated(m_license);
        emit licenseValidated(LicenseStatus::VALID);

        // Check if expiring soon
        int daysRemaining = m_license.daysRemaining();
        if (daysRemaining >= 0 && daysRemaining <= 7) {
            emit licenseExpiring(daysRemaining);
        }
    } else {
        m_license.status = serverInfo.status;
        emit licenseValidated(serverInfo.status);
    }
}

void LicenseManager::checkExpiration()
{
    if (!m_license.isValid()) return;

    int daysRemaining = m_license.daysRemaining();

    if (daysRemaining == 0) {
        m_license.status = LicenseStatus::EXPIRED;
        emit licenseExpired();
    } else if (daysRemaining > 0 && daysRemaining <= 7) {
        emit licenseExpiring(daysRemaining);
    }
}

bool LicenseManager::hasFeature(const QString& featureId) const
{
    // Free tier has minimal features
    if (m_license.tier == SubscriptionTier::FREE) {
        static const QStringList freeFeatures = {
            "basic_vacuum", "safety_controls", "emergency_stop"
        };
        return freeFeatures.contains(featureId);
    }

    // Check specific feature unlocks
    if (m_license.unlockedFeatures.contains(featureId)) {
        return true;
    }

    // Check tier-based features
    switch (m_license.tier) {
        case SubscriptionTier::LIFETIME:
        case SubscriptionTier::PREMIUM:
            if (featureId == "tens_control" || featureId == "intense_modes" ||
                featureId == "dom_features" || featureId == "beta_access") {
                return true;
            }
            [[fallthrough]];

        case SubscriptionTier::STANDARD:
            if (featureId == "all_patterns" || featureId == "all_games" ||
                featureId == "multi_user" || featureId == "custom_games" ||
                featureId == "statistics") {
                return true;
            }
            [[fallthrough]];

        case SubscriptionTier::BASIC:
            if (featureId == "basic_patterns" || featureId == "basic_games" ||
                featureId == "progress_tracking") {
                return true;
            }
            [[fallthrough]];

        case SubscriptionTier::FREE:
        default:
            break;
    }

    return false;
}

QStringList LicenseManager::availableFeatures() const
{
    QStringList features;

    // Add all features available at current tier
    switch (m_license.tier) {
        case SubscriptionTier::LIFETIME:
        case SubscriptionTier::PREMIUM:
            features << "tens_control" << "intense_modes" << "dom_features" << "beta_access";
            [[fallthrough]];
        case SubscriptionTier::STANDARD:
            features << "all_patterns" << "all_games" << "multi_user" << "custom_games" << "statistics";
            [[fallthrough]];
        case SubscriptionTier::BASIC:
            features << "basic_patterns" << "basic_games" << "progress_tracking";
            [[fallthrough]];
        case SubscriptionTier::FREE:
            features << "basic_vacuum" << "safety_controls" << "emergency_stop";
            break;
    }

    // Add individually unlocked features
    features.append(m_license.unlockedFeatures);
    features.removeDuplicates();

    return features;
}

QStringList LicenseManager::lockedFeatures() const
{
    static const QStringList allFeatures = {
        "basic_vacuum", "safety_controls", "emergency_stop",
        "basic_patterns", "basic_games", "progress_tracking",
        "all_patterns", "all_games", "multi_user", "custom_games", "statistics",
        "tens_control", "intense_modes", "dom_features", "beta_access"
    };

    QStringList available = availableFeatures();
    QStringList locked;

    for (const QString& f : allFeatures) {
        if (!available.contains(f)) {
            locked.append(f);
        }
    }

    return locked;
}

QVector<PointBundle> LicenseManager::availablePointBundles() const
{
    return m_pointBundles;
}

bool LicenseManager::purchasePointBundle(const QString& bundleId)
{
    for (const auto& bundle : m_pointBundles) {
        if (bundle.bundleId == bundleId) {
            // Initiate purchase through server
            m_server->purchaseBundle(bundleId, m_license.licenseKey);
            return true;
        }
    }
    return false;
}

QVector<SubscriptionPlan> LicenseManager::availablePlans() const
{
    return m_subscriptionPlans;
}

bool LicenseManager::upgradePlan(const QString& planId)
{
    for (const auto& plan : m_subscriptionPlans) {
        if (plan.planId == planId) {
            m_server->upgradePlan(planId, m_license.licenseKey);
            return true;
        }
    }
    return false;
}

QString LicenseManager::deviceFingerprint() const
{
    return m_deviceId;
}

void LicenseManager::setOfflineMode(bool offline)
{
    if (m_offlineMode != offline) {
        m_offlineMode = offline;
        emit offlineModeChanged(offline);
    }
}

QString LicenseManager::formatPrice(double priceUsd)
{
    return QString("$%1").arg(priceUsd, 0, 'f', 2);
}

void LicenseManager::loadLicense()
{
    QByteArray data = m_settings->value(LICENSE_STORAGE_KEY).toByteArray();
    if (data.isEmpty()) {
        m_license.tier = SubscriptionTier::FREE;
        m_license.status = LicenseStatus::INVALID;
        return;
    }

    // Decrypt and parse stored license
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_license.tier = SubscriptionTier::FREE;
        m_license.status = LicenseStatus::INVALID;
        return;
    }

    QJsonObject obj = doc.object();
    m_license.licenseKey = obj["key"].toString();
    m_license.type = static_cast<LicenseType>(obj["type"].toInt());
    m_license.tier = static_cast<SubscriptionTier>(obj["tier"].toInt());
    m_license.email = obj["email"].toString();
    m_license.activatedAt = QDateTime::fromString(obj["activated"].toString(), Qt::ISODate);
    m_license.expiresAt = QDateTime::fromString(obj["expires"].toString(), Qt::ISODate);
    m_license.maxDevices = obj["maxDevices"].toInt(1);

    QJsonArray features = obj["features"].toArray();
    for (const auto& f : features) {
        m_license.unlockedFeatures.append(f.toString());
    }

    // Validate stored license
    if (m_license.expiresAt.isValid() && m_license.expiresAt < QDateTime::currentDateTime()) {
        m_license.status = LicenseStatus::EXPIRED;
    } else if (!m_license.licenseKey.isEmpty()) {
        m_license.status = LicenseStatus::VALID;
    } else {
        m_license.status = LicenseStatus::INVALID;
    }
}

void LicenseManager::saveLicense()
{
    QJsonObject obj;
    obj["key"] = m_license.licenseKey;
    obj["type"] = static_cast<int>(m_license.type);
    obj["tier"] = static_cast<int>(m_license.tier);
    obj["email"] = m_license.email;
    obj["activated"] = m_license.activatedAt.toString(Qt::ISODate);
    obj["expires"] = m_license.expiresAt.toString(Qt::ISODate);
    obj["maxDevices"] = m_license.maxDevices;

    QJsonArray features;
    for (const auto& f : m_license.unlockedFeatures) {
        features.append(f);
    }
    obj["features"] = features;

    QJsonDocument doc(obj);
    m_settings->setValue(LICENSE_STORAGE_KEY, doc.toJson(QJsonDocument::Compact));
}

QString LicenseManager::generateDeviceId()
{
    // Create device fingerprint from hardware info
    QString fingerprint;

    // Machine unique ID
    fingerprint += QSysInfo::machineUniqueId();

    // Network interfaces
    for (const auto& iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsLoopBack)) {
            fingerprint += iface.hardwareAddress();
        }
    }

    // Hash the fingerprint
    QByteArray hash = QCryptographicHash::hash(
        fingerprint.toUtf8(), QCryptographicHash::Sha256);

    return hash.toHex().left(32);
}

bool LicenseManager::validateKeyFormat(const QString& key) const
{
    // Format: XXXX-XXXX-XXXX-XXXX (16 chars with dashes)
    QRegularExpression re("^[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$");
    return re.match(key).hasMatch();
}

bool LicenseManager::validateKeyChecksum(const QString& key) const
{
    // Remove dashes and validate Luhn-like checksum
    QString clean = QString(key).remove('-');
    if (clean.length() != 16) return false;

    int sum = 0;
    for (int i = 0; i < 15; i++) {
        int val = clean[i].isDigit() ? clean[i].digitValue()
                                     : (clean[i].unicode() - 'A' + 10);
        sum += (i % 2 == 0) ? val : val * 2;
    }

    int checkDigit = (10 - (sum % 10)) % 10;
    int lastChar = clean[15].isDigit() ? clean[15].digitValue()
                                       : (clean[15].unicode() - 'A' + 10);

    return (lastChar % 10) == checkDigit;
}

QByteArray LicenseManager::signData(const QByteArray& data) const
{
    // Simplified signing for offline validation
    QByteArray key = "VContour_Secret_Key_2024";
    return QCryptographicHash::hash(data + key, QCryptographicHash::Sha256);
}

bool LicenseManager::verifySignature(const QByteArray& data, const QByteArray& signature) const
{
    return signData(data) == signature;
}
