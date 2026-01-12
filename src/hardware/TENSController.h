#ifndef TENSCONTROLLER_H
#define TENSCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QElapsedTimer>
#include <memory>
#include <array>

class HardwareManager;

/**
 * @brief TENS (Transcutaneous Electrical Nerve Stimulation) Controller
 *
 * Controls dual-channel electrical stimulation for clitourethrovaginal complex:
 * - Channel 1: Clitoral cup electrodes (dorsal genital nerve stimulation)
 * - Channel 2: Urethral electrodes (pudendal nerve afferent stimulation)
 *
 * Based on urethral e-stim research, the dual-channel approach stimulates:
 * - Clitoral nerve endings via cup electrodes
 * - Pudendal nerve afferents via urethral/periurethral electrodes
 * - Skene's glands and urethral sponge through indirect stimulation
 *
 * Clinical parameters per channel:
 * - Frequency: 1-100 Hz (urethral: 10-100 Hz recommended)
 * - Pulse Width: 50-500 μs (urethral: 100-300 μs recommended)
 * - Amplitude: 0-80 mA clitoral, 0-50 mA urethral (safety limit)
 * - Waveform: Biphasic balanced (required for urethral safety)
 *
 * Coordinated with vacuum oscillation for synergistic stimulation.
 */
class TENSController : public QObject
{
    Q_OBJECT

public:
    // Electrode channels
    enum class ElectrodeChannel {
        CLITORAL,      // Clitoral cup electrodes (primary)
        URETHRAL,      // Urethral/periurethral electrodes (secondary)
        BOTH           // Coordinated dual-channel stimulation
    };
    Q_ENUM(ElectrodeChannel)

    // Waveform types
    enum class Waveform {
        BIPHASIC_SYMMETRIC,    // Default: equal positive/negative phases (REQUIRED for urethral)
        BIPHASIC_ASYMMETRIC,   // Unequal phases (still charge-balanced) - clitoral only
        BURST,                 // Burst of pulses with inter-burst gap
        MODULATED              // Varying frequency/amplitude (prevents accommodation)
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

    // Arousal-synchronized stimulation modes (based on research)
    enum class ArousalSyncMode {
        MANUAL,           // User controls all parameters
        ENGORGEMENT,      // Low-frequency (5-20 Hz) to enhance blood flow
        AROUSAL_BUILD,    // Moderate frequency (20-50 Hz) synchronized with oscillation
        NEAR_ORGASM,      // High frequency (50-100 Hz) bursts
        ORGASM_INDUCTION, // Rapid frequency sweeps or intense bursts
        POST_ORGASM       // Reduce to low frequency or stop
    };
    Q_ENUM(ArousalSyncMode)

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
    void pulse(int durationMs);  // Single pulse/burst for specified duration
    bool isRunning() const { return m_running; }

    // Presets based on research (Zimmerman 2018, Bottorff 2023)
    void setPresetWarmup();     // 10 Hz, 200 μs, low amplitude - neural priming
    void setPresetArousal();    // 20 Hz, 400 μs, medium amplitude - clinical standard
    void setPresetClimax();     // 30 Hz, 300 μs, higher amplitude - orgasm enhancement
    void setPresetAfterGlow();  // 10 Hz, 500 μs, low amplitude - recovery

    // ========================================================================
    // AUTOMATED ORGASM PATTERN PHASE PRESETS (matches AUTOMATED_ORGASM_PATTERNS.md)
    // ========================================================================
    // These presets are designed to sync with vacuum pattern phases 0-6
    void setPhaseEngorgement();      // Phase 0: Pre-stimulation priming (15-30s)
    void setPhaseSensitivity();      // Phase 1: Initial sensitivity (30-60s)
    void setPhaseAdaptation();       // Phase 2: Adaptation period (1-2.5 min)
    void setPhaseArousalBuild();     // Phase 3: Arousal build-up (2.5-4.5 min)
    void setPhasePreClimax();        // Phase 4: Pre-climax tension (4.5-5.5 min)
    void setPhaseClimax();           // Phase 5: Orgasm maintenance (5.5-6 min)
    void setPhaseRecovery();         // Phase 6: Post-climax recovery (6-7 min)

    // ========================================================================
    // DUAL-CHANNEL ELECTRODE CONTROL (Clitoral + Urethral)
    // ========================================================================

