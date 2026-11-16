#include "SoftwarePwm.h"

#include <algorithm>
#include <chrono>

SoftwarePwm::SoftwarePwm(std::function<void(bool)> write, int periodUs)
    : m_write(std::move(write))
    , m_periodUs(periodUs)
{
}

SoftwarePwm::~SoftwarePwm()
{
    stop();
}

void SoftwarePwm::setDuty(double percent)
{
    m_duty.store(std::clamp(percent, 0.0, 100.0));
}

void SoftwarePwm::start()
{
    if (m_running.exchange(true)) return;
    m_thread = std::thread(&SoftwarePwm::run, this);
}

void SoftwarePwm::stop()
{
    if (m_running.exchange(false) && m_thread.joinable()) {
        m_thread.join();
    }
    if (m_write) m_write(false);
}

void SoftwarePwm::run()
{
    using namespace std::chrono;
    auto next = steady_clock::now();
    bool level = false;
    m_write(false);
    while (m_running.load()) {
        const double duty = m_duty.load();
        const auto period = microseconds(m_periodUs);
        const auto high = microseconds(static_cast<long long>(m_periodUs * duty / 100.0));

        if (high.count() > 0) {
            if (!level) { m_write(true); level = true; }
            std::this_thread::sleep_until(next + high);
        }
        if (high < period) {
            if (level) { m_write(false); level = false; }
        }
        next += period;
        std::this_thread::sleep_until(next);
    }
}
