#include <QtTest>
#include <cmath>
#include "MCP3008.h"

// MPX5010DP: 0.2 V at 0 mmHg, 4.7 V at 75 mmHg, on a 5 V supply. The MCP3008 runs at 3.3 V,
// so each pressure sensor goes through a 10k/20k divider (x 2/3) before the ADC.
namespace {
uint16_t rawForSensorVolts(double sensorVolts)
{
    const double adcVolts = sensorVolts * MCP3008::PRESSURE_DIVIDER_RATIO;
    return static_cast<uint16_t>(std::lround(adcVolts / 3.3 * 1024.0));
}
constexpr double SLOPE = 16.67;    // mmHg per sensor volt (datasheet)
constexpr double OFFSET = -3.33;   // zero at 0.2 V
}  // namespace

class TestSensorScaling : public QObject
{
    Q_OBJECT
private slots:
    void fullScaleReadsSeventyFive()   // P1: used to read ~49 (top of the range was lost)
    {
        const double mmHg = MCP3008::pressureFromRaw(0, rawForSensorVolts(4.7), SLOPE, OFFSET);
        QVERIFY2(std::abs(mmHg - 75.0) < 0.6, qPrintable(QString::number(mmHg)));
    }

    void tankFillLevelIsReadable()     // the pump stops at 70 mmHg - it must be readable
    {
        const double volts = 0.2 + 70.0 / SLOPE;
        const double mmHg = MCP3008::pressureFromRaw(1, rawForSensorVolts(volts), SLOPE, OFFSET);
        QVERIFY2(std::abs(mmHg - 70.0) < 0.6, qPrintable(QString::number(mmHg)));
    }

    void zeroReadsZero()
    {
        const double mmHg = MCP3008::pressureFromRaw(2, rawForSensorVolts(0.2), SLOPE, OFFSET);
        QVERIFY2(std::abs(mmHg) < 0.6, qPrintable(QString::number(mmHg)));
    }

    void inputAtTheRailIsAFault()      // divider missing or sensor fault: never a valid number
    {
        QCOMPARE(MCP3008::pressureFromRaw(0, 1023, SLOPE, OFFSET), -1.0);
        QCOMPARE(MCP3008::pressureFromRaw(1, MCP3008::RAIL_RAW, SLOPE, OFFSET), -1.0);
        QCOMPARE(MCP3008::pressureFromRaw(2, 1023, SLOPE, OFFSET), -1.0);
    }

    void withoutDividerHighPressureIsAFault()   // 4.7 V straight in pins the ADC at 1023
    {
        const uint16_t pinned = 1023;
        QVERIFY(MCP3008::pressureFromRaw(1, pinned, SLOPE, OFFSET) < 0.0);
    }

    void sensorVoltsUndoesTheDivider()   // calibration and diagnostics see the sensor's own volts
    {
        QVERIFY(std::abs(MCP3008::sensorVolts(0, rawForSensorVolts(4.7)) - 4.7) < 0.01);
        // Channel 3 (TENS impedance) has no divider
        QVERIFY(std::abs(MCP3008::sensorVolts(3, 512) - 3.3 * 512 / 1024.0) < 1e-9);
    }
};

QTEST_GUILESS_MAIN(TestSensorScaling)
#include "tst_sensorscaling.moc"
