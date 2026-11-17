#include <QtTest>
#include <QJsonObject>
#include "PneumaticSettings.h"
#include "TankLogic.h"

class TestTankLogic : public QObject
{
    Q_OBJECT
private slots:
    void fillsFromEmpty()
    {
        TankLogic tank{PneumaticSettings{}};
        QVERIFY(tank.update(0, 0));
        QVERIFY(tank.isFilling());
    }

    void stopsAtFill()
    {
        TankLogic tank{PneumaticSettings{}};
        QVERIFY(tank.update(0, 0));
        QVERIFY(!tank.update(70, 0));
        QVERIFY(!tank.isFilling());
    }

    void hysteresis()
    {
        TankLogic tank{PneumaticSettings{}};
        tank.update(0, 0);
        tank.update(70, 0);
        QVERIFY(!tank.update(60, 0));
        QVERIFY(tank.update(55, 0));
    }

    void topUpWhenTankNotDeeper()
    {
        TankLogic tank{PneumaticSettings{}};
        tank.update(0, 0);
        tank.update(70, 0);
        QVERIFY(tank.update(62, 60));
    }

    void noTopUpWhenTankDeeper()
    {
        TankLogic tank{PneumaticSettings{}};
        tank.update(0, 0);
        tank.update(70, 0);
        QVERIFY(!tank.update(68, 50));
    }

    void saturatedSensorIsFull()
    {
        TankLogic tank{PneumaticSettings{}};
        QVERIFY(!tank.update(75, 0));
    }

    void settingsClampedToSafeValues()   // review I7
    {
        const PneumaticSettings s = PneumaticSettings::fromJson(QJsonObject{
            {"tank_fill_mmhg", 90}, {"tank_restart_mmhg", 95}, {"leg_max_mmhg", 80},
            {"clit_max_mmhg", 99}, {"feed_pulse_ms", 0}, {"average_window_ms", -5},
            {"recovery_attempts", 0}});
        QVERIFY(s.tankFillMmHg <= 72.0);
        QVERIFY(s.tankRestartMmHg < s.tankFillMmHg);
        QVERIFY(s.legMaxMmHg <= 75.0);
        QVERIFY(s.clitMaxMmHg <= 75.0);
        QVERIFY(s.feedPulseMs > 0);
        QVERIFY(s.averageWindowMs > 0);
        QVERIFY(s.recoveryAttempts >= 1);
    }

    void settingsFromJson()
    {
        const PneumaticSettings s = PneumaticSettings::fromJson(QJsonObject{{"tank_fill_mmhg", 65}});
        QCOMPARE(s.tankFillMmHg, 65.0);
        QCOMPARE(s.tankRestartMmHg, 55.0);
        QCOMPARE(s.feedPulseMs, 30);
        QCOMPARE(s.benchMode, false);
    }
};

QTEST_GUILESS_MAIN(TestTankLogic)
#include "tst_tanklogic.moc"
