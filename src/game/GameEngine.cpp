#include "GameEngine.h"
#include "ConsequenceEngine.h"
#include "AchievementSystem.h"
#include "ProgressTracker.h"
#include "../control/OrgasmControlAlgorithm.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/FluidSensor.h"
#include "../hardware/TENSController.h"
#include "../hardware/MotionSensor.h"
#include <QDebug>

GameEngine::GameEngine(HardwareManager* hardware,
                       OrgasmControlAlgorithm* orgasmControl,
                       QObject* parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_orgasmControl(orgasmControl)
    , m_fluidSensor(hardware ? hardware->getFluidSensor() : nullptr)
    , m_tensController(hardware ? hardware->getTENSController() : nullptr)
    , m_motionSensor(nullptr)
    , m_consequenceEngine(nullptr)
    , m_achievements(nullptr)
    , m_progressTracker(nullptr)
    , m_state(GameState::IDLE)
    , m_result(GameResult::NONE)
    , m_updateTimer(new QTimer(this))
    , m_countdownTimer(new QTimer(this))
    , m_countdownSeconds(COUNTDOWN_SECONDS)
    , m_edgesAchieved(0)
    , m_orgasmsDetected(0)
    , m_maxArousal(0.0)
    , m_arousalSum(0.0)
    , m_arousalSamples(0)
    , m_fluidProduced(0.0)
    , m_currentScore(0)
    , m_bonusPointsEarned(0)
    , m_motionViolations(0)
    , m_motionWarnings(0)
    , m_averageStillness(100.0)
    , m_stillnessRequired(false)
    , m_subscriptionTier(SubscriptionTier::BASIC)
{
    connect(m_updateTimer, &QTimer::timeout, this, &GameEngine::onUpdateTick);
    connect(m_countdownTimer, &QTimer::timeout, this, &GameEngine::onCountdownTick);
    
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    m_countdownTimer->setInterval(1000);
}

GameEngine::~GameEngine()
{
    stopGame();
}

// ============================================================================
// Game Lifecycle
// ============================================================================

bool GameEngine::loadGame(const QString& gameId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_state != GameState::IDLE) {
        qWarning() << "Cannot load game: engine not idle";
        return false;
    }
    
    // Load from data directory
    QString path = QString("data/games/%1.json").arg(gameId);
    auto definition = std::make_unique<GameDefinition>();
    
    if (!definition->loadFromFile(path)) {
        qWarning() << "Failed to load game:" << definition->validationError();
        return false;
    }
    
    // Check subscription tier
    if (definition->requiredTier() == SubscriptionTier::PREMIUM &&
        m_subscriptionTier != SubscriptionTier::PREMIUM) {
        qWarning() << "Game requires premium subscription";
        return false;
    }
    
    m_currentGame = std::move(definition);
    qDebug() << "Loaded game:" << m_currentGame->name();
    return true;
}

bool GameEngine::loadGameFromDefinition(const GameDefinition& definition)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_state != GameState::IDLE) {
        qWarning() << "Cannot load game: engine not idle";
        return false;
    }
    
    if (!definition.isValid()) {
        qWarning() << "Invalid game definition";
        return false;
    }
    
    m_currentGame = std::make_unique<GameDefinition>();
    m_currentGame->loadFromJson(definition.toJson());
    return true;
}

void GameEngine::startGame()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_currentGame) {
        qWarning() << "No game loaded";
        return;
    }
    
    if (m_state != GameState::IDLE) {
        qWarning() << "Cannot start: game already running";
        return;
    }
    
    resetSessionStats();
    setState(GameState::INITIALIZING);
    
    // Connect to hardware signals
    connectSignals();
    
    // Configure stimulation based on game definition
    if (m_orgasmControl) {
        const auto& stim = m_currentGame->stimulation();
        m_orgasmControl->setEdgeThreshold(stim.edgeThreshold);
        m_orgasmControl->setOrgasmThreshold(stim.orgasmThreshold);
        m_orgasmControl->setRecoveryThreshold(stim.recoveryThreshold);
        m_orgasmControl->setTENSEnabled(stim.tensEnabled &&
                                         m_subscriptionTier == SubscriptionTier::PREMIUM);
    }

    // Check if stillness is required for this game type
    GameType type = m_currentGame->type();
    m_stillnessRequired = (type == GameType::STILLNESS_CHALLENGE ||
                           type == GameType::FORCED_STILLNESS);

    // Start countdown
    m_countdownSeconds = COUNTDOWN_SECONDS;
    setState(GameState::COUNTDOWN);
    m_countdownTimer->start();
    
    emit gameStarted(m_currentGame->id(), m_currentGame->name());
    emit countdownTick(m_countdownSeconds);
}

