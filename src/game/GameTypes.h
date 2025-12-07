#ifndef GAMETYPES_H
#define GAMETYPES_H

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QMetaType>

/**
 * @brief Game type categories for V-Contour gamification system
 */
enum class GameType {
    // Edging Games
    EDGE_COUNT,           // Reach edge N times without orgasm
    EDGE_ENDURANCE,       // Maintain near-edge for duration
    
    // Denial Games
    DENIAL_MAINTENANCE,   // Stay above X% arousal for Y minutes
    DENIAL_LIMIT,         // Don't exceed threshold for duration
    
    // Fluid Games
    FLUID_PRODUCTION,     // Produce X mL in time limit
    FLUID_RATE,           // Maintain flow rate above threshold
    
    // Duration Games
    PATTERN_ENDURANCE,    // Complete pattern cycles without stopping
    STIMULATION_MARATHON, // Sustain stimulation for target duration
    
    // Premium Games (subscription required)
    ELECTRODE_AVOIDANCE,  // Avoid triggering TENS sensors
    SHOCK_ROULETTE,       // Random shock intervals to endure
    INTENSITY_CLIMB,      // Survive escalating intensity levels
    
    // Dom/Sub Games
    OBEDIENCE_TRIAL,      // Follow machine commands
    PUNISHMENT_ENDURANCE, // Endure assigned punishment duration

    // Stillness Games
    STILLNESS_CHALLENGE,  // Achieve orgasm while staying still
    FORCED_STILLNESS,     // Endure stimulation without moving

    // Custom
    CUSTOM                // User-defined via JSON
};
Q_DECLARE_METATYPE(GameType)

/**
 * @brief Game state machine states
 */
enum class GameState {
    IDLE,                 // No game active
    INITIALIZING,         // Loading and calibrating
    COUNTDOWN,            // Pre-game countdown
    RUNNING,              // Active gameplay
    PAUSED,               // Temporarily suspended
    VICTORY,              // Player won
    FAILURE,              // Player failed
    TIMEOUT,              // Time limit reached
    POST_GAME,            // Processing results
    SAFEWORD              // Safety stop triggered
};
Q_DECLARE_METATYPE(GameState)

/**
 * @brief Game result outcomes
 */
enum class GameResult {
    NONE,
    VICTORY,
    FAILURE,
    TIMEOUT,
    ABORTED,
    SAFEWORD
};
Q_DECLARE_METATYPE(GameResult)

/**
 * @brief Subscription tiers for content access and monetization
 */
enum class SubscriptionTier {
    FREE,                 // Free tier - basic features only
    BASIC,                // Paid basic tier - standard features
    STANDARD,             // Standard subscription - most features
    PREMIUM,              // Premium subscription - all features
    LIFETIME              // One-time purchase - permanent premium
};
Q_DECLARE_METATYPE(SubscriptionTier)

/**
 * @brief License key types for different purchase options
 */
enum class LicenseType {
    TRIAL,                // 7-day trial key
    MONTHLY,              // Monthly subscription
    YEARLY,               // Yearly subscription (discounted)
    LIFETIME,             // One-time permanent license
    POINT_BUNDLE,         // Consumable point purchase
    FEATURE_UNLOCK        // Specific feature unlock
};
Q_DECLARE_METATYPE(LicenseType)

/**
 * @brief License validation status
 */
enum class LicenseStatus {
    VALID,                // License is active and valid
    EXPIRED,              // License has expired
    INVALID,              // License key is invalid
    REVOKED,              // License was revoked
    EXCEEDED,             // Device limit exceeded
    PENDING,              // Awaiting validation
    OFFLINE               // Cannot validate (offline mode)
};
Q_DECLARE_METATYPE(LicenseStatus)

/**
 * @brief Privilege tiers based on points accumulation
 *
 * Users progress through tiers by earning points from game completion.
 * Higher tiers unlock multi-user control capabilities.
 */
enum class PrivilegeTier {
    BEGINNER,             // 0-1000 points: Local control only, no DOM commands
    INTERMEDIATE,         // 1000-5000: Can issue DOM commands to self
    ADVANCED,             // 5000-15000: Can control paired users, transfer points
    DOM_MASTER            // 15000+: Room control, advanced patterns, any paired user
};
Q_DECLARE_METATYPE(PrivilegeTier)

