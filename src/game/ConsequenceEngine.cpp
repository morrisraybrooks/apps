#include "ConsequenceEngine.h"
#include "../control/OrgasmControlAlgorithm.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/TENSController.h"
#include <QDateTime>
#include <QDebug>

ConsequenceEngine::ConsequenceEngine(HardwareManager* hardware,
                                     OrgasmControlAlgorithm* orgasmControl,
                                     QObject* parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_orgasmControl(orgasmControl)
    , m_tensController(hardware ? hardware->getTENSController() : nullptr)
    , m_subscriptionTier(SubscriptionTier::BASIC)
    , m_paused(false)
    , m_queueTimer(new QTimer(this))
    , m_cooldownTimer(new QTimer(this))
    , m_lastTensTime(0)
    , m_tensShocksInBurst(0)
    , m_punishmentsThisSession(0)
    , m_rewardsThisSession(0)
{
    connect(m_queueTimer, &QTimer::timeout, this, &ConsequenceEngine::processQueue);
    connect(m_cooldownTimer, &QTimer::timeout, this, &ConsequenceEngine::onCooldownExpired);
    
    m_queueTimer->setInterval(QUEUE_PROCESS_INTERVAL_MS);
}

ConsequenceEngine::~ConsequenceEngine()
{
    emergencyStop();
}

void ConsequenceEngine::setSafetyLimits(const SafetyLimits& limits)
{
    m_limits = limits;
}

void ConsequenceEngine::setSubscriptionTier(SubscriptionTier tier)
{
    m_subscriptionTier = tier;
}

// ============================================================================
// Consequence Execution
// ============================================================================

void ConsequenceEngine::applyReward(const ConsequenceConfig& config)
{
    if (m_rewardsThisSession >= m_limits.maxRewardsPerSession) {
        emit safetyLimitReached("max_rewards_per_session");
        return;
    }
    
    executeAction(config.action, config.intensity, 
                  config.durationSeconds * 1000, config.targetId);
    m_rewardsThisSession++;
    emit rewardApplied(config.action, actionDescription(config.action));
}

void ConsequenceEngine::applyPunishment(const ConsequenceConfig& config)
{
    if (m_punishmentsThisSession >= m_limits.maxPunishmentsPerSession) {
        emit safetyLimitReached("max_punishments_per_session");
        return;
    }
    
    // Check subscription tier for premium punishments
    if (isPremiumAction(config.action) && 
        m_subscriptionTier != SubscriptionTier::PREMIUM) {
        qDebug() << "Premium punishment blocked - basic tier";
        return;
    }
    
    executeAction(config.action, config.intensity,
                  config.durationSeconds * 1000, config.targetId);
    m_punishmentsThisSession++;
    emit punishmentApplied(config.action, actionDescription(config.action));
}

void ConsequenceEngine::executeAction(ConsequenceAction action, double intensity,
                                      int durationMs, const QString& targetId)
{
    if (m_paused) {
        // Queue for later
        QueuedConsequence qc;
        qc.action = action;
        qc.intensity = intensity;
        qc.durationMs = durationMs;
        qc.targetId = targetId;
        qc.scheduledTime = QDateTime::currentMSecsSinceEpoch();
        queueConsequence(qc);
        return;
    }
    
    if (!canExecute(action)) {
        qDebug() << "Cannot execute action - safety limit or cooldown";
        return;
    }
    
    // Determine if reward or punishment
    switch (action) {
        case ConsequenceAction::UNLOCK_PATTERN:
        case ConsequenceAction::UNLOCK_GAME:
        case ConsequenceAction::BONUS_XP:
        case ConsequenceAction::INTENSITY_DECREASE:
        case ConsequenceAction::PLEASURE_BURST:
            executeReward(action, intensity, durationMs, targetId);
            break;
            
        default:
            executePunishment(action, intensity, durationMs, targetId);
            break;
    }
    
    emit consequenceExecuted(action);
}

void ConsequenceEngine::queueConsequence(const QueuedConsequence& consequence)
{
    m_queue.enqueue(consequence);
    emit consequenceQueued(m_queue.size());
    
    if (!m_queueTimer->isActive()) {
        m_queueTimer->start();
    }
}

void ConsequenceEngine::clearQueue()
{
    m_queue.clear();
    m_queueTimer->stop();
}

void ConsequenceEngine::pause()
{
    m_paused = true;
    m_queueTimer->stop();
}

void ConsequenceEngine::resume()
{
    m_paused = false;
    if (!m_queue.isEmpty()) {
        m_queueTimer->start();
    }
}

void ConsequenceEngine::emergencyStop()
{
    m_paused = true;
    clearQueue();
    m_cooldownTimer->stop();
    
    if (m_tensController) {
        m_tensController->stop();
    }
}

void ConsequenceEngine::resetSessionStats()
{
    m_punishmentsThisSession = 0;
    m_rewardsThisSession = 0;
    m_tensShocksInBurst = 0;
}

// ============================================================================
// Timer Callbacks
// ============================================================================

void ConsequenceEngine::processQueue()
{
    if (m_paused || m_queue.isEmpty()) {
        m_queueTimer->stop();
        return;
    }

    QueuedConsequence qc = m_queue.dequeue();
    executeAction(qc.action, qc.intensity, qc.durationMs, qc.targetId);

    if (m_queue.isEmpty()) {
        m_queueTimer->stop();
    }
}

void ConsequenceEngine::onCooldownExpired()
{
    m_cooldownTimer->stop();
    m_tensShocksInBurst = 0;
}

// ============================================================================
// Private Helpers
// ============================================================================

bool ConsequenceEngine::canExecute(ConsequenceAction action) const
{
    // Check TENS cooldown
    if (action == ConsequenceAction::TENS_SHOCK ||
        action == ConsequenceAction::TENS_BURST_SERIES) {

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_lastTensTime < m_limits.minTensCooldownMs) {
            return false;
        }

        if (m_tensShocksInBurst >= m_limits.maxTensBurstCount) {
            return false;
        }
    }

    return true;
}

