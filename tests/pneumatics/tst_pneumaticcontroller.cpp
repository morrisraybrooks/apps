#include <QtTest>
#include <limits>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryFile>
#include "FakePlant.h"
#include "PneumaticController.h"

namespace {
// Peak 8 mmHg at 20 pps, 20 mmHg at 60 pps
QString writeCalibration(QTemporaryFile& f)
{
    auto pt = [](double pwm, double pps, double peak) {
        return QJsonObject{{"pwm_percent", pwm}, {"pulses_per_second", pps}, {"peak_mmhg", peak}};
    };
    const QJsonObject obj{{"measured", true}, {"points", QJsonArray{pt(30, 20, 8), pt(70, 60, 20)}}};
    f.open();
    f.write(QJsonDocument(obj).toJson());
    f.close();
    return f.fileName();
}

void run(PneumaticController& c, FakePlant& plant, qint64& t, qint64 forMs)
{
    for (const qint64 end = t + forMs; t < end; t += 10) {
        c.tick(t);
        plant.step();
    }
}
}  // namespace

class TestPneumaticController : public QObject
{
    Q_OBJECT
private slots:
    void fillsTankThenStopsPump()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        qint64 t = 0;
        run(c, plant, t, 3000);
        QVERIFY2(plant.tank >= 70 && plant.tank <= 75, qPrintable(QString::number(plant.tank)));
        QVERIFY(!plant.pump);
    }

    void holdsLegAndClitIndependently()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.setLegTarget(40);
        c.setClitoralTarget(25);
        qint64 t = 0;
        run(c, plant, t, 5000);
        QVERIFY2(plant.leg >= 38 && plant.leg <= 43, qPrintable(QString::number(plant.leg)));
        QVERIFY2(plant.clit >= 23 && plant.clit <= 28, qPrintable(QString::number(plant.clit)));
    }

    void clitReleaseKeepsLegs()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.setLegTarget(40);
        c.setClitoralTarget(25);
        qint64 t = 0;
        run(c, plant, t, 5000);
        c.setClitoralTarget(0);
        run(c, plant, t, 1000);
        QVERIFY2(plant.clit < 3, qPrintable(QString::number(plant.clit)));
        QVERIFY2(plant.leg >= 38 && plant.leg <= 43, qPrintable(QString::number(plant.leg)));
    }

    void pulserNeedsCalibration()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QSignalSpy spy(&c, &PneumaticController::pulserUnavailable);
        c.start();
        c.setPulseRate(40);
        QVERIFY(!c.setPulserEnabled(true));
        QCOMPARE(spy.count(), 1);
        qint64 t = 0;
        run(c, plant, t, 200);
        QCOMPARE(plant.duty, 0.0);
        QVERIFY(!plant.log.filter(QRegularExpression("^duty [1-9]")).size());
    }

    void allowanceClampsTarget()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(c.loadCalibration(writeCalibration(f)));
        c.setClitoralTarget(60);
        c.setPulseRate(60);
        QVERIFY(c.setPulserEnabled(true));
        QCOMPARE(c.clitoralTarget(), 55.0);
    }

    void raisingRateReclamps()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(c.loadCalibration(writeCalibration(f)));
        c.setClitoralTarget(60);
        c.setPulseRate(20);
        QVERIFY(c.setPulserEnabled(true));
        QCOMPARE(c.clitoralTarget(), 60.0);
        c.setPulseRate(60);
        QCOMPARE(c.clitoralTarget(), 55.0);
    }

    void sealFailureStops()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QSignalSpy spy(&c, &PneumaticController::sealFailed);
        c.start();
        c.setLegTarget(40);
        qint64 t = 0;
        run(c, plant, t, 3000);
        QVERIFY(plant.leg >= 38);
        plant.legLeakPerStep = 5.0;  // faster than the feed can refill
        run(c, plant, t, 8000);
        QCOMPARE(spy.count(), 1);
        QVERIFY(plant.valve[2] && plant.valve[5] && !plant.valve[1] && !plant.valve[4] && !plant.pump);
    }

    void emergencyStopOrder()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.setLegTarget(40);
        qint64 t = 0;
        for (int i = 0; i < 500 && !plant.valve[1]; ++i, t += 10) {
            c.tick(t);
            plant.step();
        }
        QVERIFY(plant.valve[1]);
        plant.log.clear();
        c.emergencyStop();
        QCOMPARE(plant.log, (QStringList{"duty 0", "SOL1 0", "SOL4 0", "SOL2 1", "SOL5 1", "pump 0", "SOL3 1"}));
        plant.log.clear();
        run(c, plant, t, 100);
        QVERIFY(plant.log.isEmpty());  // stopped: no more writes
    }

    void sessionEndKeepsTank()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.setLegTarget(40);
        c.setClitoralTarget(30);
        qint64 t = 0;
        run(c, plant, t, 2000);
        c.ventAll();
        run(c, plant, t, 2000);
        QVERIFY(!plant.log.contains("SOL3 1"));
        QVERIFY(plant.leg < 3 && plant.clit < 3);
    }

    void stopVentsChambersKeepsTank()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.setLegTarget(40);
        c.setClitoralTarget(30);
        qint64 t = 0;
        run(c, plant, t, 2000);
        c.stop();
        QVERIFY(!c.isActive());
        QVERIFY(plant.valve[2] && plant.valve[5] && !plant.valve[1] && !plant.valve[4]);
        QVERIFY(!plant.pump);
        QCOMPARE(plant.duty, 0.0);
        QVERIFY(!plant.log.contains("SOL3 1"));
    }

    void sensorFaultStops()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QSignalSpy spy(&c, &PneumaticController::sensorFault);
        c.start();
        c.setLegTarget(40);
        qint64 t = 0;
        run(c, plant, t, 1000);
        plant.clit = std::numeric_limits<double>::quiet_NaN();
        c.tick(t);
        QCOMPARE(spy.count(), 1);
        QVERIFY(!c.isActive());
        QVERIFY(!plant.valve[1] && !plant.valve[4] && plant.valve[2] && plant.valve[5]);
    }

    void autoStartsWhenVacuumRequested()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(!c.isActive());
        c.holdLegs();
        QVERIFY(c.isActive());
    }

    void emergencyLatchBlocksAutoStart()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.emergencyStop();
        c.setLegTarget(40);
        c.setClitoralTarget(30);
        c.holdLegs();
        QVERIFY(!c.isActive());
        c.rearm();
        c.setLegTarget(40);
        QVERIFY(c.isActive());
    }

    void legTargetClamped()
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.setLegTarget(70);
        QCOMPARE(c.legTarget(), 50.0);
        c.setLegTarget(10);
        QCOMPARE(c.legTarget(), 30.0);
        c.setLegTarget(0);
        QCOMPARE(c.legTarget(), 0.0);
    }
};

QTEST_GUILESS_MAIN(TestPneumaticController)
#include "tst_pneumaticcontroller.moc"
