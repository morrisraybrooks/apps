#ifndef TANKLOGIC_H
#define TANKLOGIC_H

#include "PneumaticSettings.h"

/**
 * @brief Fill-then-stop tank control: pump runs until the tank is full, then stays
 * off (quiet) until the tank falls to the restart level or a chamber needs topping
 * up from a tank that is not deep enough.
 */
class TankLogic
{
public:
    explicit TankLogic(const PneumaticSettings& settings);

    /// @return desired pump state
    bool update(double tankMmHg, double highestChamberDemandMmHg);
    bool isFilling() const { return m_filling; }

private:
    PneumaticSettings m_settings;
    bool m_filling = false;
};

#endif // TANKLOGIC_H
