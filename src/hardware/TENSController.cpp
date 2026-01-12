#include "TENSController.h"
#include "HardwareManager.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>

TENSController::TENSController(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_waveformTimer(new QTimer(this))
    , m_rampTimer(new QTimer(this))
    , m_urethralTimer(new QTimer(this))
    , m_initialized(false)
    , m_running(false)
    , m_enabled(false)
    , m_outputPhase(OutputPhase::IDLE)
    , m_pulseCount(0)
    , m_frequencyHz(DEFAULT_FREQUENCY_HZ)
    , m_pulseWidthUs(DEFAULT_PULSE_WIDTH_US)
    , m_amplitudePercent(0.0)
    , m_targetAmplitude(0.0)
    , m_waveformType(Waveform::BIPHASIC_SYMMETRIC)
    , m_phaseSync(PhaseSync::CONTINUOUS)
    , m_activeChannel(ElectrodeChannel::CLITORAL)
    , m_urethralEnabled(false)
    , m_urethralConnected(false)
    , m_urethralImpedance(0.0)
    , m_urethralFault(false)
    , m_dualChannelMode(DualChannelMode::SYNCHRONIZED)
    , m_phaseOffsetUs(0)
    , m_arousalSyncMode(ArousalSyncMode::MANUAL)
    , m_currentArousalLevel(0.0)
    , m_periodUs(static_cast<int>(1000000.0 / DEFAULT_FREQUENCY_HZ))
    , m_positiveDurationUs(DEFAULT_PULSE_WIDTH_US)
    , m_negativeDurationUs(DEFAULT_PULSE_WIDTH_US)
    , m_interPulseUs(0)
    , m_urethralPeriodUs(static_cast<int>(1000000.0 / URETHRAL_DEFAULT_FREQUENCY_HZ))
    , m_urethralPositiveDurationUs(URETHRAL_DEFAULT_PULSE_WIDTH_US)
    , m_urethralNegativeDurationUs(URETHRAL_DEFAULT_PULSE_WIDTH_US)
    , m_urethralInterPulseUs(0)
    , m_pulsesPerBurst(5)
    , m_burstFrequencyHz(2)
    , m_currentBurstPulse(0)
    , m_vacuumSuctionPhase(false)
    , m_syncEnabled(false)
    , m_faultDetected(false)
    , m_electrodeImpedance(0.0)
    , m_minSealPressure(MIN_SEAL_PRESSURE_MMHG)
    , m_rampStep(0.0)
{
    // Initialize per-channel parameters
    m_channelParams[0].frequencyHz = DEFAULT_FREQUENCY_HZ;
    m_channelParams[0].pulseWidthUs = DEFAULT_PULSE_WIDTH_US;
    m_channelParams[0].waveformType = Waveform::BIPHASIC_SYMMETRIC;

    m_channelParams[1].frequencyHz = URETHRAL_DEFAULT_FREQUENCY_HZ;
    m_channelParams[1].pulseWidthUs = URETHRAL_DEFAULT_PULSE_WIDTH_US;
    m_channelParams[1].waveformType = Waveform::BIPHASIC_SYMMETRIC;  // Required for urethral safety

    // High-precision timer for clitoral waveform generation
    m_waveformTimer->setTimerType(Qt::PreciseTimer);
    connect(m_waveformTimer, &QTimer::timeout, this, &TENSController::onTimerTick);

    // High-precision timer for urethral waveform generation
    m_urethralTimer->setTimerType(Qt::PreciseTimer);
    connect(m_urethralTimer, &QTimer::timeout, this, &TENSController::onUrethralTimerTick);

    // Ramp timer for soft start/stop
    m_rampTimer->setTimerType(Qt::PreciseTimer);
    m_rampTimer->setInterval(static_cast<int>(RAMP_INTERVAL_MS));
    connect(m_rampTimer, &QTimer::timeout, this, &TENSController::onRampTimer);

    calculateTiming();
    calculateUrethralTiming();
}

TENSController::~TENSController()
{
    shutdown();
}

bool TENSController::initialize()
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return true;
    }

    qDebug() << "Initializing TENS Controller...";

    // GPIO initialization is handled by HardwareManager
    // We just need to verify the hardware is ready
    if (!m_hardware || !m_hardware->isReady()) {
        qCritical() << "Hardware not ready for TENS initialization";
        return false;
    }

    // Set initial GPIO states (all disabled)
    // Note: Actual GPIO control would go through HardwareManager
    m_enabled = false;
    m_outputPhase = OutputPhase::IDLE;
    m_faultDetected = false;

    m_initialized = true;
    qDebug() << "TENS Controller initialized successfully";
    qDebug() << "  Frequency:" << m_frequencyHz << "Hz";
    qDebug() << "  Pulse Width:" << m_pulseWidthUs << "μs";
    qDebug() << "  Max Amplitude:" << MAX_AMPLITUDE_MA << "mA";

    return true;
}

void TENSController::shutdown()
{
    QMutexLocker locker(&m_mutex);

    if (m_running) {
        locker.unlock();
        stop();
        locker.relock();
    }

    m_waveformTimer->stop();
    m_rampTimer->stop();
    m_enabled = false;
    m_outputPhase = OutputPhase::IDLE;
    m_initialized = false;

    qDebug() << "TENS Controller shutdown complete";
}

void TENSController::setFrequency(double frequencyHz)
{
    QMutexLocker locker(&m_mutex);

    m_frequencyHz = std::clamp(frequencyHz, MIN_FREQUENCY_HZ, MAX_FREQUENCY_HZ);
    calculateTiming();

    qDebug() << "TENS frequency set to" << m_frequencyHz << "Hz";
    emit frequencyChanged(m_frequencyHz);
}

void TENSController::setPulseWidth(int microseconds)
{
    QMutexLocker locker(&m_mutex);

    m_pulseWidthUs = std::clamp(microseconds, MIN_PULSE_WIDTH_US, MAX_PULSE_WIDTH_US);
    calculateTiming();

    qDebug() << "TENS pulse width set to" << m_pulseWidthUs << "μs";
    emit pulseWidthChanged(m_pulseWidthUs);
}

