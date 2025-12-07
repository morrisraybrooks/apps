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
    , m_periodUs(static_cast<int>(1000000.0 / DEFAULT_FREQUENCY_HZ))
    , m_positiveDurationUs(DEFAULT_PULSE_WIDTH_US)
    , m_negativeDurationUs(DEFAULT_PULSE_WIDTH_US)
    , m_interPulseUs(0)
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
    // High-precision timer for waveform generation
    m_waveformTimer->setTimerType(Qt::PreciseTimer);
    connect(m_waveformTimer, &QTimer::timeout, this, &TENSController::onTimerTick);

    // Ramp timer for soft start/stop
    m_rampTimer->setTimerType(Qt::PreciseTimer);
    m_rampTimer->setInterval(static_cast<int>(RAMP_INTERVAL_MS));
    connect(m_rampTimer, &QTimer::timeout, this, &TENSController::onRampTimer);

    calculateTiming();
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
    // TODO: Set GPIO_TENS_ENABLE HIGH via HardwareManager

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
    // TODO: Set GPIO_TENS_ENABLE LOW via HardwareManager

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

    // TODO: Set GPIO_TENS_ENABLE LOW immediately via HardwareManager

    locker.unlock();
    emit stimulationStopped();
}

// Presets based on clinical research
void TENSController::setPresetWarmup()
{
    setFrequency(10.0);
    setPulseWidth(200);
    setAmplitude(20.0);
    setPhaseSync(PhaseSync::CONTINUOUS);
    qDebug() << "TENS preset: Warmup (10 Hz, 200 μs, 20%)";
}

void TENSController::setPresetArousal()
{
    setFrequency(20.0);
    setPulseWidth(400);
    setAmplitude(40.0);
    setPhaseSync(PhaseSync::CONTINUOUS);
    qDebug() << "TENS preset: Arousal (20 Hz, 400 μs, 40%)";
}

void TENSController::setPresetClimax()
{
    setFrequency(30.0);
    setPulseWidth(300);
    setAmplitude(70.0);
    setPhaseSync(PhaseSync::SYNC_SUCTION);
    qDebug() << "TENS preset: Climax (30 Hz, 300 μs, 70%)";
}

void TENSController::setPresetAfterGlow()
{
    setFrequency(10.0);
    setPulseWidth(500);
    setAmplitude(15.0);
    setPhaseSync(PhaseSync::CONTINUOUS);
    qDebug() << "TENS preset: Afterglow (10 Hz, 500 μs, 15%)";
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

    // Set GPIO states based on phase
    // TODO: Implement via HardwareManager GPIO control
    switch (phase) {
        case OutputPhase::POSITIVE:
            // GPIO_TENS_PHASE = HIGH, GPIO_TENS_ENABLE = HIGH
            qDebug() << "TENS: Positive phase";
            break;
        case OutputPhase::NEGATIVE:
            // GPIO_TENS_PHASE = LOW, GPIO_TENS_ENABLE = HIGH
            qDebug() << "TENS: Negative phase";
            break;
        case OutputPhase::INTER_PULSE:
        case OutputPhase::IDLE:
        default:
            // GPIO_TENS_ENABLE = LOW (or both low)
            break;
    }

    emit phaseChanged(phase);
}

void TENSController::updatePWMAmplitude()
{
    // Convert amplitude percentage to PWM duty cycle
    // TODO: Implement via HardwareManager PWM control
    int pwmValue = static_cast<int>(m_amplitudePercent * 10.24);  // 0-1024 range
    Q_UNUSED(pwmValue);
    // m_hardware->setTENSPWM(pwmValue);
}

void TENSController::checkFaultStatus()
{
    // TODO: Read GPIO_TENS_FAULT input via HardwareManager
    // For now, assume no fault
    // bool faultPin = m_hardware->readTENSFault();
    bool faultPin = false;

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
