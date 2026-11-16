#ifndef CLITORALOSCILLATOR_H
#define CLITORALOSCILLATOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include "../safety/SafetyConstants.h"

class HardwareManager;
class PneumaticController;

/**
 * @brief Clitoral air-pulse control, driving the pulser pump.
 *
 * The fast pulses come from the clitoral pulser (a second diaphragm pump with its
 * valves removed, GPIO 15). SOL4/SOL5 only hold or release the steady clitoral
 * vacuum, through PneumaticController. This class keeps its old API:
 * - setFrequency(): pulse rate, 1-100 per second (below the calibrated motor
 *   minimum the controller makes slow "deep" bursts)
 * - setAmplitude(): the steady clitoral vacuum target in mmHg
 * - start()/stop(): pulser on/off (refused until the pulser is bench-calibrated)
 * phaseChanged/vacuumPhaseChanged/cycleCompleted follow the pulser bursts.
 *
 * Design: docs/superpowers/specs/2026-11-16-clitoral-pulser-system-design.md
 */
class ClitoralOscillator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Oscillation phase states
     */
    enum class Phase {
        IDLE,       // Not oscillating
        SUCTION,    // SOL4 open, SOL5 closed (building vacuum)
        HOLD,       // Both closed (peak pressure)
        VENT,       // SOL4 closed, SOL5 open (releasing pressure)
        TRANSITION  // Both closed (before next cycle)
    };
    Q_ENUM(Phase)

    explicit ClitoralOscillator(HardwareManager* hardware, QObject *parent = nullptr);
    ~ClitoralOscillator();

    // Oscillation control
    void start();
    void stop();
    void emergencyStop();  // Bug #13 fix: Immediate stop with safety venting
    bool isRunning() const { return m_running; }

    // Single pulse for haptic feedback
    void pulse(double intensity, int durationMs);

    // Frequency control (5-13 Hz range)
    void setFrequency(double frequencyHz);
    double getFrequency() const { return m_frequencyHz; }

    // Amplitude control (target pressure in mmHg)
    void setAmplitude(double pressureMmHg);
    double getAmplitude() const { return m_targetAmplitude; }

    // Duty cycle control (ratio of suction to vent time)
    void setDutyCycle(double dutyCycle);  // 0.0 - 1.0
    double getDutyCycle() const { return m_dutyCycle; }

    // Phase timing ratios (advanced control)
    void setPhaseTiming(double suctionRatio, double holdRatio, 
                       double ventRatio, double transitionRatio);

    // Current state
    Phase getCurrentPhase() const { return m_currentPhase; }
    double getCurrentPressure() const;
    int getCycleCount() const { return m_cycleCount; }

    // Presets based on research
    void setPresetWarmup();    // 5 Hz, low amplitude
    void setPresetBuildUp();   // 8 Hz, medium amplitude
    void setPresetClimax();    // 10-12 Hz, high amplitude
    void setPresetAfterGlow(); // 3-5 Hz, low amplitude

Q_SIGNALS:
    void oscillationStarted();
    void oscillationStopped();
    void phaseChanged(Phase newPhase);
    /**
     * @brief Emitted when vacuum phase changes (for TENS synchronization)
     * @param isSuctionPhase True during SUCTION/HOLD phases, false during VENT/TRANSITION
     */
    void vacuumPhaseChanged(bool isSuctionPhase);
    void cycleCompleted(int cycleCount);
    void amplitudeReached(double pressure);
    void error(const QString& message);

private:
    PneumaticController* air() const;
    void onBurstChanged(bool on);

    HardwareManager* m_hardware;
    mutable QMutex m_mutex;

    // State
    bool m_running;
    Phase m_currentPhase;
    int m_cycleCount;

    double m_frequencyHz;         // Pulse rate (1-100 per second)
    double m_targetAmplitude;     // Steady clitoral vacuum target (mmHg)
    double m_dutyCycle;           // Kept for API compatibility (no effect: fixed-stroke pulser)
    double m_suctionRatio;        // Kept for API compatibility (no effect)
    double m_holdRatio;
    double m_ventRatio;
    double m_transitionRatio;
    QMetaObject::Connection m_burstConnection;

    // Pulse-rate limits: slow "deep" bursts up to toy-like rates
    static constexpr double MIN_FREQUENCY_HZ = 1.0;
    static constexpr double MAX_FREQUENCY_HZ = 100.0;
    static constexpr double DEFAULT_FREQUENCY_HZ = 8.0;

    // Amplitude (steady target) limits
    static constexpr double MIN_AMPLITUDE_MMHG = 0.0;
    static constexpr double DEFAULT_AMPLITUDE_MMHG = 40.0;
};

#endif // CLITORALOSCILLATOR_H

