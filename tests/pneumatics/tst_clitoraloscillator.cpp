#include <QtTest>
#include <QSignalSpy>
#include "ClitoralOscillator.h"
#include "HardwareManager.h"
#include "PneumaticController.h"

// Runs the real ClitoralOscillator against HardwareManager in simulation mode.
class TestClitoralOscillator : public QObject
{
    Q_OBJECT
private:
    HardwareManager* makeHardware()
    {
        auto* hw = new HardwareManager(this);
        hw->setSimulationMode(true);
        hw->initialize();
        return hw;
    }

private slots:
    void amplitudeSetsClitoralTarget()
    {
        HardwareManager* hw = makeHardware();
        QVERIFY(hw->pneumatics());
        ClitoralOscillator osc(hw);
        osc.setAmplitude(40);
        QCOMPARE(hw->pneumatics()->clitoralTarget(), 40.0);
    }

    void startRefusedWithoutCalibration()
    {
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        QSignalSpy errors(&osc, &ClitoralOscillator::error);
        osc.start();
        QVERIFY(!osc.isRunning());
        QCOMPARE(errors.count(), 1);
        QVERIFY(!hw->pneumatics()->isPulserEnabled());
    }

    void frequencyClampedTo1to100()
    {
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        osc.setFrequency(0.2);
        QCOMPARE(osc.getFrequency(), 1.0);
        osc.setFrequency(250);
        QCOMPARE(osc.getFrequency(), 100.0);
        osc.setFrequency(60);
        QCOMPARE(osc.getFrequency(), 60.0);
        QCOMPARE(hw->pneumatics()->pulseRate(), 60.0);
    }

    void presetClimaxSetsRateAndTarget()
    {
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        osc.setPresetClimax();
        QCOMPARE(osc.getFrequency(), 11.0);
        QCOMPARE(hw->pneumatics()->clitoralTarget(), 55.0);
    }
};

QTEST_GUILESS_MAIN(TestClitoralOscillator)
#include "tst_clitoraloscillator.moc"
