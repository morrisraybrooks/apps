#ifndef SOFTWAREPWM_H
#define SOFTWAREPWM_H

#include <atomic>
#include <functional>
#include <thread>

/**
 * @brief Software PWM on one output line, run on its own thread.
 *
 * Used for the clitoral pulser motor (GPIO 15); motor inertia smooths the jitter.
 * stop() and the destructor always leave the line low.
 */
class SoftwarePwm
{
public:
    explicit SoftwarePwm(std::function<void(bool)> write, int periodUs = 1000);
    ~SoftwarePwm();

    SoftwarePwm(const SoftwarePwm&) = delete;
    SoftwarePwm& operator=(const SoftwarePwm&) = delete;

    void start();
    void stop();
    void setDuty(double percent);
    double duty() const { return m_duty.load(); }

private:
    void run();

    std::function<void(bool)> m_write;
    int m_periodUs;
    std::atomic<double> m_duty{0.0};
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};

#endif // SOFTWAREPWM_H
