#ifndef ORGASMCONTROLALGORITHM_H
#define ORGASMCONTROLALGORITHM_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <atomic>
#include <cmath>
#include "../safety/SafetyConstants.h"

class HardwareManager;
class SensorInterface;
class ClitoralOscillator;
class TENSController;
class HeartRateSensor;
class FluidSensor;

/**
 * @brief Adaptive orgasm control algorithm with arousal detection
 *
 * Implements multi-sensor arousal estimation using:
 * - Pressure sensors (clitoral engorgement, contractions)
 * - Heart rate sensor (HR zone, HRV, acceleration)
 * - TENS feedback (if enabled)
 *
 * Control modes:
 * - Adaptive Edging: Sensor-driven approach/deny cycles
 * - Forced Orgasm: Relentless stimulation through multiple climaxes
 * - Multi-Orgasm Training: Sequential orgasm facilitation
 * - Denial Mode: Extended teasing without release
 */
class OrgasmControlAlgorithm : public QObject
{
    Q_OBJECT

public:
    // Arousal states based on physiological indicators
    enum class ArousalState {
        BASELINE,      // 0.0-0.2: Resting state
        WARMING,       // 0.2-0.5: Early arousal
        PLATEAU,       // 0.5-0.85: Sustained high arousal
        PRE_ORGASM,    // 0.85-0.95: Approaching climax
        ORGASM         // 0.95-1.0: Active orgasm with contractions
    };
    Q_ENUM(ArousalState)

    // Algorithm control states
    enum class ControlState {
        STOPPED,
        CALIBRATING,
        BUILDING,
        BACKING_OFF,
        HOLDING,
        FORCING,
        MILKING,           // Milking mode: maintaining arousal in milking zone
        DANGER_REDUCTION,  // Milking mode: arousal too high, reducing stimulation
        ORGASM_FAILURE,    // Milking mode: unwanted orgasm detected
        COOLING_DOWN,
        ERROR
    };
    Q_ENUM(ControlState)

    // Operating modes
    enum class Mode {
        MANUAL,
        ADAPTIVE_EDGING,
        FORCED_ORGASM,
        MULTI_ORGASM,
        DENIAL,
        MILKING            // Sustained sub-orgasmic stimulation (orgasm = failure)
    };
    Q_ENUM(Mode)

    // Bug #26 fix: Q_PROPERTY declarations for serialization and QML bindings
    Q_PROPERTY(double edgeThreshold READ edgeThreshold WRITE setEdgeThreshold NOTIFY edgeThresholdChanged)
    Q_PROPERTY(double orgasmThreshold READ orgasmThreshold WRITE setOrgasmThreshold NOTIFY orgasmThresholdChanged)
    Q_PROPERTY(double recoveryThreshold READ recoveryThreshold WRITE setRecoveryThreshold NOTIFY recoveryThresholdChanged)
    Q_PROPERTY(double arousalLevel READ getArousalLevel NOTIFY arousalLevelChanged)
    Q_PROPERTY(ControlState state READ getState NOTIFY stateChanged)
    Q_PROPERTY(Mode mode READ getMode NOTIFY modeChanged)
    Q_PROPERTY(bool tensEnabled READ isTENSEnabled WRITE setTENSEnabled)
    Q_PROPERTY(bool antiEscapeEnabled READ isAntiEscapeEnabled WRITE setAntiEscapeEnabled)
    Q_PROPERTY(bool verboseLogging READ isVerboseLogging WRITE setVerboseLogging)

public:
    /**
     * @brief Constructs the orgasm control algorithm
     * @param hardware Pointer to hardware manager (required, must not be null)
     * @param parent QObject parent for memory management
     */
    explicit OrgasmControlAlgorithm(HardwareManager* hardware, QObject* parent = nullptr);
    ~OrgasmControlAlgorithm();

    // ========================================================================
    // Control Methods - Bug #20 fix: Added Doxygen documentation
    // ========================================================================

    /**
     * @brief Start adaptive edging session with sensor-driven approach/deny cycles
     * @param targetCycles Number of edge cycles to complete before orgasm (default: 5)
     * @note Session enters CALIBRATING state first, then BUILDING
     */
    void startAdaptiveEdging(int targetCycles = 5);

