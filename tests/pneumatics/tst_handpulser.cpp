#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryFile>
#include "FakePlant.h"
#include "HandPulser.h"
#include "PneumaticController.h"

namespace {
QString writeCalibration(QTemporaryFile& f, double maxRate)
{
    auto pt = [](double pwm, double pps, double peak) {
        return QJsonObject{{"pwm_percent", pwm}, {"pulses_per_second", pps}, {"peak_mmhg", peak}};
    };
    const QJsonObject obj{{"measured", true},
                          {"points", QJsonArray{pt(30, 20, 8), pt(70, maxRate, 20)}}};
    f.open();
    f.resize(0);
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

class TestHandPulser : public QObject
{
    Q_OBJECT
private slots:
    void stepsUpAndDown()
    {
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        HandPulser hand(&air);
        QCOMPARE(hand.rate(), 10.0);
        hand.faster();
        QCOMPARE(hand.rate(), 12.0);
        hand.slower();
        hand.slower();
        QCOMPARE(hand.rate(), 8.0);
        for (int i = 0; i < 18; ++i) hand.slower();
        QCOMPARE(hand.rate(), 1.0);
    }

    void rateClampedToCalibratedMax()   // Review Focus 5
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        QVERIFY(air.loadCalibration(writeCalibration(f, 60)));
        HandPulser hand(&air);
        for (int i = 0; i < 30; ++i) hand.faster();
        QCOMPARE(hand.rate(), 60.0);

        QVERIFY(air.loadCalibration(writeCalibration(f, 40)));   // re-calibrated lower
        hand.faster();
        QCOMPARE(hand.rate(), 40.0);
        QVERIFY(hand.start());
        QCOMPARE(air.pulseRate(), 40.0);
    }

    void startUsesClampedRate()   // Review Focus 5: start alone also clamps
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        QVERIFY(air.loadCalibration(writeCalibration(f, 60)));
        HandPulser hand(&air);
        for (int i = 0; i < 30; ++i) hand.faster();
        QVERIFY(air.loadCalibration(writeCalibration(f, 40)));
        QVERIFY(hand.start());
        QCOMPARE(air.pulseRate(), 40.0);
        QCOMPARE(hand.rate(), 40.0);
    }

    void startSetsLegsClitAndPulser()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        QVERIFY(air.loadCalibration(writeCalibration(f, 60)));
        HandPulser hand(&air);
        QVERIFY(hand.canStart());
        QVERIFY(hand.start());
        QCOMPARE(air.legTarget(), 40.0);
        QCOMPARE(air.clitoralTarget(), 20.0);
        QCOMPARE(air.pulseRate(), 10.0);
        QVERIFY(air.isPulserEnabled());
        QVERIFY(hand.isRunning());
    }

    void handClitLevelFromSettings()
    {
        QTemporaryFile f;
        FakePlant plant;
        const PneumaticSettings settings =
            PneumaticSettings::fromJson(QJsonObject{{"hand_clit_mmhg", 25}});
        QCOMPARE(settings.handClitMmHg, 25.0);
        PneumaticController air(&plant, settings);
        QVERIFY(air.loadCalibration(writeCalibration(f, 60)));
        HandPulser hand(&air);
        QVERIFY(hand.start());
        QCOMPARE(air.clitoralTarget(), 25.0);
    }

    void startRefusedWithoutCalibration()
    {
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        HandPulser hand(&air);
        QVERIFY(!hand.canStart());
        QVERIFY(!hand.start());
        QCOMPARE(air.legTarget(), 0.0);
        QVERIFY(!hand.isRunning());
    }

    void stopVentsBoth()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        QVERIFY(air.loadCalibration(writeCalibration(f, 60)));
        HandPulser hand(&air);
        QVERIFY(hand.start());
        qint64 t = 0;
        run(air, plant, t, 2000);
        QVERIFY(plant.leg > 30);
        hand.stop();
        run(air, plant, t, 2000);
        QVERIFY2(plant.leg < 3, qPrintable(QString::number(plant.leg)));
        QVERIFY2(plant.clit < 3, qPrintable(QString::number(plant.clit)));
        QVERIFY(!hand.isRunning());
    }

    void notRunningAfterEmergencyStop()
    {
        QTemporaryFile f;
        FakePlant plant;
        PneumaticController air(&plant, PneumaticSettings{});
        QVERIFY(air.loadCalibration(writeCalibration(f, 60)));
        HandPulser hand(&air);
        QVERIFY(hand.start());
        air.emergencyStop();
        QVERIFY(!hand.isRunning());
        QVERIFY(!hand.canStart());   // latched
    }
};

QTEST_GUILESS_MAIN(TestHandPulser)
#include "tst_handpulser.moc"
