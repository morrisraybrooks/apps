#ifndef PNEUMATICCONTROLLER_H
#define PNEUMATICCONTROLLER_H

#include "ChamberRegulator.h"
#include "IPneumaticHardware.h"
#include "PneumaticSettings.h"
#include "PulserCalibration.h"
#include "PulserDrive.h"
#include "SealWatch.h"
#include "TankLogic.h"

#include <QObject>
#include <QString>

class QTimer;

/**
 * @brief Runs the air side at 100 Hz: tank fill-then-stop, leg and clitoral
 * chambers held at their own targets, seal watch on the legs, and the clitoral
 * pulser. Nothing else should switch the pump, SOL1/2/4/5 or the pulser.
 *
 * Design: docs/superpowers/specs/2026-11-16-clitoral-pulser-system-design.md
 */
class PneumaticController : public QObject
{
    Q_OBJECT

public:
    PneumaticController(IPneumaticHardware* hardware, PneumaticSettings settings,
                        QObject* parent = nullptr);
    ~PneumaticController() override;

    bool loadCalibration(const QString& path);
    bool hasCalibration() const { return m_calibration.isValid(); }

    void start();
    void tick(qint64 nowMs);
    bool isActive() const { return m_active; }

    void setLegTarget(double mmHg);
    double legTarget() const { return m_legTarget; }
    void holdLegs();
    void setClitoralTarget(double mmHg);
    double clitoralTarget() const;
    void setPulseRate(double pps);
    double pulseRate() const { return m_pulser.rate(); }
    bool setPulserEnabled(bool on);
    bool isPulserEnabled() const { return m_pulser.isEnabled(); }

    void ventAll();
    void ventTank();
    void emergencyStop();
    bool isSealRecovering() const { return m_seal.state() == SealState::Recovering; }
    SealState sealState() const { return m_seal.state(); }

    /// Bench-only raw pulser duty (PulserBench tool); refused unless settings.benchMode.
    bool setBenchDuty(double percent);

signals:
    void sealFailed(const QString& reason);
    void sealStateChanged(SealState state);
    void pulserUnavailable(const QString& reason);
    void pulseBurstChanged(bool on);

private:
    void applyClitoralTarget();
    void write(int sol, bool open);
    void writePump(bool on);
    void writeDuty(double percent);

    IPneumaticHardware* m_hw;
    PneumaticSettings m_settings;
    PulserCalibration m_calibration;
    TankLogic m_tank;
    ChamberRegulator m_legReg;
    ChamberRegulator m_clitReg;
    SealWatch m_seal;
    PulserDrive m_pulser;
    QTimer* m_timer;
    QString m_calibrationError;

    bool m_active = false;
    double m_legTarget = 0.0;
    double m_lastLegTarget = 40.0;
    double m_requestedClit = 0.0;
    bool m_ventingTank = false;
    double m_benchDuty = -1.0;   ///< >= 0 while a bench duty overrides the pulser

    // Last written states (-1 = unknown, forces a write)
    int m_valveState[6] = {-1, -1, -1, -1, -1, -1};
    int m_pumpState = -1;
    double m_dutyState = -1.0;
    bool m_burstOn = false;
};

#endif // PNEUMATICCONTROLLER_H
