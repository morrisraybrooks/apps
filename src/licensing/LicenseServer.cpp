#include "LicenseServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QDebug>

const char* LicenseServer::DEFAULT_SERVER_URL = "https://api.vcontour.com/v1/license";

LicenseServer::LicenseServer(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serverUrl(DEFAULT_SERVER_URL)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &LicenseServer::onReplyFinished);
}

LicenseServer::~LicenseServer() = default;

void LicenseServer::setServerUrl(const QUrl& url)
{
    m_serverUrl = url;
}

void LicenseServer::activateLicense(const QString& licenseKey, const QString& deviceId)
{
    QJsonObject data;
    data["action"] = "activate";
    data["license_key"] = licenseKey;
    data["device_id"] = deviceId;
    data["platform"] = QSysInfo::productType();
    data["app_version"] = "1.0.0";

    m_pendingEndpoint = "activate";
    sendRequest("/activate", data);
}

void LicenseServer::deactivateLicense(const QString& licenseKey, const QString& deviceId)
{
    QJsonObject data;
    data["action"] = "deactivate";
    data["license_key"] = licenseKey;
    data["device_id"] = deviceId;

    m_pendingEndpoint = "deactivate";
    sendRequest("/deactivate", data);
}

void LicenseServer::validateLicense(const QString& licenseKey, const QString& deviceId)
{
    QJsonObject data;
    data["action"] = "validate";
    data["license_key"] = licenseKey;
    data["device_id"] = deviceId;

    m_pendingEndpoint = "validate";
    sendRequest("/validate", data);
}

void LicenseServer::purchaseBundle(const QString& bundleId, const QString& licenseKey)
{
    QJsonObject data;
    data["action"] = "purchase";
    data["product_type"] = "point_bundle";
    data["product_id"] = bundleId;
    data["license_key"] = licenseKey;

    m_pendingEndpoint = "purchase";
    sendRequest("/purchase", data);
}

void LicenseServer::upgradePlan(const QString& planId, const QString& licenseKey)
{
    QJsonObject data;
    data["action"] = "upgrade";
    data["plan_id"] = planId;
    data["license_key"] = licenseKey;

    m_pendingEndpoint = "upgrade";
    sendRequest("/upgrade", data);
}

void LicenseServer::verifyReceipt(const PurchaseReceipt& receipt)
{
    QJsonObject data;
    data["action"] = "verify_receipt";
    data["receipt_id"] = receipt.receiptId;
    data["product_id"] = receipt.productId;
    data["platform"] = receipt.platform;
    data["transaction_id"] = receipt.transactionId;
    data["amount"] = receipt.amount;
    data["currency"] = receipt.currency;
    data["signature"] = QString::fromUtf8(receipt.signature.toBase64());

    m_pendingEndpoint = "verify";
    sendRequest("/verify-receipt", data);
}

void LicenseServer::createAccount(const QString& email, const QString& password)
{
    QJsonObject data;
    data["action"] = "register";
    data["email"] = email;
    data["password"] = password;

    m_pendingEndpoint = "register";
    sendRequest("/account/register", data);
}

void LicenseServer::login(const QString& email, const QString& password)
{
    QJsonObject data;
    data["action"] = "login";
    data["email"] = email;
    data["password"] = password;

    m_pendingEndpoint = "login";
    sendRequest("/account/login", data);
}

void LicenseServer::requestTrialKey(const QString& email, const QString& deviceId)
{
    QJsonObject data;
    data["action"] = "trial";
    data["email"] = email;
    data["device_id"] = deviceId;

    m_pendingEndpoint = "trial";
    sendRequest("/trial", data);
}

void LicenseServer::sendRequest(const QString& endpoint, const QJsonObject& data)
{
    QUrl url = m_serverUrl;
    url.setPath(url.path() + endpoint);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "VContour/1.0");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_authToken).toUtf8());
    }

    // Enable SSL
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    request.setSslConfiguration(sslConfig);

    QJsonDocument doc(data);
    m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));

    qDebug() << "LicenseServer: Sending request to" << url.toString();
}

void LicenseServer::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        emit error(m_lastError);
        qWarning() << "LicenseServer: Request failed:" << m_lastError;
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (!doc.isObject()) {
        m_lastError = "Invalid server response";
        emit error(m_lastError);
        return;
    }

    QJsonObject response = doc.object();

    if (!response["success"].toBool()) {
        m_lastError = response["error"].toString("Unknown error");
        emit error(m_lastError);
        return;
    }

    // Route to appropriate handler
    if (m_pendingEndpoint == "activate" || m_pendingEndpoint == "deactivate") {
        handleActivateResponse(response);
    } else if (m_pendingEndpoint == "validate") {
        handleValidateResponse(response);
    } else if (m_pendingEndpoint == "purchase" || m_pendingEndpoint == "upgrade" ||
               m_pendingEndpoint == "verify") {
        handlePurchaseResponse(response);
    } else if (m_pendingEndpoint == "register" || m_pendingEndpoint == "login") {
        handleAccountResponse(response);
    } else if (m_pendingEndpoint == "trial") {
        handleTrialResponse(response);
    }
}

