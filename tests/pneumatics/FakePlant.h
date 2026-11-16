#ifndef FAKEPLANT_H
#define FAKEPLANT_H

#include "IPneumaticHardware.h"
#include <QString>
#include <QStringList>
#include <algorithm>

/**
 * @brief Simple simulated tank + two chambers for controller tests (10 ms per step).
 * Feed open: chamber moves 10 % toward the tank. Vent open: chamber loses 10 %.
 * Pump on: tank +3 mmHg. Every hardware write is logged in order.
 */
class FakePlant : public IPneumaticHardware
{
public:
    double tank = 0, leg = 0, clit = 0;
    double legLeakPerStep = 0;
    bool valve[6] = {false, false, false, false, false, false};
    bool pump = false;
    double duty = 0;
    QStringList log;

    double legMmHg() override { return leg; }
    double tankMmHg() override { return tank; }
    double clitMmHg() override { return clit; }
    void setPump(bool on) override { pump = on; log << QString("pump %1").arg(on); }
    void setValve(int sol, bool open) override
    {
        valve[sol] = open;
        log << QString("SOL%1 %2").arg(sol).arg(open);
    }
    void setPulserDuty(double percent) override
    {
        duty = percent;
        log << QString("duty %1").arg(percent);
    }

    void step()
    {
        auto feed = [this](double& chamber) {
            if (tank > chamber) {
                const double d = (tank - chamber) * 0.10;
                chamber += d;
                tank -= d * 0.2;
            }
        };
        if (valve[1]) feed(leg);
        if (valve[4]) feed(clit);
        if (valve[2]) leg *= 0.90;
        if (valve[5]) clit *= 0.90;
        if (valve[3]) tank *= 0.90;
        if (pump) tank = std::min(tank + 3.0, 90.0);
        leg = std::max(0.0, leg - legLeakPerStep);
    }
};

#endif // FAKEPLANT_H
