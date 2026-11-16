#include "TankLogic.h"

TankLogic::TankLogic(const PneumaticSettings& settings)
    : m_settings(settings)
{
}

bool TankLogic::update(double tankMmHg, double highestChamberDemandMmHg)
{
    if (tankMmHg >= m_settings.tankFillMmHg) {
        // Full (also covers a sensor pinned at its 75 mmHg top)
        m_filling = false;
    } else if (!m_filling) {
        const bool lowTank = tankMmHg <= m_settings.tankRestartMmHg;
        const bool topUpNeeded = highestChamberDemandMmHg > 0
            && tankMmHg < highestChamberDemandMmHg + m_settings.topUpMarginMmHg;
        m_filling = lowTank || topUpNeeded;
    }
    return m_filling;
}
