#ifndef GAMEDEFINITION_H
#define GAMEDEFINITION_H

#include "GameTypes.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QMap>

/**
 * @brief Objective configuration for a game goal
 */
struct GameObjective {
    QString type;              // "edge_count", "fluid_volume", "duration", etc.
    double target = 0.0;       // Target value to achieve
    double threshold = 0.0;    // Threshold for rate-based objectives
    int timeLimitSeconds = 0;  // Optional time limit for this objective
    int points = 0;            // Points awarded for completion
    bool isMandatory = true;   // Must complete to win
    
    static GameObjective fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief Stimulation parameters for a game
 */
struct StimulationConfig {
    QString patternId;         // Pattern to use
    double initialIntensity = 0.4;
    double maxIntensity = 0.85;
    double edgeThreshold = 0.85;
    double orgasmThreshold = 0.95;
    double recoveryThreshold = 0.5;
    bool tensEnabled = false;
    double tensAmplitude = 0.0;
    
    static StimulationConfig fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief Fail condition for a game
 */
struct FailCondition {
    QString type;              // "orgasm", "timeout", "arousal_drop", etc.
    bool immediateFail = true; // Ends game immediately vs. penalty
    double threshold = 0.0;    // For threshold-based failures
    
    static FailCondition fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief Scoring configuration for a game
 */
struct ScoringConfig {
    int basePoints = 100;
    int perObjectiveBonus = 25;
    double timeBonusPerSecond = 0.5;
    int xpOnWin = 100;
    int xpOnLoss = 10;
    double streakMultiplierMax = 2.0;
    
    static ScoringConfig fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief Consequence configuration (reward or punishment)
 */
struct ConsequenceConfig {
    bool isReward = true;
    SubscriptionTier requiredTier = SubscriptionTier::BASIC;
    ConsequenceAction action = ConsequenceAction::BONUS_XP;
    QString targetId;          // Pattern/game to unlock
    double intensity = 0.0;    // For intensity-based consequences
    int durationSeconds = 0;
    
    static ConsequenceConfig fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief Complete game definition loaded from JSON
 */
class GameDefinition : public QObject
{
    Q_OBJECT

public:
    explicit GameDefinition(QObject* parent = nullptr);
    ~GameDefinition() = default;
    
    // Loading
    bool loadFromJson(const QJsonObject& json);
    bool loadFromFile(const QString& filePath);
    QJsonObject toJson() const;
    
    // Validation
    bool isValid() const;
    QString validationError() const;
    
    // Accessors
    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    GameType gameType() const { return m_gameType; }
    int difficulty() const { return m_difficulty; }
    SubscriptionTier requiredTier() const { return m_requiredTier; }
    QString unlockedBy() const { return m_unlockedBy; }
    QStringList unlocks() const { return m_unlocks; }
    
    const GameObjective& primaryObjective() const { return m_primaryObjective; }
    const QVector<GameObjective>& bonusObjectives() const { return m_bonusObjectives; }
    const QVector<FailCondition>& failConditions() const { return m_failConditions; }
    const StimulationConfig& stimulation() const { return m_stimulation; }
    const ScoringConfig& scoring() const { return m_scoring; }
    const ConsequenceConfig& winConsequence() const { return m_winConsequence; }
    const ConsequenceConfig& failConsequence() const { return m_failConsequence; }

private:
    static GameType stringToGameType(const QString& str);
    static QString gameTypeToString(GameType type);
    
    // Identity
    QString m_id;
    QString m_name;
    QString m_description;
    GameType m_gameType = GameType::CUSTOM;
    int m_difficulty = 1;          // 1-5
    SubscriptionTier m_requiredTier = SubscriptionTier::BASIC;
    QString m_unlockedBy;          // Game ID that unlocks this
    QStringList m_unlocks;         // Games this unlocks
    
    // Objectives
    GameObjective m_primaryObjective;
    QVector<GameObjective> m_bonusObjectives;
    QVector<FailCondition> m_failConditions;
    
    // Configuration
    StimulationConfig m_stimulation;
    ScoringConfig m_scoring;
    ConsequenceConfig m_winConsequence;
    ConsequenceConfig m_failConsequence;
    
    // Validation
    bool m_isValid = false;
    QString m_validationError;
};

#endif // GAMEDEFINITION_H