    // Channel selection
    void setActiveChannel(ElectrodeChannel channel);
    ElectrodeChannel getActiveChannel() const { return m_activeChannel; }

    // Per-channel parameter control
    void setChannelFrequency(ElectrodeChannel channel, double frequencyHz);
    double getChannelFrequency(ElectrodeChannel channel) const;
    void setChannelPulseWidth(ElectrodeChannel channel, int microseconds);
    int getChannelPulseWidth(ElectrodeChannel channel) const;
    void setChannelAmplitude(ElectrodeChannel channel, double percent);
    double getChannelAmplitude(ElectrodeChannel channel) const;
    void setChannelWaveform(ElectrodeChannel channel, Waveform type);
    Waveform getChannelWaveform(ElectrodeChannel channel) const;

    // Urethral electrode specific controls
    void setUrethralEnabled(bool enabled);
    bool isUrethralEnabled() const { return m_urethralEnabled; }
    bool isUrethralElectrodeConnected() const;
    double getUrethralImpedance() const { return m_urethralImpedance; }

    // Arousal-synchronized stimulation modes
    void setArousalSyncMode(ArousalSyncMode mode);
    ArousalSyncMode getArousalSyncMode() const { return m_arousalSyncMode; }
    void onArousalLevelChanged(double arousalLevel);  // Called by OrgasmControlAlgorithm

    // Dual-channel coordination modes
    enum class DualChannelMode {
        INDEPENDENT,      // Each channel runs independently
        SYNCHRONIZED,     // Both channels pulse together
        ALTERNATING,      // Channels alternate pulses
        PHASE_OFFSET,     // Urethral delayed by configurable offset
        WAVE_PROPAGATION  // Simulate wave from clitoral to urethral
    };
    Q_ENUM(DualChannelMode)

    void setDualChannelMode(DualChannelMode mode);
    DualChannelMode getDualChannelMode() const { return m_dualChannelMode; }
    void setPhaseOffset(int microseconds);  // For PHASE_OFFSET mode

    // Status and diagnostics
    OutputPhase getCurrentPhase() const { return m_outputPhase; }
    double getElectrodeImpedance() const { return m_electrodeImpedance; }
    bool isFaultDetected() const { return m_faultDetected; }
    QString getFaultReason() const { return m_faultReason; }
    int getPulseCount() const { return m_pulseCount; }

    // Safety
    bool canEnable() const;
    bool canEnableUrethral() const;  // Additional safety checks for urethral
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

    // Dual-channel signals
    void urethralEnabled(bool enabled);
    void urethralContact(bool good);
    void urethralFault(const QString& reason);
    void channelAmplitudeChanged(ElectrodeChannel channel, double percent);
    void arousalSyncModeChanged(ArousalSyncMode mode);

private Q_SLOTS:
    void onTimerTick();
    void onRampTimer();
    void onUrethralTimerTick();  // Separate timer for urethral channel

private:
    void generatePulse();
    void generateUrethralPulse();
    void setOutputPhase(OutputPhase phase);
    void setUrethralOutputPhase(OutputPhase phase);
    void updatePWMAmplitude();
    void updateUrethralPWMAmplitude();
    void checkFaultStatus();
    void checkElectrodeContact();
    void checkUrethralContact();
    void softStart();
    void softStop();
    void calculateTiming();
    void calculateUrethralTiming();
    void applyArousalSyncParameters(double arousalLevel);
    int getChannelIndex(ElectrodeChannel channel) const;

    HardwareManager* m_hardware;
    QTimer* m_waveformTimer;
    QTimer* m_rampTimer;
    QTimer* m_urethralTimer;      // Separate timer for urethral channel
    QElapsedTimer m_phaseTimer;
    QElapsedTimer m_urethralPhaseTimer;
    mutable QMutex m_mutex;

    // State
    bool m_initialized;
    bool m_running;
    bool m_enabled;           // Hardware enable state
    OutputPhase m_outputPhase;
    int m_pulseCount;

    // Waveform parameters (clitoral - primary channel)
    double m_frequencyHz;
    int m_pulseWidthUs;
    double m_amplitudePercent;
    double m_targetAmplitude;  // For soft start/stop ramping
    Waveform m_waveformType;
    PhaseSync m_phaseSync;