void TENSController::setAmplitude(double percent)
{
    QMutexLocker locker(&m_mutex);

    m_targetAmplitude = std::clamp(percent, 0.0, 100.0);

    // If running, ramp to new amplitude; otherwise set directly
    if (m_running) {
        // Calculate ramp step
        double steps = RAMP_TIME_MS / RAMP_INTERVAL_MS;
        m_rampStep = (m_targetAmplitude - m_amplitudePercent) / steps;
        if (!m_rampTimer->isActive()) {
            m_rampTimer->start();
        }
    } else {
        m_amplitudePercent = m_targetAmplitude;
    }

    qDebug() << "TENS amplitude target set to" << m_targetAmplitude << "%"
             << "(" << (m_targetAmplitude * MAX_AMPLITUDE_MA / 100.0) << "mA)";
}

void TENSController::setWaveform(Waveform type)
{
    QMutexLocker locker(&m_mutex);
    m_waveformType = type;
    calculateTiming();
    qDebug() << "TENS waveform set to" << static_cast<int>(type);
}

void TENSController::setPhaseSync(PhaseSync sync)
{
    QMutexLocker locker(&m_mutex);
    m_phaseSync = sync;
    m_syncEnabled = (sync != PhaseSync::CONTINUOUS);
    qDebug() << "TENS phase sync set to" << static_cast<int>(sync);
}

void TENSController::setBurstParameters(int pulsesPerBurst, int burstFrequencyHz)
{
    QMutexLocker locker(&m_mutex);
    m_pulsesPerBurst = std::clamp(pulsesPerBurst, 1, 20);
    m_burstFrequencyHz = std::clamp(burstFrequencyHz, 1, 10);
    qDebug() << "TENS burst parameters:" << m_pulsesPerBurst << "pulses at"
             << m_burstFrequencyHz << "Hz burst rate";
}

void TENSController::start()
{
    QMutexLocker locker(&m_mutex);

    if (m_running) {
        qDebug() << "TENS already running";
        return;
    }

    if (!m_initialized) {
        emit error("TENS Controller not initialized");
        return;
    }

    // Safety check
    locker.unlock();
    if (!canEnable()) {
        emit error("TENS cannot be enabled - safety check failed");
        return;
    }
    locker.relock();

    qDebug() << "Starting TENS stimulation...";
    qDebug() << "  Frequency:" << m_frequencyHz << "Hz";
    qDebug() << "  Pulse Width:" << m_pulseWidthUs << "μs";
    qDebug() << "  Target Amplitude:" << m_targetAmplitude << "%";

    m_running = true;
    m_pulseCount = 0;
    m_currentBurstPulse = 0;

    // Start with soft start (amplitude at 0, ramp up)
    m_amplitudePercent = 0.0;
    m_rampStep = m_targetAmplitude / (RAMP_TIME_MS / RAMP_INTERVAL_MS);

    // Enable hardware
    m_enabled = true;
    if (m_hardware) {
        m_hardware->setTENSOutputEnable(true);
    }

    // Start timers
    m_phaseTimer.start();
    m_rampTimer->start();

    // Calculate timer interval based on pulse width (need at least 2 ticks per pulse phase)
    int timerIntervalMs = std::max(1, m_pulseWidthUs / 1000);
    m_waveformTimer->setInterval(timerIntervalMs);
    m_waveformTimer->start();

    locker.unlock();
    emit stimulationStarted();
}

void TENSController::stop()
{
    QMutexLocker locker(&m_mutex);

    if (!m_running) {
        return;
    }

    qDebug() << "Stopping TENS stimulation...";

    // Soft stop - ramp amplitude down
    m_targetAmplitude = 0.0;
    m_rampStep = -m_amplitudePercent / (RAMP_TIME_MS / RAMP_INTERVAL_MS);

    // Will complete stop when amplitude reaches 0
    // For now, just stop the waveform timer
    m_waveformTimer->stop();
    m_running = false;

    // Set to idle phase
    setOutputPhase(OutputPhase::IDLE);

    // Disable hardware
    m_enabled = false;
    if (m_hardware) {
        m_hardware->setTENSOutputEnable(false);
        m_hardware->setTENSPWMDutyCycle(0);
    }

    locker.unlock();
    emit stimulationStopped();

    qDebug() << "TENS stimulation stopped. Total pulses:" << m_pulseCount;
}

void TENSController::emergencyStop()
{
    QMutexLocker locker(&m_mutex);

    qWarning() << "TENS EMERGENCY STOP";

    // Immediate stop - no soft ramp
    m_waveformTimer->stop();
    m_rampTimer->stop();
    m_running = false;
    m_enabled = false;
    m_amplitudePercent = 0.0;
    m_targetAmplitude = 0.0;

    // Set to idle immediately
    setOutputPhase(OutputPhase::IDLE);

    // Immediately disable TENS hardware
    if (m_hardware) {
        m_hardware->setTENSOutputEnable(false);
        m_hardware->setTENSPWMDutyCycle(0);
    }

    locker.unlock();
    emit stimulationStopped();
}

void TENSController::pulse(int durationMs)
{
    // Start stimulation for a fixed duration, then stop
    if (durationMs <= 0 || durationMs > 5000) {
        qWarning() << "Invalid pulse duration:" << durationMs << "ms (max 5000ms)";
        return;
    }

    start();

    // Use a single-shot timer to stop after duration
    QTimer::singleShot(durationMs, this, [this]() {
        stop();
    });

    qDebug() << "TENS pulse started for" << durationMs << "ms";
}

// ============================================================================
// PRESETS BASED ON CLINICAL RESEARCH AND AUTOMATED ORGASM PATTERNS
// ============================================================================
// Reference: Zimmerman et al. (2018) - 20 Hz, 400 μs clinical standard
// Reference: Bottorff et al. (2023) - DGNS arousal enhancement
// Integrated with V-Contour automated orgasm pattern phases (0-6)

