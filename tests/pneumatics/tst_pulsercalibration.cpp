#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryFile>
#include "PulserCalibration.h"

namespace {
QJsonObject point(double pwm, double pps, double peak)
{
    return QJsonObject{{"pwm_percent", pwm}, {"pulses_per_second", pps}, {"peak_mmhg", peak}};
}

QJsonObject table(bool measured, const QJsonArray& points)
{
    return QJsonObject{{"measured", measured}, {"measured_on", "2026-11-20"},
                       {"pump", "test"}, {"points", points}};
}

QJsonObject threePoints()
{
    return table(true, QJsonArray{point(30, 20, 8), point(50, 40, 10), point(70, 60, 14)});
}
}  // namespace

class TestPulserCalibration : public QObject
{
    Q_OBJECT
private slots:
    void loadsValidTable()
    {
        PulserCalibration c;
        QVERIFY(c.loadFromJson(threePoints()));
        QVERIFY(c.isValid());
        QCOMPARE(c.minRate(), 20.0);
        QCOMPARE(c.maxRate(), 60.0);
    }

    void interpolates()
    {
        PulserCalibration c;
        QVERIFY(c.loadFromJson(threePoints()));
        QCOMPARE(c.pwmForRate(30), 40.0);
        QCOMPARE(c.peakAtRate(50), 12.0);
        QCOMPARE(c.maxClitoralTarget(60), 61.0);
    }

    void clampsOutsideTable()
    {
        PulserCalibration c;
        QVERIFY(c.loadFromJson(threePoints()));
        QCOMPARE(c.pwmForRate(5), 30.0);
        QCOMPARE(c.pwmForRate(200), 70.0);
        QCOMPARE(c.peakAtRate(5), 8.0);
    }

    void rejectsUnmeasured()
    {
        PulserCalibration c;
        QString error;
        QVERIFY(!c.loadFromJson(table(false, QJsonArray{point(30, 20, 8), point(50, 40, 10)}), &error));
        QVERIFY(!c.isValid());
        QVERIFY2(error.contains("measured"), qPrintable(error));
    }

    void rejectsTooFewPoints()
    {
        PulserCalibration c;
        QVERIFY(!c.loadFromJson(table(true, QJsonArray{point(30, 20, 8)})));
        QVERIFY(!c.isValid());
    }

    void rejectsNonIncreasing()
    {
        PulserCalibration c;
        QVERIFY(!c.loadFromJson(table(true, QJsonArray{point(30, 20, 8), point(50, 20, 10)})));
        QVERIFY(!c.isValid());
    }

    void rejectsMissingFile()
    {
        PulserCalibration c;
        QString error;
        QVERIFY(!c.loadFromFile("/nonexistent.json", &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!c.isValid());
    }

    void rejectsGarbage()
    {
        QTemporaryFile f;
        QVERIFY(f.open());
        f.write("not json");
        f.close();
        PulserCalibration c;
        QVERIFY(!c.loadFromFile(f.fileName()));
        QVERIFY(!c.isValid());
    }
};

QTEST_GUILESS_MAIN(TestPulserCalibration)
#include "tst_pulsercalibration.moc"
