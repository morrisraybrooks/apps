#include "PneumaticController.h"

#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <algorithm>
#include <cmath>

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
    connect(m_timer, &QTimer::timeout, this, [this]() { tick(QDateTime::currentMSecsSinceEpoch()); });
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

void PneumaticController::start()
{
    m_active = true;
    m_timer->start();
}

void PneumaticController::setLegTarget(double mmHg)
{
    m_legTarget = mmHg <= 0.0 ? 0.0 : std::clamp(mmHg, m_settings.legMinMmHg, m_settings.legMaxMmHg);
    if (m_legTarget > 0.0) m_lastLegTarget = m_legTarget;
    m_legReg.setTarget(m_legTarget);
}

void PneumaticController::holdLegs()
{
    setLegTarget(m_lastLegTarget);
}

void PneumaticController::setClitoralTarget(double mmHg)
{
    m_requestedClit = std::max(0.0, mmHg);
    applyClitoralTarget();
}

double PneumaticController::clitoralTarget() const
{
    return m_clitReg.target();
}

void PneumaticController::applyClitoralTarget()
{
    double ceiling = std::min(m_settings.clitMaxMmHg, PulserCalibration::CAP_MMHG);
    if (m_pulser.isEnabled() && m_calibration.isValid()) {
        ceiling = std::min(ceiling, m_calibration.maxClitoralTarget(m_pulser.rate()));
    }
    m_clitReg.setTarget(std::min(m_requestedClit, ceiling));
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
    m_clitReg.setAveraging(on);
    applyClitoralTarget();
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
    m_pulser.setEnabled(false);
    m_benchDuty = -1.0;
    m_clitReg.setAveraging(false);
    setLegTarget(0.0);
    m_requestedClit = 0.0;
    applyClitoralTarget();
}

void PneumaticController::stop()
{
    ventAll();
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
    m_ventingTank = true;
}

void PneumaticController::emergencyStop()
{
    m_active = false;
    m_timer->stop();
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

    // Chambers
    const ValveCommand legCmd = m_legReg.update(leg, nowMs);
    const ValveCommand clitCmd = m_clitReg.update(clit, nowMs);

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
    if (m_benchDuty >= 0.0) pulse = PulserOutput{m_benchDuty > 0.0, m_benchDuty};
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
