#include <QtTest>
#include "SealWatch.h"

namespace {
// Rise to the target and get armed
SealWatch sealedAt(double target, qint64* t)
{
    SealWatch w{PneumaticSettings{}};
    for (double p = 0; p <= target; p += 5, *t += 10) w.update(p, target, *t);
    w.update(target, target, *t);
    *t += 10;
    return w;
}
}  // namespace

class TestSealWatch : public QObject
{
    Q_OBJECT
private slots:
    void waitsUntilFirstSeal()
    {
        SealWatch w{PneumaticSettings{}};
        qint64 t = 0;
        for (double p = 0; p <= 37; p += 1, t += 10) {
            QCOMPARE(w.update(p, 40, t), SealState::WaitingForSeal);
        }
        QCOMPARE(w.update(38.5, 40, t), SealState::Sealed);
    }

    void slowDropBeyond10IsLeak()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(40, &t);
        for (double p = 40; p > 30; p -= 0.2, t += 100) {
            QCOMPARE(w.update(p, 40, t), SealState::Sealed);
        }
        QCOMPARE(w.update(29, 40, t), SealState::Recovering);
    }

    void fastFallIsLeak()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(40, &t);
        w.update(40, 40, t);
        QCOMPARE(w.update(37, 40, t + 50), SealState::Sealed);
        QCOMPARE(w.update(34, 40, t + 100), SealState::Recovering);
    }

    void recovers()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(40, &t);
        QCOMPARE(w.update(25, 40, t), SealState::Recovering);
        QCOMPARE(w.update(39, 40, t + 1500), SealState::Sealed);
        QCOMPARE(w.attemptsUsed(), 1);
    }

    void failsAfterThreeAttempts()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(40, &t);
        const qint64 start = t;
        QCOMPARE(w.update(20, 40, t), SealState::Recovering);
        for (; t < start + 6100; t += 100) w.update(20, 40, t);
        QCOMPARE(w.state(), SealState::Failed);
        QCOMPARE(w.attemptsUsed(), 3);
        QCOMPARE(w.update(40, 40, t + 100), SealState::Failed);  // sticky
        w.reset();
        QCOMPARE(w.update(0, 40, t + 200), SealState::WaitingForSeal);
    }

    void loweredTargetIsNotLeak()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(50, &t);
        for (double p = 50; p >= 31; p -= 0.7, t += 10) {
            QVERIFY2(w.update(p, 30, t) != SealState::Recovering, qPrintable(QString("leak at p=%1").arg(p)));
        }
    }

    void raisedTargetRearms()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(30, &t);
        QCOMPARE(w.update(30, 50, t), SealState::WaitingForSeal);
        QCOMPARE(w.update(48.5, 50, t + 500), SealState::Sealed);
    }

    void zeroTargetDisarms()
    {
        qint64 t = 0;
        SealWatch w = sealedAt(40, &t);
        QCOMPARE(w.update(10, 0, t), SealState::WaitingForSeal);
    }
};

QTEST_GUILESS_MAIN(TestSealWatch)
#include "tst_sealwatch.moc"
