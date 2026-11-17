#include <QtTest>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include "SoftwarePwm.h"

namespace {
using Clock = std::chrono::steady_clock;

struct WriteLog {
    std::mutex mutex;
    std::vector<std::pair<Clock::time_point, bool>> writes;
    void record(bool level)
    {
        std::lock_guard<std::mutex> lock(mutex);
        writes.emplace_back(Clock::now(), level);
    }
};

double onFraction(WriteLog& log, Clock::time_point end)
{
    std::lock_guard<std::mutex> lock(log.mutex);
    double high = 0, total = 0;
    for (size_t i = 0; i < log.writes.size(); ++i) {
        const auto until = i + 1 < log.writes.size() ? log.writes[i + 1].first : end;
        const double span = std::chrono::duration<double>(until - log.writes[i].first).count();
        total += span;
        if (log.writes[i].second) high += span;
    }
    return total > 0 ? high / total : 0;
}
}  // namespace

class TestSoftwarePwm : public QObject
{
    Q_OBJECT
private slots:
    void dutyThirtyPercent()
    {
        WriteLog log;
        SoftwarePwm pwm([&log](bool level) { log.record(level); });
        pwm.setDuty(30);
        pwm.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const auto end = Clock::now();
        const double fraction = onFraction(log, end);
        pwm.stop();
        QVERIFY2(fraction >= 0.25 && fraction <= 0.35, qPrintable(QString::number(fraction)));
    }

    void zeroNeverHigh()
    {
        WriteLog log;
        SoftwarePwm pwm([&log](bool level) { log.record(level); });
        pwm.setDuty(0);
        pwm.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        pwm.stop();
        std::lock_guard<std::mutex> lock(log.mutex);
        for (const auto& w : log.writes) QVERIFY(!w.second);
    }

    void stopLeavesLow()
    {
        WriteLog log;
        SoftwarePwm pwm([&log](bool level) { log.record(level); });
        pwm.setDuty(80);
        pwm.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        pwm.stop();
        std::lock_guard<std::mutex> lock(log.mutex);
        QVERIFY(!log.writes.empty());
        QCOMPARE(log.writes.back().second, false);
    }
};

QTEST_GUILESS_MAIN(TestSoftwarePwm)
#include "tst_softwarepwm.moc"
