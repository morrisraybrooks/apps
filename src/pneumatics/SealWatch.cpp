#include "SealWatch.h"

SealWatch::SealWatch(const PneumaticSettings& settings)
    : m_settings(settings)
{
}

void SealWatch::reset()
{
    m_state = SealState::WaitingForSeal;
    m_lastTarget = 0.0;
    m_attempts = 0;
    m_attemptStart = 0;
    m_history.clear();
}

double SealWatch::fallOver100ms(double legMmHg, qint64 nowMs)
{
    // Keep the newest sample that is at least 100 ms old, plus everything newer
    while (m_history.size() > 1 && m_history[1].first <= nowMs - 100) {
        m_history.pop_front();
    }
    const double reference = m_history.empty() ? legMmHg : m_history.front().second;
    m_history.emplace_back(nowMs, legMmHg);
    return reference - legMmHg;
}

SealState SealWatch::update(double legMmHg, double legTarget, qint64 nowMs)
{
    if (m_state == SealState::Failed) return m_state;

    const double fall = fallOver100ms(legMmHg, nowMs);

    if (legTarget <= 0.0) {
        m_state = SealState::WaitingForSeal;
        m_attempts = 0;
        m_lastTarget = 0.0;
        return m_state;
    }
    if (legTarget > m_lastTarget + 0.01) {
        m_state = SealState::WaitingForSeal;  // commanded rise: re-arm at the new level
    }
    m_lastTarget = legTarget;

    const double band = legTarget - m_settings.feedBelowMmHg;
    switch (m_state) {
    case SealState::WaitingForSeal:
        if (legMmHg >= band) m_state = SealState::Sealed;
        break;
    case SealState::Sealed: {
        const bool deepDrop = legMmHg < legTarget - m_settings.leakDropMmHg;
        const bool fastFall = legMmHg < band && fall > m_settings.leakRateMmHgPer100ms;
        if (deepDrop || fastFall) {
            m_state = SealState::Recovering;
            m_attempts = 1;
            m_attemptStart = nowMs;
        }
        break;
    }
    case SealState::Recovering:
        if (legMmHg >= band) {
            m_state = SealState::Sealed;
        } else if (nowMs - m_attemptStart >= m_settings.recoveryWindowMs) {
            if (m_attempts >= m_settings.recoveryAttempts) {
                m_state = SealState::Failed;
            } else {
                ++m_attempts;
                m_attemptStart = nowMs;
            }
        }
        break;
    case SealState::Failed:
        break;
    }
    return m_state;
}
