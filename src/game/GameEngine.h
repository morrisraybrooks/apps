#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "GameTypes.h"
#include "GameDefinition.h"
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <memory>

// Forward declarations
class OrgasmControlAlgorithm;
class FluidSensor;
class TENSController;
class HardwareManager;
class ConsequenceEngine;
class AchievementSystem;
class ProgressTracker;

/**
 * @brief Core game engine for V-Contour gamification system
 * 
 * Manages game lifecycle, tracks objectives, calculates scoring,
 * and coordinates with hardware systems for stimulation control.
 */
class GameEngine : public QObject
{
    Q_OBJECT

public:
    explicit GameEngine(HardwareManager* hardware,
                        OrgasmControlAlgorithm* orgasmControl,
                        QObject* parent = nullptr);
    ~GameEngine();

    // Game lifecycle
    bool loadGame(const QString& gameId);
    bool loadGameFromDefinition(const GameDefinition& definition);
    void startGame();
    void pauseGame();
    void resumeGame();
    void stopGame();
    void abortGame();
    
    // Safety
    void triggerSafetyAction(SafetyAction action);
    void emergencyStop();
    
    // State queries
    GameState state() const { return m_state; }
    bool isRunning() const { return m_state == GameState::RUNNING; }
    GameResult result() const { return m_result; }
    
    // Progress queries
    double objectiveProgress() const;
    int currentScore() const { return m_currentScore; }
    int elapsedSeconds() const;
    int remainingSeconds() const;
    
    // Current game info
    const GameDefinition* currentGame() const { return m_currentGame.get(); }
    QString currentGameId() const;
    QString currentGameName() const;
    
    // Statistics for current session
    int edgesAchieved() const { return m_edgesAchieved; }
    int orgasmsDetected() const { return m_orgasmsDetected; }
    double maxArousal() const { return m_maxArousal; }
    double avgArousal() const;
    double fluidProduced() const { return m_fluidProduced; }
    
    // Subscription
    void setSubscriptionTier(SubscriptionTier tier);
    SubscriptionTier subscriptionTier() const { return m_subscriptionTier; }
    
    // Sub-system access
    void setConsequenceEngine(ConsequenceEngine* engine);
    void setAchievementSystem(AchievementSystem* achievements);
    void setProgressTracker(ProgressTracker* tracker);

Q_SIGNALS:
    // State signals
    void stateChanged(GameState newState);
    void gameStarted(const QString& gameId, const QString& gameName);
    void gamePaused();
    void gameResumed();
    void gameEnded(GameResult result, int finalScore, int xpEarned);
    
    // Progress signals
    void objectiveProgressUpdated(double progress, double target);
    void scoreUpdated(int score);
    void bonusObjectiveCompleted(const QString& objectiveType, int points);
    void timeWarning(int secondsRemaining);
    
    // Event signals
    void edgeDetectedInGame(int edgeNumber);
    void orgasmDetectedInGame(int orgasmNumber);
    void fluidMilestone(double totalMl);
    void failConditionTriggered(const QString& conditionType);
    
    // Countdown
    void countdownTick(int secondsRemaining);
    
    // Safety
    void safetyActionTriggered(SafetyAction action);

private Q_SLOTS:
    void onUpdateTick();
    void onCountdownTick();
    void onArousalChanged(double arousal);
    void onEdgeDetected(int edgeNumber, double intensity);
    void onOrgasmDetected(int orgasmNumber, qint64 timeMs);
    void onFluidVolumeChanged(double currentMl, double cumulativeMl);

private:
    void setState(GameState newState);
    void evaluateObjectives();
    void checkFailConditions();
    void calculateScore();
    void applyConsequences(GameResult result);
    void recordSession();
    void connectSignals();
    void disconnectSignals();
    void resetSessionStats();
    void startStimulation();
    void stopStimulation();
    
    // Hardware interfaces
    HardwareManager* m_hardware;
    OrgasmControlAlgorithm* m_orgasmControl;
    FluidSensor* m_fluidSensor;
    TENSController* m_tensController;
    
    // Sub-systems
    ConsequenceEngine* m_consequenceEngine;
    AchievementSystem* m_achievements;
    ProgressTracker* m_progressTracker;
    
    // Current game
    std::unique_ptr<GameDefinition> m_currentGame;
    GameState m_state;
    GameResult m_result;
    
    // Timers
    QTimer* m_updateTimer;
    QTimer* m_countdownTimer;
    QElapsedTimer m_gameTimer;
    int m_countdownSeconds;
    
    // Session statistics
    int m_edgesAchieved;
    int m_orgasmsDetected;
    double m_maxArousal;
    double m_arousalSum;
    int m_arousalSamples;
    double m_fluidProduced;
    int m_currentScore;
    int m_bonusPointsEarned;
    
    // Configuration
    SubscriptionTier m_subscriptionTier;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Constants
    static const int UPDATE_INTERVAL_MS = 100;
    static const int COUNTDOWN_SECONDS = 3;
};

#endif // GAMEENGINE_H