    // ========================================================================
    // DUAL-CHANNEL STATE
    // ========================================================================

    // Per-channel parameters structure
    struct ChannelParams {
        double frequencyHz = DEFAULT_FREQUENCY_HZ;
        int pulseWidthUs = DEFAULT_PULSE_WIDTH_US;
        double amplitudePercent = 0.0;
        double targetAmplitude = 0.0;
        Waveform waveformType = Waveform::BIPHASIC_SYMMETRIC;
        OutputPhase outputPhase = OutputPhase::IDLE;
        int pulseCount = 0;
        bool enabled = false;
    };

    // Channel parameters: [0] = CLITORAL, [1] = URETHRAL
    std::array<ChannelParams, 2> m_channelParams;

    // Active channel selection
    ElectrodeChannel m_activeChannel;

    // Urethral electrode state
    bool m_urethralEnabled;
    bool m_urethralConnected;
    double m_urethralImpedance;
    bool m_urethralFault;
    QString m_urethralFaultReason;

    // Dual-channel coordination
    DualChannelMode m_dualChannelMode;
    int m_phaseOffsetUs;  // Offset for PHASE_OFFSET mode

    // Arousal synchronization
    ArousalSyncMode m_arousalSyncMode;
    double m_currentArousalLevel;

    // Timing (microseconds) - clitoral
    int m_periodUs;           // Total period = 1/frequency
    int m_positiveDurationUs;
    int m_negativeDurationUs;
    int m_interPulseUs;

    // Timing (microseconds) - urethral
    int m_urethralPeriodUs;
    int m_urethralPositiveDurationUs;
    int m_urethralNegativeDurationUs;
    int m_urethralInterPulseUs;

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

    // GPIO pin definitions - Clitoral channel
    static const int GPIO_TENS_ENABLE = 5;   // Master enable
    static const int GPIO_TENS_PHASE = 6;    // Polarity control
    static const int GPIO_TENS_PWM = 12;     // Amplitude PWM (hardware PWM1)
    static const int GPIO_TENS_FAULT = 16;   // Fault input

    // GPIO pin definitions - Urethral channel (new)
    static const int GPIO_URETHRAL_ENABLE = 25;  // Urethral master enable
    static const int GPIO_URETHRAL_PHASE = 26;   // Urethral polarity control
    static const int GPIO_URETHRAL_PWM = 13;     // Urethral amplitude PWM (hardware PWM0)
    static const int GPIO_URETHRAL_FAULT = 17;   // Urethral fault input

    // Limits - Clitoral
    static constexpr double MIN_FREQUENCY_HZ = 1.0;
    static constexpr double MAX_FREQUENCY_HZ = 100.0;
    static constexpr double DEFAULT_FREQUENCY_HZ = 20.0;
    static constexpr int MIN_PULSE_WIDTH_US = 50;
    static constexpr int MAX_PULSE_WIDTH_US = 500;
    static constexpr int DEFAULT_PULSE_WIDTH_US = 400;
    static constexpr double MAX_AMPLITUDE_MA = 80.0;
    static constexpr double MIN_SEAL_PRESSURE_MMHG = 10.0;
    static constexpr double MAX_IMPEDANCE_OHMS = 10000.0;

    // Limits - Urethral (more conservative per research)
    static constexpr double URETHRAL_MIN_FREQUENCY_HZ = 10.0;
    static constexpr double URETHRAL_MAX_FREQUENCY_HZ = 100.0;
    static constexpr double URETHRAL_DEFAULT_FREQUENCY_HZ = 30.0;
    static constexpr int URETHRAL_MIN_PULSE_WIDTH_US = 100;
    static constexpr int URETHRAL_MAX_PULSE_WIDTH_US = 300;
    static constexpr int URETHRAL_DEFAULT_PULSE_WIDTH_US = 200;
    static constexpr double URETHRAL_MAX_AMPLITUDE_MA = 50.0;  // Lower limit for safety
    static constexpr double URETHRAL_MAX_IMPEDANCE_OHMS = 5000.0;  // Stricter for contact

    // Timer resolution
    static constexpr int TIMER_RESOLUTION_US = 50;  // 50μs resolution
};

#endif // TENSCONTROLLER_H