void TENSController::setPresetWarmup()
{
    // Phase 0 equivalent: Pre-treatment neural priming
    // Low frequency to enhance blood flow (parasympathetic activation)
    setChannelFrequency(ElectrodeChannel::CLITORAL, 10.0);
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 200);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 20.0);

    // Urethral: gentle priming at lower intensity
    setChannelFrequency(ElectrodeChannel::URETHRAL, 15.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 150);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 15.0);

    setPhaseSync(PhaseSync::CONTINUOUS);
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);
    qDebug() << "TENS preset: Warmup/Engorgement (10 Hz, 200 μs, 20%) - Neural priming";
}

void TENSController::setPresetArousal()
{
    // Phase 1-2 equivalent: Clinical standard for arousal enhancement
    // 20 Hz, 400 μs - validated by Zimmerman et al. (2018)
    setChannelFrequency(ElectrodeChannel::CLITORAL, 20.0);
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 400);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 40.0);

    // Urethral: synchronized stimulation of pudendal afferents
    setChannelFrequency(ElectrodeChannel::URETHRAL, 20.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 30.0);

    setPhaseSync(PhaseSync::SYNC_SUCTION);  // Sync with vacuum oscillation
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);
    qDebug() << "TENS preset: Arousal (20 Hz, 400 μs, 40%) - Clinical standard";
}

void TENSController::setPresetClimax()
{
    // Phase 4-5 equivalent: Pre-climax and orgasm
    // Higher frequency (30 Hz) for intense sensory stimulation
    // Synchronized with 11-13 Hz vacuum oscillation
    setChannelFrequency(ElectrodeChannel::CLITORAL, 30.0);
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 300);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 70.0);

    // Urethral: intensified for orgasm enhancement
    setChannelFrequency(ElectrodeChannel::URETHRAL, 50.0);  // Higher for sensory gate
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 45.0);  // Near urethral max

    setPhaseSync(PhaseSync::SYNC_SUCTION);
    setDualChannelMode(DualChannelMode::WAVE_PROPAGATION);  // Wave from clitoral to urethral
    qDebug() << "TENS preset: Climax (30 Hz, 300 μs, 70%) - Orgasm enhancement";
}

void TENSController::setPresetAfterGlow()
{
    // Phase 6 equivalent: Post-climax recovery
    // Low frequency, wider pulse for sustained gentle sensation
    setChannelFrequency(ElectrodeChannel::CLITORAL, 10.0);
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 500);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 15.0);

    // Urethral: very gentle or disabled
    setChannelFrequency(ElectrodeChannel::URETHRAL, 10.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 150);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 10.0);

    setPhaseSync(PhaseSync::CONTINUOUS);
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);
    qDebug() << "TENS preset: Afterglow (10 Hz, 500 μs, 15%) - Recovery";
}

// ============================================================================
// AUTOMATED ORGASM PATTERN PHASE PRESETS
// Matches AUTOMATED_ORGASM_PATTERNS.md phases 0-6
// Synergistic with V-Contour vacuum system
// ============================================================================

void TENSController::setPhaseEngorgement()
{
    // Phase 0: Active Engorgement (15-30 seconds)
    // Purpose: Neural priming during vacuum engorgement
    // Vacuum: 40-50 mmHg sustained - NO pulsing
    // TENS: Very low frequency parasympathetic activation for vasodilation

    setChannelFrequency(ElectrodeChannel::CLITORAL, 5.0);   // Low Hz for vasodilation
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 500);  // Wide pulse for deep penetration
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 15.0);  // Sub-sensory threshold

    setChannelFrequency(ElectrodeChannel::URETHRAL, 5.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 10.0);

    setPhaseSync(PhaseSync::CONTINUOUS);
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);

    qDebug() << "TENS Phase 0: Engorgement (5 Hz, 500 μs) - Parasympathetic activation";
}

void TENSController::setPhaseSensitivity()
{
    // Phase 1: Initial Sensitivity (30-60 seconds)
    // Purpose: Gentle introduction on pre-engorged tissue
    // Vacuum: 5-6 Hz air-pulse, 35% → 55% intensity
    // TENS: Begin sensory nerve activation

    setChannelFrequency(ElectrodeChannel::CLITORAL, 15.0);   // Mid-range arousal
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 300);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 25.0);   // At sensory threshold

    setChannelFrequency(ElectrodeChannel::URETHRAL, 15.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 150);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 15.0);

    setPhaseSync(PhaseSync::SYNC_SUCTION);  // Sync with vacuum oscillation
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);

    qDebug() << "TENS Phase 1: Sensitivity (15 Hz, 300 μs) - Sensory introduction";
}

void TENSController::setPhaseAdaptation()
{
    // Phase 2: Adaptation Period (1-2.5 minutes)
    // Purpose: Consistent moderate intensity during body adaptation
    // Vacuum: 6-8 Hz, 60% intensity
    // TENS: Clinical standard 20 Hz

    setChannelFrequency(ElectrodeChannel::CLITORAL, 20.0);   // Clinical standard
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 400);   // Clinical standard
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 35.0);

    setChannelFrequency(ElectrodeChannel::URETHRAL, 20.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 25.0);

    setPhaseSync(PhaseSync::SYNC_SUCTION);
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);

    qDebug() << "TENS Phase 2: Adaptation (20 Hz, 400 μs) - Clinical standard";
}

void TENSController::setPhaseArousalBuild()
{
    // Phase 3: Arousal Build-up (2.5-4.5 minutes)
    // Purpose: Gradually increase intensity to match building arousal
    // Vacuum: 8-10 Hz, 60% → 85% intensity
    // TENS: Increasing frequency and amplitude

    setChannelFrequency(ElectrodeChannel::CLITORAL, 25.0);   // Increased for arousal
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 400);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 50.0);   // Rising intensity

    setChannelFrequency(ElectrodeChannel::URETHRAL, 30.0);   // Higher for added sensation
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 35.0);

    setPhaseSync(PhaseSync::SYNC_SUCTION);
    setDualChannelMode(DualChannelMode::PHASE_OFFSET);  // Slight offset for wave sensation

    qDebug() << "TENS Phase 3: Arousal Build (25 Hz, 400 μs) - Building intensity";
}