    /**
     * @brief Start forced orgasm session with relentless stimulation
     * @param targetOrgasms Number of orgasms to induce (default: 3)
     * @param maxDurationMs Maximum session duration in milliseconds (default: 30 min)
     */
    void startForcedOrgasm(int targetOrgasms = 3, int maxDurationMs = 1800000);

    /**
     * @brief Start denial session (extended teasing without release)
     * @param durationMs Duration of denial period in milliseconds (default: 10 min)
     */
    void startDenial(int durationMs = 600000);

    /**
     * @brief Start milking session (sustained sub-orgasmic stimulation)
     * @param durationMs Session duration in milliseconds (default: 30 min)
     * @param failureMode What to do on orgasm: 0=stop, 1=ruined, 2=punish, 3=continue
     */
    void startMilking(int durationMs = 1800000, int failureMode = 0);

    /**
     * @brief Stop current session gracefully with cooldown
     */
    void stop();

    /**
     * @brief Emergency stop - immediate cessation with safety venting
     * @note Thread-safe: can be called from any thread
     */
    void emergencyStop();

    // ========================================================================
    // Configuration - Bug #20 fix: Added Doxygen documentation
    // ========================================================================

    /**
     * @brief Set arousal threshold for edge detection
     * @param threshold Value between 0.5 and 0.95 (validated)
     */
    void setEdgeThreshold(double threshold);
    double edgeThreshold() const { return m_edgeThreshold; }

    /**
     * @brief Set arousal threshold for orgasm detection
     * @param threshold Value between 0.85 and 1.0 (validated)
     */
    void setOrgasmThreshold(double threshold);
    double orgasmThreshold() const { return m_orgasmThreshold; }

    /**
     * @brief Set arousal threshold for recovery from edge
     * @param threshold Value between 0.3 and 0.8 (validated)
     */
    void setRecoveryThreshold(double threshold);
    double recoveryThreshold() const { return m_recoveryThreshold; }

    /**
     * @brief Enable or disable TENS unit integration
     * @param enabled True to enable TENS stimulation
     */
    void setTENSEnabled(bool enabled);
    bool isTENSEnabled() const { return m_tensEnabled; }

    /**
     * @brief Enable or disable anti-escape intensity boost
     * @param enabled True to boost intensity when arousal drops unexpectedly
     */
    void setAntiEscapeEnabled(bool enabled);
    bool isAntiEscapeEnabled() const { return m_antiEscapeEnabled; }

    // Milking mode configuration
    void setMilkingZoneLower(double threshold);
    void setMilkingZoneUpper(double threshold);
    void setDangerThreshold(double threshold);
    void setMilkingFailureMode(int mode);

    // Heart rate sensor configuration
    void setHeartRateSensor(HeartRateSensor* sensor);
    void setHeartRateEnabled(bool enabled);
    void setHeartRateWeight(double weight);  // Weight in arousal calculation (0.0-0.5)

    // Debug/diagnostic configuration
    void setVerboseLogging(bool enabled) { m_verboseLogging = enabled; }
    bool isVerboseLogging() const { return m_verboseLogging; }

    // ========================================================================
    // Status Getters - Thread-safe access for UI
    // ========================================================================

    ControlState getState() const { return m_state.load(std::memory_order_acquire); }
    Mode getMode() const { return m_mode.load(std::memory_order_acquire); }
    double getArousalLevel() const { return m_arousalLevel; }
    ArousalState getArousalState() const { return m_arousalState.load(std::memory_order_acquire); }
    int getEdgeCount() const { return m_edgeCount; }
    int getOrgasmCount() const { return m_orgasmCount; }
    double getCurrentIntensity() const { return m_intensity; }
    double getCurrentFrequency() const { return m_frequency; }
    int getCurrentHeartRate() const { return m_currentHeartRate; }
    double getHeartRateContribution() const { return m_heartRateContribution; }
    QVector<double> getArousalHistory() const;

    // Milking mode status getters
    qint64 getMilkingZoneTime() const { return m_milkingZoneTime; }
    int getDangerZoneEntries() const { return m_dangerZoneEntries; }
    bool isMilkingSuccess() const { return m_milkingOrgasmCount == 0; }
    int getMilkingOrgasmCount() const { return m_milkingOrgasmCount; }
    double getMilkingAvgArousal() const { return m_milkingAvgArousal; }

Q_SIGNALS:
    // Arousal signals
    void arousalLevelChanged(double level);
    void arousalStateChanged(ArousalState state);
    
