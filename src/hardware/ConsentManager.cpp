#include "ConsentManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

ConsentManager::ConsentManager(QObject* parent)
    : QObject(parent)
    , m_retentionDays(30)
    , m_privacyOverride(false)
    , m_appVersion(QCoreApplication::applicationVersion())
{
    loadFromSettings();
}

ConsentManager::~ConsentManager()
{
    saveToSettings();
}

bool ConsentManager::hasConsent(ConsentType type) const
{
    if (m_privacyOverride) return false;
    
    // Check session consent first (temporary override)
    if (m_sessionConsents.contains(type)) {
        return m_sessionConsents[type];
    }
    
    return m_consents.contains(type) && m_consents[type].granted;
}

bool ConsentManager::hasAllCameraConsent() const
{
    return hasConsent(ConsentType::CAMERA_BODY) && hasConsent(ConsentType::CAMERA_CUP);
}

bool ConsentManager::hasAnyConsent() const
{
    for (auto it = m_consents.begin(); it != m_consents.end(); ++it) {
        if (it.value().granted) return true;
    }
    return false;
}

ConsentManager::ConsentRecord ConsentManager::getConsentRecord(ConsentType type) const
{
    return m_consents.value(type, ConsentRecord());
}

void ConsentManager::grantConsent(ConsentType type, const QString& description)
{
    ConsentRecord record;
    record.granted = true;
    record.timestamp = QDateTime::currentDateTime();
    record.version = m_appVersion;
    record.description = description.isEmpty() ? getConsentDescription(type) : description;
    
    m_consents[type] = record;
    saveToSettings();
    
    qDebug() << "Consent granted:" << consentTypeToString(type);
    emit consentChanged(type, true);
}

void ConsentManager::revokeConsent(ConsentType type)
{
    if (m_consents.contains(type)) {
        m_consents[type].granted = false;
        m_consents[type].timestamp = QDateTime::currentDateTime();
        saveToSettings();
        
        qDebug() << "Consent revoked:" << consentTypeToString(type);
        emit consentChanged(type, false);
    }
}

void ConsentManager::revokeAllConsent()
{
    for (auto it = m_consents.begin(); it != m_consents.end(); ++it) {
        it.value().granted = false;
        it.value().timestamp = QDateTime::currentDateTime();
    }
    m_sessionConsents.clear();
    saveToSettings();
    
    qDebug() << "All consent revoked";
    emit allConsentRevoked();
}

QString ConsentManager::getConsentDescription(ConsentType type) const
{
    switch (type) {
        case ConsentType::CAMERA_BODY:
            return tr("Allow the body camera to monitor your movements during sessions. "
                     "Video is processed locally and not stored unless session recording is enabled.");
        case ConsentType::CAMERA_CUP:
            return tr("Allow the cup-area camera to detect physiological responses. "
                     "This enables orgasm detection features. Video is processed locally.");
        case ConsentType::MOTION_DATA:
            return tr("Allow collection of motion sensor data for stillness detection and game mechanics.");
        case ConsentType::SESSION_RECORDING:
            return tr("Allow recording of sessions for later playback. "
                     "Recordings are stored locally and encrypted.");
        case ConsentType::ANALYTICS:
            return tr("Allow anonymous usage analytics to help improve the application. "
                     "No personal or identifying information is collected.");
        case ConsentType::ORGASM_DETECTION:
            return tr("Allow AI-based detection of orgasm indicators using camera and sensor data. "
                     "This enables automatic game responses to detected orgasms.");
    }
    return QString();
}

QString ConsentManager::getConsentTitle(ConsentType type) const
{
    switch (type) {
        case ConsentType::CAMERA_BODY: return tr("Body Camera");
        case ConsentType::CAMERA_CUP: return tr("Cup Area Camera");
        case ConsentType::MOTION_DATA: return tr("Motion Data");
        case ConsentType::SESSION_RECORDING: return tr("Session Recording");
        case ConsentType::ANALYTICS: return tr("Usage Analytics");
        case ConsentType::ORGASM_DETECTION: return tr("Orgasm Detection");
    }
    return QString();
}

