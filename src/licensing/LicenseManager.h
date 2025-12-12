#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include "../game/GameTypes.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QSettings>
#include <QCryptographicHash>
#include <QTimer>

/**
 * @brief License information structure
 */
struct LicenseInfo {
    QString licenseKey;
    LicenseType type = LicenseType::TRIAL;
    LicenseStatus status = LicenseStatus::PENDING;
    SubscriptionTier tier = SubscriptionTier::FREE;
    QDateTime activatedAt;
    QDateTime expiresAt;
    QString email;
    QString deviceId;
    int maxDevices = 1;
    int currentDevices = 0;
    QStringList unlockedFeatures;
    int bonusPoints = 0;
    
    bool isValid() const {
        return status == LicenseStatus::VALID && 
               (expiresAt.isNull() || expiresAt > QDateTime::currentDateTime());
    }
    
    int daysRemaining() const {
        if (expiresAt.isNull()) return -1;  // Lifetime
        return QDateTime::currentDateTime().daysTo(expiresAt);
    }
};

/**
 * @brief Point bundle purchase options
 */
struct PointBundle {
    QString bundleId;
    QString name;
    int points;
    double priceUsd;
    double bonusPercent;  // Extra points percentage
    
    int totalPoints() const {
        return points + static_cast<int>(points * bonusPercent / 100.0);
    }
};

/**
 * @brief Subscription plan details
 */
struct SubscriptionPlan {
    QString planId;
    QString name;
    QString description;
    SubscriptionTier tier;
    LicenseType licenseType;
    double priceUsd;
    int durationDays;
    QStringList features;
    int monthlyPoints;  // Bonus points per month
};

/**
 * @brief License Manager for monetization and feature gating
 * 
 * Handles:
 * - License key validation (online and offline)
 * - Subscription management
 * - Feature unlocking
 * - Point bundle purchases
 * - Device fingerprinting
 */
class LicenseManager : public QObject
{
    Q_OBJECT

public:
    explicit LicenseManager(QObject* parent = nullptr);
    ~LicenseManager();

    // License activation
    bool activateLicense(const QString& licenseKey);
    bool deactivateLicense();
    bool validateLicense();
    
    // License status
    LicenseInfo licenseInfo() const { return m_license; }
    LicenseStatus status() const { return m_license.status; }
    SubscriptionTier subscriptionTier() const { return m_license.tier; }
    bool isLicensed() const { return m_license.isValid(); }
    bool isPremium() const { return m_license.tier >= SubscriptionTier::PREMIUM; }
    
    // Feature access
    bool hasFeature(const QString& featureId) const;
    QStringList availableFeatures() const;
    QStringList lockedFeatures() const;
    
    // Point bundles
    QVector<PointBundle> availablePointBundles() const;
    bool purchasePointBundle(const QString& bundleId);
    
    // Subscription plans
    QVector<SubscriptionPlan> availablePlans() const;
    bool upgradePlan(const QString& planId);
    
    // Device management
    QString deviceFingerprint() const;
    int deviceCount() const { return m_license.currentDevices; }
    int maxDevices() const { return m_license.maxDevices; }
    
    // Offline mode
    bool isOfflineMode() const { return m_offlineMode; }
    void setOfflineMode(bool offline);
    int offlineGraceDays() const { return OFFLINE_GRACE_DAYS; }

    // Pricing (for display)
    static QString formatPrice(double priceUsd);

Q_SIGNALS:
    void licenseActivated(const LicenseInfo& info);
    void licenseDeactivated();
    void licenseExpiring(int daysRemaining);
    void licenseExpired();
    void licenseValidated(LicenseStatus status);
    void featureUnlocked(const QString& featureId);
    void pointsPurchased(int points, const QString& bundleId);
    void subscriptionUpgraded(SubscriptionTier newTier);
    void validationError(const QString& error);
    void offlineModeChanged(bool offline);

private Q_SLOTS:
    void onValidationComplete();
    void checkExpiration();

private:
    void loadLicense();
    void saveLicense();
    QString generateDeviceId();
    bool validateKeyFormat(const QString& key) const;
    bool validateKeyChecksum(const QString& key) const;
    QByteArray signData(const QByteArray& data) const;
    bool verifySignature(const QByteArray& data, const QByteArray& signature) const;
    void initializePointBundles();
    void initializeSubscriptionPlans();

    LicenseInfo m_license;
    QVector<PointBundle> m_pointBundles;
    QVector<SubscriptionPlan> m_subscriptionPlans;
    QSettings* m_settings;
    QTimer* m_expirationTimer;
    bool m_offlineMode;
    QString m_deviceId;

    // Server communication handled by LicenseServer class
    class LicenseServer* m_server;

    static const int OFFLINE_GRACE_DAYS = 7;
    static const char* LICENSE_STORAGE_KEY;
    static const char* DEVICE_ID_KEY;
};

#endif // LICENSEMANAGER_H