    // Edging signals
    void edgeDetected(int edgeNumber, double intensityAtEdge);
    void edgeCycleCompleted(int current, int total);
    void edgingComplete(int totalEdges);
    void buildUpProgress(double arousal, double threshold);
    void backOffProgress(double arousal, double threshold);
    void unexpectedOrgasmDuringEdging(int orgasmCount, int edgeCount);  // Orgasm occurred during edging
    void pointOfNoReturnReached(int edgeCount);  // Arousal rising during back-off, orgasm inevitable

    // Forced orgasm signals
    void orgasmDetected(int orgasmNumber, qint64 timeMs);
    void antiEscapeTriggered(double newIntensity, double newFrequency);
    void forcedOrgasmProgress(int orgasms, int target, qint64 elapsed, qint64 max);
    void forcedOrgasmComplete(int totalOrgasms, qint64 durationMs);
    
    // Safety signals
    void emergencyStopActivated();
    void sealIntegrityWarning(double pressure);
    void sealLostEmergencyStop();  // Bug #17: Emitted when seal cannot be re-established
    void resealAttemptStarted();   // Bug #15: Emitted when attempting to re-establish seal
    void overpressureWarning(double pressure);
    void tissueProtectionTriggered();
    void sessionTimeoutWarning();
    void tensFault(const QString& reason);
    void sensorError(const QString& sensor, const QString& error);  // Bug #12: Sensor failure detected

    // Heart rate signals
    void heartRateUpdated(int bpm, double contribution);
    void heartRateOrgasmSignature();  // HR pattern indicates orgasm
    void heartRateSensorLost();

    // Fluid tracking signals
    void fluidVolumeUpdated(double currentMl, double cumulativeMl);
    void fluidOrgasmBurst(double volumeMl, int orgasmNumber);
    void lubricationRateChanged(double mlPerMin);
    void fluidOverflowWarning(double volumeMl);

    // State signals
    void stateChanged(ControlState state);
    void modeChanged(Mode mode);

    // Bug #26 fix: Configuration changed signals for Q_PROPERTY bindings
    void edgeThresholdChanged(double threshold);
    void orgasmThresholdChanged(double threshold);
    void recoveryThresholdChanged(double threshold);

    // Milking mode signals
    void milkingZoneEntered(double arousalLevel);
    void milkingZoneMaintained(qint64 durationMs, double avgArousal);
    void dangerZoneEntered(double arousalLevel);
    void dangerZoneExited(double arousalLevel);
    void unwantedOrgasm(int orgasmCount, qint64 sessionDurationMs);
    void milkingIntensityAdjusted(double newIntensity, double arousalError);
    void milkingSessionComplete(qint64 durationMs, bool success, int dangerEntries);

private Q_SLOTS:
    void onUpdateTick();
    void onSafetyCheck();

private:
    // Arousal detection methods
    void calibrateBaseline(int durationMs);
    void updateArousalLevel();
    // Bug #3 fix: Accept currentIdx parameter to prevent multiple atomic loads per cycle
    double calculateArousalLevel(int currentIdx);
    // Bug #1 fix: Pass currentIdx to ensure consistent index across all calculations
    double calculateVariance(const QVector<double>& data, int windowSize, int currentIdx);
    double calculateBandPower(const QVector<double>& data, double freqLow, double freqHigh, int currentIdx);
    double calculateDerivative(const QVector<double>& data, int currentIdx);
    bool detectContractions();
    void updateArousalState();
    
    // Algorithm execution methods
    void runAdaptiveEdging();
    void runForcedOrgasm();
    void runMilking();
    void startCoolDown();
    void startAdaptiveEdgingInternal(int targetCycles);
    void startMilkingInternal(int durationMs, int failureMode);
    double calculateMilkingIntensityAdjustment();
    void handleMilkingOrgasmFailure();
    
    // Safety methods
    void performSafetyCheck();
    void handleEmergencyStop();
    
    // Utility methods
    void setState(ControlState state);
    void setMode(Mode mode);
    static double clamp(double value, double min, double max);