void TENSController::setPhasePreClimax()
{
    // Phase 4: Pre-Climax Tension (4.5-5.5 minutes)
    // Purpose: Build tension immediately preceding orgasm
    // Vacuum: 10-12 Hz optimal orgasm frequency, 85% intensity
    // TENS: High intensity sensory stimulation

    setChannelFrequency(ElectrodeChannel::CLITORAL, 30.0);   // Peak arousal frequency
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 350);
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 65.0);   // High but not max

    setChannelFrequency(ElectrodeChannel::URETHRAL, 50.0);   // Higher for sensory gate
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 40.0);

    setPhaseSync(PhaseSync::SYNC_SUCTION);
    setDualChannelMode(DualChannelMode::WAVE_PROPAGATION);

    qDebug() << "TENS Phase 4: Pre-Climax (30 Hz, 350 μs) - Peak tension";
}

void TENSController::setPhaseClimax()
{
    // Phase 5: Climax/Orgasm (5.5-6 minutes)
    // Purpose: Maintain optimal stimulation through orgasmic contractions
    // Vacuum: 11-13 Hz peak orgasm frequency, 90% intensity
    // TENS: Maximum safe intensity, high frequency for sustained sensation

    setChannelFrequency(ElectrodeChannel::CLITORAL, 30.0);   // Maintain not overwhelm
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 300);   // Slightly narrower for sharpness
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 75.0);   // Near maximum safe

    // Urethral: near-max for orgasm enhancement
    setChannelFrequency(ElectrodeChannel::URETHRAL, 50.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 200);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 48.0);   // Near urethral max (50mA)

    setPhaseSync(PhaseSync::SYNC_SUCTION);
    setDualChannelMode(DualChannelMode::WAVE_PROPAGATION);

    qDebug() << "TENS Phase 5: Climax (30 Hz, 300 μs) - Orgasm maintenance";
}

void TENSController::setPhaseRecovery()
{
    // Phase 6: Post-Climax Recovery (6-7 minutes)
    // Purpose: Gentle cooldown to prevent overstimulation
    // Vacuum: 4-5 Hz or OFF, 25-30% → 0% intensity
    // TENS: Very gentle or disabled

    setChannelFrequency(ElectrodeChannel::CLITORAL, 8.0);    // Very low
    setChannelPulseWidth(ElectrodeChannel::CLITORAL, 500);   // Wide for gentle sensation
    setChannelAmplitude(ElectrodeChannel::CLITORAL, 10.0);   // Sub-threshold

    // Urethral: minimal or disabled due to post-orgasm sensitivity
    setChannelFrequency(ElectrodeChannel::URETHRAL, 5.0);
    setChannelPulseWidth(ElectrodeChannel::URETHRAL, 150);
    setChannelAmplitude(ElectrodeChannel::URETHRAL, 5.0);    // Barely perceptible

    setPhaseSync(PhaseSync::CONTINUOUS);
    setDualChannelMode(DualChannelMode::SYNCHRONIZED);

    qDebug() << "TENS Phase 6: Recovery (8 Hz, 500 μs) - Post-climax cooldown";
}

// ============================================================================
// DUAL-CHANNEL ELECTRODE CONTROL
// ============================================================================

void TENSController::setActiveChannel(ElectrodeChannel channel)
{
    QMutexLocker locker(&m_mutex);
    m_activeChannel = channel;
    qDebug() << "TENS active channel set to" << static_cast<int>(channel);
}

int TENSController::getChannelIndex(ElectrodeChannel channel) const
{
    return (channel == ElectrodeChannel::URETHRAL) ? 1 : 0;
}

void TENSController::setChannelFrequency(ElectrodeChannel channel, double frequencyHz)
{
    QMutexLocker locker(&m_mutex);

    if (channel == ElectrodeChannel::BOTH) {
        setChannelFrequency(ElectrodeChannel::CLITORAL, frequencyHz);
        setChannelFrequency(ElectrodeChannel::URETHRAL, frequencyHz);
        return;
    }

    int idx = getChannelIndex(channel);
    double minFreq = (channel == ElectrodeChannel::URETHRAL) ? URETHRAL_MIN_FREQUENCY_HZ : MIN_FREQUENCY_HZ;
    double maxFreq = (channel == ElectrodeChannel::URETHRAL) ? URETHRAL_MAX_FREQUENCY_HZ : MAX_FREQUENCY_HZ;

    m_channelParams[idx].frequencyHz = std::clamp(frequencyHz, minFreq, maxFreq);

    // Update primary parameters if this is the active channel
    if (channel == ElectrodeChannel::CLITORAL) {
        m_frequencyHz = m_channelParams[idx].frequencyHz;
        calculateTiming();
    } else {
        calculateUrethralTiming();
    }

    qDebug() << "TENS channel" << idx << "frequency set to" << m_channelParams[idx].frequencyHz << "Hz";
}

double TENSController::getChannelFrequency(ElectrodeChannel channel) const
{
    if (channel == ElectrodeChannel::BOTH) return m_channelParams[0].frequencyHz;
    return m_channelParams[getChannelIndex(channel)].frequencyHz;
}

