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

    if (!createPointsTables()) {
        qWarning() << "Failed to create points tables";
        return false;
    }

    if (!createPairingTables()) {
        qWarning() << "Failed to create pairing tables";
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
        m_profile.pointsBalance = 0;
        m_profile.privilegeTier = PrivilegeTier::BEGINNER;
        m_profile.createdAt = QDateTime::currentDateTime();
        m_profile.lastPlayedAt = QDateTime::currentDateTime();
        saveProfile();
    }

    loadStats();
    loadUnlocks();
    loadPairings();

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
            points_balance INTEGER DEFAULT 0,
            privilege_tier TEXT DEFAULT 'beginner',
            safe_word TEXT,
            created_at TEXT,
            last_played_at TEXT
        )
    )")) {
        qWarning() << "Failed to create user_profile:" << query.lastError().text();
        return false;
    }

    // Add columns if they don't exist (for database migration)
    query.exec("ALTER TABLE user_profile ADD COLUMN points_balance INTEGER DEFAULT 0");
    query.exec("ALTER TABLE user_profile ADD COLUMN privilege_tier TEXT DEFAULT 'beginner'");
    query.exec("ALTER TABLE user_profile ADD COLUMN safe_word TEXT");

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

        // Load points economy fields
        m_profile.pointsBalance = query.value("points_balance").toInt();
        QString privTierStr = query.value("privilege_tier").toString();
        if (privTierStr == "dom_master") m_profile.privilegeTier = PrivilegeTier::DOM_MASTER;
        else if (privTierStr == "advanced") m_profile.privilegeTier = PrivilegeTier::ADVANCED;
        else if (privTierStr == "intermediate") m_profile.privilegeTier = PrivilegeTier::INTERMEDIATE;
        else m_profile.privilegeTier = PrivilegeTier::BEGINNER;
        m_profile.safeWord = query.value("safe_word").toString();

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
        (id, display_name, level, current_xp, total_xp, subscription_tier,
         points_balance, privilege_tier, safe_word, created_at, last_played_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(m_profile.id);
    query.addBindValue(m_profile.displayName);
    query.addBindValue(m_profile.level);
    query.addBindValue(m_profile.currentXp);
    query.addBindValue(m_profile.totalXp);
    query.addBindValue(m_profile.tier == SubscriptionTier::PREMIUM ? "premium" : "basic");
    query.addBindValue(m_profile.pointsBalance);

    // Convert privilege tier to string
    QString privTierStr;
    switch (m_profile.privilegeTier) {
        case PrivilegeTier::DOM_MASTER: privTierStr = "dom_master"; break;
        case PrivilegeTier::ADVANCED: privTierStr = "advanced"; break;
        case PrivilegeTier::INTERMEDIATE: privTierStr = "intermediate"; break;
        default: privTierStr = "beginner"; break;
    }
    query.addBindValue(privTierStr);
    query.addBindValue(m_profile.safeWord);
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

// ============================================================================
// Points Economy - Table Creation
// ============================================================================

bool ProgressTracker::createPointsTables()
{
    QSqlQuery query(m_db);

    // Point transactions table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS point_transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT,
            transaction_type TEXT,
            amount INTEGER,
            balance_after INTEGER,
            description TEXT,
            related_user_id TEXT,
            related_game_id TEXT,
            timestamp TEXT
        )
    )")) {
        qWarning() << "Failed to create point_transactions:" << query.lastError().text();
        return false;
    }

    // Create index for faster queries
    query.exec("CREATE INDEX IF NOT EXISTS idx_transactions_user ON point_transactions(user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_transactions_type ON point_transactions(transaction_type)");

    return true;
}

bool ProgressTracker::createPairingTables()
{
    QSqlQuery query(m_db);

    // User pairings table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS user_pairings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT,
            partner_id TEXT,
            partner_display_name TEXT,
            consent_status TEXT DEFAULT 'none',
            paired_at TEXT,
            consent_expires_at TEXT,
            can_control INTEGER DEFAULT 0,
            can_be_controlled INTEGER DEFAULT 0,
            UNIQUE(user_id, partner_id)
        )
    )")) {
        qWarning() << "Failed to create user_pairings:" << query.lastError().text();
        return false;
    }

    // Command audit log table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS command_audit_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            controller_id TEXT,
            target_id TEXT,
            command_type TEXT,
            point_cost INTEGER,
            success INTEGER,
            details TEXT,
            timestamp TEXT
        )
    )")) {
        qWarning() << "Failed to create command_audit_log:" << query.lastError().text();
        return false;
    }

    return true;
}


// ============================================================================
// Points Economy - Tier Management
// ============================================================================