bool ConsequenceEngine::isPremiumAction(ConsequenceAction action) const
{
    switch (action) {
        case ConsequenceAction::TENS_SHOCK:
        case ConsequenceAction::TENS_BURST_SERIES:
        case ConsequenceAction::MAX_VACUUM_PULSE:
        case ConsequenceAction::COMBINED_ASSAULT:
        case ConsequenceAction::RANDOM_SHOCK_INTERVAL:
            return true;
        default:
            return false;
    }
}

void ConsequenceEngine::executeReward(ConsequenceAction action, double intensity,
                                      int durationMs, const QString& targetId)
{
    Q_UNUSED(durationMs)

    switch (action) {
        case ConsequenceAction::UNLOCK_PATTERN:
            // Would notify ProgressTracker to unlock pattern
            qDebug() << "Reward: Unlocked pattern" << targetId;
            break;

        case ConsequenceAction::UNLOCK_GAME:
            qDebug() << "Reward: Unlocked game" << targetId;
            break;

        case ConsequenceAction::BONUS_XP:
            qDebug() << "Reward: Bonus XP" << static_cast<int>(intensity);
            break;

        case ConsequenceAction::INTENSITY_DECREASE:
            if (m_orgasmControl) {
                // Decrease intensity for comfort
                qDebug() << "Reward: Intensity decreased by" << intensity;
            }
            break;

        case ConsequenceAction::PLEASURE_BURST:
            if (m_orgasmControl) {
                // Brief high-intensity pleasure
                qDebug() << "Reward: Pleasure burst";
            }
            break;

        default:
            break;
    }
}