void TENSController::setChannelPulseWidth(ElectrodeChannel channel, int microseconds)
{
    QMutexLocker locker(&m_mutex);

    if (channel == ElectrodeChannel::BOTH) {
        setChannelPulseWidth(ElectrodeChannel::CLITORAL, microseconds);
        setChannelPulseWidth(ElectrodeChannel::URETHRAL, microseconds);
        return;
    }

    int idx = getChannelIndex(channel);
    int minPW = (channel == ElectrodeChannel::URETHRAL) ? URETHRAL_MIN_PULSE_WIDTH_US : MIN_PULSE_WIDTH_US;
    int maxPW = (channel == ElectrodeChannel::URETHRAL) ? URETHRAL_MAX_PULSE_WIDTH_US : MAX_PULSE_WIDTH_US;

    m_channelParams[idx].pulseWidthUs = std::clamp(microseconds, minPW, maxPW);

    if (channel == ElectrodeChannel::CLITORAL) {
        m_pulseWidthUs = m_channelParams[idx].pulseWidthUs;
        calculateTiming();
    } else {
        calculateUrethralTiming();
    }

    qDebug() << "TENS channel" << idx << "pulse width set to" << m_channelParams[idx].pulseWidthUs << "μs";
}

int TENSController::getChannelPulseWidth(ElectrodeChannel channel) const
{
    if (channel == ElectrodeChannel::BOTH) return m_channelParams[0].pulseWidthUs;
    return m_channelParams[getChannelIndex(channel)].pulseWidthUs;
}

void TENSController::setChannelAmplitude(ElectrodeChannel channel, double percent)
{
    QMutexLocker locker(&m_mutex);

    if (channel == ElectrodeChannel::BOTH) {
        setChannelAmplitude(ElectrodeChannel::CLITORAL, percent);
        setChannelAmplitude(ElectrodeChannel::URETHRAL, percent);
        return;
    }

    int idx = getChannelIndex(channel);
    double maxAmp = (channel == ElectrodeChannel::URETHRAL) ?
                    (URETHRAL_MAX_AMPLITUDE_MA / MAX_AMPLITUDE_MA * 100.0) : 100.0;

    m_channelParams[idx].targetAmplitude = std::clamp(percent, 0.0, maxAmp);

    if (channel == ElectrodeChannel::CLITORAL) {
        m_targetAmplitude = m_channelParams[idx].targetAmplitude;
    }

    qDebug() << "TENS channel" << idx << "amplitude target set to" << m_channelParams[idx].targetAmplitude << "%";
    emit channelAmplitudeChanged(channel, m_channelParams[idx].targetAmplitude);
}

double TENSController::getChannelAmplitude(ElectrodeChannel channel) const
{
    if (channel == ElectrodeChannel::BOTH) return m_channelParams[0].amplitudePercent;
    return m_channelParams[getChannelIndex(channel)].amplitudePercent;
}

void TENSController::setChannelWaveform(ElectrodeChannel channel, Waveform type)
{
    QMutexLocker locker(&m_mutex);

    if (channel == ElectrodeChannel::BOTH) {
        setChannelWaveform(ElectrodeChannel::CLITORAL, type);
        setChannelWaveform(ElectrodeChannel::URETHRAL, type);
        return;
    }

    int idx = getChannelIndex(channel);

    // Urethral channel MUST use biphasic symmetric for safety
    if (channel == ElectrodeChannel::URETHRAL && type != Waveform::BIPHASIC_SYMMETRIC) {
        qWarning() << "TENS: Urethral channel requires BIPHASIC_SYMMETRIC waveform for safety";
        type = Waveform::BIPHASIC_SYMMETRIC;
    }

    m_channelParams[idx].waveformType = type;

    if (channel == ElectrodeChannel::CLITORAL) {
        m_waveformType = type;
    }

    qDebug() << "TENS channel" << idx << "waveform set to" << static_cast<int>(type);
}

TENSController::Waveform TENSController::getChannelWaveform(ElectrodeChannel channel) const
{
    if (channel == ElectrodeChannel::BOTH) return m_channelParams[0].waveformType;
    return m_channelParams[getChannelIndex(channel)].waveformType;
}

void TENSController::setUrethralEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);

    if (enabled && !canEnableUrethral()) {
        qWarning() << "TENS: Cannot enable urethral channel - safety check failed";
        return;
    }

    m_urethralEnabled = enabled;
    m_channelParams[1].enabled = enabled;

    if (enabled && m_running) {
        // Start urethral timer if main stimulation is running
        int timerIntervalMs = std::max(1, m_channelParams[1].pulseWidthUs / 1000);
        m_urethralTimer->setInterval(timerIntervalMs);
        m_urethralTimer->start();
        m_urethralPhaseTimer.start();
    } else if (!enabled) {
        m_urethralTimer->stop();
        setUrethralOutputPhase(OutputPhase::IDLE);
    }

    qDebug() << "TENS urethral channel" << (enabled ? "enabled" : "disabled");
    emit urethralEnabled(enabled);
}

bool TENSController::isUrethralElectrodeConnected() const
{
    // Check urethral electrode impedance to determine connection
    return m_urethralConnected && m_urethralImpedance < URETHRAL_MAX_IMPEDANCE_OHMS;
}

void TENSController::setArousalSyncMode(ArousalSyncMode mode)
{
    QMutexLocker locker(&m_mutex);
    m_arousalSyncMode = mode;
    qDebug() << "TENS arousal sync mode set to" << static_cast<int>(mode);
    emit arousalSyncModeChanged(mode);
}

void TENSController::onArousalLevelChanged(double arousalLevel)
{
    QMutexLocker locker(&m_mutex);
    m_currentArousalLevel = std::clamp(arousalLevel, 0.0, 1.0);

    if (m_arousalSyncMode != ArousalSyncMode::MANUAL) {
        applyArousalSyncParameters(m_currentArousalLevel);
    }
}

