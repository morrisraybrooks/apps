#include <QtTest>
#include <QJsonArray>
#include "PulserCalibration.h"
#include "PulserDrive.h"

namespace {
PulserCalibration table()
{
    auto pt = [](double pwm, double pps, double peak) {
        return QJsonObject{{"pwm_percent", pwm}, {"pulses_per_second", pps}, {"peak_mmhg", peak}};
    };
    PulserCalibration c;
    c.loadFromJson(QJsonObject{{"measured", true},
                               {"points", QJsonArray{pt(30, 20, 8), pt(50, 40, 10), pt(70, 60, 14)}}});
    return c;
}
}  // namespace

class TestPulserDrive : public QObject
{
    Q_OBJECT
private slots:
    void continuousAboveMinRate()
    {
        const PulserCalibration cal = table();
        PulserDrive d(&cal);
        d.setEnabled(true);
        d.setRate(40);
        const PulserOutput out = d.update(0);
        QVERIFY(out.on);
        QCOMPARE(out.pwmPercent, cal.pwmForRate(40));
        QVERIFY(!d.isBursting());
    }

    void burstsBelowMinRate()
    {
        const PulserCalibration cal = table();
        PulserDrive d(&cal);
        d.setEnabled(true);
        d.setRate(5);
        QVERIFY(d.isBursting());
        for (qint64 t = 0; t < 400; t += 10) {
            const PulserOutput out = d.update(t);
            const bool expectOn = (t % 200) < 100;
            QVERIFY2(out.on == expectOn, qPrintable(QString("t=%1").arg(t)));
            if (out.on) QCOMPARE(out.pwmPercent, cal.pwmForRate(cal.minRate()));
        }
    }

    void repeatedSetRateKeepsRhythm()   // review I4
    {
        const PulserCalibration cal = table();
        PulserDrive d(&cal);
        d.setEnabled(true);
        int on = 0, off = 0;
        for (qint64 t = 0; t < 2000; t += 10) {
            if (t % 100 == 0) d.setRate(3);   // caller re-sends the rate every 100 ms
            d.update(t).on ? ++on : ++off;
        }
        QVERIFY2(off > 70 && on > 70, qPrintable(QString("on %1 off %2").arg(on).arg(off)));
    }

    void offWhenDisabled()
    {
        const PulserCalibration cal = table();
        PulserDrive d(&cal);
        d.setRate(40);
        d.setEnabled(false);
        QVERIFY(!d.update(0).on);
    }

    void offWithoutCalibration()
    {
        const PulserCalibration empty;
        PulserDrive d(&empty);
        d.setEnabled(true);
        d.setRate(40);
        QVERIFY(!d.update(0).on);
    }
};

QTEST_GUILESS_MAIN(TestPulserDrive)
#include "tst_pulserdrive.moc"