QStringList ConsentManager::getPendingConsentTypes() const
{
    QStringList pending;
    QList<ConsentType> allTypes = {
        ConsentType::CAMERA_BODY, ConsentType::CAMERA_CUP,
        ConsentType::MOTION_DATA, ConsentType::SESSION_RECORDING,
        ConsentType::ANALYTICS, ConsentType::ORGASM_DETECTION
    };
    
    for (ConsentType type : allTypes) {
        if (!m_consents.contains(type) || !m_consents[type].granted) {
            pending << consentTypeToString(type);
        }
    }
    return pending;
}

void ConsentManager::setDataRetentionDays(int days)
{
    m_retentionDays = qMax(1, days);
    saveToSettings();
}

void ConsentManager::deleteAllStoredData()
{
    // Delete session recordings
    QDir dataDir(QCoreApplication::applicationDirPath() + "/data");
    if (dataDir.exists()) {
        dataDir.removeRecursively();
    }

    qDebug() << "All stored data deleted";
    emit dataDeleted();
}

// Session-specific consent
void ConsentManager::grantSessionConsent(ConsentType type)
{
    m_sessionConsents[type] = true;
    emit consentChanged(type, true);
}

void ConsentManager::revokeSessionConsent(ConsentType type)
{
    m_sessionConsents[type] = false;
    emit consentChanged(type, false);
}

bool ConsentManager::hasSessionConsent(ConsentType type) const
{
    return m_sessionConsents.value(type, false);
}

// Privacy mode override
void ConsentManager::setPrivacyModeOverride(bool enabled)
{
    m_privacyOverride = enabled;
    if (enabled) {
        qDebug() << "Privacy mode enabled - all recording disabled";
    }
}

// Persistence
void ConsentManager::loadFromSettings()
{
    QSettings settings;
    settings.beginGroup("Consent");

    m_retentionDays = settings.value("retentionDays", 30).toInt();

    QStringList types = settings.childGroups();
    for (const QString& typeStr : types) {
        settings.beginGroup(typeStr);

        ConsentType type = stringToConsentType(typeStr);
        ConsentRecord record;
        record.granted = settings.value("granted", false).toBool();
        record.timestamp = settings.value("timestamp").toDateTime();
        record.version = settings.value("version").toString();
        record.description = settings.value("description").toString();

        m_consents[type] = record;
        settings.endGroup();
    }

    settings.endGroup();
}

void ConsentManager::saveToSettings()
{
    QSettings settings;
    settings.beginGroup("Consent");

    settings.setValue("retentionDays", m_retentionDays);

    for (auto it = m_consents.begin(); it != m_consents.end(); ++it) {
        QString typeStr = consentTypeToString(it.key());
        settings.beginGroup(typeStr);

        settings.setValue("granted", it.value().granted);
        settings.setValue("timestamp", it.value().timestamp);
        settings.setValue("version", it.value().version);
        settings.setValue("description", it.value().description);

        settings.endGroup();
    }

    settings.endGroup();
}

QString ConsentManager::consentTypeToString(ConsentType type) const
{
    switch (type) {
        case ConsentType::CAMERA_BODY: return "camera_body";
        case ConsentType::CAMERA_CUP: return "camera_cup";
        case ConsentType::MOTION_DATA: return "motion_data";
        case ConsentType::SESSION_RECORDING: return "session_recording";
        case ConsentType::ANALYTICS: return "analytics";
        case ConsentType::ORGASM_DETECTION: return "orgasm_detection";
    }
    return "unknown";
}

ConsentManager::ConsentType ConsentManager::stringToConsentType(const QString& str) const
{
    if (str == "camera_body") return ConsentType::CAMERA_BODY;
    if (str == "camera_cup") return ConsentType::CAMERA_CUP;
    if (str == "motion_data") return ConsentType::MOTION_DATA;
    if (str == "session_recording") return ConsentType::SESSION_RECORDING;
    if (str == "analytics") return ConsentType::ANALYTICS;
    if (str == "orgasm_detection") return ConsentType::ORGASM_DETECTION;
    return ConsentType::MOTION_DATA;  // Default
}