PrivilegeTier ProgressTracker::tierForPoints(int points)
{
    if (points >= TIER_DOM_MASTER_POINTS) return PrivilegeTier::DOM_MASTER;
    if (points >= TIER_ADVANCED_POINTS) return PrivilegeTier::ADVANCED;
    if (points >= TIER_INTERMEDIATE_POINTS) return PrivilegeTier::INTERMEDIATE;
    return PrivilegeTier::BEGINNER;
}

int ProgressTracker::pointsForTier(PrivilegeTier tier)
{
    switch (tier) {
        case PrivilegeTier::DOM_MASTER: return TIER_DOM_MASTER_POINTS;
        case PrivilegeTier::ADVANCED: return TIER_ADVANCED_POINTS;
        case PrivilegeTier::INTERMEDIATE: return TIER_INTERMEDIATE_POINTS;
        case PrivilegeTier::BEGINNER:
        default: return 0;
    }
}

QString ProgressTracker::tierName(PrivilegeTier tier)
{
    switch (tier) {
        case PrivilegeTier::DOM_MASTER: return "DOM Master";
        case PrivilegeTier::ADVANCED: return "Advanced";
        case PrivilegeTier::INTERMEDIATE: return "Intermediate";
        case PrivilegeTier::BEGINNER:
        default: return "Beginner";
    }
}

void ProgressTracker::updatePrivilegeTier()
{
    PrivilegeTier newTier = tierForPoints(m_profile.pointsBalance);
    if (newTier != m_profile.privilegeTier) {
        m_profile.privilegeTier = newTier;
        saveProfile();
        emit privilegeTierChanged(newTier);
    }
}

// ============================================================================
// Points Economy - Transactions
// ============================================================================

bool ProgressTracker::addPoints(int amount, PointTransactionType type,
                                 const QString& description,
                                 const QString& relatedUserId,
                                 const QString& relatedGameId)
{
    if (amount <= 0) return false;

    int oldBalance = m_profile.pointsBalance;
    m_profile.pointsBalance += amount;

    // Record transaction
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO point_transactions
        (user_id, transaction_type, amount, balance_after, description,
         related_user_id, related_game_id, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(static_cast<int>(type));
    query.addBindValue(amount);
    query.addBindValue(m_profile.pointsBalance);
    query.addBindValue(description);
    query.addBindValue(relatedUserId);
    query.addBindValue(relatedGameId);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to record point transaction:" << query.lastError().text();
    }

    saveProfile();
    updatePrivilegeTier();

    emit pointsChanged(m_profile.pointsBalance, amount);

    PointTransaction tx;
    tx.userId = m_profile.id;
    tx.type = type;
    tx.amount = amount;
    tx.balanceAfter = m_profile.pointsBalance;
    tx.description = description;
    tx.relatedUserId = relatedUserId;
    tx.relatedGameId = relatedGameId;
    tx.timestamp = QDateTime::currentDateTime();
    emit transactionRecorded(tx);

    return true;
}

bool ProgressTracker::spendPoints(int amount, PointTransactionType type,
                                   const QString& description,
                                   const QString& relatedUserId)
{
    if (amount <= 0) return false;
    if (!canAfford(amount)) return false;

    m_profile.pointsBalance -= amount;

    // Record transaction (negative amount for spending)
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO point_transactions
        (user_id, transaction_type, amount, balance_after, description,
         related_user_id, related_game_id, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(static_cast<int>(type));
    query.addBindValue(-amount);
    query.addBindValue(m_profile.pointsBalance);
    query.addBindValue(description);
    query.addBindValue(relatedUserId);
    query.addBindValue(QString());
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to record point transaction:" << query.lastError().text();
    }

    saveProfile();
    // Note: Don't downgrade tier when spending points

    emit pointsChanged(m_profile.pointsBalance, -amount);

    PointTransaction tx;
    tx.userId = m_profile.id;
    tx.type = type;
    tx.amount = -amount;
    tx.balanceAfter = m_profile.pointsBalance;
    tx.description = description;
    tx.relatedUserId = relatedUserId;
    tx.timestamp = QDateTime::currentDateTime();
    emit transactionRecorded(tx);

    return true;
}

bool ProgressTracker::transferPoints(const QString& recipientId, int amount)
{
    if (m_profile.privilegeTier < PrivilegeTier::ADVANCED) {
        qWarning() << "Point transfer requires Advanced tier or higher";
        return false;
    }

    if (!canAfford(amount)) {
        qWarning() << "Insufficient points for transfer";
        return false;
    }

    QString desc = QString("Transfer to %1").arg(recipientId);
    return spendPoints(amount, PointTransactionType::POINT_TRANSFER, desc, recipientId);
}


