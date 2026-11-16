#ifndef SEALWATCH_H
#define SEALWATCH_H

#include "PneumaticSettings.h"
#include <QtGlobal>
#include <deque>
#include <utility>

enum class SealState { WaitingForSeal, Sealed, Recovering, Failed };

/**
 * @brief Target-relative leak detection for the leg (outer) chamber.
 *
 * Replaces AntiDetachmentMonitor's fixed 50/60/65 mmHg thresholds. It never
 * touches valves: refilling is the ChamberRegulator's job; this class only
 * decides whether the seal is holding and when to give up.
 *
 * - Arms once the legs first reach the target band; a raised target re-arms.
 * - Leak: more than leakDropMmHg below target, or (below the band and) falling
 *   faster than leakRateMmHgPer100ms over the last 100 ms.
 * - Each leak gets recoveryAttempts windows of recoveryWindowMs; then Failed (sticky).
 */
class SealWatch
{
public:
    explicit SealWatch(const PneumaticSettings& settings);

    SealState update(double legMmHg, double legTarget, qint64 nowMs);
    SealState state() const { return m_state; }
    int attemptsUsed() const { return m_attempts; }
    void reset();

private:
    double fallOver100ms(double legMmHg, qint64 nowMs);

    PneumaticSettings m_settings;
    SealState m_state = SealState::WaitingForSeal;
    double m_lastTarget = 0.0;
    int m_attempts = 0;
    qint64 m_attemptStart = 0;
    std::deque<std::pair<qint64, double>> m_history;
};

#endif // SEALWATCH_H