void LicenseServer::handleActivateResponse(const QJsonObject& response)
{
    QJsonObject licenseData = response["license"].toObject();

    m_lastLicenseInfo.licenseKey = licenseData["key"].toString();
    m_lastLicenseInfo.type = static_cast<LicenseType>(licenseData["type"].toInt());
    m_lastLicenseInfo.tier = static_cast<SubscriptionTier>(licenseData["tier"].toInt());
    m_lastLicenseInfo.status = LicenseStatus::VALID;
    m_lastLicenseInfo.email = licenseData["email"].toString();
    m_lastLicenseInfo.activatedAt = QDateTime::fromString(
        licenseData["activated_at"].toString(), Qt::ISODate);
    m_lastLicenseInfo.expiresAt = QDateTime::fromString(
        licenseData["expires_at"].toString(), Qt::ISODate);
    m_lastLicenseInfo.maxDevices = licenseData["max_devices"].toInt(1);
    m_lastLicenseInfo.currentDevices = licenseData["current_devices"].toInt(1);

    QJsonArray features = licenseData["features"].toArray();
    m_lastLicenseInfo.unlockedFeatures.clear();
    for (const auto& f : features) {
        m_lastLicenseInfo.unlockedFeatures.append(f.toString());
    }

    emit activationComplete(true);
    emit validationComplete();
}

void LicenseServer::handleValidateResponse(const QJsonObject& response)
{
    QString statusStr = response["status"].toString();

    if (statusStr == "valid") {
        m_lastLicenseInfo.status = LicenseStatus::VALID;
    } else if (statusStr == "expired") {
        m_lastLicenseInfo.status = LicenseStatus::EXPIRED;
    } else if (statusStr == "revoked") {
        m_lastLicenseInfo.status = LicenseStatus::REVOKED;
    } else if (statusStr == "exceeded") {
        m_lastLicenseInfo.status = LicenseStatus::EXCEEDED;
    } else {
        m_lastLicenseInfo.status = LicenseStatus::INVALID;
    }

    // Update tier if provided
    if (response.contains("tier")) {
        m_lastLicenseInfo.tier = static_cast<SubscriptionTier>(response["tier"].toInt());
    }

    // Update expiration if provided
    if (response.contains("expires_at")) {
        m_lastLicenseInfo.expiresAt = QDateTime::fromString(
            response["expires_at"].toString(), Qt::ISODate);
    }

    emit validationComplete();
}

void LicenseServer::handlePurchaseResponse(const QJsonObject& response)
{
    QString productId = response["product_id"].toString();
    int pointsAwarded = response["points_awarded"].toInt(0);

    // Update bonus points in license info
    m_lastLicenseInfo.bonusPoints += pointsAwarded;

    // Check if tier was upgraded
    if (response.contains("new_tier")) {
        m_lastLicenseInfo.tier = static_cast<SubscriptionTier>(response["new_tier"].toInt());
    }

    // Update expiration if subscription
    if (response.contains("new_expires_at")) {
        m_lastLicenseInfo.expiresAt = QDateTime::fromString(
            response["new_expires_at"].toString(), Qt::ISODate);
    }

    emit purchaseComplete(productId, pointsAwarded);
}

void LicenseServer::handleAccountResponse(const QJsonObject& response)
{
    if (m_pendingEndpoint == "register") {
        QString email = response["email"].toString();
        emit accountCreated(email);
    } else if (m_pendingEndpoint == "login") {
        m_authToken = response["token"].toString();
        bool success = !m_authToken.isEmpty();
        emit loginComplete(success, m_authToken);
    }
}

void LicenseServer::handleTrialResponse(const QJsonObject& response)
{
    QString trialKey = response["trial_key"].toString();

    if (!trialKey.isEmpty()) {
        m_lastLicenseInfo.licenseKey = trialKey;
        m_lastLicenseInfo.type = LicenseType::TRIAL;
        m_lastLicenseInfo.tier = SubscriptionTier::PREMIUM;  // Full access during trial
        m_lastLicenseInfo.status = LicenseStatus::VALID;
        m_lastLicenseInfo.expiresAt = QDateTime::currentDateTime().addDays(7);

        emit trialKeyGenerated(trialKey);
        emit validationComplete();
    }
}
