#ifndef MOCKORGASMCONTROLALGORITHM_H
#define MOCKORGASMCONTROLALGORITHM_H

#include <QObject>
#include <QMutex>

/**
 * @brief Mock OrgasmControlAlgorithm for testing GUI components
 * 
 * Provides a complete mock implementation of OrgasmControlAlgorithm
 * for testing purposes. Simulates all arousal detection and control operations.
 */
class MockOrgasmControlAlgorithm : public QObject
{
    Q_OBJECT

public:
    // Execution modes (matching OrgasmControlAlgorithm)
    enum class Mode {
        MANUAL = 0,
        ADAPTIVE_EDGING,
        FORCED_ORGASM,
        MULTI_ORGASM,
        DENIAL,
        MILKING
    };
    Q_ENUM(Mode)

    // Control states (matching OrgasmControlAlgorithm)
    enum class ControlState {
        STOPPED = 0,
        CALIBRATING,
        BUILDING,
        BACKING_OFF,
        HOLDING,
        FORCING,
        MILKING,
        DANGER_REDUCTION,
        ORGASM_FAILURE,
        COOLING_DOWN,
        ERROR
    };
    Q_ENUM(ControlState)

    explicit MockOrgasmControlAlgorithm(QObject *parent = nullptr);
    ~MockOrgasmControlAlgorithm() override = default;

    // Mode control
    void startAdaptiveEdging(int targetCycles = 5);
    void startForcedOrgasm(int targetOrgasms = 3, int maxDurationMs = 1800000);
    void startDenial(int durationMs = 600000);
    void startMilking(int durationMs = 1800000, int failureMode = 0);
    void stop();
    void emergencyStop();

    // Threshold setters
    void setEdgeThreshold(double threshold);
    void setOrgasmThreshold(double threshold);
    void setRecoveryThreshold(double threshold);
    void setMilkingZoneLower(double threshold);
    void setMilkingZoneUpper(double threshold);
    void setDangerThreshold(double threshold);
    void setMilkingFailureMode(int mode);
    void setTENSEnabled(bool enabled);
    void setAntiEscapeEnabled(bool enabled);

    // Getters
    double edgeThreshold() const { return m_edgeThreshold; }
    double orgasmThreshold() const { return m_orgasmThreshold; }
    double recoveryThreshold() const { return m_recoveryThreshold; }
    double milkingZoneLower() const { return m_milkingZoneLower; }
    double milkingZoneUpper() const { return m_milkingZoneUpper; }
    double dangerThreshold() const { return m_dangerThreshold; }
    int milkingFailureMode() const { return m_milkingFailureMode; }
    bool isTENSEnabled() const { return m_tensEnabled; }
    bool isAntiEscapeEnabled() const { return m_antiEscapeEnabled; }
    Mode currentMode() const { return m_mode; }
    ControlState currentState() const { return m_state; }
    double arousalLevel() const { return m_arousalLevel; }

    // Test helper methods
    void simulateArousalChange(double level);
    void simulateStateChange(ControlState state);

signals:
    void arousalLevelChanged(double level);
    void stateChanged(ControlState state);
    void modeChanged(Mode mode);
    void edgeThresholdChanged(double threshold);
    void orgasmThresholdChanged(double threshold);
    void recoveryThresholdChanged(double threshold);
    void milkingZoneEntered(double arousalLevel);
    void milkingZoneMaintained(qint64 durationMs, double avgArousal);
    void dangerZoneEntered(double arousalLevel);
    void dangerZoneExited(double arousalLevel);
    void unwantedOrgasm(int orgasmCount, qint64 sessionDurationMs);
    void milkingSessionComplete(qint64 durationMs, bool success, int dangerEntries);
    void milkingIntensityAdjusted(double newIntensity, double arousalError);

private:
    double m_edgeThreshold = 0.70;
    double m_orgasmThreshold = 0.85;
    double m_recoveryThreshold = 0.45;
    double m_milkingZoneLower = 0.75;
    double m_milkingZoneUpper = 0.90;
    double m_dangerThreshold = 0.92;
    int m_milkingFailureMode = 0;
    bool m_tensEnabled = false;
    bool m_antiEscapeEnabled = false;
    Mode m_mode = Mode::MANUAL;
    ControlState m_state = ControlState::STOPPED;
    double m_arousalLevel = 0.0;
    mutable QMutex m_mutex;
};

#endif // MOCKORGASMCONTROLALGORITHM_H