void TENSController::applyArousalSyncParameters(double arousalLevel)
{
    // Apply stimulation parameters based on arousal level and sync mode
    // Based on research: different phases benefit from different stimulation patterns

    switch (m_arousalSyncMode) {
        case ArousalSyncMode::ENGORGEMENT:
            // Low frequency to enhance blood flow (5-20 Hz)
            setChannelFrequency(ElectrodeChannel::BOTH, 5.0 + arousalLevel * 15.0);
            setChannelAmplitude(ElectrodeChannel::BOTH, 20.0 + arousalLevel * 20.0);
            break;

        case ArousalSyncMode::AROUSAL_BUILD:
            // Moderate frequency synchronized with oscillation (20-50 Hz)
            setChannelFrequency(ElectrodeChannel::BOTH, 20.0 + arousalLevel * 30.0);
            setChannelAmplitude(ElectrodeChannel::BOTH, 30.0 + arousalLevel * 30.0);
            setPhaseSync(PhaseSync::SYNC_SUCTION);
            break;

        case ArousalSyncMode::NEAR_ORGASM:
            // High frequency bursts (50-100 Hz)
            setChannelFrequency(ElectrodeChannel::BOTH, 50.0 + arousalLevel * 50.0);
            setChannelAmplitude(ElectrodeChannel::BOTH, 50.0 + arousalLevel * 30.0);
            break;

        case ArousalSyncMode::ORGASM_INDUCTION:
            // Maximum intensity with rapid frequency sweeps
            setChannelFrequency(ElectrodeChannel::BOTH, 80.0 + arousalLevel * 20.0);
            setChannelAmplitude(ElectrodeChannel::BOTH, 70.0 + arousalLevel * 30.0);
            break;

        case ArousalSyncMode::POST_ORGASM:
            // Reduce to low frequency or stop
            setChannelFrequency(ElectrodeChannel::BOTH, 10.0);
            setChannelAmplitude(ElectrodeChannel::BOTH, 15.0 * (1.0 - arousalLevel));
            break;

        case ArousalSyncMode::MANUAL:
        default:
            // No automatic adjustment
            break;
    }
}

void TENSController::setDualChannelMode(DualChannelMode mode)
{
    QMutexLocker locker(&m_mutex);
    m_dualChannelMode = mode;
    qDebug() << "TENS dual-channel mode set to" << static_cast<int>(mode);
}

void TENSController::setPhaseOffset(int microseconds)
{
    QMutexLocker locker(&m_mutex);
    m_phaseOffsetUs = std::clamp(microseconds, 0, 10000);  // Max 10ms offset
    qDebug() << "TENS phase offset set to" << m_phaseOffsetUs << "μs";
}

bool TENSController::canEnableUrethral() const
{
    if (!m_hardware) return false;

    // All standard safety checks
    if (!canEnable()) return false;

    // Additional urethral-specific checks
    if (m_urethralFault) {
        qWarning() << "TENS: Cannot enable urethral - fault detected:" << m_urethralFaultReason;
        return false;
    }

    // Check urethral electrode impedance (stricter limit)
    if (m_urethralImpedance > URETHRAL_MAX_IMPEDANCE_OHMS) {
        qWarning() << "TENS: Cannot enable urethral - impedance too high:"
                   << m_urethralImpedance << "Ω (max:" << URETHRAL_MAX_IMPEDANCE_OHMS << ")";
        return false;
    }

    // Verify biphasic symmetric waveform is set (required for urethral safety)
    if (m_channelParams[1].waveformType != Waveform::BIPHASIC_SYMMETRIC) {
        qWarning() << "TENS: Cannot enable urethral - must use BIPHASIC_SYMMETRIC waveform";
        return false;
    }

    return true;
}

bool TENSController::canEnable() const
{
    if (!m_hardware) return false;

    // Check emergency stop
    if (m_hardware->isEmergencyStop()) {
        qWarning() << "TENS: Cannot enable - emergency stop active";
        return false;
    }

    // Check vacuum seal integrity (clitoral cup must be sealed)
    double clitoralPressure = m_hardware->readClitoralPressure();
    if (clitoralPressure < m_minSealPressure) {
        qWarning() << "TENS: Cannot enable - seal pressure too low:"
                   << clitoralPressure << "mmHg (min:" << m_minSealPressure << ")";
        return false;
    }

    // Check for existing fault
    if (m_faultDetected) {
        qWarning() << "TENS: Cannot enable - fault detected:" << m_faultReason;
        return false;
    }

    // Check electrode impedance (if measurable)
    if (m_electrodeImpedance > MAX_IMPEDANCE_OHMS) {
        qWarning() << "TENS: Cannot enable - electrode impedance too high:"
                   << m_electrodeImpedance << "Ω";
        return false;
    }

    return true;
}

void TENSController::onVacuumPhaseChanged(bool isSuctionPhase)
{
    QMutexLocker locker(&m_mutex);
    m_vacuumSuctionPhase = isSuctionPhase;

    // Only relevant if sync is enabled
    if (!m_syncEnabled || !m_running) return;

    switch (m_phaseSync) {
        case PhaseSync::SYNC_SUCTION:
            // Enable TENS during suction, disable during vent
            m_enabled = isSuctionPhase;
            break;
        case PhaseSync::SYNC_VENT:
            // Enable TENS during vent, disable during suction
            m_enabled = !isSuctionPhase;
            break;
        case PhaseSync::ALTERNATING:
            // Toggle on each phase change
            m_enabled = !m_enabled;
            break;
        default:
            break;
    }
}

void TENSController::onTimerTick()
{
    QMutexLocker locker(&m_mutex);

    if (!m_running || !m_enabled) {
        return;
    }

    // Check fault status periodically
    checkFaultStatus();
    if (m_faultDetected) {
        locker.unlock();
        emergencyStop();
        return;
    }

    // Check sync conditions
    if (m_syncEnabled) {
        bool shouldOutput = false;
        switch (m_phaseSync) {
            case PhaseSync::SYNC_SUCTION:
                shouldOutput = m_vacuumSuctionPhase;
                break;
            case PhaseSync::SYNC_VENT:
                shouldOutput = !m_vacuumSuctionPhase;
                break;
            case PhaseSync::ALTERNATING:
            case PhaseSync::CONTINUOUS:
            default:
                shouldOutput = true;
                break;
        }

        if (!shouldOutput) {
            setOutputPhase(OutputPhase::IDLE);
            return;
        }
    }

    // Generate waveform based on type
    generatePulse();
}

