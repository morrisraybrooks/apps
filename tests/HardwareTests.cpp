#include "HardwareTests.h"
#include "TestFramework.h"
#include <QTest>
#include <QSignalSpy>
#include <QTimer>

HardwareTests::HardwareTests(QObject *parent)
    : QObject(parent)
{
}

void HardwareTests::initTestCase()
{
    // Initialize test framework
    TestFramework::initialize();
    
    // Set up mock hardware for testing
    TestFramework::enableMockHardware(true);
    
    qDebug() << "Hardware Tests initialized";
}

void HardwareTests::cleanupTestCase()
{
    TestFramework::cleanup();
    qDebug() << "Hardware Tests cleanup completed";
}

void HardwareTests::init()
{
    // Reset hardware state before each test
    TestFramework::resetHardwareState();
}

void HardwareTests::cleanup()
{
    // Clean up after each test
    TestFramework::stopAllOperations();
}

void HardwareTests::testGPIOInitialization()
{
    qDebug() << "Testing GPIO initialization...";
    
    // Test GPIO chip access
    QVERIFY(TestFramework::isGPIOAvailable());
    
    // Test pin configuration
    QVERIFY(TestFramework::configurePins());
    
    // Verify all required pins are accessible
    QList<int> requiredPins = {17, 27, 22, 25, 21}; // SOL1, SOL2, SOL3, PUMP, EMERGENCY
    for (int pin : requiredPins) {
        QVERIFY2(TestFramework::isPinAccessible(pin), 
                QString("Pin %1 should be accessible").arg(pin).toLocal8Bit());
    }
}

void HardwareTests::testSolenoidControl()
{
    qDebug() << "Testing solenoid control...";
    
    // Test SOL1 (Applied Vacuum Line)
    QVERIFY(TestFramework::setSolenoid(1, true));
    QCOMPARE(TestFramework::getSolenoidState(1), true);
    
    QVERIFY(TestFramework::setSolenoid(1, false));
    QCOMPARE(TestFramework::getSolenoidState(1), false);
    
    // Test SOL2 (AVL vent valve)
    QVERIFY(TestFramework::setSolenoid(2, true));
    QCOMPARE(TestFramework::getSolenoidState(2), true);
    
    // Test SOL3 (Tank vent valve)
    QVERIFY(TestFramework::setSolenoid(3, true));
    QCOMPARE(TestFramework::getSolenoidState(3), true);
    
    // Test bulk solenoid control
    QVERIFY(TestFramework::setAllSolenoids(true, false, true));
    QCOMPARE(TestFramework::getSolenoidState(1), true);
    QCOMPARE(TestFramework::getSolenoidState(2), false);
    QCOMPARE(TestFramework::getSolenoidState(3), true);
}

void HardwareTests::testPumpControl()
{
    qDebug() << "Testing pump control...";
    
    // Test pump enable/disable
    QVERIFY(TestFramework::setPump(true));
    QCOMPARE(TestFramework::getPumpState(), true);
    
    QVERIFY(TestFramework::setPump(false));
    QCOMPARE(TestFramework::getPumpState(), false);
    
    // Test pump PWM control
    QVERIFY(TestFramework::setPumpPWM(50)); // 50% duty cycle
    QCOMPARE(TestFramework::getPumpPWM(), 50);
    
    QVERIFY(TestFramework::setPumpPWM(100)); // Full power
    QCOMPARE(TestFramework::getPumpPWM(), 100);
    
    QVERIFY(TestFramework::setPumpPWM(0)); // Off
    QCOMPARE(TestFramework::getPumpPWM(), 0);
}

