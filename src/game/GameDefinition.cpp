#include "GameDefinition.h"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

// ============================================================================
// GameObjective
// ============================================================================

GameObjective GameObjective::fromJson(const QJsonObject& json)
{
    GameObjective obj;
    obj.type = json["type"].toString();
    obj.target = json["target"].toDouble();
    obj.threshold = json["threshold"].toDouble();
    obj.timeLimitSeconds = json["time_limit_seconds"].toInt();
    obj.points = json["points"].toInt();
    obj.isMandatory = json["mandatory"].toBool(true);
    return obj;
}

QJsonObject GameObjective::toJson() const
{
    QJsonObject json;
    json["type"] = type;
    json["target"] = target;
    json["threshold"] = threshold;
    json["time_limit_seconds"] = timeLimitSeconds;
    json["points"] = points;
    json["mandatory"] = isMandatory;
    return json;
}

// ============================================================================
// StimulationConfig
// ============================================================================

StimulationConfig StimulationConfig::fromJson(const QJsonObject& json)
{
    StimulationConfig cfg;
    cfg.patternId = json["pattern"].toString();
    cfg.initialIntensity = json["initial_intensity"].toDouble(0.4);
    cfg.maxIntensity = json["max_intensity"].toDouble(0.85);
    cfg.edgeThreshold = json["edge_threshold"].toDouble(0.85);
    cfg.orgasmThreshold = json["orgasm_threshold"].toDouble(0.95);
    cfg.recoveryThreshold = json["recovery_threshold"].toDouble(0.5);
    cfg.tensEnabled = json["tens_enabled"].toBool(false);
    cfg.tensAmplitude = json["tens_amplitude"].toDouble(0.0);
    return cfg;
}

QJsonObject StimulationConfig::toJson() const
{
    QJsonObject json;
    json["pattern"] = patternId;
    json["initial_intensity"] = initialIntensity;
    json["max_intensity"] = maxIntensity;
    json["edge_threshold"] = edgeThreshold;
    json["orgasm_threshold"] = orgasmThreshold;
    json["recovery_threshold"] = recoveryThreshold;
    json["tens_enabled"] = tensEnabled;
    json["tens_amplitude"] = tensAmplitude;
    return json;
}

// ============================================================================
// FailCondition
// ============================================================================

FailCondition FailCondition::fromJson(const QJsonObject& json)
{
    FailCondition fc;
    fc.type = json["type"].toString();
    fc.immediateFail = json["immediate_fail"].toBool(true);
    fc.threshold = json["threshold"].toDouble();
    return fc;
}

QJsonObject FailCondition::toJson() const
{
    QJsonObject json;
    json["type"] = type;
    json["immediate_fail"] = immediateFail;
    json["threshold"] = threshold;
    return json;
}

// ============================================================================
// ScoringConfig
// ============================================================================

ScoringConfig ScoringConfig::fromJson(const QJsonObject& json)
{
    ScoringConfig sc;
    sc.basePoints = json["base_points"].toInt(100);
    sc.perObjectiveBonus = json["per_objective_bonus"].toInt(25);
    sc.timeBonusPerSecond = json["time_bonus_per_second"].toDouble(0.5);
    sc.xpOnWin = json["xp_on_win"].toInt(100);
    sc.xpOnLoss = json["xp_on_loss"].toInt(10);
    sc.streakMultiplierMax = json["streak_multiplier_max"].toDouble(2.0);
    return sc;
}

QJsonObject ScoringConfig::toJson() const
{
    QJsonObject json;
    json["base_points"] = basePoints;
    json["per_objective_bonus"] = perObjectiveBonus;
    json["time_bonus_per_second"] = timeBonusPerSecond;
    json["xp_on_win"] = xpOnWin;
    json["xp_on_loss"] = xpOnLoss;
    json["streak_multiplier_max"] = streakMultiplierMax;
    return json;
}

// ============================================================================
// ConsequenceConfig
// ============================================================================