QVector<PointTransaction> ProgressTracker::recentTransactions(int count) const
{
    QVector<PointTransaction> transactions;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, user_id, transaction_type, amount, balance_after,
               description, related_user_id, related_game_id, timestamp
        FROM point_transactions
        WHERE user_id = ?
        ORDER BY timestamp DESC
        LIMIT ?
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(count);

    if (query.exec()) {
        while (query.next()) {
            PointTransaction tx;
            tx.id = query.value(0).toLongLong();
            tx.userId = query.value(1).toString();
            tx.type = static_cast<PointTransactionType>(query.value(2).toInt());
            tx.amount = query.value(3).toInt();
            tx.balanceAfter = query.value(4).toInt();
            tx.description = query.value(5).toString();
            tx.relatedUserId = query.value(6).toString();
            tx.relatedGameId = query.value(7).toString();
            tx.timestamp = QDateTime::fromString(query.value(8).toString(), Qt::ISODate);
            transactions.append(tx);
        }
    }
    return transactions;
}

QVector<PointTransaction> ProgressTracker::transactionsByType(PointTransactionType type, int count) const
{
    QVector<PointTransaction> transactions;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, user_id, transaction_type, amount, balance_after,
               description, related_user_id, related_game_id, timestamp
        FROM point_transactions
        WHERE user_id = ? AND transaction_type = ?
        ORDER BY timestamp DESC
        LIMIT ?
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(static_cast<int>(type));
    query.addBindValue(count);

    if (query.exec()) {
        while (query.next()) {
            PointTransaction tx;
            tx.id = query.value(0).toLongLong();
            tx.userId = query.value(1).toString();
            tx.type = static_cast<PointTransactionType>(query.value(2).toInt());
            tx.amount = query.value(3).toInt();
            tx.balanceAfter = query.value(4).toInt();
            tx.description = query.value(5).toString();
            tx.relatedUserId = query.value(6).toString();
            tx.relatedGameId = query.value(7).toString();
            tx.timestamp = QDateTime::fromString(query.value(8).toString(), Qt::ISODate);
            transactions.append(tx);
        }
    }
    return transactions;
}

int ProgressTracker::totalEarned() const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT SUM(amount) FROM point_transactions WHERE user_id = ? AND amount > 0");
    query.addBindValue(m_profile.id);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int ProgressTracker::totalSpent() const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT SUM(ABS(amount)) FROM point_transactions WHERE user_id = ? AND amount < 0");
    query.addBindValue(m_profile.id);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}


// ============================================================================
// Paired Users / Consent Management
// ============================================================================

bool ProgressTracker::loadPairings()
{
    m_pairedUsers.clear();
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT partner_id, partner_display_name, consent_status, paired_at,
               consent_expires_at, can_control, can_be_controlled
        FROM user_pairings
        WHERE user_id = ?
    )");
    query.addBindValue(m_profile.id);

    if (query.exec()) {
        while (query.next()) {
            PairedUser pu;
            pu.id = m_profile.id;
            pu.partnerId = query.value(0).toString();
            pu.partnerDisplayName = query.value(1).toString();
            QString statusStr = query.value(2).toString();
            if (statusStr == "granted") pu.consentStatus = ConsentStatus::GRANTED;
            else if (statusStr == "pending") pu.consentStatus = ConsentStatus::PENDING;
            else if (statusStr == "revoked") pu.consentStatus = ConsentStatus::REVOKED;
            else if (statusStr == "expired") pu.consentStatus = ConsentStatus::EXPIRED;
            else pu.consentStatus = ConsentStatus::NONE;
            pu.pairedAt = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
            pu.consentExpiresAt = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
            pu.canControl = query.value(5).toBool();
            pu.canBeControlled = query.value(6).toBool();
            m_pairedUsers.append(pu);
        }
    }
    return true;
}

bool ProgressTracker::addPairedUser(const QString& partnerId, const QString& partnerName)
{
    if (isPaired(partnerId)) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO user_pairings
        (user_id, partner_id, partner_display_name, consent_status, paired_at)
        VALUES (?, ?, ?, 'pending', ?)
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(partnerId);
    query.addBindValue(partnerName);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to add paired user:" << query.lastError().text();
        return false;
    }

    PairedUser pu;
    pu.id = m_profile.id;
    pu.partnerId = partnerId;
    pu.partnerDisplayName = partnerName;
    pu.consentStatus = ConsentStatus::PENDING;
    pu.pairedAt = QDateTime::currentDateTime();
    pu.canControl = false;
    pu.canBeControlled = false;
    m_pairedUsers.append(pu);

    emit pairingAdded(pu);
    return true;
}

