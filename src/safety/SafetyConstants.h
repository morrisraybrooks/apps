#ifndef SAFETYCONSTANTS_H
#define SAFETYCONSTANTS_H

/**
 * @file SafetyConstants.h
 * @brief Centralized safety constants for the A-Contour vacuum therapy system.
 * 
 * This header provides a single source of truth for all safety-critical
 * pressure limits, thresholds, and timing constants used across the
 * safety monitoring subsystem.
 * 
 * IMPORTANT: All safety-related code should use these constants rather than
 * defining local values to ensure consistent behavior and prevent
 * safety-critical inconsistencies.
 */

namespace SafetyConstants {

// ============================================================================
// PRESSURE LIMITS (mmHg)
// ============================================================================

/// Maximum safe operating pressure for stimulation patterns (75 mmHg)
/// Based on MPX5010DP sensor full-scale range and tissue safety margins
constexpr double MAX_PRESSURE_STIMULATION_MMHG = 75.0;

/// Maximum safe operating pressure for therapeutic patterns (50 mmHg)
/// Lower limit for medical/therapeutic use with extended sessions
constexpr double MAX_PRESSURE_THERAPEUTIC_MMHG = 50.0;

/// Warning threshold - triggers alert before reaching max (60 mmHg = 80% of max)
constexpr double WARNING_THRESHOLD_MMHG = 60.0;

/// Emergency stop pressure - immediate shutdown if exceeded (80 mmHg)
constexpr double EMERGENCY_STOP_PRESSURE_MMHG = 80.0;

/// Tissue damage risk threshold - hard limit never to be exceeded (150 mmHg)
/// Exceeding this pressure risks tissue damage and requires immediate intervention
constexpr double TISSUE_DAMAGE_RISK_MMHG = 150.0;

// ============================================================================
// PRESSURE VALIDATION RANGE
// ============================================================================

/// Minimum valid pressure reading (0 mmHg) - below this indicates sensor error
constexpr double MIN_VALID_PRESSURE = 0.0;

/// Maximum valid pressure reading (200 mmHg) - above this indicates sensor error
constexpr double MAX_VALID_PRESSURE = 200.0;

// ============================================================================
// ANTI-DETACHMENT THRESHOLDS
// ============================================================================

/// Default detachment detection threshold (50 mmHg)
/// Pressure drop below this indicates potential cup detachment
constexpr double DEFAULT_DETACHMENT_THRESHOLD_MMHG = 50.0;

/// Hysteresis value to prevent oscillation during threshold crossings (5 mmHg)
constexpr double DEFAULT_HYSTERESIS_MMHG = 5.0;

/// Maximum vacuum increase allowed during anti-detachment response (20%)
constexpr double MAX_VACUUM_INCREASE_PERCENT = 20.0;

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

/// Safety monitoring interval (100 ms = 10 Hz)
constexpr int MONITORING_INTERVAL_MS = 100;

/// Default monitoring rate for lightweight monitor (20 Hz)
constexpr int DEFAULT_MONITORING_RATE_HZ = 20;

/// High-speed monitoring rate for anti-detachment (100 Hz)
constexpr int ANTI_DETACHMENT_MONITORING_RATE_HZ = 100;

/// Sensor timeout - time without valid reading triggers error (1000 ms)
constexpr int SENSOR_TIMEOUT_MS = 1000;

/// Response delay for anti-detachment action (100 ms)
constexpr int ANTI_DETACHMENT_RESPONSE_DELAY_MS = 100;

// ============================================================================
// ERROR HANDLING
// ============================================================================

/// Maximum consecutive errors before triggering emergency stop
constexpr int MAX_CONSECUTIVE_ERRORS = 3;

/// Number of pressure samples to keep in history buffer
constexpr int PRESSURE_HISTORY_SIZE = 10;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Validates if a pressure reading is within the valid sensor range.
 * @param pressure The pressure value to validate (mmHg)
 * @return true if pressure is within valid range, false if sensor error likely
 */
inline bool isValidPressure(double pressure) {
    return pressure >= MIN_VALID_PRESSURE && pressure <= MAX_VALID_PRESSURE;
}

/**
 * @brief Checks if pressure exceeds the emergency stop threshold.
 * @param pressure The current pressure reading (mmHg)
 * @return true if emergency stop should be triggered
 */
inline bool isEmergencyStopRequired(double pressure) {
    return pressure >= EMERGENCY_STOP_PRESSURE_MMHG;
}

/**
 * @brief Checks if pressure exceeds the warning threshold.
 * @param pressure The current pressure reading (mmHg)
 * @return true if warning should be issued
 */
inline bool isWarningLevel(double pressure) {
    return pressure >= WARNING_THRESHOLD_MMHG && pressure < EMERGENCY_STOP_PRESSURE_MMHG;
}

/**
 * @brief Checks if pressure poses tissue damage risk.
 * @param pressure The current pressure reading (mmHg)
 * @return true if tissue damage risk exists
 */
inline bool isTissueDamageRisk(double pressure) {
    return pressure >= TISSUE_DAMAGE_RISK_MMHG;
}

} // namespace SafetyConstants

#endif // SAFETYCONSTANTS_H