ConsequenceConfig ConsequenceConfig::fromJson(const QJsonObject& json)
{
    ConsequenceConfig cc;
    cc.isReward = json["type"].toString() == "reward";
    
    QString tierStr = json["tier"].toString("basic");
    cc.requiredTier = (tierStr == "premium") ? 
                      SubscriptionTier::PREMIUM : SubscriptionTier::BASIC;
    
    // Parse action
    QString actionStr = json["action"].toString();
    if (actionStr == "unlock_pattern") cc.action = ConsequenceAction::UNLOCK_PATTERN;
    else if (actionStr == "unlock_game") cc.action = ConsequenceAction::UNLOCK_GAME;
    else if (actionStr == "bonus_xp") cc.action = ConsequenceAction::BONUS_XP;
    else if (actionStr == "intensity_decrease") cc.action = ConsequenceAction::INTENSITY_DECREASE;
    else if (actionStr == "pleasure_burst") cc.action = ConsequenceAction::PLEASURE_BURST;
    else if (actionStr == "intensity_increase") cc.action = ConsequenceAction::INTENSITY_INCREASE;
    else if (actionStr == "denial_extension") cc.action = ConsequenceAction::DENIAL_EXTENSION;
    else if (actionStr == "pattern_switch") cc.action = ConsequenceAction::PATTERN_SWITCH;
    else if (actionStr == "tens_shock") cc.action = ConsequenceAction::TENS_SHOCK;
    else if (actionStr == "tens_burst_series") cc.action = ConsequenceAction::TENS_BURST_SERIES;
    else if (actionStr == "max_vacuum_pulse") cc.action = ConsequenceAction::MAX_VACUUM_PULSE;
    
    cc.targetId = json["target_id"].toString();
    cc.intensity = json["intensity_boost"].toDouble();
    cc.durationSeconds = json["duration_seconds"].toInt();
    
    return cc;
}

QJsonObject ConsequenceConfig::toJson() const
{
    QJsonObject json;
    json["type"] = isReward ? "reward" : "punishment";
    json["tier"] = (requiredTier == SubscriptionTier::PREMIUM) ? "premium" : "basic";
    // Action serialization would go here
    json["target_id"] = targetId;
    json["intensity_boost"] = intensity;
    json["duration_seconds"] = durationSeconds;
    return json;
}

// ============================================================================
// GameDefinition
// ============================================================================

GameDefinition::GameDefinition(QObject* parent)
    : QObject(parent)
{
}

bool GameDefinition::loadFromJson(const QJsonObject& json)
{
    m_isValid = false;
    m_validationError.clear();

    // Required fields
    m_id = json["id"].toString();
    if (m_id.isEmpty()) {
        m_validationError = "Missing required field: id";
        return false;
    }

    m_name = json["name"].toString();
    m_description = json["description"].toString();
    m_gameType = stringToGameType(json["type"].toString());
    m_difficulty = json["difficulty"].toInt(1);

    // Subscription tier
    QString tierStr = json["subscription_tier"].toString("basic");
    m_requiredTier = (tierStr == "premium") ?
                     SubscriptionTier::PREMIUM : SubscriptionTier::BASIC;

    // Unlock chain
    m_unlockedBy = json["unlocked_by"].toString();
    QJsonArray unlocksArray = json["unlocks"].toArray();
    for (const QJsonValue& val : unlocksArray) {
        m_unlocks.append(val.toString());
    }

    // Objectives
    QJsonObject objectives = json["objectives"].toObject();
    m_primaryObjective = GameObjective::fromJson(objectives["primary"].toObject());

    QJsonArray bonusArray = objectives["bonus"].toArray();
    for (const QJsonValue& val : bonusArray) {
        m_bonusObjectives.append(GameObjective::fromJson(val.toObject()));
    }

    // Fail conditions
    QJsonArray failArray = json["fail_conditions"].toArray();
    for (const QJsonValue& val : failArray) {
        m_failConditions.append(FailCondition::fromJson(val.toObject()));
    }

    // Stimulation
    m_stimulation = StimulationConfig::fromJson(json["stimulation"].toObject());

    // Scoring
    m_scoring = ScoringConfig::fromJson(json["scoring"].toObject());

    // Consequences
    QJsonObject consequences = json["consequences"].toObject();
    m_winConsequence = ConsequenceConfig::fromJson(consequences["on_win"].toObject());
    m_failConsequence = ConsequenceConfig::fromJson(consequences["on_fail"].toObject());

    m_isValid = true;
    qDebug() << "Loaded game definition:" << m_name << "(" << m_id << ")";
    return true;
}

