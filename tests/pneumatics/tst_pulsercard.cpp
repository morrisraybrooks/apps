#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include "HandPulser.h"
#include "HardwareManager.h"
#include "PneumaticController.h"
#include "PulserCard.h"

// PulserCard against the real HardwareManager in simulation mode.
class TestPulserCard : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir* m_configDir = nullptr;
    bool m_automation = false;

    void writeCalibration()
    {
        auto pt = [](double pwm, double pps, double peak) {
            return QJsonObject{{"pwm_percent", pwm}, {"pulses_per_second", pps}, {"peak_mmhg", peak}};
        };
        QFile f(m_configDir->filePath("pulser_calibration.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(QJsonObject{{"measured", true},
                                          {"points", QJsonArray{pt(30, 20, 8), pt(70, 60, 20)}}})
                    .toJson());
    }

    HardwareManager* makeHardware()
    {
        auto* hw = new HardwareManager(this);
        hw->setSimulationMode(true);
        hw->initialize();
        return hw;
    }

    PulserCard* makeCard(HardwareManager* hw)
    {
        return new PulserCard(hw, [this]() { return m_automation; });
    }

    template <typename T>
    static T* child(QWidget* card, const char* name)
    {
        T* w = card->findChild<T*>(name);
        if (!w) qFatal("missing child %s", name);
        return w;
    }

private slots:
    void init()
    {
        m_automation = false;
        m_configDir = new QTemporaryDir;
        QVERIFY(m_configDir->isValid());
        qputenv("VACUUM_CONFIG_DIR", m_configDir->path().toUtf8());
    }

    void cleanup()
    {
        qunsetenv("VACUUM_CONFIG_DIR");
        delete m_configDir;
        m_configDir = nullptr;
    }

    void notCalibratedGraysStart()
    {
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        QVERIFY(!child<QPushButton>(card.data(), "startStopPulses")->isEnabled());
        QCOMPARE(child<QLabel>(card.data(), "statusLabel")->text(), QString("Pulser not calibrated"));
        QVERIFY(child<QPushButton>(card.data(), "pauseButton")->isEnabled());
        QVERIFY(child<QPushButton>(card.data(), "ventTankButton")->isEnabled());
    }

    void startAndStop()
    {
        writeCalibration();
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        auto* start = child<QPushButton>(card.data(), "startStopPulses");
        QVERIFY(start->isEnabled());
        QCOMPARE(start->text(), QString("START PULSES"));
        QTest::mouseClick(start, Qt::LeftButton);
        QVERIFY(hw->handPulser()->isRunning());
        QCOMPARE(start->text(), QString("STOP PULSES"));
        QTest::mouseClick(start, Qt::LeftButton);
        QVERIFY(!hw->handPulser()->isRunning());
        QCOMPARE(start->text(), QString("START PULSES"));
    }

    void fasterUpdatesLabel()
    {
        writeCalibration();
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        QCOMPARE(child<QLabel>(card.data(), "rateLabel")->text(), QString("10 /s"));
        QTest::mouseClick(child<QPushButton>(card.data(), "fasterButton"), Qt::LeftButton);
        QCOMPARE(child<QLabel>(card.data(), "rateLabel")->text(), QString("12 /s"));
        QTest::mouseClick(child<QPushButton>(card.data(), "slowerButton"), Qt::LeftButton);
        QTest::mouseClick(child<QPushButton>(card.data(), "slowerButton"), Qt::LeftButton);
        QCOMPARE(child<QLabel>(card.data(), "rateLabel")->text(), QString("8 /s"));
    }

    void pauseTogglesTextAndController()
    {
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        auto* pause = child<QPushButton>(card.data(), "pauseButton");
        QCOMPARE(pause->text(), QString("PAUSE PULSES"));
        QTest::mouseClick(pause, Qt::LeftButton);
        QVERIFY(hw->pneumatics()->isUserPaused());
        QCOMPARE(pause->text(), QString("RESUME"));
        QCOMPARE(child<QLabel>(card.data(), "statusLabel")->text(), QString("Paused"));
        QTest::mouseClick(pause, Qt::LeftButton);
        QVERIFY(!hw->pneumatics()->isUserPaused());
        QCOMPARE(pause->text(), QString("PAUSE PULSES"));
    }

    void pauseFromElsewhereShows()   // e.g. emergency stop clears the pause
    {
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        hw->pneumatics()->setUserPaused(true);
        QCOMPARE(child<QPushButton>(card.data(), "pauseButton")->text(), QString("RESUME"));
        hw->pneumatics()->emergencyStop();
        QCOMPARE(child<QPushButton>(card.data(), "pauseButton")->text(), QString("PAUSE PULSES"));
    }

    void automationGraysRateButtons()   // Review Focus 3
    {
        writeCalibration();
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        m_automation = true;
        hw->pneumatics()->setPulseRate(25);   // the program's rate
        card->refresh();
        QVERIFY(!child<QPushButton>(card.data(), "slowerButton")->isEnabled());
        QVERIFY(!child<QPushButton>(card.data(), "fasterButton")->isEnabled());
        QVERIFY(!child<QPushButton>(card.data(), "startStopPulses")->isEnabled());
        QVERIFY(child<QPushButton>(card.data(), "pauseButton")->isEnabled());
        QCOMPARE(child<QLabel>(card.data(), "statusLabel")->text(), QString("Program in charge"));
        QCOMPARE(child<QLabel>(card.data(), "rateLabel")->text(), QString("25 /s"));
    }

    void stopStaysAvailableWhenProgramStarts()   // Review Focus 3: never strand a hand session
    {
        writeCalibration();
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        auto* start = child<QPushButton>(card.data(), "startStopPulses");
        QTest::mouseClick(start, Qt::LeftButton);
        m_automation = true;
        card->refresh();
        QCOMPARE(start->text(), QString("STOP PULSES"));
        QVERIFY(start->isEnabled());
    }

    void ventTankOnlyWhenIdle()
    {
        writeCalibration();
        HardwareManager* hw = makeHardware();
        QScopedPointer<PulserCard> card(makeCard(hw));
        auto* vent = child<QPushButton>(card.data(), "ventTankButton");
        QVERIFY(vent->isEnabled());
        QTest::mouseClick(child<QPushButton>(card.data(), "startStopPulses"), Qt::LeftButton);
        card->refresh();
        QVERIFY(!vent->isEnabled());
    }
};

QTEST_MAIN(TestPulserCard)
#include "tst_pulsercard.moc"
