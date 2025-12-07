#ifndef ORGASMCONTROLALGORITHM_H
#define ORGASMCONTROLALGORITHM_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <cmath>

class HardwareManager;
class SensorInterface;
class ClitoralOscillator;
class TENSController;

/**
 * @brief Adaptive orgasm control algorithm with arousal detection
 * 
 * Implements sensor-driven arousal estimation and control modes:
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
        DENIAL
    };
    Q_ENUM(Mode)

    explicit OrgasmControlAlgorithm(HardwareManager* hardware, QObject* parent = nullptr);
    ~OrgasmControlAlgorithm();

    // Control methods
    void startAdaptiveEdging(int targetCycles = 5);
    void startForcedOrgasm(int targetOrgasms = 3, int maxDurationMs = 1800000);
    void startDenial(int durationMs = 600000);
    void stop();
    void emergencyStop();

    // Configuration
    void setEdgeThreshold(double threshold);
    void setOrgasmThreshold(double threshold);
    void setRecoveryThreshold(double threshold);
    void setTENSEnabled(bool enabled);
    void setAntiEscapeEnabled(bool enabled);

    // Status getters
    ControlState getState() const { return m_state; }
    Mode getMode() const { return m_mode; }
    double getArousalLevel() const { return m_arousalLevel; }
    ArousalState getArousalState() const { return m_arousalState; }
    int getEdgeCount() const { return m_edgeCount; }
    int getOrgasmCount() const { return m_orgasmCount; }
    double getCurrentIntensity() const { return m_intensity; }
    double getCurrentFrequency() const { return m_frequency; }
    QVector<double> getArousalHistory() const;

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
    
    // Forced orgasm signals
    void orgasmDetected(int orgasmNumber, qint64 timeMs);
    void antiEscapeTriggered(double newIntensity, double newFrequency);
    void forcedOrgasmProgress(int orgasms, int target, qint64 elapsed, qint64 max);
    void forcedOrgasmComplete(int totalOrgasms, qint64 durationMs);
    
    // Safety signals
    void emergencyStopActivated();
    void sealIntegrityWarning(double pressure);
    void overpressureWarning(double pressure);
    void tissueProtectionTriggered();
    void sessionTimeoutWarning();
    void tensFault(const QString& reason);
    
    // State signals
    void stateChanged(ControlState state);
    void modeChanged(Mode mode);

private Q_SLOTS:
    void onUpdateTick();
    void onSafetyCheck();

private:
    // Arousal detection methods
    void calibrateBaseline(int durationMs);
    void updateArousalLevel();
    double calculateArousalLevel();
    double calculateVariance(const QVector<double>& data, int windowSize);
    double calculateBandPower(const QVector<double>& data, double freqLow, double freqHigh);
    double calculateDerivative(const QVector<double>& data);
    bool detectContractions();
    void updateArousalState();
    
    // Algorithm execution methods
    void runAdaptiveEdging(int targetCycles);
    void runForcedOrgasm(int targetOrgasms, int maxDurationMs);
    void runCoolDown(int durationMs);
    
    // Safety methods
    void performSafetyCheck();
    void handleEmergencyStop();
    
    // Utility methods
    void setState(ControlState state);
    void setMode(Mode mode);
    static double clamp(double value, double min, double max);
    
    // Hardware interfaces
    HardwareManager* m_hardware;
    SensorInterface* m_sensorInterface;
    ClitoralOscillator* m_clitoralOscillator;
    TENSController* m_tensController;

    // Timers
    QTimer* m_updateTimer;
    QTimer* m_safetyTimer;
    QElapsedTimer m_sessionTimer;

    // State
    ControlState m_state;
    Mode m_mode;
    bool m_emergencyStop;
    bool m_tensEnabled;
    bool m_antiEscapeEnabled;

    // Arousal tracking
    double m_arousalLevel;
    double m_smoothedArousal;
    ArousalState m_arousalState;
    double m_baselineClitoral;
    double m_baselineAVL;
    QVector<double> m_pressureHistory;
    QVector<double> m_arousalHistory;
    int m_historyIndex;

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

    // Thread safety
    mutable QMutex m_mutex;

    // Constants
    static const int UPDATE_INTERVAL_MS = 100;
    static const int SAFETY_INTERVAL_MS = 100;
    static const int BASELINE_DURATION_MS = 10000;
    static const int HISTORY_SIZE = 200;
    static const int VARIANCE_WINDOW_SAMPLES = 100;

    // Arousal calculation weights
    static constexpr double WEIGHT_DEVIATION = 0.35;
    static constexpr double WEIGHT_VARIANCE = 0.25;
    static constexpr double WEIGHT_CONTRACTION = 0.30;
    static constexpr double WEIGHT_ROC = 0.10;
    static constexpr double AROUSAL_ALPHA = 0.15;

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

    // Forced orgasm defaults
    static constexpr double FORCED_BASE_INTENSITY = 0.60;
    static constexpr double FORCED_BASE_FREQUENCY = 10.0;
    static constexpr double FORCED_TENS_AMPLITUDE = 0.80;
    static constexpr double AROUSAL_DROP_THRESHOLD = 0.05;
    static constexpr double ANTI_ESCAPE_RATE = 0.02;
    static constexpr double ANTI_ESCAPE_FREQ_RATE = 0.1;
    static constexpr double THROUGH_ORGASM_BOOST = 0.05;
    static constexpr double TENS_FORCED_FREQUENCY = 25.0;
    static constexpr int ORGASM_DURATION_MS = 15000;
    static constexpr int POST_ORGASM_PAUSE_MS = 2000;
    static constexpr int COOLDOWN_DURATION_MS = 60000;

    // Safety limits
    static constexpr double SEAL_LOST_THRESHOLD = 10.0;
    static constexpr double MAX_SAFE_CLITORAL_PRESSURE = 80.0;
    static constexpr double MAX_OUTER_PRESSURE = 75.0;
    static constexpr double MAX_CLITORAL_AMPLITUDE = 60.0;
    static constexpr double MAX_INTENSITY = 0.95;
    static constexpr double MAX_FREQUENCY = 13.0;
    static constexpr int MAX_SESSION_DURATION_MS = 1800000;
    static constexpr int MAX_HIGH_PRESSURE_DURATION_MS = 120000;
};

#endif // ORGASMCONTROLALGORITHM_H

