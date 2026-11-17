#ifndef CHAMBERREGULATOR_H
#define CHAMBERREGULATOR_H

#include "PneumaticSettings.h"
#include <QtGlobal>
#include <deque>
#include <utility>

/// Desired state of one chamber's feed (vacuum) and vent valves for this tick.
struct ValveCommand {
    bool feedOpen = false;
    bool ventOpen = false;
};

/**
 * @brief Holds one chamber at its target using short feed-valve openings from the
 * tank and the vent valve. Feed and vent are never open together.
 *
 * - target 0: vent open (released)
 * - below target - feedBelow: feed opens for feedPulseMs, then one closed tick to re-read
 * - above target + ventAbove: vent open until the pressure is back at the target
 * - averaging (used while the pulser runs): decisions use the mean over averageWindowMs,
 *   nothing opens until the window has filled, and every action is a single short
 *   opening (one tick) followed by a full window of waiting so the mean can catch up
 *   - a lagging mean must never drive long openings (overshoot)
 */
class ChamberRegulator
{
public:
    explicit ChamberRegulator(const PneumaticSettings& settings);

    void setTarget(double mmHg);
    double target() const { return m_target; }
    void setAveraging(bool on) { m_averaging = on; }
    ValveCommand update(double pressureMmHg, qint64 nowMs);
    bool needsTopUp() const;
    double lastAverage() const { return m_lastValue; }
    void reset();

private:
    double windowMean(qint64 nowMs, bool* windowFull);

    PneumaticSettings m_settings;
    double m_target = 0.0;
    bool m_averaging = false;
    bool m_venting = false;
    qint64 m_feedUntil = -1;   ///< end of the current feed opening, -1 = none
    qint64 m_holdUntil = -1;   ///< averaging mode: no new action before this
    qint64 m_lastNow = -1;
    double m_lastValue = 0.0;
    std::deque<std::pair<qint64, double>> m_samples;
};

#endif // CHAMBERREGULATOR_H
