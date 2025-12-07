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
 * @brief Subscription tiers for content access
 */
enum class SubscriptionTier {
    BASIC,                // Free tier
    PREMIUM               // "Intense Stimulation" subscription
};
Q_DECLARE_METATYPE(SubscriptionTier)

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

