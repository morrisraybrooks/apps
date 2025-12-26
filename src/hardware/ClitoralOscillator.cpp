#include "ClitoralOscillator.h"
#include "HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <cmath>
#include <algorithm>

ClitoralOscillator::ClitoralOscillator(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_oscillationTimer(new QTimer(this))
    , m_running(false)
    , m_currentPhase(Phase::IDLE)
    , m_cycleCount(0)
    , m_frequencyHz(DEFAULT_FREQUENCY_HZ)
    , m_periodMs(static_cast<int>(1000.0 / DEFAULT_FREQUENCY_HZ))
    , m_suctionRatio(DEFAULT_SUCTION_RATIO)
    , m_holdRatio(DEFAULT_HOLD_RATIO)
    , m_ventRatio(DEFAULT_VENT_RATIO)
    , m_transitionRatio(DEFAULT_TRANSITION_RATIO)
    , m_targetAmplitude(DEFAULT_AMPLITUDE_MMHG)
    , m_dutyCycle(0.5)
    , m_measuredPeakPressure(0.0)
    , m_measuredTroughPressure(0.0)
{
    // High-precision timer for smooth oscillation
    m_oscillationTimer->setTimerType(Qt::PreciseTimer);
    m_oscillationTimer->setInterval(TIMER_RESOLUTION_MS);
    connect(m_oscillationTimer, &QTimer::timeout, this, &ClitoralOscillator::onTimerTick);

    // Calculate initial phase durations
    calculatePhaseDurations();

    qDebug() << "ClitoralOscillator initialized:"
             << "Frequency:" << m_frequencyHz << "Hz"
             << "Period:" << m_periodMs << "ms"
             << "Amplitude:" << m_targetAmplitude << "mmHg";
}

ClitoralOscillator::~ClitoralOscillator()
{
    stop();
}

void ClitoralOscillator::start()
{
    QMutexLocker locker(&m_mutex);

    if (m_running) {
        qWarning() << "ClitoralOscillator already running";
        return;
    }

    if (!m_hardware || !m_hardware->isReady()) {
        emit error("Hardware not ready");
        return;
    }

    qDebug() << "Starting ClitoralOscillator at" << m_frequencyHz << "Hz";

    m_running = true;
    m_cycleCount = 0;
    m_currentPhase = Phase::SUCTION;
    m_phaseTimer.start();

    // Start with suction phase
    executePhase(Phase::SUCTION);

    m_oscillationTimer->start();
    emit oscillationStarted();
}

void ClitoralOscillator::stop()
{
    QMutexLocker locker(&m_mutex);

    if (!m_running) {
        return;
    }

    qDebug() << "Stopping ClitoralOscillator after" << m_cycleCount << "cycles";

    m_oscillationTimer->stop();
    m_running = false;
    m_currentPhase = Phase::IDLE;

    // Safety: vent the clitoral cylinder
    if (m_hardware) {
        m_hardware->setSOL4(false);  // Close vacuum
        m_hardware->setSOL5(true);   // Open vent
    }

    emit oscillationStopped();
}

void ClitoralOscillator::emergencyStop()
{
    QMutexLocker locker(&m_mutex);

    qWarning() << "ClitoralOscillator EMERGENCY STOP";

    // Immediately stop the timer
    m_oscillationTimer->stop();
    m_running = false;
    m_currentPhase = Phase::IDLE;

    // Safety: Immediately vent the clitoral cylinder to release all pressure
    if (m_hardware) {
        m_hardware->setSOL4(false);  // Close vacuum source
        m_hardware->setSOL5(true);   // Open vent to atmosphere
    }

    emit oscillationStopped();
}

void ClitoralOscillator::pulse(double intensity, int durationMs)
{
    QMutexLocker locker(&m_mutex);

    // Clamp intensity to valid range
    intensity = std::clamp(intensity, 0.0, 1.0);
    durationMs = std::clamp(durationMs, 10, 1000);

    if (!m_hardware) {
        return;
    }

    // Apply a single vacuum pulse for haptic feedback
    double savedAmplitude = m_targetAmplitude;
    m_targetAmplitude = intensity * 100.0;  // Convert to mmHg

    // Quick suction pulse
    m_hardware->setSOL5(false);  // Close vent
    m_hardware->setSOL4(true);   // Open vacuum

    // Use a single-shot timer to end the pulse
    QTimer::singleShot(durationMs, this, [this, savedAmplitude]() {
        QMutexLocker locker(&m_mutex);
        m_hardware->setSOL4(false);  // Close vacuum
        m_hardware->setSOL5(true);   // Open vent
        m_targetAmplitude = savedAmplitude;
    });
}

void ClitoralOscillator::setFrequency(double frequencyHz)
{
    QMutexLocker locker(&m_mutex);

    // Clamp to valid range
    frequencyHz = std::clamp(frequencyHz, MIN_FREQUENCY_HZ, MAX_FREQUENCY_HZ);

    if (std::abs(m_frequencyHz - frequencyHz) < 0.01) {
        return;  // No significant change
    }

    m_frequencyHz = frequencyHz;
    m_periodMs = static_cast<int>(1000.0 / frequencyHz);

    calculatePhaseDurations();

    qDebug() << "ClitoralOscillator frequency set to" << frequencyHz << "Hz"
             << "(period:" << m_periodMs << "ms)";
}

void ClitoralOscillator::setAmplitude(double pressureMmHg)
{
    QMutexLocker locker(&m_mutex);

    m_targetAmplitude = std::clamp(pressureMmHg, MIN_AMPLITUDE_MMHG, SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG);

    qDebug() << "ClitoralOscillator amplitude set to" << m_targetAmplitude << "mmHg";
}

void ClitoralOscillator::setDutyCycle(double dutyCycle)
{
    QMutexLocker locker(&m_mutex);

    m_dutyCycle = std::clamp(dutyCycle, 0.1, 0.9);

    // Adjust phase ratios based on duty cycle
    m_suctionRatio = m_dutyCycle * 0.7;  // 70% of duty for suction
    m_holdRatio = m_dutyCycle * 0.3;      // 30% of duty for hold
    m_ventRatio = (1.0 - m_dutyCycle) * 0.7;
    m_transitionRatio = (1.0 - m_dutyCycle) * 0.3;

    calculatePhaseDurations();

    qDebug() << "ClitoralOscillator duty cycle set to" << (m_dutyCycle * 100) << "%";
}

void ClitoralOscillator::setPhaseTiming(double suctionRatio, double holdRatio,
                                        double ventRatio, double transitionRatio)
{
    QMutexLocker locker(&m_mutex);

    // Normalize ratios to sum to 1.0
    double total = suctionRatio + holdRatio + ventRatio + transitionRatio;
    if (total <= 0) {
        return;
    }

    m_suctionRatio = suctionRatio / total;
    m_holdRatio = holdRatio / total;
    m_ventRatio = ventRatio / total;
    m_transitionRatio = transitionRatio / total;

    calculatePhaseDurations();
}

double ClitoralOscillator::getCurrentPressure() const
{
    if (m_hardware) {
        return m_hardware->readClitoralPressure();
    }
    return 0.0;
}

// ============================================================================
// Presets based on research (8-13 Hz optimal orgasm frequency band)
// ============================================================================

void ClitoralOscillator::setPresetWarmup()
{
    setFrequency(5.0);
    setAmplitude(20.0);
    setDutyCycle(0.4);
    qDebug() << "ClitoralOscillator preset: Warmup (5 Hz, 20 mmHg)";
}

void ClitoralOscillator::setPresetBuildUp()
{
    setFrequency(8.0);
    setAmplitude(40.0);
    setDutyCycle(0.5);
    qDebug() << "ClitoralOscillator preset: Build-up (8 Hz, 40 mmHg)";
}

void ClitoralOscillator::setPresetClimax()
{
    setFrequency(11.0);  // In the 8-13 Hz optimal band
    setAmplitude(55.0);
    setDutyCycle(0.6);
    qDebug() << "ClitoralOscillator preset: Climax (11 Hz, 55 mmHg)";
}

void ClitoralOscillator::setPresetAfterGlow()
{
    setFrequency(4.0);
    setAmplitude(15.0);
    setDutyCycle(0.35);
    qDebug() << "ClitoralOscillator preset: Afterglow (4 Hz, 15 mmHg)";
}

// ============================================================================
// Phase calculation and execution
// ============================================================================

void ClitoralOscillator::calculatePhaseDurations()
{
    // Calculate phase durations based on period and ratios
    m_suctionDurationMs = static_cast<int>(m_periodMs * m_suctionRatio);
    m_holdDurationMs = static_cast<int>(m_periodMs * m_holdRatio);
    m_ventDurationMs = static_cast<int>(m_periodMs * m_ventRatio);
    m_transitionDurationMs = static_cast<int>(m_periodMs * m_transitionRatio);

    // Ensure minimum 1ms for each phase
    m_suctionDurationMs = std::max(1, m_suctionDurationMs);
    m_holdDurationMs = std::max(1, m_holdDurationMs);
    m_ventDurationMs = std::max(1, m_ventDurationMs);
    m_transitionDurationMs = std::max(1, m_transitionDurationMs);

    qDebug() << "Phase durations (ms): Suction=" << m_suctionDurationMs
             << "Hold=" << m_holdDurationMs
             << "Vent=" << m_ventDurationMs
             << "Transition=" << m_transitionDurationMs;
}

void ClitoralOscillator::executePhase(Phase phase)
{
    if (!m_hardware) return;

    switch (phase) {
        case Phase::SUCTION:
            // Open vacuum valve, close vent - build vacuum rapidly
            m_hardware->setSOL5(false);  // Close vent first (safety)
            m_hardware->setSOL4(true);   // Open vacuum
            break;

        case Phase::HOLD:
            // Both valves closed - maintain peak pressure
            m_hardware->setSOL4(false);
            m_hardware->setSOL5(false);
            // Measure peak pressure for amplitude adjustment
            m_measuredPeakPressure = getCurrentPressure();
            break;

        case Phase::VENT:
            // Close vacuum, open vent - release pressure rapidly
            m_hardware->setSOL4(false);  // Close vacuum first (safety)
            m_hardware->setSOL5(true);   // Open vent
            break;

        case Phase::TRANSITION:
            // Both valves closed - settle at minimum pressure
            m_hardware->setSOL4(false);
            m_hardware->setSOL5(false);
            // Measure trough pressure
            m_measuredTroughPressure = getCurrentPressure();
            break;

        case Phase::IDLE:
            // Safety: vent and close
            m_hardware->setSOL4(false);
            m_hardware->setSOL5(true);
            break;
    }

    emit phaseChanged(phase);
}

void ClitoralOscillator::advancePhase()
{
    Phase nextPhase;

    switch (m_currentPhase) {
        case Phase::SUCTION:
            nextPhase = Phase::HOLD;
            break;
        case Phase::HOLD:
            nextPhase = Phase::VENT;
            break;
        case Phase::VENT:
            nextPhase = Phase::TRANSITION;
            break;
        case Phase::TRANSITION:
            nextPhase = Phase::SUCTION;
            m_cycleCount++;
            emit cycleCompleted(m_cycleCount);
            // Adjust amplitude every few cycles based on measured pressure
            if (m_cycleCount % 5 == 0) {
                adjustAmplitude();
            }
            break;
        default:
            return;
    }

    m_currentPhase = nextPhase;
    m_phaseTimer.restart();
    executePhase(nextPhase);
}

void ClitoralOscillator::onTimerTick()
{
    if (!m_running) return;

    qint64 elapsedMs = m_phaseTimer.elapsed();
    int currentPhaseDuration = 0;

    switch (m_currentPhase) {
        case Phase::SUCTION:
            currentPhaseDuration = m_suctionDurationMs;
            break;
        case Phase::HOLD:
            currentPhaseDuration = m_holdDurationMs;
            break;
        case Phase::VENT:
            currentPhaseDuration = m_ventDurationMs;
            break;
        case Phase::TRANSITION:
            currentPhaseDuration = m_transitionDurationMs;
            break;
        default:
            return;
    }

    if (elapsedMs >= currentPhaseDuration) {
        advancePhase();
    }
}

void ClitoralOscillator::adjustAmplitude()
{
    // Simple amplitude feedback: adjust duty cycle based on measured peak
    double peakError = m_targetAmplitude - m_measuredPeakPressure;

    if (std::abs(peakError) > 5.0) {  // Only adjust if error > 5 mmHg
        // Increase duty cycle if we need more vacuum, decrease if too much
        double adjustment = peakError * 0.01;  // Small adjustment factor
        double newDutyCycle = m_dutyCycle + adjustment;

        // Apply with clamping (done inside setDutyCycle)
        setDutyCycle(newDutyCycle);

        qDebug() << "Amplitude adjustment: target=" << m_targetAmplitude
                 << "measured=" << m_measuredPeakPressure
                 << "new duty cycle=" << (m_dutyCycle * 100) << "%";
    }

    emit amplitudeReached(m_measuredPeakPressure);
}
