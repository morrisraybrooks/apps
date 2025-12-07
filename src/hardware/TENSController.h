#ifndef TENSCONTROLLER_H
#define TENSCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QElapsedTimer>
#include <memory>

class HardwareManager;

/**
 * @brief TENS (Transcutaneous Electrical Nerve Stimulation) Controller
 *
 * Controls electrical stimulation for dorsal genital nerve stimulation (DGNS).
 * Generates biphasic symmetric waveforms at clinical parameters:
 * - Frequency: 1-100 Hz (default 20 Hz)
 * - Pulse Width: 50-500 μs (default 400 μs)
 * - Amplitude: 0-80 mA (adjustable)
 *
 * Integrated with V-Contour clitoral cup electrodes for combined
 * vacuum oscillation + electrical stimulation therapy.
 */
class TENSController : public QObject
{
    Q_OBJECT

public:
    // Waveform types
    enum class Waveform {
        BIPHASIC_SYMMETRIC,    // Default: equal positive/negative phases
        BIPHASIC_ASYMMETRIC,   // Unequal phases (still charge-balanced)
        BURST                  // Burst of pulses with inter-burst gap
    };
    Q_ENUM(Waveform)

    // Phase synchronization with vacuum oscillation
    enum class PhaseSync {
        CONTINUOUS,       // TENS runs continuously
        SYNC_SUCTION,     // TENS active during vacuum suction phase
        SYNC_VENT,        // TENS active during vent phase (contrast)
        ALTERNATING       // TENS alternates with vacuum phases
    };
    Q_ENUM(PhaseSync)

    // Current phase of biphasic waveform
    enum class OutputPhase {
        IDLE,
        POSITIVE,
        NEGATIVE,
        INTER_PULSE
    };
    Q_ENUM(OutputPhase)

    explicit TENSController(HardwareManager* hardware, QObject *parent = nullptr);
    ~TENSController();

    // Initialization
    bool initialize();
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Frequency control (1-100 Hz)
    void setFrequency(double frequencyHz);
    double getFrequency() const { return m_frequencyHz; }

    // Pulse width control (50-500 μs)
    void setPulseWidth(int microseconds);
    int getPulseWidth() const { return m_pulseWidthUs; }

    // Amplitude control (0-80 mA, as percentage 0-100%)
    void setAmplitude(double percent);
    double getAmplitude() const { return m_amplitudePercent; }
    double getAmplitudeMa() const { return m_amplitudePercent * MAX_AMPLITUDE_MA / 100.0; }

    // Waveform selection
    void setWaveform(Waveform type);
    Waveform getWaveform() const { return m_waveformType; }

    // Phase synchronization
    void setPhaseSync(PhaseSync sync);
    PhaseSync getPhaseSync() const { return m_phaseSync; }

    // Burst mode parameters
    void setBurstParameters(int pulsesPerBurst, int burstFrequencyHz);

    // Control
    void start();
    void stop();
    void emergencyStop();
    bool isRunning() const { return m_running; }

    // Presets based on research
    void setPresetWarmup();     // 10 Hz, 200 μs, low amplitude
    void setPresetArousal();    // 20 Hz, 400 μs, medium amplitude
    void setPresetClimax();     // 30 Hz, 300 μs, higher amplitude
    void setPresetAfterGlow();  // 10 Hz, 500 μs, low amplitude

    // Status and diagnostics
    OutputPhase getCurrentPhase() const { return m_outputPhase; }
    double getElectrodeImpedance() const { return m_electrodeImpedance; }
    bool isFaultDetected() const { return m_faultDetected; }
    QString getFaultReason() const { return m_faultReason; }
    int getPulseCount() const { return m_pulseCount; }

    // Safety
    bool canEnable() const;
    void setMinSealPressure(double mmHg) { m_minSealPressure = mmHg; }

    // Vacuum sync interface (called by ClitoralOscillator)
    void onVacuumPhaseChanged(bool isSuctionPhase);

Q_SIGNALS:
    void stimulationStarted();
    void stimulationStopped();
    void amplitudeChanged(double percent);
    void frequencyChanged(double hz);
    void pulseWidthChanged(int microseconds);
    void phaseChanged(OutputPhase phase);
    void pulseCompleted(int count);
    void faultDetected(const QString& reason);
    void faultCleared();
    void electrodeContact(bool good);
    void error(const QString& message);

private Q_SLOTS:
    void onTimerTick();
    void onRampTimer();

private:
    void generatePulse();
    void setOutputPhase(OutputPhase phase);
    void updatePWMAmplitude();
    void checkFaultStatus();
    void checkElectrodeContact();
    void softStart();
    void softStop();
    void calculateTiming();

    HardwareManager* m_hardware;
    QTimer* m_waveformTimer;
    QTimer* m_rampTimer;
    QElapsedTimer m_phaseTimer;
    mutable QMutex m_mutex;

    // State
    bool m_initialized;
    bool m_running;
    bool m_enabled;           // Hardware enable state
    OutputPhase m_outputPhase;
    int m_pulseCount;

    // Waveform parameters
    double m_frequencyHz;
    int m_pulseWidthUs;
    double m_amplitudePercent;
    double m_targetAmplitude;  // For soft start/stop ramping
    Waveform m_waveformType;
    PhaseSync m_phaseSync;

    // Timing (microseconds)
    int m_periodUs;           // Total period = 1/frequency
    int m_positiveDurationUs;
    int m_negativeDurationUs;
    int m_interPulseUs;

    // Burst mode
    int m_pulsesPerBurst;
    int m_burstFrequencyHz;
    int m_currentBurstPulse;

    // Vacuum synchronization
    bool m_vacuumSuctionPhase;
    bool m_syncEnabled;

    // Safety
    bool m_faultDetected;
    QString m_faultReason;
    double m_electrodeImpedance;
    double m_minSealPressure;

    // Ramping
    double m_rampStep;
    static constexpr double RAMP_TIME_MS = 500.0;
    static constexpr double RAMP_INTERVAL_MS = 20.0;

    // GPIO pin definitions
    static const int GPIO_TENS_ENABLE = 5;   // Master enable
    static const int GPIO_TENS_PHASE = 6;    // Polarity control
    static const int GPIO_TENS_PWM = 12;     // Amplitude PWM (hardware PWM1)
    static const int GPIO_TENS_FAULT = 16;   // Fault input

    // Limits
    static constexpr double MIN_FREQUENCY_HZ = 1.0;
    static constexpr double MAX_FREQUENCY_HZ = 100.0;
    static constexpr double DEFAULT_FREQUENCY_HZ = 20.0;
    static constexpr int MIN_PULSE_WIDTH_US = 50;
    static constexpr int MAX_PULSE_WIDTH_US = 500;
    static constexpr int DEFAULT_PULSE_WIDTH_US = 400;
    static constexpr double MAX_AMPLITUDE_MA = 80.0;
    static constexpr double MIN_SEAL_PRESSURE_MMHG = 10.0;
    static constexpr double MAX_IMPEDANCE_OHMS = 10000.0;

    // Timer resolution
    static constexpr int TIMER_RESOLUTION_US = 50;  // 50μs resolution
};

#endif // TENSCONTROLLER_H

