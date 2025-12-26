#ifndef ACHIEVEMENTSYSTEM_H
#define ACHIEVEMENTSYSTEM_H

#include "GameTypes.h"
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QDateTime>
#include <QJsonObject>

/**
 * @brief Achievement definition
 */
struct Achievement {
    QString id;
    QString name;
    QString description;
    AchievementCategory category;
    int xpBonus = 0;
    bool isSecret = false;
    QString iconPath;
    
    // Condition for unlocking
    QString conditionType;      // "edges", "wins", "games", "streak", "fluid", etc.
    int conditionValue = 0;     // Target value to achieve
    QString conditionGameId;    // Optional: specific game requirement
    
    static Achievement fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief Unlocked achievement record
 */
struct UnlockedAchievement {
    QString achievementId;
    QDateTime unlockedAt;
    int xpAwarded;
};

// Forward declaration
class ProgressTracker;

/**
 * @brief Achievement tracking and milestone detection system
 * 
 * Manages achievement definitions, tracks progress toward milestones,
 * and handles content unlocking based on achievements.
 */
class AchievementSystem : public QObject
{
    Q_OBJECT

public:
    explicit AchievementSystem(QObject* parent = nullptr);
    ~AchievementSystem() = default;

    // Initialization
    bool loadAchievements(const QString& filePath);
    bool loadBuiltInAchievements();
    void setProgressTracker(ProgressTracker* tracker);

    // Achievement queries
    QVector<Achievement> allAchievements() const;
    QVector<Achievement> achievementsByCategory(AchievementCategory category) const;
    Achievement getAchievement(const QString& id) const;
    bool hasAchievement(const QString& id) const;
    
    // Unlocked achievements
    QVector<UnlockedAchievement> unlockedAchievements() const;
    bool isUnlocked(const QString& achievementId) const;
    int totalXpFromAchievements() const;
    
    // Progress tracking
    double progressToward(const QString& achievementId) const;
    QVector<QPair<QString, double>> nearestAchievements(int count = 3) const;
    
    // Check triggers
    void checkGameCompletion(GameResult result, const QString& gameId);
    void checkMilestones();
    void checkSpecificAchievement(const QString& achievementId);
    
    // Unlock management
    void unlockAchievement(const QString& achievementId);
    void resetAchievements();  // For testing

Q_SIGNALS:
    void achievementUnlocked(const Achievement& achievement, int xpBonus);
    void progressUpdated(const QString& achievementId, double progress);
    void milestoneReached(const QString& milestoneName, int value);

private:
    void registerBuiltInAchievements();
    bool evaluateCondition(const Achievement& achievement) const;
    double calculateProgress(const Achievement& achievement) const;

    /**
     * @brief Get current value for an achievement condition type
     *
     * Consolidates the condition type lookup logic used by both
     * evaluateCondition and calculateProgress.
     *
     * @param conditionType The condition type string (e.g., "wins", "total_edges")
     * @return Current value for the specified condition type
     */
    int getConditionValue(const QString& conditionType) const;
    
    // Definitions
    QMap<QString, Achievement> m_achievements;
    
    // Unlocked
    QMap<QString, UnlockedAchievement> m_unlocked;
    
    // Progress tracker reference
    ProgressTracker* m_progressTracker;
    
    // Cache of career stats
    int m_totalGames = 0;
    int m_totalWins = 0;
    int m_currentStreak = 0;
    int m_bestStreak = 0;
    int m_totalEdges = 0;
    int m_totalOrgasms = 0;
    double m_totalFluidMl = 0.0;
};

#endif // ACHIEVEMENTSYSTEM_H

