#include "ChamberRegulator.h"

ChamberRegulator::ChamberRegulator(const PneumaticSettings& settings)
    : m_settings(settings)
{
}

void ChamberRegulator::setTarget(double mmHg)
{
    m_target = mmHg > 0.0 ? mmHg : 0.0;
}

void ChamberRegulator::reset()
{
    m_venting = false;
    m_feedUntil = -1;
    m_lastValue = 0.0;
    m_samples.clear();
}

double ChamberRegulator::windowMean(qint64 nowMs, bool* windowFull)
{
    while (!m_samples.empty() && m_samples.front().first < nowMs - m_settings.averageWindowMs) {
        m_samples.pop_front();
    }
    // Full once the oldest kept sample spans (almost) the whole window
    *windowFull = !m_samples.empty()
        && nowMs - m_samples.front().first >= m_settings.averageWindowMs - 10;
    double sum = 0.0;
    for (const auto& s : m_samples) sum += s.second;
    return m_samples.empty() ? 0.0 : sum / static_cast<double>(m_samples.size());
}

ValveCommand ChamberRegulator::update(double pressureMmHg, qint64 nowMs)
{
    m_samples.emplace_back(nowMs, pressureMmHg);
    bool windowFull = true;
    const double mean = windowMean(nowMs, &windowFull);
    const double value = m_averaging ? mean : pressureMmHg;
    m_lastValue = value;

    if (m_target <= 0.0) {
        m_feedUntil = -1;
        m_venting = true;
        return {false, true};
    }
    if (m_averaging && !windowFull) {
        m_feedUntil = -1;
        return {false, false};  // wait for a full window before acting
    }

    if (m_venting) {
        if (value > m_target) return {false, true};
        m_venting = false;
    }
    if (value > m_target + m_settings.ventAboveMmHg) {
        m_feedUntil = -1;
        m_venting = true;
        return {false, true};
    }

    if (m_feedUntil >= 0) {
        if (nowMs < m_feedUntil && value < m_target) return {true, false};
        m_feedUntil = -1;          // opening over (or target reached): one closed tick to re-read
        return {false, false};
    }
    if (value < m_target - m_settings.feedBelowMmHg) {
        m_feedUntil = nowMs + m_settings.feedPulseMs;
        return {true, false};
    }
    return {false, false};
}

bool ChamberRegulator::needsTopUp() const
{
    return m_target > 0.0 && m_lastValue < m_target - m_settings.feedBelowMmHg;
}
