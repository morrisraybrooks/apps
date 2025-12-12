#ifndef CLITORALOSCILLATOR_H
#define CLITORALOSCILLATOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>

class HardwareManager;

/**
 * @brief High-frequency oscillation controller for clitoral cylinder air-pulse stimulation
 * 
 * This class creates oscillating pressure waves in the clitoral cylinder using
 * SOL4 (vacuum) and SOL5 (vent) valves. It generates air-pulse stimulation similar
 * to commercial toys (Womanizer, Satisfyer) but using solenoid valve control.
 * 
 * Key Features:
 * - Frequency range: 5-13 Hz (research shows 8-13 Hz optimal for orgasm)
 * - 4-phase asymmetric duty cycle for smooth pressure oscillations
 * - Amplitude control via duty cycle modulation
 * - Independent from outer chamber control (dual-chamber coordination)
 * 
 * Valve Timing Strategy (4-Phase Cycle):
 * 1. SUCTION: SOL4 open, SOL5 closed - rapid vacuum build
 * 2. HOLD:    Both closed - peak pressure maintained
 * 3. VENT:    SOL4 closed, SOL5 open - rapid pressure release
 * 4. TRANSITION: Both closed - minimum pressure before next cycle
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
    void cycleCompleted(int cycleCount);
    void amplitudeReached(double pressure);
    void error(const QString& message);

private Q_SLOTS:
    void onTimerTick();

private:
    void calculatePhaseDurations();
    void executePhase(Phase phase);
    void advancePhase();
    void adjustAmplitude();

    HardwareManager* m_hardware;
    QTimer* m_oscillationTimer;
    QElapsedTimer m_phaseTimer;
    mutable QMutex m_mutex;

    // State
    bool m_running;
    Phase m_currentPhase;
    int m_cycleCount;

    // Frequency and timing (in milliseconds)
    double m_frequencyHz;         // Oscillation frequency (5-13 Hz)
    int m_periodMs;               // Total period = 1000/frequency
    int m_suctionDurationMs;      // Duration of suction phase
    int m_holdDurationMs;         // Duration of hold phase
    int m_ventDurationMs;         // Duration of vent phase
    int m_transitionDurationMs;   // Duration of transition phase

    // Phase timing ratios (must sum to 1.0)
    double m_suctionRatio;
    double m_holdRatio;
    double m_ventRatio;
    double m_transitionRatio;

    // Amplitude control
    double m_targetAmplitude;     // Target peak pressure (mmHg)
    double m_dutyCycle;           // Suction duty cycle (0.0 - 1.0)
    double m_measuredPeakPressure;
    double m_measuredTroughPressure;

    // Timer resolution
    static const int TIMER_RESOLUTION_MS = 1;  // 1ms for high-frequency control

    // Frequency limits (based on research)
    static constexpr double MIN_FREQUENCY_HZ = 3.0;
    static constexpr double MAX_FREQUENCY_HZ = 15.0;
    static constexpr double DEFAULT_FREQUENCY_HZ = 8.0;

    // Amplitude limits
    static constexpr double MIN_AMPLITUDE_MMHG = 5.0;
    static constexpr double MAX_AMPLITUDE_MMHG = 75.0;
    static constexpr double DEFAULT_AMPLITUDE_MMHG = 40.0;

    // Default phase ratios (optimized for smooth oscillation)
    static constexpr double DEFAULT_SUCTION_RATIO = 0.35;
    static constexpr double DEFAULT_HOLD_RATIO = 0.10;
    static constexpr double DEFAULT_VENT_RATIO = 0.35;
    static constexpr double DEFAULT_TRANSITION_RATIO = 0.20;
};

#endif // CLITORALOSCILLATOR_H

