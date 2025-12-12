#ifndef PROGRESSTRACKER_H
#define PROGRESSTRACKER_H

#include "GameTypes.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QSqlDatabase>

/**
 * @brief User profile data
 */
struct UserProfile {
    QString id;
    QString displayName;
    int level = 1;
    int currentXp = 0;
    int totalXp = 0;
    SubscriptionTier tier = SubscriptionTier::BASIC;
    QDateTime createdAt;
    QDateTime lastPlayedAt;

    // Points economy fields
    int pointsBalance = 0;
    PrivilegeTier privilegeTier = PrivilegeTier::BEGINNER;
    QString safeWord;             // User's chosen safe word
};

/**
 * @brief Point transaction record for audit trail
 */
struct PointTransaction {
    qint64 id;
    QString userId;
    PointTransactionType type;
    int amount;                    // Positive for earnings, negative for spending
    int balanceAfter;
    QString description;
    QString relatedUserId;         // For transfers or commands to others
    QString relatedGameId;         // For game completion
    QDateTime timestamp;
};

/**
 * @brief Paired user relationship
 */
struct PairedUser {
    QString id;
    QString partnerId;
    QString partnerDisplayName;
    ConsentStatus consentStatus;
    QDateTime pairedAt;
    QDateTime consentExpiresAt;
    bool canControl;               // Can this user control partner?
    bool canBeControlled;          // Can partner control this user?
};

/**
 * @brief Career statistics
 */
struct CareerStats {
    int totalGames = 0;
    int totalWins = 0;
    int totalLosses = 0;
    int currentWinStreak = 0;
    int bestWinStreak = 0;
    int totalEdges = 0;
    int totalOrgasms = 0;
    double totalFluidMl = 0.0;
    int totalPlayTimeSeconds = 0;
    double highestArousal = 0.0;
    int longestDenialSeconds = 0;
};

/**
 * @brief Game session record
 */
struct GameSession {
    qint64 id;
    QString gameId;
    GameType gameType;
    GameResult result;
    int score;
    int durationSeconds;
    int edgesAchieved;
    int orgasmsDetected;
    double maxArousal;
    double avgArousal;
    double fluidProducedMl;
    int xpEarned;
    QDateTime playedAt;
};

/**
 * @brief Unlocked content record
 */
struct UnlockedContent {
    QString contentId;
    QString contentType;  // "pattern", "game", "achievement"
    QDateTime unlockedAt;
};

/**
 * @brief Progress tracker with SQLite persistence
 * 
 * Manages user profiles, career statistics, game history,
 * and content unlocks with local SQLite database storage.
 */
class ProgressTracker : public QObject
{
    Q_OBJECT

public:
    explicit ProgressTracker(QObject* parent = nullptr);
    ~ProgressTracker();

    // Database
    bool initialize(const QString& dbPath = "vcontour_progress.db");
    bool isInitialized() const { return m_initialized; }
    void close();

    // User profile
    UserProfile currentProfile() const { return m_profile; }
    const UserProfile& profile() const { return m_profile; }  // Alias for currentProfile
    void setDisplayName(const QString& name);
    void setSubscriptionTier(SubscriptionTier tier);
    
    // XP and leveling
    void addXp(int amount);
    int xpToNextLevel() const;
    int xpForLevel(int level) const;
    double levelProgress() const;
    
    // Career stats
    CareerStats careerStats() const { return m_stats; }
    void updateCareerStats(const CareerStats& stats);
    
    // Game sessions
    void recordGameSession(const QString& gameId, GameType type, GameResult result,
                           int score, int durationSeconds, int edges, int orgasms,
                           double maxArousal, double avgArousal, double fluidMl);
    QVector<GameSession> recentSessions(int count = 10) const;
    QVector<GameSession> sessionsByGame(const QString& gameId, int count = 10) const;
    int bestScoreForGame(const QString& gameId) const;
    
    // Unlocks
    void unlockContent(const QString& contentId, const QString& contentType);
    bool isContentUnlocked(const QString& contentId) const;
    QVector<UnlockedContent> allUnlockedContent() const;
    QVector<QString> unlockedPatterns() const;
    QVector<QString> unlockedGames() const;
    