void GameEngine::pauseGame()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_state != GameState::RUNNING) return;
    
    m_updateTimer->stop();
    stopStimulation();
    setState(GameState::PAUSED);
    emit gamePaused();
}

void GameEngine::resumeGame()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_state != GameState::PAUSED) return;
    
    startStimulation();
    m_updateTimer->start();
    setState(GameState::RUNNING);
    emit gameResumed();
}

void GameEngine::stopGame()
{
    QMutexLocker locker(&m_mutex);

    m_updateTimer->stop();
    m_countdownTimer->stop();
    stopStimulation();
    disconnectSignals();

    if (m_state != GameState::IDLE) {
        setState(GameState::IDLE);
    }
}

void GameEngine::abortGame()
{
    QMutexLocker locker(&m_mutex);

    if (m_state == GameState::IDLE) return;

    m_result = GameResult::ABORTED;
    stopStimulation();

    // Still record partial session
    recordSession();

    stopGame();
    emit gameEnded(GameResult::ABORTED, m_currentScore, 0);
}

void GameEngine::triggerSafetyAction(SafetyAction action)
{
    QMutexLocker locker(&m_mutex);

    emit safetyActionTriggered(action);

    switch (action) {
        case SafetyAction::YELLOW:
            // Reduce intensity, pause consequences
            if (m_orgasmControl) {
                // Reduce intensity by 30%
            }
            if (m_consequenceEngine) {
                m_consequenceEngine->pause();
            }
            break;

        case SafetyAction::RED:
            // End game gracefully, no penalties
            m_result = GameResult::SAFEWORD;
            stopStimulation();
            setState(GameState::SAFEWORD);
            recordSession();
            stopGame();
            emit gameEnded(GameResult::SAFEWORD, m_currentScore, 0);
            break;

        case SafetyAction::EMERGENCY_STOP:
            emergencyStop();
            break;

        default:
            break;
    }
}

void GameEngine::emergencyStop()
{
    m_updateTimer->stop();
    m_countdownTimer->stop();

    if (m_orgasmControl) {
        m_orgasmControl->emergencyStop();
    }

    disconnectSignals();
    m_result = GameResult::ABORTED;
    setState(GameState::IDLE);
}

// ============================================================================
// Queries
// ============================================================================

double GameEngine::objectiveProgress() const
{
    QMutexLocker locker(&m_mutex);

    if (!m_currentGame) return 0.0;

    const auto& primary = m_currentGame->primaryObjective();
    double target = primary.target;
    if (target <= 0.0) return 1.0;

    double achieved = 0.0;

    switch (m_currentGame->gameType()) {
        case GameType::EDGE_COUNT:
            achieved = static_cast<double>(m_edgesAchieved);
            break;
        case GameType::FLUID_PRODUCTION:
            achieved = m_fluidProduced;
            break;
        case GameType::PATTERN_ENDURANCE:
        case GameType::STIMULATION_MARATHON:
            achieved = static_cast<double>(elapsedSeconds());
            break;
        default:
            achieved = static_cast<double>(m_edgesAchieved);
            break;
    }

    return qMin(achieved / target, 1.0);
}

int GameEngine::elapsedSeconds() const
{
    if (m_state == GameState::IDLE) return 0;
    return static_cast<int>(m_gameTimer.elapsed() / 1000);
}

int GameEngine::remainingSeconds() const
{
    if (!m_currentGame) return 0;

    int limit = m_currentGame->primaryObjective().timeLimitSeconds;
    if (limit <= 0) return -1;  // No time limit

    return qMax(0, limit - elapsedSeconds());
}

QString GameEngine::currentGameId() const
{
    return m_currentGame ? m_currentGame->id() : QString();
}

QString GameEngine::currentGameName() const
{
    return m_currentGame ? m_currentGame->name() : QString();
}

double GameEngine::avgArousal() const
{
    if (m_arousalSamples == 0) return 0.0;
    return m_arousalSum / m_arousalSamples;
}

void GameEngine::setSubscriptionTier(SubscriptionTier tier)
{
    m_subscriptionTier = tier;
}

void GameEngine::setConsequenceEngine(ConsequenceEngine* engine)
{
    m_consequenceEngine = engine;
}

void GameEngine::setAchievementSystem(AchievementSystem* achievements)
{
    m_achievements = achievements;
}

void GameEngine::setProgressTracker(ProgressTracker* tracker)
{
    m_progressTracker = tracker;
}

void GameEngine::setMotionSensor(MotionSensor* sensor)
{
    m_motionSensor = sensor;
}