    /**
     * @brief Resets all session state to prepare for a new session
     *
     * Consolidates common state reset logic shared across startAdaptiveEdging,
     * startForcedOrgasm, and startMilking methods. Thread-safe: assumes caller holds m_mutex.
     *
     * Resets:
     * - Session counters (edgeCount, orgasmCount)
     * - Safety tracking (highPressureDuration, sealLossCount)
     * - Arousal state (arousalLevel, smoothedArousal, previousArousal)
     * - Orgasm detection state (inOrgasm, pointOfNoReturnReached)
     * - Calibration state (calibSumClitoral, calibSumAVL, calibSamples)
     * - Fluid tracking (sessionFluidMl, lubricationMl, etc.)
     * - Milking-specific state (milkingZoneTime, dangerZoneEntries, PID state)
     * - Starts fluid sensor session if enabled
     */
    void resetSessionState();

    /**
     * @brief Start all session timers
     *
     * Consolidates duplicate timer start logic from startAdaptiveEdgingInternal,
     * startForcedOrgasm, and startMilkingInternal. Starts m_sessionTimer, m_stateTimer,
     * m_updateTimer, and m_safetyTimer. Thread-safe: assumes caller holds m_mutex.
     */
    void startSessionTimers();

    // Hardware interfaces
    HardwareManager* m_hardware;
    SensorInterface* m_sensorInterface;
    ClitoralOscillator* m_clitoralOscillator;
    TENSController* m_tensController;
    HeartRateSensor* m_heartRateSensor;
    FluidSensor* m_fluidSensor;

    // Timers
    QTimer* m_updateTimer;
    QTimer* m_safetyTimer;
    QElapsedTimer m_sessionTimer;

    // State
    // LOW-3 fix: Made atomic for thread-safe access from UI thread via getters
    std::atomic<ControlState> m_state;
    std::atomic<Mode> m_mode;
    bool m_emergencyStop;
    bool m_tensEnabled;
    bool m_antiEscapeEnabled;
    bool m_heartRateEnabled;
    bool m_verboseLogging;  // Non-critical fix: Reduce logging overhead in high-frequency paths

    // Arousal tracking
    double m_arousalLevel;
    double m_smoothedArousal;
    std::atomic<ArousalState> m_arousalState;  // Bug #11 fix: atomic for thread-safe access
    double m_baselineClitoral;
    double m_baselineAVL;
    QVector<double> m_pressureHistory;
    QVector<double> m_arousalHistory;
    std::atomic<int> m_historyIndex;  // Bug #1 fix: atomic to prevent race condition

    // Heart rate tracking
    int m_currentHeartRate;
    int m_baselineHeartRate;
    double m_heartRateContribution;  // HR's contribution to arousal (0.0-1.0)
    double m_heartRateWeight;        // Weight in arousal formula

    // Stimulation parameters
    double m_intensity;
    double m_frequency;
    double m_tensAmplitude;

    // Counters
    int m_edgeCount;
    int m_orgasmCount;
    int m_targetEdges;
    int m_targetOrgasms;
    int m_maxDurationMs;
    qint64 m_highPressureDuration;

    // Thresholds (configurable)
    double m_edgeThreshold;
    double m_orgasmThreshold;
    double m_recoveryThreshold;

    // Milking mode thresholds
    double m_milkingZoneLower;      // Lower bound of milking zone (default 0.75)
    double m_milkingZoneUpper;      // Upper bound of milking zone (default 0.90)
    double m_dangerThreshold;       // Danger zone threshold (default 0.92)

    // Internal state for algorithm logic
    QElapsedTimer m_stateTimer; // General purpose timer for states like HOLDING, BACKING_OFF
    double m_previousArousal;
    bool m_inOrgasm;
    bool m_pointOfNoReturnReached;     // Orgasm became inevitable during back-off
    int m_unexpectedOrgasmCount;       // Orgasms that occurred during edging (not forced)

    // Internal state for calibration
    double m_calibSumClitoral;
    double m_calibSumAVL;
    int m_calibSamples;

    // Internal state for cooldown
    double m_cooldownStartIntensity;
    double m_cooldownStartFrequency;

    // Bug #15, #16, #17: Internal state for seal integrity monitoring
    int m_sealLossCount;           // Consecutive seal loss detections
    bool m_resealAttemptInProgress;
    QElapsedTimer m_resealTimer;   // Tracks duration of re-seal boost

    // Arousal-adaptive seal integrity (differentiates swelling from leak)
    double m_previousAVLPressure;  // For rate of change calculation
    double m_previousClitoralPressure;  // For detecting tissue swelling

