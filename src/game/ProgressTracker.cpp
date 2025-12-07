#include "ProgressTracker.h"
#include "GameDefinition.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>
#include <QDebug>
#include <cmath>

const double ProgressTracker::XP_MULTIPLIER = 1.5;

ProgressTracker::ProgressTracker(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

ProgressTracker::~ProgressTracker()
{
    close();
}

bool ProgressTracker::initialize(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "progress_db");
    m_db.setDatabaseName(dbPath);
    
    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }
    
    if (!createTables()) {
        qWarning() << "Failed to create tables";
        return false;
    }
    
    if (!loadProfile()) {
        // Create new profile
        m_profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_profile.displayName = "Player";
        m_profile.level = 1;
        m_profile.currentXp = 0;
        m_profile.totalXp = 0;
        m_profile.tier = SubscriptionTier::BASIC;
        m_profile.createdAt = QDateTime::currentDateTime();
        m_profile.lastPlayedAt = QDateTime::currentDateTime();
        saveProfile();
    }
    
    loadStats();
    loadUnlocks();
    
    m_initialized = true;
    qDebug() << "ProgressTracker initialized for user:" << m_profile.displayName;
    return true;
}

void ProgressTracker::close()
{
    if (m_db.isOpen()) {
        saveProfile();
        saveStats();
        m_db.close();
    }
    m_initialized = false;
}

bool ProgressTracker::createTables()
{
    QSqlQuery query(m_db);
    
    // User profile table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS user_profile (
            id TEXT PRIMARY KEY,
            display_name TEXT,
            level INTEGER DEFAULT 1,
            current_xp INTEGER DEFAULT 0,
            total_xp INTEGER DEFAULT 0,
            subscription_tier TEXT DEFAULT 'basic',
            created_at TEXT,
            last_played_at TEXT
        )
    )")) {
        qWarning() << "Failed to create user_profile:" << query.lastError().text();
        return false;
    }
    
    // Career stats table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS career_stats (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            total_games INTEGER DEFAULT 0,
            total_wins INTEGER DEFAULT 0,
            total_losses INTEGER DEFAULT 0,
            current_win_streak INTEGER DEFAULT 0,
            best_win_streak INTEGER DEFAULT 0,
            total_edges INTEGER DEFAULT 0,
            total_orgasms INTEGER DEFAULT 0,
            total_fluid_ml REAL DEFAULT 0.0,
            total_play_time_seconds INTEGER DEFAULT 0,
            highest_arousal REAL DEFAULT 0.0,
            longest_denial_seconds INTEGER DEFAULT 0
        )
    )")) {
        qWarning() << "Failed to create career_stats:" << query.lastError().text();
        return false;
    }
    
    // Game sessions table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS game_sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            game_id TEXT,
            game_type TEXT,
            result TEXT,
            score INTEGER,
            duration_seconds INTEGER,
            edges_achieved INTEGER,
            orgasms_detected INTEGER,
            max_arousal REAL,
            avg_arousal REAL,
            fluid_produced_ml REAL,
            xp_earned INTEGER,
            played_at TEXT
        )
    )")) {
        qWarning() << "Failed to create game_sessions:" << query.lastError().text();
        return false;
    }
    
    // Unlocked content table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS unlocked_content (
            content_id TEXT PRIMARY KEY,
            content_type TEXT,
            unlocked_at TEXT
        )
    )")) {
        qWarning() << "Failed to create unlocked_content:" << query.lastError().text();
        return false;
    }
    
    // Initialize career stats if not exists
    query.exec("INSERT OR IGNORE INTO career_stats (id) VALUES (1)");
    
    return true;
}

bool ProgressTracker::loadProfile()
{
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM user_profile LIMIT 1");
    
    if (query.next()) {
        m_profile.id = query.value("id").toString();
        m_profile.displayName = query.value("display_name").toString();
        m_profile.level = query.value("level").toInt();
        m_profile.currentXp = query.value("current_xp").toInt();
        m_profile.totalXp = query.value("total_xp").toInt();
        QString tierStr = query.value("subscription_tier").toString();
        m_profile.tier = (tierStr == "premium") ? SubscriptionTier::PREMIUM : SubscriptionTier::BASIC;
        m_profile.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
        m_profile.lastPlayedAt = QDateTime::fromString(query.value("last_played_at").toString(), Qt::ISODate);
        return true;
    }
    return false;
}

bool ProgressTracker::loadStats()
{
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM career_stats WHERE id = 1");

    if (query.next()) {
        m_stats.totalGames = query.value("total_games").toInt();
        m_stats.totalWins = query.value("total_wins").toInt();
        m_stats.totalLosses = query.value("total_losses").toInt();
        m_stats.currentWinStreak = query.value("current_win_streak").toInt();
        m_stats.bestWinStreak = query.value("best_win_streak").toInt();
        m_stats.totalEdges = query.value("total_edges").toInt();
        m_stats.totalOrgasms = query.value("total_orgasms").toInt();
        m_stats.totalFluidMl = query.value("total_fluid_ml").toDouble();
        m_stats.totalPlayTimeSeconds = query.value("total_play_time_seconds").toInt();
        m_stats.highestArousal = query.value("highest_arousal").toDouble();
        m_stats.longestDenialSeconds = query.value("longest_denial_seconds").toInt();
        return true;
    }
    return false;
}

