#include <QtTest>
#include <limits>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryFile>
#include "FakePlant.h"
#include "PneumaticController.h"

Q_DECLARE_METATYPE(SealState)

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
/// Legs 40, clitoral 30, pulser on at 20/s, run until pulsing
void startPulsing(PneumaticController& c, FakePlant& plant, qint64& t, QTemporaryFile& f)
{
    QVERIFY(c.loadCalibration(writeCalibration(f)));
    c.setLegTarget(40);
    c.setClitoralTarget(30);
    c.setPulseRate(20);
    QVERIFY(c.setPulserEnabled(true));
    run(c, plant, t, 3000);
    QVERIFY(plant.duty > 0);
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
        QCOMPARE(c.clitoralTarget(), 52.0);  // 75 - 20 peak - 3 vent band
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
        QCOMPARE(c.clitoralTarget(), 52.0);
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

    void loweredLegTargetIsNotLeak()   // review I1 (Review Focus 4, controller level)
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.setLegTarget(50);
        qint64 t = 0;
        run(c, plant, t, 4000);
        QCOMPARE(c.sealState(), SealState::Sealed);
        qRegisterMetaType<SealState>("SealState");
        QSignalSpy states(&c, &PneumaticController::sealStateChanged);
        QVERIFY(states.isValid());
        c.setLegTarget(30);
        run(c, plant, t, 3000);
        for (const auto& args : states) {
            QVERIFY(args.at(0).value<SealState>() != SealState::Recovering);
        }
        QVERIFY2(plant.leg >= 27 && plant.leg <= 34, qPrintable(QString::number(plant.leg)));
    }

    void restartAfterSealFailureWorks()   // review C3
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QSignalSpy failed(&c, &PneumaticController::sealFailed);
        c.setLegTarget(40);
        qint64 t = 0;
        run(c, plant, t, 3000);
        plant.legLeakPerStep = 5.0;
        run(c, plant, t, 8000);
        QCOMPARE(failed.count(), 1);
        plant.legLeakPerStep = 0.0;
        c.rearm();
        c.start();
        c.setLegTarget(40);
        run(c, plant, t, 4000);
        QCOMPARE(failed.count(), 1);
        QVERIFY(c.isActive());
        QVERIFY2(plant.leg >= 38, qPrintable(QString::number(plant.leg)));
    }

    void pumpResumesAfterRearm()   // review I3
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.start();
        c.emergencyStop();
        plant.refuseWrites = true;   // hardware still in emergency stop
        c.start();                   // e.g. PatternEngine::startPattern
        qint64 t = 0;
        run(c, plant, t, 200);
        QVERIFY(!plant.pump);
        plant.refuseWrites = false;  // HardwareManager::resetEmergencyStop()
        c.rearm();
        run(c, plant, t, 200);
        QVERIFY(plant.pump);
    }

    void pulserWaitsForChamberBelowCeiling()   // review I6
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(c.loadCalibration(writeCalibration(f)));
        c.setLegTarget(40);
        c.setClitoralTarget(60);
        qint64 t = 0;
        run(c, plant, t, 5000);
        QVERIFY(plant.clit > 57);
        c.setPulseRate(60);              // peak 20 -> ceiling 55
        QVERIFY(c.setPulserEnabled(true));
        for (const qint64 end = t + 4000; t < end; t += 10) {
            c.tick(t);
            if (plant.duty > 0) {
                QVERIFY2(plant.clit + 20 <= 75.0 + 1e-9,
                         qPrintable(QString("pulsing at clit %1").arg(plant.clit)));
            }
            plant.step();
        }
        QVERIFY(plant.duty > 0);         // pulses did start once it was safe
    }

    void sessionEndStopsLoop()   // review I9
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.setLegTarget(40);
        c.setClitoralTarget(30);
        qint64 t = 0;
        run(c, plant, t, 3000);
        c.ventAll();
        run(c, plant, t, 3000);
        QVERIFY(!c.isActive());
        QVERIFY(!plant.pump);
        plant.tank = 40;                 // tank leaks down while idle
        run(c, plant, t, 1000);
        QVERIFY(!plant.pump);            // no unattended pump restarts
    }

    void clitoralOverpressureStops()   // review I10
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QSignalSpy faults(&c, &PneumaticController::sensorFault);
        c.setLegTarget(40);
        c.setClitoralTarget(30);
        qint64 t = 0;
        run(c, plant, t, 2000);
        plant.pinClit = 75.0;
        run(c, plant, t, 500);
        QCOMPARE(faults.count(), 1);
        QVERIFY(!c.isActive());
        QVERIFY(plant.valve[5]);
    }

    void feedWaitsForDeeperTank()   // review M1: valves flow both ways
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.setLegTarget(40);
        qint64 t = 0;
        run(c, plant, t, 5000);
        QVERIFY2(plant.leg >= 38, qPrintable(QString::number(plant.leg)));
        plant.pumpRate = 0.0;            // pump not keeping up...
        plant.tank = 20.0;               // ...and the tank is shallower than the legs
        plant.legLeakPerStep = 0.05;     // slow seep so the leg feed wants to open
        double lowest = plant.leg;
        for (const qint64 end = t + 1000; t < end; t += 10) {
            c.tick(t);
            plant.step();
            lowest = std::min(lowest, plant.leg);
        }
        QVERIFY2(lowest >= 34, qPrintable(QString("legs fell to %1 through the feed").arg(lowest)));
    }

    void ventTankWorksWhenIdle()   // review M9
    {
        FakePlant plant;
        plant.tank = 60;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(!c.isActive());
        c.ventTank();
        QVERIFY(c.isActive());
        bool pumpRan = false;
        qint64 t = 0;
        for (const qint64 end = t + 3000; t < end; t += 10) {
            c.tick(t);
            plant.step();
            pumpRan = pumpRan || plant.pump;
        }
        QVERIFY2(plant.tank < 5, qPrintable(QString::number(plant.tank)));
        QVERIFY(!pumpRan);               // venting the tank never refills it
        QVERIFY(!c.isActive());          // and the loop goes idle afterwards
    }

    void calibrationErrorIsSpecific()   // review M4
    {
        QTemporaryFile f;
        f.open();
        f.write("{ not json");
        f.close();
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(!c.loadCalibration(f.fileName()));
        QVERIFY2(c.calibrationError().contains("not a JSON object"), qPrintable(c.calibrationError()));
    }

    // --- user pause (pulser card) ---
    void pauseStopsPulsesVentsClitKeepsLegs()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        qint64 t = 0;
        startPulsing(c, plant, t, f);
        c.setUserPaused(true);
        QVERIFY(c.isUserPaused());
        run(c, plant, t, 1500);
        QCOMPARE(plant.duty, 0.0);
        QVERIFY2(plant.clit < 3, qPrintable(QString::number(plant.clit)));
        QVERIFY2(plant.leg >= 38 && plant.leg <= 43, qPrintable(QString::number(plant.leg)));
        QVERIFY(c.isPulserEnabled());    // the request is kept for Resume
    }

    void requestsWhilePausedWaitForResume()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QVERIFY(c.loadCalibration(writeCalibration(f)));
        c.setUserPaused(true);
        c.setLegTarget(40);
        c.setClitoralTarget(30);
        c.setPulseRate(20);
        QVERIFY(c.setPulserEnabled(true));
        qint64 t = 0;
        run(c, plant, t, 2000);
        QCOMPARE(c.clitoralTarget(), 0.0);
        QVERIFY2(plant.clit < 3, qPrintable(QString::number(plant.clit)));
        QVERIFY(!plant.log.filter(QRegularExpression("^duty [1-9]")).size());
    }

    void resumeRestoresLevelThenPulses()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        qint64 t = 0;
        startPulsing(c, plant, t, f);
        c.setUserPaused(true);
        run(c, plant, t, 1500);
        c.setUserPaused(false);
        QCOMPARE(c.clitoralTarget(), 30.0);
        bool pulsedEarly = false;
        for (const qint64 end = t + 4000; t < end; t += 10) {
            c.tick(t);
            if (plant.duty > 0 && plant.clit < 28) pulsedEarly = true;
            plant.step();
        }
        QVERIFY(!pulsedEarly);           // pulses wait for the suction to come back
        QVERIFY(plant.duty > 0);
    }

    void resumeStartsPulsesAfterTimeout()   // Review Focus 1
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        qint64 t = 0;
        startPulsing(c, plant, t, f);
        c.setUserPaused(true);
        run(c, plant, t, 1500);
        plant.pumpRate = 0.0;            // level can't come back
        plant.tank = 10.0;
        c.setUserPaused(false);
        run(c, plant, t, 3500);
        QVERIFY(plant.duty > 0);
    }

    void pauseWhileIdleDoesNotStart()   // Review Focus 2
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        c.setUserPaused(true);
        QVERIFY(!c.isActive());
        QVERIFY(plant.log.isEmpty());
    }

    void sessionEndClearsPause()   // Review Focus 4
    {
        FakePlant plant;
        PneumaticController c(&plant, PneumaticSettings{});
        QSignalSpy spy(&c, &PneumaticController::userPauseChanged);
        c.setLegTarget(40);
        c.setUserPaused(true);
        c.ventAll();
        QVERIFY(!c.isUserPaused());
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.last().at(0).toBool(), false);

        c.setLegTarget(40);
        c.setUserPaused(true);
        c.emergencyStop();
        QVERIFY(!c.isUserPaused());
        QCOMPARE(spy.last().at(0).toBool(), false);
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
