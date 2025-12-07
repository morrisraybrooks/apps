#include "AchievementSystem.h"
#include "ProgressTracker.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

// ============================================================================
// Achievement struct
// ============================================================================

Achievement Achievement::fromJson(const QJsonObject& json)
{
    Achievement a;
    a.id = json["id"].toString();
    a.name = json["name"].toString();
    a.description = json["description"].toString();
    a.xpBonus = json["xp_bonus"].toInt();
    a.isSecret = json["secret"].toBool(false);
    a.iconPath = json["icon"].toString();
    a.conditionType = json["condition_type"].toString();
    a.conditionValue = json["condition_value"].toInt();
    a.conditionGameId = json["condition_game_id"].toString();
    
    QString catStr = json["category"].toString("gameplay");
    if (catStr == "career") a.category = AchievementCategory::CAREER;
    else if (catStr == "skill") a.category = AchievementCategory::SKILL;
    else if (catStr == "collection") a.category = AchievementCategory::COLLECTION;
    else if (catStr == "secret") a.category = AchievementCategory::SECRET;
    else a.category = AchievementCategory::GAMEPLAY;
    
    return a;
}

QJsonObject Achievement::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["xp_bonus"] = xpBonus;
    json["secret"] = isSecret;
    json["icon"] = iconPath;
    json["condition_type"] = conditionType;
    json["condition_value"] = conditionValue;
    json["condition_game_id"] = conditionGameId;
    return json;
}

// ============================================================================
// AchievementSystem
// ============================================================================

AchievementSystem::AchievementSystem(QObject* parent)
    : QObject(parent)
    , m_progressTracker(nullptr)
{
    registerBuiltInAchievements();
}

void AchievementSystem::setProgressTracker(ProgressTracker* tracker)
{
    m_progressTracker = tracker;
}

bool AchievementSystem::loadAchievements(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open achievements file:" << filePath;
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return false;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        Achievement a = Achievement::fromJson(val.toObject());
        if (!a.id.isEmpty()) {
            m_achievements[a.id] = a;
        }
    }
    
    qDebug() << "Loaded" << m_achievements.size() << "achievements";
    return true;
}

bool AchievementSystem::loadBuiltInAchievements()
{
    registerBuiltInAchievements();
    return true;
}

void AchievementSystem::registerBuiltInAchievements()
{
    // Gameplay achievements
    m_achievements["first_win"] = {
        "first_win", "First Victory", "Win your first game",
        AchievementCategory::GAMEPLAY, 50, false, "",
        "wins", 1, ""
    };
    
    m_achievements["edge_10"] = {
        "edge_10", "Edge Apprentice", "Achieve 10 edges in your career",
        AchievementCategory::CAREER, 100, false, "",
        "total_edges", 10, ""
    };
    
    m_achievements["edge_100"] = {
        "edge_100", "Edge Master", "Achieve 100 edges in your career",
        AchievementCategory::CAREER, 500, false, "",
        "total_edges", 100, ""
    };
    
    m_achievements["edge_1000"] = {
        "edge_1000", "Edge Legend", "Achieve 1000 edges in your career",
        AchievementCategory::CAREER, 2000, false, "",
        "total_edges", 1000, ""
    };
    
    m_achievements["win_streak_5"] = {
        "win_streak_5", "Hot Streak", "Win 5 games in a row",
        AchievementCategory::SKILL, 200, false, "",
        "win_streak", 5, ""
    };
    
    m_achievements["win_streak_10"] = {
        "win_streak_10", "Unstoppable", "Win 10 games in a row",
        AchievementCategory::SKILL, 500, false, "",
        "win_streak", 10, ""
    };
    
    m_achievements["games_100"] = {
        "games_100", "Centurion", "Play 100 games",
        AchievementCategory::CAREER, 1000, false, "",
        "total_games", 100, ""
    };
    
    m_achievements["fluid_100ml"] = {
        "fluid_100ml", "Fountain", "Produce 100 mL of fluid cumulatively",
        AchievementCategory::CAREER, 300, false, "",
        "total_fluid", 100, ""
    };
    
    m_achievements["no_orgasm_1hr"] = {
        "no_orgasm_1hr", "Iron Will", "Complete a 1-hour denial game",
        AchievementCategory::SKILL, 750, false, "",
        "denial_minutes", 60, ""
    };
}

// ============================================================================
// Queries
// ============================================================================

QVector<Achievement> AchievementSystem::allAchievements() const
{
    QVector<Achievement> result;
    for (const auto& a : m_achievements) {
        result.append(a);
    }
    return result;
}

QVector<Achievement> AchievementSystem::achievementsByCategory(AchievementCategory category) const
{
    QVector<Achievement> result;
    for (const auto& a : m_achievements) {
        if (a.category == category) {
            result.append(a);
        }
    }
    return result;
}

Achievement AchievementSystem::getAchievement(const QString& id) const
{
    return m_achievements.value(id);
}

