#ifndef CONSENTMANAGER_H
#define CONSENTMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QSettings>
#include <QString>
#include <QMap>

/**
 * @brief Manages user consent for camera recording and data collection
 *
 * Provides explicit consent management for:
 * - Camera recording (body and cup cameras)
 * - Motion data collection
 * - Session recording/playback
 * - Data retention policies
 *
 * All consent is opt-in and can be revoked at any time.
 * Consent state is persisted across sessions.
 */
class ConsentManager : public QObject
{
    Q_OBJECT

public:
    // Types of consent that can be granted
    enum class ConsentType {
        CAMERA_BODY,           // Body camera recording
        CAMERA_CUP,            // Cup/genital area camera recording
        MOTION_DATA,           // Motion sensor data collection
        SESSION_RECORDING,     // Full session recording for playback
        ANALYTICS,             // Anonymous usage analytics
        ORGASM_DETECTION       // AI-based orgasm detection
    };
    Q_ENUM(ConsentType)

    // Consent record with timestamp
    struct ConsentRecord {
        bool granted = false;
        QDateTime timestamp;
        QString version;       // App version when consent was given
        QString description;   // What was consented to
    };

    explicit ConsentManager(QObject* parent = nullptr);
    ~ConsentManager();

    // Check consent status
    bool hasConsent(ConsentType type) const;
    bool hasAllCameraConsent() const;
    bool hasAnyConsent() const;
    ConsentRecord getConsentRecord(ConsentType type) const;

    // Grant/revoke consent
    void grantConsent(ConsentType type, const QString& description = QString());
    void revokeConsent(ConsentType type);
    void revokeAllConsent();

    // Consent dialog helpers
    QString getConsentDescription(ConsentType type) const;
    QString getConsentTitle(ConsentType type) const;
    QStringList getPendingConsentTypes() const;

    // Data retention
    void setDataRetentionDays(int days);
    int getDataRetentionDays() const { return m_retentionDays; }
    void deleteAllStoredData();

    // Session-specific consent (temporary, not persisted)
    void grantSessionConsent(ConsentType type);
    void revokeSessionConsent(ConsentType type);
    bool hasSessionConsent(ConsentType type) const;

    // Persistence
    void loadFromSettings();
    void saveToSettings();

    // Privacy mode (disables all recording regardless of consent)
    void setPrivacyModeOverride(bool enabled);
    bool isPrivacyModeOverride() const { return m_privacyOverride; }

Q_SIGNALS:
    void consentChanged(ConsentType type, bool granted);
    void allConsentRevoked();
    void consentRequired(ConsentType type);
    void dataDeleted();

private:
    QString consentTypeToString(ConsentType type) const;
    ConsentType stringToConsentType(const QString& str) const;

    QMap<ConsentType, ConsentRecord> m_consents;
    QMap<ConsentType, bool> m_sessionConsents;
    int m_retentionDays;
    bool m_privacyOverride;
    QString m_appVersion;
};

#endif // CONSENTMANAGER_H

