#include "PulserDrive.h"

PulserDrive::PulserDrive(const PulserCalibration* calibration)
    : m_calibration(calibration)
{
}

void PulserDrive::setRate(double pulsesPerSecond)
{
    m_rate = pulsesPerSecond > 0.0 ? pulsesPerSecond : 0.0;
    m_burstStart = -1;
}

void PulserDrive::setEnabled(bool enabled)
{
    m_enabled = enabled;
    m_burstStart = -1;
}

bool PulserDrive::isBursting() const
{
    return m_calibration && m_calibration->isValid() && m_rate > 0.0
        && m_rate < m_calibration->minRate();
}

PulserOutput PulserDrive::update(qint64 nowMs)
{
    if (!m_enabled || m_rate <= 0.0 || !m_calibration || !m_calibration->isValid()) {
        return {};
    }
    if (!isBursting()) {
        return {true, m_calibration->pwmForRate(m_rate)};
    }

    if (m_burstStart < 0) m_burstStart = nowMs;
    const qint64 periodMs = static_cast<qint64>(1000.0 / m_rate);
    const qint64 phase = (nowMs - m_burstStart) % periodMs;
    if (phase < periodMs / 2) {
        return {true, m_calibration->pwmForRate(m_calibration->minRate())};
    }
    return {};
}
