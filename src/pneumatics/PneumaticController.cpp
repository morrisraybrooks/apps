#include "PneumaticController.h"

#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace {
/// A feed valve only opens while the tank is at least this much deeper than the
/// chamber: solenoids flow both ways, so a shallow tank would pull the chamber down.
constexpr double FEED_MARGIN_MMHG = 1.0;
}  // namespace

PneumaticController::PneumaticController(IPneumaticHardware* hardware, PneumaticSettings settings,
                                         QObject* parent)
    : QObject(parent)
    , m_hw(hardware)
    , m_settings(settings)
    , m_tank(settings)
    , m_legReg(settings)
    , m_clitReg(settings)
    , m_seal(settings)
    , m_pulser(&m_calibration)
    , m_timer(new QTimer(this))
    , m_calibrationError("No pulser calibration loaded - measure it on the bench first")
{
    m_timer->setInterval(10);  // 100 Hz
    m_clock.start();           // monotonic: the Pi has no RTC and its wall clock can jump
    connect(m_timer, &QTimer::timeout, this, [this]() { tick(m_clock.elapsed()); });
}

PneumaticController::~PneumaticController() = default;

bool PneumaticController::loadCalibration(const QString& path)
{
    QString error;
    const bool ok = m_calibration.loadFromFile(path, &error);
    m_calibrationError = ok ? QString() : error;
    if (!ok) {
        qWarning() << "Pulser unavailable:" << error;
        m_pulser.setEnabled(false);
    }
    applyClitoralTarget();
    return ok;
}

void PneumaticController::resetState()
{
    m_seal.reset();
    m_legReg.reset();
    m_clitReg.reset();
    m_clitSamples.clear();
    // Forget what we think the outputs are, so the next tick rewrites all of them
    // (writes refused during a hardware emergency stop must not stay "done")
    std::fill(std::begin(m_valveState), std::end(m_valveState), -1);
    m_pumpState = -1;
    m_dutyState = -1.0;
}

void PneumaticController::rearm()
{
    m_latched = false;
    resetState();
}

void PneumaticController::start()
{
    m_latched = false;
    resetState();
    m_active = true;
    m_timer->start();
}

void PneumaticController::autoStart()
{
    if (!m_active && !m_latched) start();
}

void PneumaticController::setLegTarget(double mmHg)
{
    m_legTarget = mmHg <= 0.0 ? 0.0 : std::clamp(mmHg, m_settings.legMinMmHg, m_settings.legMaxMmHg);
    if (m_legTarget > 0.0) m_lastLegTarget = m_legTarget;
    m_legReg.setTarget(m_legTarget);
    if (m_legTarget > 0.0) {
        m_hadSession = true;
        autoStart();
    }
}

void PneumaticController::holdLegs()
{
    setLegTarget(m_lastLegTarget);
}

void PneumaticController::setClitoralTarget(double mmHg)
{
    m_requestedClit = std::max(0.0, mmHg);
    applyClitoralTarget();
    if (m_requestedClit > 0.0) {
        m_hadSession = true;
        autoStart();
    }
}

double PneumaticController::clitoralTarget() const
{
    return m_clitReg.target();
}

double PneumaticController::pulseCeiling() const
{
    // Highest steady clitoral pressure at which pulses at the current rate stay under 75 mmHg
    if (!m_pulser.isEnabled() || !m_calibration.isValid()) return PulserCalibration::CAP_MMHG;
    return m_calibration.maxClitoralTarget(m_pulser.rate());
}

void PneumaticController::setUserPaused(bool paused)
{
    if (paused == m_userPaused) return;
    m_userPaused = paused;
    if (!paused) {
        m_resumeWait = true;
        m_resumeWaitUntil = -1;
    }
    applyClitoralTarget();
    emit userPauseChanged(paused);
}

void PneumaticController::clearUserPause()
{
    m_resumeWait = false;
    if (!m_userPaused) return;
    m_userPaused = false;
    emit userPauseChanged(false);
}

double PneumaticController::maxPulseRate() const
{
    return m_calibration.isValid() ? m_calibration.maxRate() : 100.0;
}