bool ProgressTracker::removePairedUser(const QString& partnerId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM user_pairings WHERE user_id = ? AND partner_id = ?");
    query.addBindValue(m_profile.id);
    query.addBindValue(partnerId);

    if (!query.exec()) {
        qWarning() << "Failed to remove paired user:" << query.lastError().text();
        return false;
    }

    for (int i = 0; i < m_pairedUsers.size(); ++i) {
        if (m_pairedUsers[i].partnerId == partnerId) {
            m_pairedUsers.removeAt(i);
            break;
        }
    }

    emit pairingRemoved(partnerId);
    return true;
}

bool ProgressTracker::isPaired(const QString& partnerId) const
{
    for (const auto& pu : m_pairedUsers) {
        if (pu.partnerId == partnerId) return true;
    }
    return false;
}

PairedUser* ProgressTracker::getPairedUser(const QString& partnerId)
{
    for (int i = 0; i < m_pairedUsers.size(); ++i) {
        if (m_pairedUsers[i].partnerId == partnerId) {
            return &m_pairedUsers[i];
        }
    }
    return nullptr;
}

bool ProgressTracker::grantConsent(const QString& partnerId, int expirationMinutes)
{
    PairedUser* pu = getPairedUser(partnerId);
    if (!pu) return false;

    QDateTime expiresAt = QDateTime::currentDateTime().addSecs(expirationMinutes * 60);

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE user_pairings
        SET consent_status = 'granted', consent_expires_at = ?, can_be_controlled = 1
        WHERE user_id = ? AND partner_id = ?
    )");
    query.addBindValue(expiresAt.toString(Qt::ISODate));
    query.addBindValue(m_profile.id);
    query.addBindValue(partnerId);

    if (!query.exec()) {
        qWarning() << "Failed to grant consent:" << query.lastError().text();
        return false;
    }

    pu->consentStatus = ConsentStatus::GRANTED;
    pu->consentExpiresAt = expiresAt;
    pu->canBeControlled = true;

    emit consentChanged(partnerId, ConsentStatus::GRANTED);
    return true;
}

bool ProgressTracker::revokeConsent(const QString& partnerId)
{
    PairedUser* pu = getPairedUser(partnerId);
    if (!pu) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE user_pairings
        SET consent_status = 'revoked', can_be_controlled = 0
        WHERE user_id = ? AND partner_id = ?
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(partnerId);

    if (!query.exec()) {
        qWarning() << "Failed to revoke consent:" << query.lastError().text();
        return false;
    }

    pu->consentStatus = ConsentStatus::REVOKED;
    pu->canBeControlled = false;

    emit consentChanged(partnerId, ConsentStatus::REVOKED);
    return true;
}


bool ProgressTracker::hasValidConsent(const QString& partnerId) const
{
    for (const auto& pu : m_pairedUsers) {
        if (pu.partnerId == partnerId) {
            if (pu.consentStatus != ConsentStatus::GRANTED) return false;
            if (pu.consentExpiresAt.isValid() &&
                pu.consentExpiresAt < QDateTime::currentDateTime()) {
                return false;
            }
            return pu.canBeControlled;
        }
    }
    return false;
}

ConsentStatus ProgressTracker::consentStatus(const QString& partnerId) const
{
    for (const auto& pu : m_pairedUsers) {
        if (pu.partnerId == partnerId) {
            // Check for expiration
            if (pu.consentStatus == ConsentStatus::GRANTED &&
                pu.consentExpiresAt.isValid() &&
                pu.consentExpiresAt < QDateTime::currentDateTime()) {
                return ConsentStatus::EXPIRED;
            }
            return pu.consentStatus;
        }
    }
    return ConsentStatus::NONE;
}

// ============================================================================
// Safe Word
// ============================================================================

void ProgressTracker::setSafeWord(const QString& safeWord)
{
    m_profile.safeWord = safeWord;
    saveProfile();
}

bool ProgressTracker::verifySafeWord(const QString& word) const
{
    if (m_profile.safeWord.isEmpty()) return false;
    return m_profile.safeWord.compare(word, Qt::CaseInsensitive) == 0;
}

// ============================================================================
// Audit Logging
// ============================================================================

void ProgressTracker::logCommand(const QString& commandType, const QString& targetUserId,
                                  int pointCost, bool success, const QString& details)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO command_audit_log
        (controller_id, target_id, command_type, point_cost, success, details, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(m_profile.id);
    query.addBindValue(targetUserId);
    query.addBindValue(commandType);
    query.addBindValue(pointCost);
    query.addBindValue(success ? 1 : 0);
    query.addBindValue(details);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to log command:" << query.lastError().text();
    }
}

QVector<PointTransaction> ProgressTracker::commandAuditLog(int count) const
{
    // Return command-related transactions from the audit log
    return transactionsByType(PointTransactionType::COMMAND_COST, count);
}