#include "ConsequenceEngine.h"
#include "../control/OrgasmControlAlgorithm.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/TENSController.h"
#include "../hardware/ClitoralOscillator.h"
#include "../network/MultiUserController.h"
#include <QDateTime>
#include <QDebug>
#include <QSoundEffect>
#include <QUrl>
#include <QFile>

ConsequenceEngine::ConsequenceEngine(HardwareManager* hardware,
                                     OrgasmControlAlgorithm* orgasmControl,
                                     QObject* parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_orgasmControl(orgasmControl)
    , m_tensController(hardware ? hardware->getTENSController() : nullptr)
    , m_clitoralOscillator(nullptr)  // Will be set if available
    , m_subscriptionTier(SubscriptionTier::BASIC)
    , m_paused(false)
    , m_queueTimer(new QTimer(this))
    , m_cooldownTimer(new QTimer(this))
    , m_lastTensTime(0)
    , m_tensShocksInBurst(0)
    , m_punishmentsThisSession(0)
    , m_rewardsThisSession(0)
    , m_audioEnabled(true)
    , m_hapticEnabled(true)
    , m_audioVolume(0.8)
    , m_audioPath("data/sounds/")
    , m_soundEffect(new QSoundEffect(this))
    , m_warningEscalationLevel(0)
    , m_lastWarningTime(0)
    , m_multiUserController(nullptr)
{
    connect(m_queueTimer, &QTimer::timeout, this, &ConsequenceEngine::processQueue);
    connect(m_cooldownTimer, &QTimer::timeout, this, &ConsequenceEngine::onCooldownExpired);

    m_queueTimer->setInterval(QUEUE_PROCESS_INTERVAL_MS);
    m_soundEffect->setVolume(m_audioVolume);
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
    resetWarningEscalation();
}

void ConsequenceEngine::resetWarningEscalation()
{
    m_warningEscalationLevel = 0;
    m_lastWarningTime = 0;
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
        case ConsequenceAction::MOTION_VIOLATION_SHOCK:
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
                                          int durationMs, const QString& targetId)
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

        case ConsequenceAction::MOTION_WARNING:
            qDebug() << "Motion Warning: Movement detected, stay still!";
            // Could trigger audio/visual warning
            break;

        case ConsequenceAction::MOTION_VIOLATION_SHOCK:
            if (m_tensController && m_subscriptionTier == SubscriptionTier::PREMIUM) {
                // Moderate shock for movement violation
                double violationAmplitude = qMin(intensity * 80.0, m_limits.maxTensAmplitudePercent);
                int violationDuration = qMin(200, m_limits.maxTensDurationMs);

                m_tensController->setAmplitude(violationAmplitude / 100.0);
                m_tensController->pulse(violationDuration);

                m_lastTensTime = QDateTime::currentMSecsSinceEpoch();
                m_tensShocksInBurst++;
                m_cooldownTimer->start(m_limits.minTensCooldownMs);

                qDebug() << "Punishment: Motion violation shock at" << violationAmplitude
                         << "% for" << violationDuration << "ms";
            }
            break;

        case ConsequenceAction::MOTION_ESCALATION:
            // Escalating punishment based on violation count
            if (m_subscriptionTier == SubscriptionTier::PREMIUM) {
                // First 3 violations: warning + light shock
                // 4-6 violations: moderate shock
                // 7+ violations: combined assault
                int violationLevel = static_cast<int>(intensity * 10);  // intensity encodes violation count

                if (violationLevel <= 3) {
                    executePunishment(ConsequenceAction::MOTION_VIOLATION_SHOCK, 0.3, 150, QString());
                } else if (violationLevel <= 6) {
                    executePunishment(ConsequenceAction::MOTION_VIOLATION_SHOCK, 0.5, 250, QString());
                } else {
                    executePunishment(ConsequenceAction::COMBINED_ASSAULT, 0.6, 300, QString());
                }

                qDebug() << "Punishment: Motion escalation level" << violationLevel;
            } else {
                // Basic tier: increase vacuum intensity
                executePunishment(ConsequenceAction::INTENSITY_INCREASE, intensity, durationMs, QString());
            }
            break;

        case ConsequenceAction::AUDIO_WARNING:
            playAudioWarning(targetId.isEmpty() ? "warning.wav" : targetId);
            qDebug() << "Punishment: Audio warning played";
            break;

        case ConsequenceAction::AUDIO_ANNOUNCEMENT:
            playAudioWarning(targetId.isEmpty() ? "announcement.wav" : targetId);
            qDebug() << "Punishment: Audio announcement played";
            break;

        case ConsequenceAction::HAPTIC_PULSE:
            triggerHapticPulse(intensity, durationMs, 1);
            qDebug() << "Punishment: Haptic pulse at" << intensity << "for" << durationMs << "ms";
            break;

        case ConsequenceAction::HAPTIC_PATTERN:
            // Complex pattern: multiple pulses with varying intensity
            triggerHapticPulse(intensity * 0.5, durationMs / 4, 1);
            triggerHapticPulse(intensity, durationMs / 2, 1);
            triggerHapticPulse(intensity * 0.7, durationMs / 4, 1);
            qDebug() << "Punishment: Haptic pattern executed";
            break;

        case ConsequenceAction::AUDIO_HAPTIC_COMBINED:
            playAudioWarning(targetId.isEmpty() ? "motion_warning.wav" : targetId);
            triggerHapticPulse(intensity, durationMs, 2);
            qDebug() << "Punishment: Combined audio + haptic warning";
            break;

        case ConsequenceAction::PROGRESSIVE_WARNING:
            executeProgressiveWarning();
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
        case ConsequenceAction::MOTION_WARNING: return "Motion Warning";
        case ConsequenceAction::MOTION_VIOLATION_SHOCK: return "Motion Violation Shock";
        case ConsequenceAction::MOTION_ESCALATION: return "Motion Escalation";
        case ConsequenceAction::AUDIO_WARNING: return "Audio Warning";
        case ConsequenceAction::AUDIO_ANNOUNCEMENT: return "Audio Announcement";
        case ConsequenceAction::HAPTIC_PULSE: return "Haptic Pulse";
        case ConsequenceAction::HAPTIC_PATTERN: return "Haptic Pattern";
        case ConsequenceAction::AUDIO_HAPTIC_COMBINED: return "Audio + Haptic Warning";
        case ConsequenceAction::PROGRESSIVE_WARNING: return "Progressive Warning";
        default: return "Unknown";
    }
}