bool AchievementSystem::hasAchievement(const QString& id) const
{
    return m_achievements.contains(id);
}

QVector<UnlockedAchievement> AchievementSystem::unlockedAchievements() const
{
    QVector<UnlockedAchievement> result;
    for (const auto& u : m_unlocked) {
        result.append(u);
    }
    return result;
}

bool AchievementSystem::isUnlocked(const QString& achievementId) const
{
    return m_unlocked.contains(achievementId);
}

int AchievementSystem::totalXpFromAchievements() const
{
    int total = 0;
    for (const auto& u : m_unlocked) {
        total += u.xpAwarded;
    }
    return total;
}

double AchievementSystem::progressToward(const QString& achievementId) const
{
    if (!m_achievements.contains(achievementId)) return 0.0;
    if (isUnlocked(achievementId)) return 1.0;
    return calculateProgress(m_achievements[achievementId]);
}

QVector<QPair<QString, double>> AchievementSystem::nearestAchievements(int count) const
{
    QVector<QPair<QString, double>> result;

    for (const auto& a : m_achievements) {
        if (isUnlocked(a.id)) continue;
        double progress = calculateProgress(a);
        result.append({a.id, progress});
    }

    // Sort by progress descending
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (result.size() > count) {
        result.resize(count);
    }

    return result;
}

// ============================================================================
// Check Triggers
// ============================================================================

void AchievementSystem::checkGameCompletion(GameResult result, const QString& gameId)
{
    Q_UNUSED(gameId)

    if (result == GameResult::VICTORY) {
        m_totalWins++;
        m_currentStreak++;
        if (m_currentStreak > m_bestStreak) {
            m_bestStreak = m_currentStreak;
        }
    } else if (result == GameResult::FAILURE) {
        m_currentStreak = 0;
    }

    m_totalGames++;
    checkMilestones();
}

void AchievementSystem::checkMilestones()
{
    for (const auto& a : m_achievements) {
        if (!isUnlocked(a.id) && evaluateCondition(a)) {
            unlockAchievement(a.id);
        }
    }
}

void AchievementSystem::checkSpecificAchievement(const QString& achievementId)
{
    if (!m_achievements.contains(achievementId)) return;
    if (isUnlocked(achievementId)) return;

    if (evaluateCondition(m_achievements[achievementId])) {
        unlockAchievement(achievementId);
    }
}

void AchievementSystem::unlockAchievement(const QString& achievementId)
{
    if (!m_achievements.contains(achievementId)) return;
    if (isUnlocked(achievementId)) return;

    const Achievement& a = m_achievements[achievementId];

    UnlockedAchievement unlock;
    unlock.achievementId = achievementId;
    unlock.unlockedAt = QDateTime::currentDateTime();
    unlock.xpAwarded = a.xpBonus;

    m_unlocked[achievementId] = unlock;

    qDebug() << "Achievement unlocked:" << a.name << "(+" << a.xpBonus << "XP)";
    emit achievementUnlocked(a, a.xpBonus);
}

void AchievementSystem::resetAchievements()
{
    m_unlocked.clear();
    m_totalGames = 0;
    m_totalWins = 0;
    m_currentStreak = 0;
    m_bestStreak = 0;
    m_totalEdges = 0;
    m_totalOrgasms = 0;
    m_totalFluidMl = 0.0;
}

// ============================================================================
// Private Helpers
// ============================================================================

bool AchievementSystem::evaluateCondition(const Achievement& achievement) const
{
    int current = 0;

    if (achievement.conditionType == "wins") {
        current = m_totalWins;
    } else if (achievement.conditionType == "total_edges") {
        current = m_totalEdges;
    } else if (achievement.conditionType == "total_games") {
        current = m_totalGames;
    } else if (achievement.conditionType == "win_streak") {
        current = m_bestStreak;
    } else if (achievement.conditionType == "total_fluid") {
        current = static_cast<int>(m_totalFluidMl);
    } else if (achievement.conditionType == "total_orgasms") {
        current = m_totalOrgasms;
    }

    return current >= achievement.conditionValue;
}

double AchievementSystem::calculateProgress(const Achievement& achievement) const
{
    int current = 0;

    if (achievement.conditionType == "wins") {
        current = m_totalWins;
    } else if (achievement.conditionType == "total_edges") {
        current = m_totalEdges;
    } else if (achievement.conditionType == "total_games") {
        current = m_totalGames;
    } else if (achievement.conditionType == "win_streak") {
        current = m_bestStreak;
    } else if (achievement.conditionType == "total_fluid") {
        current = static_cast<int>(m_totalFluidMl);
    } else if (achievement.conditionType == "total_orgasms") {
        current = m_totalOrgasms;
    }

    if (achievement.conditionValue <= 0) return 1.0;
    return qMin(1.0, static_cast<double>(current) / achievement.conditionValue);
}

