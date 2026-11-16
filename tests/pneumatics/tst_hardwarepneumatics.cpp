#include <QtTest>
#include <QSignalSpy>
#include <limits>
#include "HardwareManager.h"
#include "PatternEngine.h"
#include "PneumaticController.h"

// HardwareManager (simulation mode) wiring around the PneumaticController.
class TestHardwarePneumatics : public QObject
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
    void sealFailureTriggersHardwareEmergencyStop()   // review C3
    {
        HardwareManager* hw = makeHardware();
        QSignalSpy errors(hw, &HardwareManager::hardwareError);
        PneumaticController* air = hw->pneumatics();
        air->setLegTarget(40);
        hw->setSimulatedSensorValues(40, 70);
        qint64 t = 0;
        for (; t < 1000; t += 10) air->tick(t);          // seal reached
        hw->setSimulatedSensorValues(5, 70);             // cup comes off
        for (; t < 9000; t += 10) air->tick(t);          // 3 x 2 s reseal attempts
        QVERIFY(hw->isEmergencyStop());
        QVERIFY(errors.count() >= 1);
        QVERIFY(errors.last().at(0).toString().contains("seal", Qt::CaseInsensitive));
    }

    void sensorFaultTriggersHardwareEmergencyStop()   // review C3 / I2
    {
        HardwareManager* hw = makeHardware();
        PneumaticController* air = hw->pneumatics();
        air->setLegTarget(40);
        hw->setSimulatedSensorValues(std::numeric_limits<double>::quiet_NaN(), 70);
        air->tick(0);
        QVERIFY(hw->isEmergencyStop());
    }

    void sensorErrorValueTriggersStop()   // review I2: SensorInterface returns -1 on a read error
    {
        HardwareManager* hw = makeHardware();
        PneumaticController* air = hw->pneumatics();
        air->setLegTarget(40);
        hw->setSimulatedSensorValues(-1.0, 70);
        air->tick(0);
        QVERIFY(hw->isEmergencyStop());
    }

    void fullTankIsNotOverpressure()   // review C2
    {
        HardwareManager* hw = makeHardware();
        PatternEngine engine(hw);
        QVERIFY(!engine.isPressureLimitExceeded(40.0, 70.0));   // tank full at 70: normal
        QVERIFY(engine.isPressureLimitExceeded(70.0, 10.0));    // legs at 70 mmHg: too high
    }
};

QTEST_GUILESS_MAIN(TestHardwarePneumatics)
#include "tst_hardwarepneumatics.moc"