// ============================================================================
// Timer Callbacks
// ============================================================================

void GameEngine::onUpdateTick()
{
    QMutexLocker locker(&m_mutex);

    if (m_state != GameState::RUNNING) return;

    evaluateObjectives();
    checkFailConditions();
    calculateScore();

    // Check time limit
    if (m_currentGame) {
        int limit = m_currentGame->primaryObjective().timeLimitSeconds;
        if (limit > 0) {
            int remaining = remainingSeconds();

            // Time warnings at 60, 30, 10 seconds
            if (remaining == 60 || remaining == 30 || remaining == 10) {
                emit timeWarning(remaining);
            }

            if (remaining <= 0) {
                // Time's up!
                m_result = GameResult::TIMEOUT;
                setState(GameState::TIMEOUT);
                stopStimulation();
                applyConsequences(GameResult::TIMEOUT);
                recordSession();

                int xp = m_currentGame->scoring().xpOnLoss;
                emit gameEnded(GameResult::TIMEOUT, m_currentScore, xp);
                setState(GameState::IDLE);
            }
        }
    }
}

void GameEngine::onCountdownTick()
{
    m_countdownSeconds--;
    emit countdownTick(m_countdownSeconds);

    if (m_countdownSeconds <= 0) {
        m_countdownTimer->stop();

        // Start the actual game
        m_gameTimer.start();
        startStimulation();
        m_updateTimer->start();
        setState(GameState::RUNNING);
    }
}

// ============================================================================
// Signal Handlers
// ============================================================================

void GameEngine::onArousalChanged(double arousal)
{
    m_arousalSum += arousal;
    m_arousalSamples++;

    if (arousal > m_maxArousal) {
        m_maxArousal = arousal;
    }
}

void GameEngine::onEdgeDetected(int edgeNumber, double /*intensity*/)
{
    m_edgesAchieved = edgeNumber;
    emit edgeDetectedInGame(edgeNumber);

    // Check if this completes an edge-based objective
    if (m_currentGame && m_currentGame->gameType() == GameType::EDGE_COUNT) {
        if (edgeNumber >= static_cast<int>(m_currentGame->primaryObjective().target)) {
            // Victory!
            m_result = GameResult::VICTORY;
            setState(GameState::VICTORY);
            stopStimulation();
            applyConsequences(GameResult::VICTORY);
            recordSession();

            calculateScore();
            int xp = m_currentGame->scoring().xpOnWin;
            emit gameEnded(GameResult::VICTORY, m_currentScore, xp);
            setState(GameState::IDLE);
        }
    }

    emit objectiveProgressUpdated(objectiveProgress(),
                                   m_currentGame ? m_currentGame->primaryObjective().target : 0);
}

void GameEngine::onOrgasmDetected(int orgasmNumber, qint64 /*timeMs*/)
{
    m_orgasmsDetected = orgasmNumber;
    emit orgasmDetectedInGame(orgasmNumber);

    // Check fail conditions for orgasm
    if (m_currentGame) {
        for (const auto& fc : m_currentGame->failConditions()) {
            if (fc.type == "orgasm" && fc.immediateFail) {
                // Failure!
                emit failConditionTriggered("orgasm");
                m_result = GameResult::FAILURE;
                setState(GameState::FAILURE);
                stopStimulation();
                applyConsequences(GameResult::FAILURE);
                recordSession();

                int xp = m_currentGame->scoring().xpOnLoss;
                emit gameEnded(GameResult::FAILURE, m_currentScore, xp);
                setState(GameState::IDLE);
                return;
            }
        }
    }
}

