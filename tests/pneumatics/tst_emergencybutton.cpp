#include <QtTest>
#include <QSignalSpy>
#include "EmergencyStop.h"
#include "HardwareManager.h"
#include "PneumaticController.h"
#include "VacuumController.h"

// The physical emergency stop button (GPIO 21), against HardwareManager in simulation mode.
// The GPIO read is replaced by a fake button; a real press reads "pressed" the same way.
class TestEmergencyButton : public QObject
{
    Q_OBJECT
private:
    bool m_pressed = false;

    HardwareManager* makeHardware()
    {
        auto* hw = new HardwareManager(this);
        hw->setSimulationMode(true);
        hw->initialize();
        return hw;
    }

    EmergencyStop* fakeButton(HardwareManager* hw)
    {
        EmergencyStop* button = hw->emergencyButton();
        if (!button) qFatal("HardwareManager did not create the emergency button");
        button->setButtonReader([this]() { return m_pressed; });
        return button;
    }

private slots:
    void init() { m_pressed = false; }

    void hardwareManagerStartsTheButton()   // P2: it used to be compiled but never created
    {
        HardwareManager* hw = makeHardware();
        QVERIFY(hw->emergencyButton() != nullptr);
        QVERIFY(hw->emergencyButton()->isActive());
    }

    void pressStopsEverything()
    {
        HardwareManager* hw = makeHardware();
        EmergencyStop* button = fakeButton(hw);
        QSignalSpy pressed(hw, &HardwareManager::emergencyButtonPressed);
        hw->pneumatics()->setLegTarget(40);   // a session is running
        QVERIFY(hw->pneumatics()->isActive());

        button->checkHardwareButton();
        QVERIFY(!hw->isEmergencyStop());       // not pressed yet
        m_pressed = true;
        button->checkHardwareButton();

        QVERIFY(hw->isEmergencyStop());
        QVERIFY(hw->pneumatics()->isLatched());
        QVERIFY(!hw->pneumatics()->isActive());
        QCOMPARE(pressed.count(), 1);
    }

    void heldAtStartupStops()   // a button already down when the app starts must still stop it
    {
        HardwareManager* hw = makeHardware();
        m_pressed = true;
        EmergencyStop* button = fakeButton(hw);
        button->checkHardwareButton();
        QVERIFY(hw->isEmergencyStop());
    }

    void holdingFiresOnce()
    {
        HardwareManager* hw = makeHardware();
        EmergencyStop* button = fakeButton(hw);
        QSignalSpy pressed(hw, &HardwareManager::emergencyButtonPressed);
        m_pressed = true;
        for (int i = 0; i < 5; ++i) button->checkHardwareButton();
        QCOMPARE(pressed.count(), 1);
    }

    void pressAfterResetStopsAgain()   // the old code ignored every press after the first
    {
        HardwareManager* hw = makeHardware();
        EmergencyStop* button = fakeButton(hw);
        m_pressed = true;
        button->checkHardwareButton();
        m_pressed = false;
        button->checkHardwareButton();
        QVERIFY(hw->resetEmergencyStop());     // reset from the screen
        QVERIFY(!hw->isEmergencyStop());
        hw->pneumatics()->setLegTarget(40);
        QVERIFY(hw->pneumatics()->isActive());

        m_pressed = true;
        button->checkHardwareButton();
        QVERIFY(hw->isEmergencyStop());
        QVERIFY(hw->pneumatics()->isLatched());
    }

    void screenShowsTheStop()   // the main controller hears the button, so the screen shows EMERGENCY STOP
    {
        VacuumController controller;
        controller.setSimulationMode(true);
        QVERIFY(controller.initialize());
        QSignalSpy stopped(&controller, &VacuumController::emergencyStopTriggered);
        EmergencyStop* button = fakeButton(controller.getHardwareManager());
        m_pressed = true;
        button->checkHardwareButton();
        QCOMPARE(controller.getSystemState(), VacuumController::EMERGENCY_STOP);
        QCOMPARE(stopped.count(), 1);
    }

    void pollsOnItsOwn()   // the 20 Hz timer reads the button without anyone calling it
    {
        HardwareManager* hw = makeHardware();
        fakeButton(hw);
        m_pressed = true;
        QTRY_VERIFY_WITH_TIMEOUT(hw->isEmergencyStop(), 1000);
    }
};

QTEST_GUILESS_MAIN(TestEmergencyButton)
#include "tst_emergencybutton.moc"