void ConsequenceEngine::executePunishment(ConsequenceAction action, double intensity,
                                          int durationMs, const QString& /*targetId*/)
{
    // Apply safety limits
    intensity = qMin(intensity, 1.0);

    switch (action) {
        case ConsequenceAction::INTENSITY_INCREASE:
            if (m_orgasmControl) {
                double boost = qMin(intensity * 0.5, m_limits.maxIntensityBoost);
                qDebug() << "Punishment: Intensity increased by" << boost;
            }
            break;

        case ConsequenceAction::DENIAL_EXTENSION:
            qDebug() << "Punishment: Denial extended by"
                     << qMin(durationMs / 60000, m_limits.maxDenialExtensionMinutes) << "min";
            break;

        case ConsequenceAction::PATTERN_SWITCH:
            qDebug() << "Punishment: Pattern switched to more intense";
            break;

        case ConsequenceAction::TENS_SHOCK:
            if (m_tensController && m_subscriptionTier == SubscriptionTier::PREMIUM) {
                double safeAmplitude = qMin(intensity * 100.0, m_limits.maxTensAmplitudePercent);
                int safeDuration = qMin(durationMs, m_limits.maxTensDurationMs);

                m_tensController->setAmplitude(safeAmplitude / 100.0);
                m_tensController->pulse(safeDuration);

                m_lastTensTime = QDateTime::currentMSecsSinceEpoch();
                m_tensShocksInBurst++;

                // Start cooldown timer
                m_cooldownTimer->start(m_limits.minTensCooldownMs);

                qDebug() << "Punishment: TENS shock at" << safeAmplitude << "% for" << safeDuration << "ms";
            }
            break;

        case ConsequenceAction::TENS_BURST_SERIES:
            if (m_tensController && m_subscriptionTier == SubscriptionTier::PREMIUM) {
                // Queue multiple shocks
                for (int i = 0; i < m_limits.maxTensBurstCount; i++) {
                    QueuedConsequence qc;
                    qc.action = ConsequenceAction::TENS_SHOCK;
                    qc.intensity = intensity;
                    qc.durationMs = qMin(durationMs, m_limits.maxTensDurationMs);
                    qc.scheduledTime = QDateTime::currentMSecsSinceEpoch() +
                                       (i * m_limits.minTensCooldownMs);
                    m_queue.enqueue(qc);
                }
                if (!m_queueTimer->isActive()) {
                    m_queueTimer->start();
                }
                qDebug() << "Punishment: TENS burst series queued";
            }
            break;

        case ConsequenceAction::MAX_VACUUM_PULSE:
            if (m_hardware && m_subscriptionTier == SubscriptionTier::PREMIUM) {
                int safeDuration = qMin(durationMs, m_limits.maxVacuumPulseDurationMs);
                qDebug() << "Punishment: Max vacuum pulse for" << safeDuration << "ms";
            }
            break;

        case ConsequenceAction::COMBINED_ASSAULT:
            if (m_subscriptionTier == SubscriptionTier::PREMIUM) {
                // Execute both TENS and vacuum
                executePunishment(ConsequenceAction::TENS_SHOCK, intensity, durationMs, QString());
                executePunishment(ConsequenceAction::MAX_VACUUM_PULSE, intensity, durationMs, QString());
            }
            break;

        default:
            break;
    }
}

QString ConsequenceEngine::actionDescription(ConsequenceAction action) const
{
    switch (action) {
        case ConsequenceAction::UNLOCK_PATTERN: return "Pattern Unlocked";
        case ConsequenceAction::UNLOCK_GAME: return "Game Unlocked";
        case ConsequenceAction::BONUS_XP: return "Bonus XP Awarded";
        case ConsequenceAction::INTENSITY_DECREASE: return "Intensity Decreased";
        case ConsequenceAction::PLEASURE_BURST: return "Pleasure Burst";
        case ConsequenceAction::INTENSITY_INCREASE: return "Intensity Increased";
        case ConsequenceAction::DENIAL_EXTENSION: return "Denial Extended";
        case ConsequenceAction::PATTERN_SWITCH: return "Pattern Switched";
        case ConsequenceAction::AROUSAL_MAINTENANCE: return "Arousal Maintenance";
        case ConsequenceAction::FORCED_EDGE: return "Forced Edge";
        case ConsequenceAction::TENS_SHOCK: return "TENS Shock";
        case ConsequenceAction::TENS_BURST_SERIES: return "TENS Burst Series";
        case ConsequenceAction::MAX_VACUUM_PULSE: return "Max Vacuum Pulse";
        case ConsequenceAction::COMBINED_ASSAULT: return "Combined Assault";
        case ConsequenceAction::RANDOM_SHOCK_INTERVAL: return "Random Shock Interval";
        default: return "Unknown";
    }
}