// ============================================================================
// Audio/Haptic Feedback Implementation
// ============================================================================

void ConsequenceEngine::playAudioWarning(const QString& soundFile)
{
    if (!m_audioEnabled) {
        qDebug() << "Audio disabled, skipping:" << soundFile;
        return;
    }

    QString fullPath = m_audioPath + soundFile;

    // Check if file exists
    if (!QFile::exists(fullPath)) {
        qDebug() << "Audio file not found:" << fullPath;
        return;
    }

    m_soundEffect->setSource(QUrl::fromLocalFile(fullPath));
    m_soundEffect->setVolume(m_audioVolume);
    m_soundEffect->play();

    emit audioWarningPlayed(soundFile);
    qDebug() << "Playing audio:" << fullPath << "at volume" << m_audioVolume;
}

void ConsequenceEngine::triggerHapticPulse(double intensity, int durationMs, int pulseCount)
{
    if (!m_hapticEnabled) {
        qDebug() << "Haptic disabled, skipping pulse";
        return;
    }

    // Clamp intensity to safe range
    intensity = qBound(0.0, intensity, 1.0);

    // Use clitoral oscillator for haptic feedback via SOL4/SOL5 vacuum oscillation
    if (m_clitoralOscillator) {
        // Oscillate vacuum for tactile feedback
        for (int i = 0; i < pulseCount; i++) {
            m_clitoralOscillator->pulse(intensity, durationMs);
        }
        qDebug() << "Haptic pulse via ClitoralOscillator:" << intensity << "x" << pulseCount;
    } else if (m_hardware) {
        // Fallback: use hardware manager directly for vacuum oscillation
        // This creates a brief vacuum pulse in the clitoral cylinder
        double targetPressure = intensity * 40.0;  // Max 40 mmHg for haptic
        int pulseDuration = qMin(durationMs, 500);  // Max 500ms per pulse

        for (int i = 0; i < pulseCount; i++) {
            // Queue vacuum pulses with gaps
            QueuedConsequence qc;
            qc.action = ConsequenceAction::MAX_VACUUM_PULSE;
            qc.intensity = targetPressure / 65.0;  // Normalize to 0-1
            qc.durationMs = pulseDuration;
            qc.scheduledTime = QDateTime::currentMSecsSinceEpoch() + (i * (pulseDuration + 100));
            m_queue.enqueue(qc);
        }

        if (!m_queueTimer->isActive()) {
            m_queueTimer->start();
        }

        qDebug() << "Haptic pulse via vacuum:" << targetPressure << "mmHg x" << pulseCount;
    }

    emit hapticFeedbackTriggered(intensity, durationMs);
}