bool GameDefinition::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_validationError = "Cannot open file: " + filePath;
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        m_validationError = "JSON parse error: " + parseError.errorString();
        return false;
    }

    return loadFromJson(doc.object());
}

QJsonObject GameDefinition::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["description"] = m_description;
    json["type"] = gameTypeToString(m_gameType);
    json["difficulty"] = m_difficulty;
    json["subscription_tier"] = (m_requiredTier == SubscriptionTier::PREMIUM) ?
                                 "premium" : "basic";
    json["unlocked_by"] = m_unlockedBy;

    QJsonArray unlocksArray;
    for (const QString& u : m_unlocks) {
        unlocksArray.append(u);
    }
    json["unlocks"] = unlocksArray;

    QJsonObject objectives;
    objectives["primary"] = m_primaryObjective.toJson();
    QJsonArray bonusArray;
    for (const GameObjective& obj : m_bonusObjectives) {
        bonusArray.append(obj.toJson());
    }
    objectives["bonus"] = bonusArray;
    json["objectives"] = objectives;

    json["stimulation"] = m_stimulation.toJson();
    json["scoring"] = m_scoring.toJson();

    QJsonObject consequences;
    consequences["on_win"] = m_winConsequence.toJson();
    consequences["on_fail"] = m_failConsequence.toJson();
    json["consequences"] = consequences;

    return json;
}

bool GameDefinition::isValid() const
{
    return m_isValid;
}

QString GameDefinition::validationError() const
{
    return m_validationError;
}

GameType GameDefinition::stringToGameType(const QString& str)
{
    static const QMap<QString, GameType> typeMap = {
        {"EDGE_COUNT", GameType::EDGE_COUNT},
        {"EDGE_ENDURANCE", GameType::EDGE_ENDURANCE},
        {"DENIAL_MAINTENANCE", GameType::DENIAL_MAINTENANCE},
        {"DENIAL_LIMIT", GameType::DENIAL_LIMIT},
        {"FLUID_PRODUCTION", GameType::FLUID_PRODUCTION},
        {"FLUID_RATE", GameType::FLUID_RATE},
        {"PATTERN_ENDURANCE", GameType::PATTERN_ENDURANCE},
        {"STIMULATION_MARATHON", GameType::STIMULATION_MARATHON},
        {"ELECTRODE_AVOIDANCE", GameType::ELECTRODE_AVOIDANCE},
        {"SHOCK_ROULETTE", GameType::SHOCK_ROULETTE},
        {"INTENSITY_CLIMB", GameType::INTENSITY_CLIMB},
        {"OBEDIENCE_TRIAL", GameType::OBEDIENCE_TRIAL},
        {"PUNISHMENT_ENDURANCE", GameType::PUNISHMENT_ENDURANCE},
        {"CUSTOM", GameType::CUSTOM}
    };

    return typeMap.value(str.toUpper(), GameType::CUSTOM);
}

QString GameDefinition::gameTypeToString(GameType type)
{
    static const QMap<GameType, QString> typeMap = {
        {GameType::EDGE_COUNT, "EDGE_COUNT"},
        {GameType::EDGE_ENDURANCE, "EDGE_ENDURANCE"},
        {GameType::DENIAL_MAINTENANCE, "DENIAL_MAINTENANCE"},
        {GameType::DENIAL_LIMIT, "DENIAL_LIMIT"},
        {GameType::FLUID_PRODUCTION, "FLUID_PRODUCTION"},
        {GameType::FLUID_RATE, "FLUID_RATE"},
        {GameType::PATTERN_ENDURANCE, "PATTERN_ENDURANCE"},
        {GameType::STIMULATION_MARATHON, "STIMULATION_MARATHON"},
        {GameType::ELECTRODE_AVOIDANCE, "ELECTRODE_AVOIDANCE"},
        {GameType::SHOCK_ROULETTE, "SHOCK_ROULETTE"},
        {GameType::INTENSITY_CLIMB, "INTENSITY_CLIMB"},
        {GameType::OBEDIENCE_TRIAL, "OBEDIENCE_TRIAL"},
        {GameType::PUNISHMENT_ENDURANCE, "PUNISHMENT_ENDURANCE"},
        {GameType::CUSTOM, "CUSTOM"}
    };

    return typeMap.value(type, "CUSTOM");
}

