#include <QtTest>
#include "ChamberRegulator.h"

class TestChamberRegulator : public QObject
{
    Q_OBJECT
private slots:
    void opensFeedBelowBand()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        QVERIFY(r.update(37, 0).feedOpen);
    }

    void feedPulseIs30ms()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        QVERIFY(r.update(30, 0).feedOpen);
        QVERIFY(r.update(30, 10).feedOpen);
        QVERIFY(r.update(30, 20).feedOpen);
        QVERIFY(!r.update(30, 30).feedOpen);  // re-read tick
        QVERIFY(r.update(30, 40).feedOpen);   // next opening
    }

    void holdsInsideBand()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        const ValveCommand c = r.update(39.5, 0);
        QVERIFY(!c.feedOpen);
        QVERIFY(!c.ventOpen);
    }

    void ventsAboveBandUntilTarget()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        QVERIFY(r.update(44, 0).ventOpen);
        QVERIFY(r.update(41, 10).ventOpen);
        QVERIFY(!r.update(40, 20).ventOpen);
    }

    void zeroTargetVents()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(0);
        const ValveCommand c = r.update(20, 0);
        QVERIFY(c.ventOpen);
        QVERIFY(!c.feedOpen);
    }

    void neverBothOpen()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        qint64 t = 0;
        for (double p = 0; p <= 80; p += 0.5, t += 10) {
            const ValveCommand c = r.update(p, t);
            QVERIFY2(!(c.feedOpen && c.ventOpen), qPrintable(QString("both open at p=%1").arg(p)));
        }
        for (double p = 80; p >= 0; p -= 0.5, t += 10) {
            const ValveCommand c = r.update(p, t);
            QVERIFY2(!(c.feedOpen && c.ventOpen), qPrintable(QString("both open at p=%1").arg(p)));
        }
    }

    void averagingIgnoresPulses()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        r.setAveraging(true);
        for (qint64 t = 0; t <= 300; t += 10) {
            const ValveCommand c = r.update((t / 10) % 2 ? 50 : 30, t);
            QVERIFY2(!c.feedOpen && !c.ventOpen, qPrintable(QString("valve opened at t=%1").arg(t)));
        }
    }

    void needsTopUpReflectsDemand()
    {
        ChamberRegulator r{PneumaticSettings{}};
        r.setTarget(40);
        r.update(30, 0);
        QVERIFY(r.needsTopUp());
        r.update(40, 10);
        QVERIFY(!r.needsTopUp());
    }
};

QTEST_GUILESS_MAIN(TestChamberRegulator)
#include "tst_chamberregulator.moc"