    // Milking mode state
    int m_milkingFailureMode;       // 0=stop, 1=ruined, 2=punish, 3=continue
    int m_milkingOrgasmCount;       // Orgasms during milking (should be 0)
    int m_dangerZoneEntries;        // Times arousal exceeded danger threshold
    qint64 m_milkingZoneTime;       // Cumulative time in milking zone (ms)
    double m_milkingAvgArousal;     // Running average arousal in zone
    int m_milkingAvgSamples;        // Sample count for running average

    // Milking PID control state
    double m_milkingIntegralError;  // Accumulated error for I term
    double m_milkingPreviousError;  // Previous error for D term
    double m_milkingTargetArousal;  // Center of milking zone (0.82)

    // Thread safety
    mutable QMutex m_mutex;

    // Fluid tracking
    bool m_fluidTrackingEnabled;
    double m_sessionFluidMl;
    double m_lubricationMl;
    double m_orgasmicFluidMl;
    QVector<double> m_fluidPerOrgasm;  // Volume per orgasm event

    // Constants
    static const int UPDATE_INTERVAL_MS = 100;
    static const int SAFETY_INTERVAL_MS = 100;
    static const int BASELINE_DURATION_MS = 10000;
    static const int HISTORY_SIZE = 200;
    static const int VARIANCE_WINDOW_SAMPLES = 100;

    // HIGH-5 fix: Minimum calibration samples required for valid baseline
    // At 100ms intervals over 10 seconds, we expect ~100 samples; require at least 50
    static const int MIN_CALIBRATION_SAMPLES = 50;

    // Pressure validation: Use SafetyConstants::MIN_VALID_PRESSURE (0.0 mmHg)
    // For max valid, this algorithm uses a stricter limit (100 mmHg) than SafetyConstants (200 mmHg)
    // because readings above ~100 mmHg in the control context indicate sensor malfunction
    // (MPX5010DP sensor max is 75 mmHg, so 100 mmHg provides reasonable margin for noise)
    static constexpr double PRESSURE_MAX_VALID_CONTROL = 100.0;  // Stricter than SafetyConstants

    // Arousal calculation weights (pressure-based, sum to ~0.70 when HR enabled)
    static constexpr double WEIGHT_DEVIATION = 0.25;
    static constexpr double WEIGHT_VARIANCE = 0.15;
    static constexpr double WEIGHT_CONTRACTION = 0.20;
    static constexpr double WEIGHT_ROC = 0.10;
    static constexpr double AROUSAL_ALPHA = 0.15;

    // Heart rate arousal weights (default 0.30 when enabled)
    static constexpr double DEFAULT_HR_WEIGHT = 0.30;
    static constexpr double WEIGHT_HR_ZONE = 0.50;       // HR zone contribution
    static constexpr double WEIGHT_HR_ACCEL = 0.25;      // HR acceleration contribution
    static constexpr double WEIGHT_HRV = 0.25;           // HRV (inverted) contribution

    // Normalization maxima
    static constexpr double MAX_DEVIATION = 0.5;
    static constexpr double MAX_VARIANCE = 25.0;
    static constexpr double MAX_CONTRACTION_POWER = 10.0;
    static constexpr double MAX_RATE_OF_CHANGE = 5.0;

    // Edging defaults
    static constexpr double DEFAULT_EDGE_THRESHOLD = 0.85;
    static constexpr double DEFAULT_RECOVERY_THRESHOLD = 0.70;
    static constexpr double DEFAULT_ORGASM_THRESHOLD = 0.95;
    static constexpr double INITIAL_INTENSITY = 0.30;
    static constexpr double INITIAL_FREQUENCY = 6.0;
    static constexpr double RAMP_RATE = 0.005;
    static constexpr double FREQ_RAMP_RATE = 0.02;
    static constexpr double BACKOFF_PRESSURE = 0.10;
    static constexpr double HOLD_PRESSURE = 0.20;
    static constexpr double HOLD_FREQUENCY = 5.0;
    static constexpr double HOLD_AMPLITUDE = 0.30;
    static constexpr double ESCALATION_RATE = 0.05;
    static constexpr int MIN_BACKOFF_MS = 5000;
    static constexpr int HOLD_DURATION_MS = 5000;

    // Point-of-no-return and unexpected orgasm handling
    static constexpr double PONR_AROUSAL_RISE_THRESHOLD = 0.02;  // 2% rise during back-off = orgasm inevitable
    static constexpr int POST_UNEXPECTED_ORGASM_RECOVERY_MS = 30000;  // 30 sec recovery after unexpected orgasm

