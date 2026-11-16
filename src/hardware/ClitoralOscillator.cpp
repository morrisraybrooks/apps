#include "ClitoralOscillator.h"
#include "HardwareManager.h"
#include "PneumaticController.h"

#include <QDebug>
#include <QMutexLocker>
#include <algorithm>

ClitoralOscillator::ClitoralOscillator(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_running(false)
    , m_currentPhase(Phase::IDLE)
    , m_cycleCount(0)
    , m_frequencyHz(DEFAULT_FREQUENCY_HZ)
    , m_targetAmplitude(DEFAULT_AMPLITUDE_MMHG)
    , m_dutyCycle(0.5)
    , m_suctionRatio(0.35)
    , m_holdRatio(0.10)
    , m_ventRatio(0.35)
    , m_transitionRatio(0.20)
{
    // Nothing is sent to the air side here: constructing the oscillator must not
    // request vacuum. Targets go out on setAmplitude()/start().
}

ClitoralOscillator::~ClitoralOscillator()
{
    if (m_running) stop();
}

PneumaticController* ClitoralOscillator::air() const
{
    return m_hardware ? m_hardware->pneumatics() : nullptr;
}

void ClitoralOscillator::onBurstChanged(bool on)
{
    m_currentPhase = on ? Phase::SUCTION : Phase::VENT;
    emit phaseChanged(m_currentPhase);
    emit vacuumPhaseChanged(on);
    if (on) emit cycleCompleted(++m_cycleCount);
}

void ClitoralOscillator::start()
{
    PneumaticController* controller = air();
    if (!controller) {
        emit error("Clitoral pulser unavailable: hardware not initialized");
        return;
    }
    if (m_running) return;

    controller->setPulseRate(m_frequencyHz);
    if (!controller->setPulserEnabled(true)) {
        emit error("Clitoral pulser not calibrated - run the bench calibration first");
        return;
    }

    m_running = true;
    m_cycleCount = 0;
    m_burstConnection = connect(controller, &PneumaticController::pulseBurstChanged,
                                this, &ClitoralOscillator::onBurstChanged);
    emit oscillationStarted();
    qDebug() << "Clitoral pulser started at" << m_frequencyHz << "pulses/s";
}

void ClitoralOscillator::stop()
{
    if (PneumaticController* controller = air()) {
        controller->setPulserEnabled(false);  // steady clitoral target is left as it is
    }
    disconnect(m_burstConnection);
    const bool wasRunning = m_running;
    m_running = false;
    m_currentPhase = Phase::IDLE;
    if (wasRunning) {
        emit vacuumPhaseChanged(false);
        emit oscillationStopped();
    }
}

void ClitoralOscillator::emergencyStop()
{
    if (PneumaticController* controller = air()) {
        controller->emergencyStop();
    }
    disconnect(m_burstConnection);
    m_running = false;
    m_currentPhase = Phase::IDLE;
    emit vacuumPhaseChanged(false);
    emit oscillationStopped();
}

void ClitoralOscillator::pulse(double intensity, int durationMs)
{
    Q_UNUSED(intensity);  // fixed-stroke pulser: strength is not adjustable
    durationMs = std::clamp(durationMs, 10, 1000);
    PneumaticController* controller = air();
    if (!controller || m_running) return;

    controller->setPulseRate(m_frequencyHz);
    if (!controller->setPulserEnabled(true)) {
        emit error("Clitoral pulser not calibrated - run the bench calibration first");
        return;
    }
    QTimer::singleShot(durationMs, this, [this]() {
        if (!m_running) {
            if (PneumaticController* c = air()) c->setPulserEnabled(false);
        }
    });
}

void ClitoralOscillator::setFrequency(double frequencyHz)
{
    m_frequencyHz = std::clamp(frequencyHz, MIN_FREQUENCY_HZ, MAX_FREQUENCY_HZ);
    if (PneumaticController* controller = air()) {
        controller->setPulseRate(m_frequencyHz);
    }
}

void ClitoralOscillator::setAmplitude(double pressureMmHg)
{
    m_targetAmplitude = std::clamp(pressureMmHg, MIN_AMPLITUDE_MMHG,
                                   SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG);
    if (PneumaticController* controller = air()) {
        controller->setClitoralTarget(m_targetAmplitude);  // further capped by the pulse allowance
    }
}

void ClitoralOscillator::setDutyCycle(double dutyCycle)
{
    m_dutyCycle = std::clamp(dutyCycle, 0.0, 1.0);  // no effect: fixed-stroke pulser
}

void ClitoralOscillator::setPhaseTiming(double suctionRatio, double holdRatio,
                                        double ventRatio, double transitionRatio)
{
    // No effect with the pulser; stored for API compatibility
    m_suctionRatio = suctionRatio;
    m_holdRatio = holdRatio;
    m_ventRatio = ventRatio;
    m_transitionRatio = transitionRatio;
}

double ClitoralOscillator::getCurrentPressure() const
{
    if (!m_hardware) return 0.0;
    try {
        return m_hardware->readClitoralPressure();
    } catch (const std::exception&) {
        return 0.0;
    }
}

// Presets: rates are now pulse rhythms (below the motor minimum they run as bursts);
// pressures are the steady clitoral targets. Re-map after bench measurements.
void ClitoralOscillator::setPresetWarmup()
{
    setFrequency(5.0);
    setAmplitude(20.0);
}

void ClitoralOscillator::setPresetBuildUp()
{
    setFrequency(8.0);
    setAmplitude(40.0);
}

void ClitoralOscillator::setPresetClimax()
{
    setFrequency(11.0);
    setAmplitude(55.0);
}

void ClitoralOscillator::setPresetAfterGlow()
{
    setFrequency(4.0);
    setAmplitude(15.0);
}
