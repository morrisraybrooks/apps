#ifndef CONSEQUENCEENGINE_H
#define CONSEQUENCEENGINE_H

#include "GameTypes.h"
#include "GameDefinition.h"
#include <QObject>
#include <QTimer>
#include <QQueue>

// Forward declarations
class OrgasmControlAlgorithm;
class TENSController;
class HardwareManager;

/**
 * @brief Safety limits for consequence actions
 */
struct SafetyLimits {
    // TENS limits
    double maxTensAmplitudePercent = 70.0;      // Never exceed 70%
    int maxTensBurstCount = 5;                   // Max shocks in a burst
    int minTensCooldownMs = 2000;                // Min time between shocks
    int maxTensDurationMs = 500;                 // Max single shock duration
    
    // Vacuum limits
    double maxVacuumMmHg = 65.0;                 // Never exceed 65 mmHg
    int maxVacuumPulseDurationMs = 3000;         // Max pulse duration
    
    // Intensity limits
    double maxIntensityBoost = 0.3;              // Max +30% intensity
    int maxDenialExtensionMinutes = 10;          // Max 10 min extension
    
    // Session limits
    int maxPunishmentsPerSession = 20;           // Prevent abuse
    int maxRewardsPerSession = 50;
};

/**
 * @brief Queued consequence action
 */
struct QueuedConsequence {
    ConsequenceAction action;
    double intensity;
    int durationMs;
    QString targetId;
    qint64 scheduledTime;
};

/**
 * @brief Consequence engine for rewards and punishments
 * 
 * Manages the execution of game consequences with safety limits,
 * cooldowns, and subscription tier enforcement.
 */
class ConsequenceEngine : public QObject
{
    Q_OBJECT

public:
    explicit ConsequenceEngine(HardwareManager* hardware,
                               OrgasmControlAlgorithm* orgasmControl,
                               QObject* parent = nullptr);
    ~ConsequenceEngine();

    // Configuration
    void setSafetyLimits(const SafetyLimits& limits);
    SafetyLimits safetyLimits() const { return m_limits; }
    void setSubscriptionTier(SubscriptionTier tier);
    
    // Consequence execution
    void applyReward(const ConsequenceConfig& config);
    void applyPunishment(const ConsequenceConfig& config);
    void executeAction(ConsequenceAction action, double intensity = 0.5, 
                       int durationMs = 500, const QString& targetId = QString());
    
    // Queue management
    void queueConsequence(const QueuedConsequence& consequence);
    void clearQueue();
    int queueSize() const { return m_queue.size(); }
    
    // Control
    void pause();
    void resume();
    void emergencyStop();
    bool isPaused() const { return m_paused; }
    
    // Statistics
    int punishmentsThisSession() const { return m_punishmentsThisSession; }
    int rewardsThisSession() const { return m_rewardsThisSession; }
    void resetSessionStats();

Q_SIGNALS:
    void rewardApplied(ConsequenceAction action, const QString& description);
    void punishmentApplied(ConsequenceAction action, const QString& description);
    void safetyLimitReached(const QString& limitType);
    void consequenceQueued(int queuePosition);
    void consequenceExecuted(ConsequenceAction action);
    void cooldownActive(int remainingMs);

private Q_SLOTS:
    void processQueue();
    void onCooldownExpired();

private:
    bool canExecute(ConsequenceAction action) const;
    bool isPremiumAction(ConsequenceAction action) const;
    void executeReward(ConsequenceAction action, double intensity, 
                       int durationMs, const QString& targetId);
    void executePunishment(ConsequenceAction action, double intensity, 
                           int durationMs, const QString& targetId);
    QString actionDescription(ConsequenceAction action) const;
    
    // Hardware
    HardwareManager* m_hardware;
    OrgasmControlAlgorithm* m_orgasmControl;
    TENSController* m_tensController;
    
    // Configuration
    SafetyLimits m_limits;
    SubscriptionTier m_subscriptionTier;
    
    // State
    bool m_paused;
    QQueue<QueuedConsequence> m_queue;
    QTimer* m_queueTimer;
    QTimer* m_cooldownTimer;
    qint64 m_lastTensTime;
    int m_tensShocksInBurst;
    
    // Session stats
    int m_punishmentsThisSession;
    int m_rewardsThisSession;
    
    // Constants
    static const int QUEUE_PROCESS_INTERVAL_MS = 100;
};

#endif // CONSEQUENCEENGINE_H