    // Forced orgasm defaults
    static constexpr double FORCED_BASE_INTENSITY = 0.60;
    static constexpr double FORCED_BASE_FREQUENCY = 10.0;
    static constexpr double FORCED_TENS_AMPLITUDE = 0.80;
    static constexpr double AROUSAL_DROP_THRESHOLD = 0.05;
    static constexpr double ANTI_ESCAPE_RATE = 0.02;
    static constexpr double ANTI_ESCAPE_FREQ_RATE = 0.1;
    static constexpr double THROUGH_ORGASM_BOOST = 0.05;
    static constexpr double TENS_FORCED_FREQUENCY = 25.0;
    static constexpr int ORGASM_DURATION_MS = 20000;  // Scientific literature: avg 19.9-35 sec
    static constexpr int POST_ORGASM_PAUSE_MS = 2000;
    static constexpr int COOLDOWN_DURATION_MS = 60000;

    // Safety limits
    static constexpr double SEAL_LOST_THRESHOLD = 10.0;
    static constexpr double MAX_SAFE_CLITORAL_PRESSURE = 80.0;
    // Removed MAX_OUTER_PRESSURE - use SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG instead
    static constexpr double MAX_CLITORAL_AMPLITUDE = 60.0;
    static constexpr double MAX_INTENSITY = 0.95;
    static constexpr double MAX_FREQUENCY = 13.0;
    static constexpr int MAX_SESSION_DURATION_MS = 1800000;
    static constexpr int MAX_HIGH_PRESSURE_DURATION_MS = 120000;

    // Bug #15, #16, #17: Seal integrity response constants
    static constexpr int RESEAL_BOOST_DURATION_MS = 500;     // How long to boost vacuum for re-seal
    static constexpr int RESEAL_ATTEMPT_THRESHOLD = 3;       // Failures before reducing intensity
    static constexpr int RESEAL_INTENSITY_THRESHOLD = 6;     // Failures before emergency stop
    static constexpr int SEAL_EMERGENCY_THRESHOLD = 7;       // Persistent seal loss triggers emergency stop
    // Non-critical fix: Named constant for seal emergency duration (replaces magic 700ms comment)
    static constexpr int SEAL_EMERGENCY_DURATION_MS = SEAL_EMERGENCY_THRESHOLD * SAFETY_INTERVAL_MS;  // 700ms

    // Arousal-adaptive seal integrity constants (differentiate swelling from leak)
    static constexpr double SEAL_AROUSAL_COMPENSATION_FACTOR = 0.5;  // Max 50% threshold reduction at full arousal
    static constexpr double RAPID_PRESSURE_DROP_THRESHOLD = 5.0;     // mmHg per 100ms - indicates sudden leak

    // Milking mode defaults
    static constexpr double DEFAULT_MILKING_ZONE_LOWER = 0.75;
    static constexpr double DEFAULT_MILKING_ZONE_UPPER = 0.90;
    static constexpr double DEFAULT_DANGER_THRESHOLD = 0.92;
    static constexpr double MILKING_TARGET_AROUSAL = 0.82;  // Center of milking zone

    // Milking intensity control
    static constexpr double MILKING_BASE_INTENSITY = 0.50;
    static constexpr double MILKING_BASE_FREQUENCY = 7.0;
    static constexpr double MILKING_MIN_INTENSITY = 0.20;
    static constexpr double MILKING_MAX_INTENSITY = 0.70;
    static constexpr double MILKING_TENS_AMPLITUDE = 0.40;

    // Milking PID gains for arousal maintenance
    static constexpr double MILKING_PID_KP = 0.30;   // Proportional gain
    static constexpr double MILKING_PID_KI = 0.05;   // Integral gain
    static constexpr double MILKING_PID_KD = 0.10;   // Derivative gain

    // Milking timing
    static constexpr int MILKING_ZONE_REPORT_INTERVAL_MS = 10000;  // Report every 10 sec
    static constexpr int MILKING_MAX_SESSION_MS = 3600000;  // 60 min max

    // Danger zone recovery
    static constexpr double DANGER_RECOVERY_THRESHOLD = 0.88;  // Exit danger when below this
    static constexpr int DANGER_PAUSE_DURATION_MS = 2000;      // Pause stimulation duration
};

#endif // ORGASMCONTROLALGORITHM_H