void PneumaticController::applyClitoralTarget()
{
    if (m_userPaused) {
        m_clitReg.setTarget(0.0);   // paused: clitoral chamber vented whatever was asked
        return;
    }
    double ceiling = std::min(m_settings.clitMaxMmHg, PulserCalibration::CAP_MMHG);
    if (m_pulser.isEnabled() && m_calibration.isValid()) {
        // The regulator lets the steady level sit up to target + ventAbove, so keep that
        // whole band under the pulse ceiling
        ceiling = std::min(ceiling, pulseCeiling() - m_settings.ventAboveMmHg);
    }
    m_clitReg.setTarget(std::min(m_requestedClit, std::max(0.0, ceiling)));
}

double PneumaticController::clitMean(double clit, qint64 nowMs)
{
    m_clitSamples.emplace_back(nowMs, clit);
    while (!m_clitSamples.empty() && m_clitSamples.front().first < nowMs - m_settings.averageWindowMs) {
        m_clitSamples.pop_front();
    }
    double sum = 0.0;
    for (const auto& s : m_clitSamples) sum += s.second;
    return sum / static_cast<double>(m_clitSamples.size());
}

void PneumaticController::goIdle()
{
    // Session over and both chambers vented: stop the loop so the pump never
    // restarts with nobody at the machine. Vents stay open.
    m_active = false;
    m_hadSession = false;
    m_timer->stop();
    writeDuty(0.0);
    write(1, false);
    write(4, false);
    writePump(false);
}

void PneumaticController::setPulseRate(double pps)
{
    m_pulser.setRate(pps);
    applyClitoralTarget();
}

bool PneumaticController::setPulserEnabled(bool on)
{
    if (on && !m_calibration.isValid()) {
        m_pulser.setEnabled(false);
        emit pulserUnavailable(m_calibrationError);
        return false;
    }
    m_pulser.setEnabled(on);
    if (on) ++m_pulserOnCount;
    m_clitReg.setAveraging(on);
    applyClitoralTarget();
    if (on) {
        m_hadSession = true;
        autoStart();
    }
    return true;
}

bool PneumaticController::setBenchDuty(double percent)
{
    if (!m_settings.benchMode) return false;
    m_benchDuty = percent < 0.0 ? -1.0 : std::clamp(percent, 0.0, 100.0);
    m_clitReg.setAveraging(m_benchDuty > 0.0);
    return true;
}

void PneumaticController::ventAll()
{
    clearUserPause();
    m_pulser.setEnabled(false);
    m_benchDuty = -1.0;
    m_clitReg.setAveraging(false);
    setLegTarget(0.0);
    m_requestedClit = 0.0;
    applyClitoralTarget();
}

void PneumaticController::stop()
{
    ventAll();   // also clears the pause
    m_active = false;
    m_timer->stop();
    m_ventingTank = false;
    writeDuty(0.0);
    write(1, false);
    write(4, false);
    write(2, true);
    write(5, true);
    writePump(false);
}

void PneumaticController::ventTank()
{
    if (m_latched) return;   // the emergency stop already opened SOL3
    m_ventingTank = true;
    if (!m_active) {
        m_hadSession = true;  // lets the loop go idle once the tank is empty
        start();
    }
}

void PneumaticController::emergencyStop()
{
    m_active = false;
    m_latched = true;
    m_timer->stop();
    clearUserPause();
    m_pulser.setEnabled(false);
    m_benchDuty = -1.0;
    m_ventingTank = false;
    m_legTarget = 0.0;
    m_legReg.setTarget(0.0);
    m_requestedClit = 0.0;
    m_clitReg.setTarget(0.0);

    // Fixed order (spec 5.5): pulser, feeds, vents, pump, tank vent
    m_hw->setPulserDuty(0.0);   m_dutyState = 0.0;
    m_hw->setValve(1, false);   m_valveState[1] = 0;
    m_hw->setValve(4, false);   m_valveState[4] = 0;
    m_hw->setValve(2, true);    m_valveState[2] = 1;
    m_hw->setValve(5, true);    m_valveState[5] = 1;
    m_hw->setPump(false);       m_pumpState = 0;
    m_hw->setValve(3, true);    m_valveState[3] = 1;
}

void PneumaticController::write(int sol, bool open)
{
    if (m_valveState[sol] != static_cast<int>(open)) {
        m_hw->setValve(sol, open);
        m_valveState[sol] = open;
    }
}