void TENSController::onRampTimer()
{
    QMutexLocker locker(&m_mutex);

    // Ramp amplitude toward target
    if (qAbs(m_amplitudePercent - m_targetAmplitude) < qAbs(m_rampStep)) {
        m_amplitudePercent = m_targetAmplitude;
        m_rampTimer->stop();
    } else {
        m_amplitudePercent += m_rampStep;
        m_amplitudePercent = std::clamp(m_amplitudePercent, 0.0, 100.0);
    }

    updatePWMAmplitude();

    locker.unlock();
    emit amplitudeChanged(m_amplitudePercent);
}

void TENSController::generatePulse()
{
    qint64 elapsedUs = m_phaseTimer.nsecsElapsed() / 1000;

    // State machine for biphasic waveform
    switch (m_outputPhase) {
        case OutputPhase::IDLE:
            setOutputPhase(OutputPhase::POSITIVE);
            m_phaseTimer.restart();
            break;

        case OutputPhase::POSITIVE:
            if (elapsedUs >= m_positiveDurationUs) {
                setOutputPhase(OutputPhase::NEGATIVE);
                m_phaseTimer.restart();
            }
            break;

        case OutputPhase::NEGATIVE:
            if (elapsedUs >= m_negativeDurationUs) {
                setOutputPhase(OutputPhase::INTER_PULSE);
                m_phaseTimer.restart();
                m_pulseCount++;
                emit pulseCompleted(m_pulseCount);
            }
            break;

        case OutputPhase::INTER_PULSE:
            if (elapsedUs >= m_interPulseUs) {
                // Start next pulse
                setOutputPhase(OutputPhase::POSITIVE);
                m_phaseTimer.restart();
            }
            break;
    }
}

void TENSController::setOutputPhase(OutputPhase phase)
{
    if (m_outputPhase == phase) return;

    m_outputPhase = phase;

    // Set GPIO states based on phase via HardwareManager
    if (m_hardware) {
        switch (phase) {
            case OutputPhase::POSITIVE:
                // GPIO_TENS_PHASE = HIGH (positive polarity), enable active
                m_hardware->setTENSPhasePolarity(true);
                m_hardware->setTENSOutputEnable(true);
                break;
            case OutputPhase::NEGATIVE:
                // GPIO_TENS_PHASE = LOW (negative polarity), enable active
                m_hardware->setTENSPhasePolarity(false);
                m_hardware->setTENSOutputEnable(true);
                break;
            case OutputPhase::INTER_PULSE:
                // Brief pause between pulses - disable output but keep phase
                m_hardware->setTENSOutputEnable(false);
                break;
            case OutputPhase::IDLE:
            default:
                // Fully disabled
                m_hardware->setTENSOutputEnable(false);
                break;
        }
    }

    emit phaseChanged(phase);
}

void TENSController::updatePWMAmplitude()
{
    // Convert amplitude percentage to PWM duty cycle (0-1024 range)
    int pwmValue = static_cast<int>(m_amplitudePercent * 10.24);

    if (m_hardware) {
        m_hardware->setTENSPWMDutyCycle(pwmValue);
    }
}

void TENSController::checkFaultStatus()
{
    // Read GPIO_TENS_FAULT input via HardwareManager
    bool faultPin = false;
    if (m_hardware) {
        faultPin = m_hardware->readTENSFaultPin();
    }

    if (faultPin && !m_faultDetected) {
        m_faultDetected = true;
        m_faultReason = "Overcurrent or open circuit detected";
        emit faultDetected(m_faultReason);
    } else if (!faultPin && m_faultDetected) {
        m_faultDetected = false;
        m_faultReason.clear();
        emit faultCleared();
    }
}

void TENSController::checkElectrodeContact()
{
    // Electrode impedance measurement would require dedicated circuitry
    // For now, estimate based on seal pressure
    if (m_hardware) {
        double pressure = m_hardware->readClitoralPressure();
        // Good seal = likely good electrode contact
        bool goodContact = (pressure >= m_minSealPressure);
        emit electrodeContact(goodContact);
    }
}

void TENSController::calculateTiming()
{
    // Period in microseconds
    m_periodUs = static_cast<int>(1000000.0 / m_frequencyHz);

    // For biphasic symmetric: equal positive and negative phases
    m_positiveDurationUs = m_pulseWidthUs;
    m_negativeDurationUs = m_pulseWidthUs;

    // Inter-pulse gap is the remainder of the period
    m_interPulseUs = m_periodUs - m_positiveDurationUs - m_negativeDurationUs;

    // Ensure inter-pulse is never negative
    if (m_interPulseUs < 0) {
        // Reduce pulse width proportionally
        int maxPulseWidth = m_periodUs / 2;
        m_positiveDurationUs = maxPulseWidth;
        m_negativeDurationUs = maxPulseWidth;
        m_interPulseUs = 0;
        qWarning() << "TENS: Pulse width adjusted to" << maxPulseWidth
                   << "μs to fit frequency";
    }

    qDebug() << "TENS timing calculated:";
    qDebug() << "  Period:" << m_periodUs << "μs (" << m_frequencyHz << "Hz)";
    qDebug() << "  Positive phase:" << m_positiveDurationUs << "μs";
    qDebug() << "  Negative phase:" << m_negativeDurationUs << "μs";
    qDebug() << "  Inter-pulse:" << m_interPulseUs << "μs";
}

// ============================================================================
// URETHRAL CHANNEL IMPLEMENTATION
// ============================================================================

