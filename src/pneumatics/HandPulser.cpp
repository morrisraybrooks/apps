#include "HandPulser.h"
#include "PneumaticController.h"

HandPulser::HandPulser(PneumaticController* air, QObject* parent)
    : QObject(parent)
    , m_air(air)
{
}

const QVector<double>& HandPulser::rateSteps()
{
    static const QVector<double> steps{1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100};
    return steps;
}

void HandPulser::setRate(double rate)
{
    if (rate == m_rate) return;
    m_rate = rate;
    if (isRunning()) m_air->setPulseRate(m_rate);
    emit changed();
}

void HandPulser::clampToMax()
{
    // Highest step the calibration covers (at least the lowest step)
    const double max = m_air->maxPulseRate();
    double allowed = rateSteps().first();
    for (double step : rateSteps()) {
        if (step <= max) allowed = step;
    }
    if (m_rate > allowed) setRate(allowed);
}

void HandPulser::slower()
{
    clampToMax();
    for (int i = rateSteps().size() - 1; i >= 0; --i) {
        if (rateSteps()[i] < m_rate) {
            setRate(rateSteps()[i]);
            return;
        }
    }
}

void HandPulser::faster()
{
    clampToMax();
    for (double step : rateSteps()) {
        if (step > m_rate) {
            if (step <= m_air->maxPulseRate()) setRate(step);
            return;
        }
    }
}

bool HandPulser::canStart() const
{
    return m_air->hasCalibration() && !m_air->isLatched();
}

bool HandPulser::start()
{
    if (!canStart()) return false;
    clampToMax();
    m_air->setLegTarget(LEG_MMHG);
    m_air->setClitoralTarget(m_air->settings().handClitMmHg);
    m_air->setPulseRate(m_rate);
    if (!m_air->setPulserEnabled(true)) return false;
    m_started = true;
    emit changed();
    return true;
}

void HandPulser::stop()
{
    m_started = false;
    m_air->setPulserEnabled(false);
    m_air->ventAll();
    emit changed();
}

bool HandPulser::isRunning() const
{
    return m_started && m_air->isPulserEnabled();
}
