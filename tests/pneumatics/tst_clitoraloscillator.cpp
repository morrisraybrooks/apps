#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "ClitoralOscillator.h"
#include "HardwareManager.h"
#include "PneumaticController.h"

// Runs the real ClitoralOscillator against HardwareManager in simulation mode.
class TestClitoralOscillator : public QObject
{
    Q_OBJECT
private:
    // Each test gets its own config folder (VACUUM_CONFIG_DIR), so a real
    // calibration file on the machine running the tests can't change the result
    QTemporaryDir* m_configDir = nullptr;

    void writeConfig(const QByteArray& calibration)
    {
        QFile f(m_configDir->filePath("pulser_calibration.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(calibration);
    }

    static QByteArray validCalibration()
    {
        auto pt = [](double pwm, double pps, double peak) {
            return QJsonObject{{"pwm_percent", pwm}, {"pulses_per_second", pps}, {"peak_mmhg", peak}};
        };
        return QJsonDocument(QJsonObject{{"measured", true},
                                         {"points", QJsonArray{pt(30, 20, 8), pt(70, 60, 20)}}})
            .toJson();
    }

    HardwareManager* makeHardware()
    {
        auto* hw = new HardwareManager(this);
        hw->setSimulationMode(true);
        hw->initialize();
        return hw;
    }

private slots:
    void init()
    {
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

    void calibrationFoundInConfigDir()   // review M3: not tied to the working directory
    {
        writeConfig(validCalibration());
        HardwareManager* hw = makeHardware();
        QVERIFY(hw->pneumatics()->hasCalibration());
        ClitoralOscillator osc(hw);
        osc.start();
        QVERIFY(osc.isRunning());
        QVERIFY(hw->pneumatics()->isPulserEnabled());
        osc.stop();
    }

    void errorGivesTheRealReason()   // review M4
    {
        writeConfig("{ garbled");
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        QSignalSpy errors(&osc, &ClitoralOscillator::error);
        osc.start();
        QCOMPARE(errors.count(), 1);
        const QString message = errors.at(0).at(0).toString();
        QVERIFY2(message.contains("not a JSON object"), qPrintable(message));
    }

    void restartsAfterOutsideEmergencyStop()   // review M5
    {
        writeConfig(validCalibration());
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        osc.start();
        QVERIFY(osc.isRunning());
        hw->pneumatics()->emergencyStop();   // raised elsewhere (seal, sensor, SafetyManager)
        hw->pneumatics()->rearm();
        QVERIFY(!osc.isRunning());
        osc.start();
        QVERIFY(osc.isRunning());
        QVERIFY(hw->pneumatics()->isPulserEnabled());
        osc.stop();
    }

    void pulseDoesNotStopAnotherOscillator()   // review M6
    {
        writeConfig(validCalibration());
        HardwareManager* hw = makeHardware();
        ClitoralOscillator a(hw);
        ClitoralOscillator b(hw);
        a.pulse(1.0, 30);
        b.start();                           // b takes over while a's pulse is timing out
        QTest::qWait(80);
        QVERIFY(hw->pneumatics()->isPulserEnabled());
        b.stop();

        b.start();
        a.pulse(1.0, 30);                    // a pulse while b runs must leave b alone
        QTest::qWait(80);
        QVERIFY(hw->pneumatics()->isPulserEnabled());
        b.stop();
    }

    void amplitudeSetsClitoralTarget()
    {
        HardwareManager* hw = makeHardware();
        QVERIFY(hw->pneumatics());
        ClitoralOscillator osc(hw);
        osc.setAmplitude(40);
        QCOMPARE(hw->pneumatics()->clitoralTarget(), 40.0);
    }

    void startRefusedWithoutCalibration()
    {
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        QSignalSpy errors(&osc, &ClitoralOscillator::error);
        osc.start();
        QVERIFY(!osc.isRunning());
        QCOMPARE(errors.count(), 1);
        QVERIFY(!hw->pneumatics()->isPulserEnabled());
    }

    void frequencyClampedTo1to100()
    {
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        osc.setFrequency(0.2);
        QCOMPARE(osc.getFrequency(), 1.0);
        osc.setFrequency(250);
        QCOMPARE(osc.getFrequency(), 100.0);
        osc.setFrequency(60);
        QCOMPARE(osc.getFrequency(), 60.0);
        QCOMPARE(hw->pneumatics()->pulseRate(), 60.0);
    }

    void presetClimaxSetsRateAndTarget()
    {
        HardwareManager* hw = makeHardware();
        ClitoralOscillator osc(hw);
        osc.setPresetClimax();
        QCOMPARE(osc.getFrequency(), 11.0);
        QCOMPARE(hw->pneumatics()->clitoralTarget(), 55.0);
    }
};

QTEST_GUILESS_MAIN(TestClitoralOscillator)
#include "tst_clitoraloscillator.moc"