void TENSController::calculateUrethralTiming()
{
    double freq = m_channelParams[1].frequencyHz;
    int pulseWidth = m_channelParams[1].pulseWidthUs;

    // Period in microseconds
    m_urethralPeriodUs = static_cast<int>(1000000.0 / freq);

    // For biphasic symmetric: equal positive and negative phases
    m_urethralPositiveDurationUs = pulseWidth;
    m_urethralNegativeDurationUs = pulseWidth;

    // Inter-pulse gap is the remainder of the period
    m_urethralInterPulseUs = m_urethralPeriodUs - m_urethralPositiveDurationUs - m_urethralNegativeDurationUs;

    // Ensure inter-pulse is never negative
    if (m_urethralInterPulseUs < 0) {
        int maxPulseWidth = m_urethralPeriodUs / 2;
        m_urethralPositiveDurationUs = maxPulseWidth;
        m_urethralNegativeDurationUs = maxPulseWidth;
        m_urethralInterPulseUs = 0;
        qWarning() << "TENS Urethral: Pulse width adjusted to" << maxPulseWidth << "μs";
    }

    qDebug() << "TENS urethral timing calculated:";
    qDebug() << "  Period:" << m_urethralPeriodUs << "μs (" << freq << "Hz)";
    qDebug() << "  Positive phase:" << m_urethralPositiveDurationUs << "μs";
    qDebug() << "  Negative phase:" << m_urethralNegativeDurationUs << "μs";
    qDebug() << "  Inter-pulse:" << m_urethralInterPulseUs << "μs";
}

void TENSController::onUrethralTimerTick()
{
    QMutexLocker locker(&m_mutex);

    if (!m_running || !m_urethralEnabled) {
        return;
    }

    // Check urethral fault status
    checkUrethralContact();
    if (m_urethralFault) {
        m_urethralEnabled = false;
        emit urethralFault(m_urethralFaultReason);
        m_urethralTimer->stop();
        setUrethralOutputPhase(OutputPhase::IDLE);
        return;
    }

    // Handle dual-channel coordination modes
    switch (m_dualChannelMode) {
        case DualChannelMode::SYNCHRONIZED:
            // Urethral follows clitoral phase
            m_channelParams[1].outputPhase = m_outputPhase;
            break;

        case DualChannelMode::ALTERNATING:
            // Urethral is opposite phase
            if (m_outputPhase == OutputPhase::POSITIVE) {
                m_channelParams[1].outputPhase = OutputPhase::NEGATIVE;
            } else if (m_outputPhase == OutputPhase::NEGATIVE) {
                m_channelParams[1].outputPhase = OutputPhase::POSITIVE;
            } else {
                m_channelParams[1].outputPhase = m_outputPhase;
            }
            break;

        case DualChannelMode::PHASE_OFFSET:
        case DualChannelMode::WAVE_PROPAGATION:
        case DualChannelMode::INDEPENDENT:
        default:
            // Generate independent pulse
            generateUrethralPulse();
            break;
    }

    // Apply the urethral output phase
    setUrethralOutputPhase(m_channelParams[1].outputPhase);
}

void TENSController::generateUrethralPulse()
{
    qint64 elapsedUs = m_urethralPhaseTimer.nsecsElapsed() / 1000;

    // State machine for biphasic waveform (urethral channel)
    switch (m_channelParams[1].outputPhase) {
        case OutputPhase::IDLE:
            m_channelParams[1].outputPhase = OutputPhase::POSITIVE;
            m_urethralPhaseTimer.restart();
            break;

        case OutputPhase::POSITIVE:
            if (elapsedUs >= m_urethralPositiveDurationUs) {
                m_channelParams[1].outputPhase = OutputPhase::NEGATIVE;
                m_urethralPhaseTimer.restart();
            }
            break;

        case OutputPhase::NEGATIVE:
            if (elapsedUs >= m_urethralNegativeDurationUs) {
                m_channelParams[1].outputPhase = OutputPhase::INTER_PULSE;
                m_urethralPhaseTimer.restart();
                m_channelParams[1].pulseCount++;
            }
            break;

        case OutputPhase::INTER_PULSE:
            if (elapsedUs >= m_urethralInterPulseUs) {
                m_channelParams[1].outputPhase = OutputPhase::POSITIVE;
                m_urethralPhaseTimer.restart();
            }
            break;
    }
}

void TENSController::setUrethralOutputPhase(OutputPhase phase)
{
    if (m_channelParams[1].outputPhase == phase) return;

    m_channelParams[1].outputPhase = phase;

    // Set GPIO states for urethral channel via HardwareManager
    if (m_hardware) {
        switch (phase) {
            case OutputPhase::POSITIVE:
                m_hardware->setUrethralPhasePolarity(true);
                m_hardware->setUrethralOutputEnable(true);
                break;
            case OutputPhase::NEGATIVE:
                m_hardware->setUrethralPhasePolarity(false);
                m_hardware->setUrethralOutputEnable(true);
                break;
            case OutputPhase::INTER_PULSE:
            case OutputPhase::IDLE:
            default:
                m_hardware->setUrethralOutputEnable(false);
                break;
        }
    }
}

void TENSController::updateUrethralPWMAmplitude()
{
    // Convert amplitude percentage to PWM duty cycle (0-1024 range)
    // Apply urethral safety limit
    double safeAmplitude = std::min(m_channelParams[1].amplitudePercent,
                                     URETHRAL_MAX_AMPLITUDE_MA / MAX_AMPLITUDE_MA * 100.0);
    int pwmValue = static_cast<int>(safeAmplitude * 10.24);

    if (m_hardware) {
        m_hardware->setUrethralPWMDutyCycle(pwmValue);
    }
}

void TENSController::checkUrethralContact()
{
    // Check urethral electrode impedance
    // In a real implementation, this would read from dedicated impedance measurement circuitry
    // For now, we estimate based on whether the electrode is detected

    if (m_hardware) {
        // Placeholder: would read actual impedance measurement
        // m_urethralImpedance = m_hardware->readUrethralImpedance();

        bool goodContact = m_urethralImpedance < URETHRAL_MAX_IMPEDANCE_OHMS;
        m_urethralConnected = goodContact;

        if (!goodContact && m_urethralEnabled) {
            m_urethralFault = true;
            m_urethralFaultReason = "Urethral electrode contact lost";
        }

        emit urethralContact(goodContact);
    }
}