void GameEngine::onFluidVolumeChanged(double /*currentMl*/, double cumulativeMl)
{
    m_fluidProduced = cumulativeMl;

    // Milestones at 5, 10, 25, 50, 100 mL
    static const double milestones[] = {5.0, 10.0, 25.0, 50.0, 100.0};
    static double lastMilestone = 0.0;

    for (double m : milestones) {
        if (cumulativeMl >= m && lastMilestone < m) {
            emit fluidMilestone(m);
            lastMilestone = m;
        }
    }

    // Check fluid-based victory
    if (m_currentGame && m_currentGame->gameType() == GameType::FLUID_PRODUCTION) {
        if (cumulativeMl >= m_currentGame->primaryObjective().target) {
            m_result = GameResult::VICTORY;
            setState(GameState::VICTORY);
            stopStimulation();
            applyConsequences(GameResult::VICTORY);
            recordSession();

            calculateScore();
            int xp = m_currentGame->scoring().xpOnWin;
            emit gameEnded(GameResult::VICTORY, m_currentScore, xp);
            setState(GameState::IDLE);
        }
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

void GameEngine::setState(GameState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

void GameEngine::evaluateObjectives()
{
    if (!m_currentGame) return;

    // Evaluate bonus objectives
    for (const auto& bonus : m_currentGame->bonusObjectives()) {
        bool completed = false;

        if (bonus.type == "avg_arousal_above") {
            completed = avgArousal() >= bonus.threshold;
        } else if (bonus.type == "no_backoff_pause") {
            // Check if we never paused during backoff
            completed = true;  // Simplified - would need tracking
        } else if (bonus.type == "fluid_above") {
            completed = m_fluidProduced >= bonus.target;
        }

        if (completed) {
            m_bonusPointsEarned += bonus.points;
            emit bonusObjectiveCompleted(bonus.type, bonus.points);
        }
    }

    emit objectiveProgressUpdated(objectiveProgress(),
                                   m_currentGame->primaryObjective().target);
}

void GameEngine::checkFailConditions()
{
    if (!m_currentGame) return;

    for (const auto& fc : m_currentGame->failConditions()) {
        bool failed = false;

        if (fc.type == "arousal_drop") {
            // Check if arousal dropped below threshold
            // Would need current arousal tracking
        } else if (fc.type == "movement_detected") {
            // Premium: would need motion sensor
        }

        if (failed && fc.immediateFail) {
            emit failConditionTriggered(fc.type);
            m_result = GameResult::FAILURE;
            setState(GameState::FAILURE);
            stopStimulation();
            applyConsequences(GameResult::FAILURE);
            recordSession();

            int xp = m_currentGame->scoring().xpOnLoss;
            emit gameEnded(GameResult::FAILURE, m_currentScore, xp);
            setState(GameState::IDLE);
            return;
        }
    }
}

void GameEngine::calculateScore()
{
    if (!m_currentGame) return;

    const auto& scoring = m_currentGame->scoring();

    // Base points
    int score = scoring.basePoints;

    // Objective completion bonus
    double progress = objectiveProgress();
    score += static_cast<int>(scoring.perObjectiveBonus * progress *
                               m_currentGame->primaryObjective().target);

    // Time bonus (for time-limited games)
    int remaining = remainingSeconds();
    if (remaining > 0) {
        score += static_cast<int>(remaining * scoring.timeBonusPerSecond);
    }

    // Add bonus points from completed bonus objectives
    score += m_bonusPointsEarned;

    // Streak multiplier (would need progress tracker)
    // double multiplier = 1.0 + (winStreak * 0.05);
    // score = static_cast<int>(score * qMin(multiplier, scoring.streakMultiplierMax));

    m_currentScore = score;
    emit scoreUpdated(m_currentScore);
}

void GameEngine::applyConsequences(GameResult result)
{
    if (!m_consequenceEngine || !m_currentGame) return;

    if (result == GameResult::VICTORY) {
        m_consequenceEngine->applyReward(m_currentGame->winConsequence());
    } else if (result == GameResult::FAILURE) {
        m_consequenceEngine->applyPunishment(m_currentGame->failConsequence());
    }
}

void GameEngine::recordSession()
{
    if (!m_progressTracker || !m_currentGame) return;

    m_progressTracker->recordGameSession(
        m_currentGame->id(),
        m_currentGame->gameType(),
        m_result,
        m_currentScore,
        elapsedSeconds(),
        m_edgesAchieved,
        m_orgasmsDetected,
        m_maxArousal,
        avgArousal(),
        m_fluidProduced
    );

    // Check achievements
    if (m_achievements) {
        m_achievements->checkGameCompletion(m_result, m_currentGame->id());
        m_achievements->checkMilestones();
    }
}

void GameEngine::connectSignals()
{
    if (m_orgasmControl) {
        connect(m_orgasmControl, &OrgasmControlAlgorithm::arousalLevelChanged,
                this, &GameEngine::onArousalChanged);
        connect(m_orgasmControl, &OrgasmControlAlgorithm::edgeDetected,
                this, &GameEngine::onEdgeDetected);
        connect(m_orgasmControl, &OrgasmControlAlgorithm::orgasmDetected,
                this, &GameEngine::onOrgasmDetected);
    }

    if (m_fluidSensor) {
        connect(m_fluidSensor, &FluidSensor::volumeUpdated,
                this, &GameEngine::onFluidVolumeChanged);
    }

    if (m_motionSensor && m_stillnessRequired) {
        connect(m_motionSensor, &MotionSensor::violationDetected,
                this, &GameEngine::onMotionViolation);
        connect(m_motionSensor, &MotionSensor::warningIssued,
                this, &GameEngine::onMotionWarning);
        connect(m_motionSensor, &MotionSensor::stillnessChanged,
                this, &GameEngine::onStillnessChanged);
        m_motionSensor->startSession();
    }
}

void GameEngine::disconnectSignals()
{
    if (m_orgasmControl) {
        disconnect(m_orgasmControl, &OrgasmControlAlgorithm::arousalLevelChanged,
                   this, &GameEngine::onArousalChanged);
        disconnect(m_orgasmControl, &OrgasmControlAlgorithm::edgeDetected,
                   this, &GameEngine::onEdgeDetected);
        disconnect(m_orgasmControl, &OrgasmControlAlgorithm::orgasmDetected,
                   this, &GameEngine::onOrgasmDetected);
    }

    if (m_fluidSensor) {
        disconnect(m_fluidSensor, &FluidSensor::volumeUpdated,
                   this, &GameEngine::onFluidVolumeChanged);
    }

    if (m_motionSensor) {
        disconnect(m_motionSensor, &MotionSensor::violationDetected,
                   this, &GameEngine::onMotionViolation);
        disconnect(m_motionSensor, &MotionSensor::warningIssued,
                   this, &GameEngine::onMotionWarning);
        disconnect(m_motionSensor, &MotionSensor::stillnessChanged,
                   this, &GameEngine::onStillnessChanged);
        m_motionSensor->endSession();
    }
}

void GameEngine::resetSessionStats()
{
    m_edgesAchieved = 0;
    m_orgasmsDetected = 0;
    m_maxArousal = 0.0;
    m_arousalSum = 0.0;
    m_arousalSamples = 0;
    m_fluidProduced = 0.0;
    m_currentScore = 0;
    m_bonusPointsEarned = 0;
    m_motionViolations = 0;
    m_motionWarnings = 0;
    m_averageStillness = 100.0;
    m_result = GameResult::NONE;
}

void GameEngine::startStimulation()
{
    if (!m_orgasmControl || !m_currentGame) return;

    const auto& stim = m_currentGame->stimulation();

    // Start appropriate mode based on game type
    switch (m_currentGame->gameType()) {
        case GameType::EDGE_COUNT:
        case GameType::EDGE_ENDURANCE:
            m_orgasmControl->startAdaptiveEdging(
                static_cast<int>(m_currentGame->primaryObjective().target));
            break;

        case GameType::DENIAL_MAINTENANCE:
        case GameType::DENIAL_LIMIT:
            m_orgasmControl->startDenial(
                m_currentGame->primaryObjective().timeLimitSeconds * 1000);
            break;

        default:
            m_orgasmControl->startAdaptiveEdging(5);
            break;
    }
}

void GameEngine::stopStimulation()
{
    if (m_orgasmControl) {
        m_orgasmControl->stop();
    }
}

// ============================================================================
// Motion Event Handlers
// ============================================================================

void GameEngine::onMotionViolation(int level, double intensity)
{
    if (m_state != GameState::RUNNING || !m_stillnessRequired) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_motionViolations++;

    qDebug() << "Motion violation #" << m_motionViolations
             << "level:" << level << "intensity:" << intensity;

    locker.unlock();

    // Apply escalating punishment via consequence engine
    if (m_consequenceEngine) {
        // Encode violation count in intensity for escalation logic
        double escalationIntensity = m_motionViolations / 10.0;
        m_consequenceEngine->executeAction(ConsequenceAction::MOTION_ESCALATION,
                                           escalationIntensity, 300, QString());
    }

    emit motionViolationDetected(m_motionViolations, intensity);

    // Check if max violations reached (fail condition)
    if (m_currentGame && m_motionViolations >= 10) {
        emit failConditionTriggered("max_motion_violations");
        setState(GameState::FAILURE);
    }
}

void GameEngine::onMotionWarning(const QString& message)
{
    if (m_state != GameState::RUNNING || !m_stillnessRequired) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_motionWarnings++;
    locker.unlock();

    qDebug() << "Motion warning #" << m_motionWarnings << ":" << message;

    // Issue warning consequence
    if (m_consequenceEngine) {
        m_consequenceEngine->executeAction(ConsequenceAction::MOTION_WARNING,
                                           0.2, 0, QString());
    }

    emit motionWarningIssued(m_motionWarnings);
}

void GameEngine::onStillnessChanged(bool isStill, double score)
{
    if (m_state != GameState::RUNNING) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Update running average
    static int sampleCount = 0;
    sampleCount++;
    m_averageStillness = ((m_averageStillness * (sampleCount - 1)) + score) / sampleCount;

    locker.unlock();

    emit stillnessScoreUpdated(score);

    Q_UNUSED(isStill)
}