void HardwareTests::testSensorReading()
{
    qDebug() << "Testing sensor reading...";
    
    // Test pressure sensor 1
    double pressure1 = TestFramework::readPressureSensor(1);
    QVERIFY(pressure1 >= -200.0 && pressure1 <= 200.0); // Valid pressure range
    
    // Test pressure sensor 2
    double pressure2 = TestFramework::readPressureSensor(2);
    QVERIFY(pressure2 >= -200.0 && pressure2 <= 200.0);
    
    // Test sensor calibration
    QVERIFY(TestFramework::calibrateSensor(1));
    QVERIFY(TestFramework::calibrateSensor(2));
    
    // Test sensor error detection
    TestFramework::simulateSensorError(1, true);
    QVERIFY(TestFramework::isSensorError(1));
    
    TestFramework::simulateSensorError(1, false);
    QVERIFY(!TestFramework::isSensorError(1));
}

void HardwareTests::testEmergencyStop()
{
    qDebug() << "Testing emergency stop...";
    
    // Set up initial state
    QVERIFY(TestFramework::setPump(true));
    QVERIFY(TestFramework::setAllSolenoids(true, true, true));
    
    // Trigger emergency stop
    TestFramework::triggerEmergencyStop();
    
    // Verify all systems are shut down
    QCOMPARE(TestFramework::getPumpState(), false);
    QCOMPARE(TestFramework::getSolenoidState(1), false);
    QCOMPARE(TestFramework::getSolenoidState(2), false);
    QCOMPARE(TestFramework::getSolenoidState(3), false);
    
    // Verify emergency stop state
    QVERIFY(TestFramework::isEmergencyStop());
    
    // Test emergency stop reset
    TestFramework::resetEmergencyStop();
    QVERIFY(!TestFramework::isEmergencyStop());
}

void HardwareTests::testSPICommunication()
{
    qDebug() << "Testing SPI communication...";
    
    // Test SPI initialization
    QVERIFY(TestFramework::initializeSPI());
    
    // Test ADC communication
    QVERIFY(TestFramework::testADCCommunication());
    
    // Test SPI data transfer
    QByteArray testData = {0x01, 0x80, 0x00}; // Start bit, channel 0, padding
    QByteArray response = TestFramework::spiTransfer(testData);
    QCOMPARE(response.size(), 3);
    
    // Verify ADC response format
    QVERIFY((response[1] & 0x03) == 0x00); // Check null bits
}

void HardwareTests::testHardwareTimings()
{
    qDebug() << "Testing hardware response timings...";
    
    // Test solenoid response time
    QElapsedTimer timer;
    timer.start();
    
    TestFramework::setSolenoid(1, true);
    qint64 solenoidTime = timer.elapsed();
    
    QVERIFY2(solenoidTime < 10, "Solenoid response should be < 10ms");
    
    // Test pump response time
    timer.restart();
    TestFramework::setPump(true);
    qint64 pumpTime = timer.elapsed();
    
    QVERIFY2(pumpTime < 20, "Pump response should be < 20ms");
    
    // Test sensor reading time
    timer.restart();
    TestFramework::readPressureSensor(1);
    qint64 sensorTime = timer.elapsed();
    
    QVERIFY2(sensorTime < 5, "Sensor reading should be < 5ms");
}

void HardwareTests::testConcurrentOperations()
{
    qDebug() << "Testing concurrent hardware operations...";
    
    // Test simultaneous solenoid operations
    QVERIFY(TestFramework::setAllSolenoids(true, true, true));
    
    // Verify all solenoids are active
    QCOMPARE(TestFramework::getSolenoidState(1), true);
    QCOMPARE(TestFramework::getSolenoidState(2), true);
    QCOMPARE(TestFramework::getSolenoidState(3), true);
    
    // Test pump and solenoid coordination
    QVERIFY(TestFramework::setPump(true));
    QVERIFY(TestFramework::setSolenoid(1, false)); // Should work while pump is on
    
    QCOMPARE(TestFramework::getPumpState(), true);
    QCOMPARE(TestFramework::getSolenoidState(1), false);
}

// Test runner for this specific test class
QTEST_MAIN(HardwareTests)