void PneumaticController::writePump(bool on)
{
    if (m_pumpState != static_cast<int>(on)) {
        m_hw->setPump(on);
        m_pumpState = on;
    }
}

void PneumaticController::writeDuty(double percent)
{
    if (m_dutyState != percent) {
        m_hw->setPulserDuty(percent);
        m_dutyState = percent;
    }
}

void PneumaticController::tick(qint64 nowMs)
{
    if (!m_active) return;

    const double leg = m_hw->legMmHg();
    const double tank = m_hw->tankMmHg();
    const double clit = m_hw->clitMmHg();
    if (!std::isfinite(leg) || !std::isfinite(tank) || !std::isfinite(clit)) {
        emergencyStop();
        emit sensorFault(QString("Invalid pressure reading (legs %1, tank %2, clitoral %3)")
                             .arg(leg).arg(tank).arg(clit));
        return;
    }
    const double clitAverage = clitMean(clit, nowMs);
    if (clitAverage >= PulserCalibration::CAP_MMHG - 0.5) {
        emergencyStop();
        emit sensorFault(QString("Clitoral chamber at %1 mmHg - at or above the 75 mmHg cap")
                             .arg(clitAverage, 0, 'f', 1));
        return;
    }
    if (m_hadSession && m_legTarget <= 0.0 && m_requestedClit <= 0.0 && !m_pulser.isEnabled()
        && m_benchDuty < 0.0 && !m_ventingTank && leg < 3.0 && clit < 3.0) {
        goIdle();
        return;
    }

    // Seal watch on the legs
    const SealState before = m_seal.state();
    const SealState seal = m_seal.update(leg, m_legTarget, nowMs);
    if (seal != before) emit sealStateChanged(seal);
    if (seal == SealState::Failed) {
        emergencyStop();
        emit sealFailed(QString("Leg seal lost: %1 mmHg against a %2 mmHg target after %3 reseal attempts")
                            .arg(leg, 0, 'f', 1).arg(m_lastLegTarget, 0, 'f', 1)
                            .arg(m_seal.attemptsUsed()));
        return;
    }

    // Chambers (a feed waits while the tank is not deeper than the chamber)
    ValveCommand legCmd = m_legReg.update(leg, nowMs);
    ValveCommand clitCmd = m_clitReg.update(clit, nowMs);
    if (tank < leg + FEED_MARGIN_MMHG) legCmd.feedOpen = false;
    if (tank < clit + FEED_MARGIN_MMHG) clitCmd.feedOpen = false;

    // Tank
    double demand = 0.0;
    if (m_legReg.needsTopUp()) demand = std::max(demand, m_legReg.target());
    if (m_clitReg.needsTopUp()) demand = std::max(demand, m_clitReg.target());
    bool pumpOn = m_tank.update(tank, demand);
    if (m_ventingTank) {
        pumpOn = false;
        if (tank < 5.0) m_ventingTank = false;
    }

    // Pulser
    PulserOutput pulse = m_pulser.update(nowMs);
    if (pulse.on && clitAverage > pulseCeiling()) {
        pulse = PulserOutput{};  // wait until the steady level is low enough for this rate
    }
    if (m_benchDuty >= 0.0) pulse = PulserOutput{m_benchDuty > 0.0, m_benchDuty};
    if (m_resumeWait) {
        if (m_resumeWaitUntil < 0) m_resumeWaitUntil = nowMs + 3000;
        const double target = m_clitReg.target();
        if (target <= 0.0 || clitAverage >= target - m_settings.feedBelowMmHg
            || nowMs >= m_resumeWaitUntil) {
            m_resumeWait = false;
        }
    }
    if (m_userPaused || m_resumeWait) pulse = PulserOutput{};
    if (pulse.on != m_burstOn) {
        m_burstOn = pulse.on;
        emit pulseBurstChanged(pulse.on);
    }

    writeDuty(pulse.on ? pulse.pwmPercent : 0.0);
    write(1, legCmd.feedOpen);
    write(2, legCmd.ventOpen);
    write(4, clitCmd.feedOpen);
    write(5, clitCmd.ventOpen);
    write(3, m_ventingTank);
    writePump(pumpOn);
}