bool ProgressTracker::loadUnlocks()
{
    m_unlocks.clear();
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM unlocked_content");

    while (query.next()) {
        UnlockedContent uc;
        uc.contentId = query.value("content_id").toString();
        uc.contentType = query.value("content_type").toString();
        uc.unlockedAt = QDateTime::fromString(query.value("unlocked_at").toString(), Qt::ISODate);
        m_unlocks.append(uc);
    }
    return true;
}

bool ProgressTracker::saveProfile()
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT OR REPLACE INTO user_profile
        (id, display_name, level, current_xp, total_xp, subscription_tier, created_at, last_played_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(m_profile.id);
    query.addBindValue(m_profile.displayName);
    query.addBindValue(m_profile.level);
    query.addBindValue(m_profile.currentXp);
    query.addBindValue(m_profile.totalXp);
    query.addBindValue(m_profile.tier == SubscriptionTier::PREMIUM ? "premium" : "basic");
    query.addBindValue(m_profile.createdAt.toString(Qt::ISODate));
    query.addBindValue(m_profile.lastPlayedAt.toString(Qt::ISODate));

    return query.exec();
}

bool ProgressTracker::saveStats()
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE career_stats SET
            total_games = ?, total_wins = ?, total_losses = ?,
            current_win_streak = ?, best_win_streak = ?,
            total_edges = ?, total_orgasms = ?, total_fluid_ml = ?,
            total_play_time_seconds = ?, highest_arousal = ?, longest_denial_seconds = ?
        WHERE id = 1
    )");

    query.addBindValue(m_stats.totalGames);
    query.addBindValue(m_stats.totalWins);
    query.addBindValue(m_stats.totalLosses);
    query.addBindValue(m_stats.currentWinStreak);
    query.addBindValue(m_stats.bestWinStreak);
    query.addBindValue(m_stats.totalEdges);
    query.addBindValue(m_stats.totalOrgasms);
    query.addBindValue(m_stats.totalFluidMl);
    query.addBindValue(m_stats.totalPlayTimeSeconds);
    query.addBindValue(m_stats.highestArousal);
    query.addBindValue(m_stats.longestDenialSeconds);

    return query.exec();
}

// ============================================================================
// User Profile
// ============================================================================

void ProgressTracker::setDisplayName(const QString& name)
{
    m_profile.displayName = name;
    saveProfile();
    emit profileUpdated(m_profile);
}

void ProgressTracker::setSubscriptionTier(SubscriptionTier tier)
{
    m_profile.tier = tier;
    saveProfile();
    emit profileUpdated(m_profile);
}

// ============================================================================
// XP and Leveling
// ============================================================================

void ProgressTracker::addXp(int amount)
{
    m_profile.currentXp += amount;
    m_profile.totalXp += amount;

    emit xpGained(amount, m_profile.totalXp);

    // Check for level up
    while (m_profile.currentXp >= xpToNextLevel()) {
        m_profile.currentXp -= xpToNextLevel();
        m_profile.level++;

        int levelBonus = m_profile.level * 10;
        emit levelUp(m_profile.level, levelBonus);
    }

    saveProfile();
    emit profileUpdated(m_profile);
}

int ProgressTracker::xpToNextLevel() const
{
    return xpForLevel(m_profile.level + 1) - xpForLevel(m_profile.level);
}

int ProgressTracker::xpForLevel(int level) const
{
    // XP = 100 * 1.5^(level-1)
    return static_cast<int>(XP_BASE * std::pow(XP_MULTIPLIER, level - 1));
}

double ProgressTracker::levelProgress() const
{
    int needed = xpToNextLevel();
    if (needed <= 0) return 1.0;
    return static_cast<double>(m_profile.currentXp) / needed;
}

// ============================================================================
// Career Stats
// ============================================================================

void ProgressTracker::updateCareerStats(const CareerStats& stats)
{
    m_stats = stats;
    saveStats();
    emit statsUpdated(m_stats);
}

// ============================================================================
// Game Sessions
// ============================================================================