    // Streaks
    void recordWin();
    void recordLoss();
    void resetStreak();

    // =========================================================================
    // Points Economy
    // =========================================================================

    // Points balance
    int pointsBalance() const { return m_profile.pointsBalance; }
    PrivilegeTier privilegeTier() const { return m_profile.privilegeTier; }
    static PrivilegeTier tierForPoints(int points);
    static int pointsForTier(PrivilegeTier tier);
    static QString tierName(PrivilegeTier tier);

    // Point transactions
    bool addPoints(int amount, PointTransactionType type,
                   const QString& description = QString(),
                   const QString& relatedUserId = QString(),
                   const QString& relatedGameId = QString());
    bool spendPoints(int amount, PointTransactionType type,
                     const QString& description = QString(),
                     const QString& relatedUserId = QString());
    bool transferPoints(const QString& recipientId, int amount);
    bool canAfford(int amount) const { return m_profile.pointsBalance >= amount; }

    // Transaction history
    QVector<PointTransaction> recentTransactions(int count = 20) const;
    QVector<PointTransaction> transactionsByType(PointTransactionType type, int count = 20) const;
    int totalEarned() const;
    int totalSpent() const;

    // =========================================================================
    // Paired Users / Consent Management
    // =========================================================================

    // Pairing
    QVector<PairedUser> pairedUsers() const { return m_pairedUsers; }
    bool addPairedUser(const QString& partnerId, const QString& partnerName);
    bool removePairedUser(const QString& partnerId);
    bool isPaired(const QString& partnerId) const;
    PairedUser* getPairedUser(const QString& partnerId);

    // Consent
    bool grantConsent(const QString& partnerId, int expirationMinutes = 60);
    bool revokeConsent(const QString& partnerId);
    bool hasValidConsent(const QString& partnerId) const;
    ConsentStatus consentStatus(const QString& partnerId) const;

    // Safe word
    void setSafeWord(const QString& safeWord);
    QString safeWord() const { return m_profile.safeWord; }
    bool verifySafeWord(const QString& word) const;

    // =========================================================================
    // Audit Logging
    // =========================================================================

    void logCommand(const QString& commandType, const QString& targetUserId,
                    int pointCost, bool success, const QString& details = QString());
    QVector<PointTransaction> commandAuditLog(int count = 50) const;

Q_SIGNALS:
    void profileUpdated(const UserProfile& profile);
    void levelUp(int newLevel, int xpBonus);
    void xpGained(int amount, int total);
    void statsUpdated(const CareerStats& stats);
    void contentUnlocked(const QString& contentId, const QString& contentType);
    void streakUpdated(int currentStreak, int bestStreak);

    // Points signals
    void pointsChanged(int newBalance, int change);
    void privilegeTierChanged(PrivilegeTier newTier);
    void transactionRecorded(const PointTransaction& transaction);

    // Pairing signals
    void pairingAdded(const PairedUser& pairing);
    void pairingRemoved(const QString& partnerId);
    void consentChanged(const QString& partnerId, ConsentStatus status);

private:
    bool createTables();
    bool createPointsTables();
    bool createPairingTables();
    bool loadProfile();
    bool loadStats();
    bool loadUnlocks();
    bool loadPairings();
    bool saveProfile();
    bool saveStats();
    void updatePrivilegeTier();

    QSqlDatabase m_db;
    bool m_initialized;
    UserProfile m_profile;
    CareerStats m_stats;
    QVector<UnlockedContent> m_unlocks;
    QVector<PairedUser> m_pairedUsers;

    static const int XP_BASE = 100;
    static const double XP_MULTIPLIER;

    // Privilege tier thresholds
    static const int TIER_INTERMEDIATE_POINTS = 1000;
    static const int TIER_ADVANCED_POINTS = 5000;
    static const int TIER_DOM_MASTER_POINTS = 15000;
};

#endif // PROGRESSTRACKER_H

