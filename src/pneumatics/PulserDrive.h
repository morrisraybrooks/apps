#ifndef PULSERDRIVE_H
#define PULSERDRIVE_H

#include "PulserCalibration.h"
#include <QtGlobal>

/// What the pulser motor should do this tick.
struct PulserOutput {
    bool on = false;
    double pwmPercent = 0.0;
};

/**
 * @brief Turns a requested pulse rate into motor PWM.
 *
 * - rate >= calibration.minRate(): motor runs continuously at pwmForRate(rate)
 * - 0 < rate < minRate(): slow "deep" rhythm as bursts - motor on at the minRate()
 *   PWM for half of each 1000/rate ms period, off for the other half
 * - disabled, rate <= 0, or no valid calibration: off
 */
class PulserDrive
{
public:
    explicit PulserDrive(const PulserCalibration* calibration);

    void setRate(double pulsesPerSecond);
    double rate() const { return m_rate; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    bool isBursting() const;
    PulserOutput update(qint64 nowMs);

private:
    const PulserCalibration* m_calibration;
    double m_rate = 0.0;
    bool m_enabled = false;
    qint64 m_burstStart = -1;  ///< -1 = restart the burst phase on the next update
};

#endif // PULSERDRIVE_H