void ProgressTracker::recordGameSession(const QString& gameId, GameType type,
                                         GameResult result, int score,
                                         int durationSeconds, int edges, int orgasms,
                                         double maxArousal, double avgArousal, double fluidMl)
{
    // Calculate XP
    int xpEarned = 0;
    if (result == GameResult::VICTORY) {
        xpEarned = 100 + (score / 10);
    } else if (result == GameResult::FAILURE || result == GameResult::TIMEOUT) {
        xpEarned = 10 + (durationSeconds / 60);
    }

    // Insert session
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO game_sessions
        (game_id, game_type, result, score, duration_seconds, edges_achieved,
         orgasms_detected, max_arousal, avg_arousal, fluid_produced_ml, xp_earned, played_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(gameId);
    query.addBindValue(GameDefinition::gameTypeToString(type));
    query.addBindValue(static_cast<int>(result));
    query.addBindValue(score);
    query.addBindValue(durationSeconds);
    query.addBindValue(edges);
    query.addBindValue(orgasms);
    query.addBindValue(maxArousal);
    query.addBindValue(avgArousal);
    query.addBindValue(fluidMl);
    query.addBindValue(xpEarned);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    query.exec();

    // Update career stats
    m_stats.totalGames++;
    m_stats.totalEdges += edges;
    m_stats.totalOrgasms += orgasms;
    m_stats.totalFluidMl += fluidMl;
    m_stats.totalPlayTimeSeconds += durationSeconds;

    if (maxArousal > m_stats.highestArousal) {
        m_stats.highestArousal = maxArousal;
    }

    if (result == GameResult::VICTORY) {
        recordWin();
    } else if (result == GameResult::FAILURE) {
        recordLoss();
    }

    saveStats();

    // Add XP
    addXp(xpEarned);

    // Update last played
    m_profile.lastPlayedAt = QDateTime::currentDateTime();
    saveProfile();
}

QVector<GameSession> ProgressTracker::recentSessions(int count) const
{
    QVector<GameSession> sessions;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM game_sessions ORDER BY played_at DESC LIMIT ?");
    query.addBindValue(count);
    query.exec();

    while (query.next()) {
        GameSession s;
        s.id = query.value("id").toLongLong();
        s.gameId = query.value("game_id").toString();
        s.result = static_cast<GameResult>(query.value("result").toInt());
        s.score = query.value("score").toInt();
        s.durationSeconds = query.value("duration_seconds").toInt();
        s.edgesAchieved = query.value("edges_achieved").toInt();
        s.orgasmsDetected = query.value("orgasms_detected").toInt();
        s.maxArousal = query.value("max_arousal").toDouble();
        s.avgArousal = query.value("avg_arousal").toDouble();
        s.fluidProducedMl = query.value("fluid_produced_ml").toDouble();
        s.xpEarned = query.value("xp_earned").toInt();
        s.playedAt = QDateTime::fromString(query.value("played_at").toString(), Qt::ISODate);
        sessions.append(s);
    }

    return sessions;
}

QVector<GameSession> ProgressTracker::sessionsByGame(const QString& gameId, int count) const
{
    QVector<GameSession> sessions;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM game_sessions WHERE game_id = ? ORDER BY played_at DESC LIMIT ?");
    query.addBindValue(gameId);
    query.addBindValue(count);
    query.exec();

    while (query.next()) {
        GameSession s;
        s.id = query.value("id").toLongLong();
        s.gameId = query.value("game_id").toString();
        s.result = static_cast<GameResult>(query.value("result").toInt());
        s.score = query.value("score").toInt();
        sessions.append(s);
    }

    return sessions;
}

int ProgressTracker::bestScoreForGame(const QString& gameId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT MAX(score) FROM game_sessions WHERE game_id = ?");
    query.addBindValue(gameId);
    query.exec();

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ============================================================================
// Unlocks
// ============================================================================

void ProgressTracker::unlockContent(const QString& contentId, const QString& contentType)
{
    if (isContentUnlocked(contentId)) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO unlocked_content (content_id, content_type, unlocked_at) VALUES (?, ?, ?)");
    query.addBindValue(contentId);
    query.addBindValue(contentType);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.exec();

    UnlockedContent uc;
    uc.contentId = contentId;
    uc.contentType = contentType;
    uc.unlockedAt = QDateTime::currentDateTime();
    m_unlocks.append(uc);

    emit contentUnlocked(contentId, contentType);
}

bool ProgressTracker::isContentUnlocked(const QString& contentId) const
{
    for (const auto& uc : m_unlocks) {
        if (uc.contentId == contentId) return true;
    }
    return false;
}

QVector<UnlockedContent> ProgressTracker::allUnlockedContent() const
{
    return m_unlocks;
}

QVector<QString> ProgressTracker::unlockedPatterns() const
{
    QVector<QString> patterns;
    for (const auto& uc : m_unlocks) {
        if (uc.contentType == "pattern") {
            patterns.append(uc.contentId);
        }
    }
    return patterns;
}

QVector<QString> ProgressTracker::unlockedGames() const
{
    QVector<QString> games;
    for (const auto& uc : m_unlocks) {
        if (uc.contentType == "game") {
            games.append(uc.contentId);
        }
    }
    return games;
}

// ============================================================================
// Streaks
// ============================================================================

void ProgressTracker::recordWin()
{
    m_stats.totalWins++;
    m_stats.currentWinStreak++;

    if (m_stats.currentWinStreak > m_stats.bestWinStreak) {
        m_stats.bestWinStreak = m_stats.currentWinStreak;
    }

    emit streakUpdated(m_stats.currentWinStreak, m_stats.bestWinStreak);
}

void ProgressTracker::recordLoss()
{
    m_stats.totalLosses++;
    m_stats.currentWinStreak = 0;

    emit streakUpdated(m_stats.currentWinStreak, m_stats.bestWinStreak);
}

void ProgressTracker::resetStreak()
{
    m_stats.currentWinStreak = 0;
    saveStats();
    emit streakUpdated(0, m_stats.bestWinStreak);
}