/**
 * @brief Consent status for multi-user control
 */
enum class ConsentStatus {
    NONE,                 // No consent given
    PENDING,              // Consent request sent, awaiting response
    GRANTED,              // User has consented to being controlled
    REVOKED,              // Previously granted consent has been revoked
    EXPIRED               // Consent timed out
};
Q_DECLARE_METATYPE(ConsentStatus)

/**
 * @brief Point transaction types for audit logging
 */
enum class PointTransactionType {
    // Earnings
    GAME_COMPLETION,      // Points earned from completing a game
    ACHIEVEMENT_BONUS,    // Bonus points from achievements
    STREAK_BONUS,         // Bonus for win streaks
    DAILY_BONUS,          // Daily login/play bonus

    // Spending
    COMMAND_COST,         // Points spent on issuing commands to others
    POINT_TRANSFER,       // Points transferred to another user
    FEATURE_UNLOCK,       // Points spent to unlock premium features

    // Administrative
    ADMIN_ADJUSTMENT,     // Manual adjustment by admin
    REFUND               // Refund of previously spent points
};
Q_DECLARE_METATYPE(PointTransactionType)

/**
 * @brief Objective types for game goals
 */
enum class ObjectiveType {
    PRIMARY,              // Main goal to win
    BONUS,                // Optional bonus points
    HIDDEN                // Secret objectives
};
Q_DECLARE_METATYPE(ObjectiveType)

/**
 * @brief Consequence action types
 */
enum class ConsequenceAction {
    // Rewards
    UNLOCK_PATTERN,
    UNLOCK_GAME,
    BONUS_XP,
    INTENSITY_DECREASE,
    PLEASURE_BURST,
    
    // Basic Punishments
    INTENSITY_INCREASE,
    DENIAL_EXTENSION,
    PATTERN_SWITCH,
    AROUSAL_MAINTENANCE,
    FORCED_EDGE,
    
    // Premium Punishments
    TENS_SHOCK,
    TENS_BURST_SERIES,
    MAX_VACUUM_PULSE,
    COMBINED_ASSAULT,
    RANDOM_SHOCK_INTERVAL,

    // Motion-related
    MOTION_WARNING,        // Warning for detected movement
    MOTION_VIOLATION_SHOCK,// TENS shock for movement violation
    MOTION_ESCALATION,     // Escalating punishment for repeated violations

    // Audio/Haptic Feedback
    AUDIO_WARNING,         // Play warning sound
    AUDIO_ANNOUNCEMENT,    // Play speech/announcement
    HAPTIC_PULSE,          // Vacuum oscillation pulse for tactile feedback
    HAPTIC_PATTERN,        // Complex haptic pattern through SOL4/SOL5
    AUDIO_HAPTIC_COMBINED, // Combined audio + haptic warning

    // Progressive Warning System
    PROGRESSIVE_WARNING    // audio → haptic → TENS shock escalation
};
Q_DECLARE_METATYPE(ConsequenceAction)

/**
 * @brief Dom/Sub command types
 */
enum class DomCommand {
    EDGE_NOW,             // "Edge for me"
    HOLD_AROUSAL,         // "Stay at X% for Y seconds"
    NO_MOVING,            // "Stay perfectly still"
    COUNT_EDGES,          // "You will edge N times"
    DENY_RELEASE,         // "You may not cum"
    PRODUCE_FLUID,        // "Show me how wet you are"
    ENDURE_PUNISHMENT,    // "Take your punishment"
    RANDOM_CHALLENGE      // Machine picks randomly
};
Q_DECLARE_METATYPE(DomCommand)

/**
 * @brief Safety action levels
 */
enum class SafetyAction {
    NONE,
    YELLOW,               // Reduce intensity, pause consequences
    RED,                  // End game, no penalties
    EMERGENCY_STOP        // Immediate halt everything
};
Q_DECLARE_METATYPE(SafetyAction)

/**
 * @brief Achievement categories
 */
enum class AchievementCategory {
    GAMEPLAY,             // Game completion achievements
    CAREER,               // Cumulative milestones
    SKILL,                // Skill-based achievements
    COLLECTION,           // Unlock all of something
    SECRET                // Hidden achievements
};
Q_DECLARE_METATYPE(AchievementCategory)

#endif // GAMETYPES_H