void ConsequenceEngine::executeProgressiveWarning()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Check if we should reset escalation (cooldown expired)
    if (now - m_lastWarningTime > WARNING_ESCALATION_COOLDOWN_MS) {
        m_warningEscalationLevel = 0;
    }

    m_lastWarningTime = now;
    m_warningEscalationLevel++;

    emit warningEscalated(m_warningEscalationLevel);

    // Progressive escalation: audio → haptic → TENS
    switch (m_warningEscalationLevel) {
        case 1:
            // Level 1: Audio warning only
            playAudioWarning("warning_level1.wav");
            qDebug() << "Progressive warning level 1: Audio only";
            break;

        case 2:
            // Level 2: Audio + light haptic
            playAudioWarning("warning_level2.wav");
            triggerHapticPulse(0.3, 200, 1);
            qDebug() << "Progressive warning level 2: Audio + light haptic";
            break;

        case 3:
            // Level 3: Audio + stronger haptic
            playAudioWarning("warning_level3.wav");
            triggerHapticPulse(0.5, 300, 2);
            qDebug() << "Progressive warning level 3: Audio + strong haptic";
            break;

        case 4:
            // Level 4: Audio + haptic + light TENS (premium only)
            playAudioWarning("warning_level4.wav");
            triggerHapticPulse(0.6, 200, 2);
            if (m_subscriptionTier == SubscriptionTier::PREMIUM) {
                executePunishment(ConsequenceAction::TENS_SHOCK, 0.3, 100, QString());
            }
            qDebug() << "Progressive warning level 4: Audio + haptic + light TENS";
            break;

        default:
            // Level 5+: Full escalation
            playAudioWarning("warning_max.wav");
            triggerHapticPulse(0.8, 300, 3);
            if (m_subscriptionTier == SubscriptionTier::PREMIUM) {
                executePunishment(ConsequenceAction::MOTION_VIOLATION_SHOCK, 0.5, 200, QString());
            }
            qDebug() << "Progressive warning level" << m_warningEscalationLevel << ": Maximum escalation";
            break;
    }
}

// ============================================================================
// Multi-User Control Integration
// ============================================================================

void ConsequenceEngine::setMultiUserController(MultiUserController* controller)
{
    m_multiUserController = controller;

    if (m_multiUserController) {
        // Connect to receive remote commands
        connect(m_multiUserController, &MultiUserController::commandReceived,
                this, [this](const RemoteCommand& cmd) {
            // Execute the received command locally
            executeAction(cmd.action, cmd.intensity, cmd.durationMs, cmd.senderId);
        });
    }
}

void ConsequenceEngine::executeRemoteCommand(ConsequenceAction action, double intensity,
                                              int durationMs, const QString& targetId)
{
    if (!m_multiUserController) {
        qWarning() << "MultiUserController not set, cannot execute remote command";
        return;
    }

    // Send command through multi-user controller
    // The controller handles consent verification and point deduction
    m_multiUserController->sendCommand(targetId, action, intensity, durationMs);
}
