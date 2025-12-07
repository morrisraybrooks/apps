#ifndef LICENSESERVER_H
#define LICENSESERVER_H

#include "LicenseManager.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QUrl>

/**
 * @brief Purchase receipt for verification
 */
struct PurchaseReceipt {
    QString receiptId;
    QString productId;
    QString platform;  // "stripe", "paypal", "apple", "google"
    QString transactionId;
    double amount;
    QString currency;
    QDateTime purchaseDate;
    QByteArray signature;
};

/**
 * @brief License Server API client
 * 
 * Handles communication with the license validation server for:
 * - License activation and deactivation
 * - License validation
 * - Purchase verification
 * - Subscription management
 */
class LicenseServer : public QObject
{
    Q_OBJECT

public:
    explicit LicenseServer(QObject* parent = nullptr);
    ~LicenseServer();

    // License operations
    void activateLicense(const QString& licenseKey, const QString& deviceId);
    void deactivateLicense(const QString& licenseKey, const QString& deviceId);
    void validateLicense(const QString& licenseKey, const QString& deviceId);

    // Purchase operations
    void purchaseBundle(const QString& bundleId, const QString& licenseKey);
    void upgradePlan(const QString& planId, const QString& licenseKey);
    void verifyReceipt(const PurchaseReceipt& receipt);

    // Account operations
    void createAccount(const QString& email, const QString& password);
    void login(const QString& email, const QString& password);
    void requestTrialKey(const QString& email, const QString& deviceId);

    // Results
    LicenseInfo lastLicenseInfo() const { return m_lastLicenseInfo; }
    QString lastError() const { return m_lastError; }

    // Server configuration
    void setServerUrl(const QUrl& url);
    QUrl serverUrl() const { return m_serverUrl; }

Q_SIGNALS:
    void validationComplete();
    void activationComplete(bool success);
    void purchaseComplete(const QString& productId, int pointsAwarded);
    void accountCreated(const QString& email);
    void loginComplete(bool success, const QString& token);
    void trialKeyGenerated(const QString& trialKey);
    void error(const QString& message);

private Q_SLOTS:
    void onReplyFinished(QNetworkReply* reply);

private:
    void sendRequest(const QString& endpoint, const QJsonObject& data);
    void handleActivateResponse(const QJsonObject& response);
    void handleValidateResponse(const QJsonObject& response);
    void handlePurchaseResponse(const QJsonObject& response);
    void handleAccountResponse(const QJsonObject& response);
    void handleTrialResponse(const QJsonObject& response);

    QNetworkAccessManager* m_networkManager;
    QUrl m_serverUrl;
    QString m_authToken;
    LicenseInfo m_lastLicenseInfo;
    QString m_lastError;
    QString m_pendingEndpoint;

    static const char* DEFAULT_SERVER_URL;
    static const int REQUEST_TIMEOUT_MS = 30000;
};

#endif // LICENSESERVER_H

