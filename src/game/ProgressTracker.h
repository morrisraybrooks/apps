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

Q_SIGNALS:
    void profileUpdated(const UserProfile& profile);
    void levelUp(int newLevel, int xpBonus);
    void xpGained(int amount, int total);
    void statsUpdated(const CareerStats& stats);
    void contentUnlocked(const QString& contentId, const QString& contentType);
    void streakUpdated(int currentStreak, int bestStreak);

private:
    bool createTables();
    bool loadProfile();
    bool loadStats();
    bool loadUnlocks();
    bool saveProfile();
    bool saveStats();
    
    QSqlDatabase m_db;
    bool m_initialized;
    UserProfile m_profile;
    CareerStats m_stats;
    QVector<UnlockedContent> m_unlocks;
    
    static const int XP_BASE = 100;
    static const double XP_MULTIPLIER;
};

#endif // PROGRESSTRACKER_H

