#ifndef PNEUMATICCONTROLLER_H
#define PNEUMATICCONTROLLER_H

#include "ChamberRegulator.h"
#include "IPneumaticHardware.h"
#include "PneumaticSettings.h"
#include "PulserCalibration.h"
#include "PulserDrive.h"
#include "SealWatch.h"
#include "TankLogic.h"

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <deque>
#include <utility>

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
    /// Why the pulser can't run (empty once a valid calibration is loaded)
    QString calibrationError() const { return m_calibrationError; }

    /// Start the 100 Hz loop (also clears the emergency latch). Requests for vacuum
    /// (targets > 0, holdLegs, pulser on) start it automatically unless latched.
    void start();
    /// Clear the emergency latch without starting (HardwareManager::resetEmergencyStop).
    void rearm();
    bool isLatched() const { return m_latched; }
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
    /// Goes up every time the pulser is switched on, so a caller can tell whether
    /// someone else switched it on after it did (shared pulser, ClitoralOscillator::pulse)
    quint64 pulserOnCount() const { return m_pulserOnCount; }

    void ventAll();
    /// Normal end / shutdown: pulser and pump off, both chambers vented, tank keeps its vacuum.
    void stop();
    /// Open the tank vent (pump off) until the tank is below 5 mmHg. Works with no
    /// session running too; the loop goes idle again afterwards.
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
    /// A pressure reading was not a number (sensor/ADC failure) - controller has stopped.
    void sensorFault(const QString& reason);

private:
    void applyClitoralTarget();
    void autoStart();
    void resetState();
    double pulseCeiling() const;
    double clitMean(double clit, qint64 nowMs);
    void goIdle();
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
    bool m_latched = false;   ///< set by emergencyStop(); blocks auto-start
    bool m_hadSession = false; ///< vacuum was requested since the last start/idle
    QElapsedTimer m_clock;    ///< monotonic time for the 100 Hz loop
    std::deque<std::pair<qint64, double>> m_clitSamples;  ///< ~200 ms of raw clitoral readings
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
    quint64 m_pulserOnCount = 0;
};

#endif // PNEUMATICCONTROLLER_H
