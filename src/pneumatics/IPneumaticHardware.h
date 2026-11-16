#ifndef IPNEUMATICHARDWARE_H
#define IPNEUMATICHARDWARE_H

/**
 * @brief What the PneumaticController needs from the machine. HardwareManager
 * provides the real adapter; tests use a fake plant.
 * Valves: 1 leg feed, 2 leg vent, 3 tank vent, 4 clitoral feed, 5 clitoral vent.
 */
class IPneumaticHardware
{
public:
    virtual ~IPneumaticHardware() = default;
    virtual double legMmHg() = 0;
    virtual double tankMmHg() = 0;
    virtual double clitMmHg() = 0;
    virtual void setPump(bool on) = 0;
    virtual void setValve(int sol, bool open) = 0;
    virtual void setPulserDuty(double percent) = 0;
};

#endif // IPNEUMATICHARDWARE_H
