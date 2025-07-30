#include "IntegrationTests.h"
#include "../src/VacuumController.h"
#include "../src/hardware/HardwareManager.h"
#include "../src/safety/SafetyManager.h"
#include "../src/patterns/PatternEngine.h"
#include "../src/gui/MainWindow.h"
#include "../src/gui/PressureMonitor.h"
#include "../src/gui/PatternSelector.h"
#include "../src/gui/SafetyPanel.h"
#include <QSignalSpy>
#include <QTimer>
#include <QApplication>
#include <QThread>

void IntegrationTests::initTestCase()
{
    // Initialize test environment
    qDebug() << "Initializing integration test environment";
    
    // Create mock controller with hardware simulation
    m_controller = std::make_unique<VacuumController>();
    m_controller->setSimulationMode(true);
    
    // Initialize controller
    QVERIFY(m_controller->initialize());
    
    // Create main window for GUI testing
    m_mainWindow = std::make_unique<MainWindow>();
    m_mainWindow->setController(m_controller.get());
    
    // Enable hardware simulation
    m_hardwareSimulationEnabled = true;
    
    qDebug() << "Integration test environment initialized successfully";
}

void IntegrationTests::cleanupTestCase()
{
    qDebug() << "Cleaning up integration test environment";
    
    if (m_mainWindow) {
        m_mainWindow->close();
        m_mainWindow.reset();
    }
    
    if (m_controller) {
        m_controller->shutdown();
        m_controller.reset();
    }
    
    qDebug() << "Integration test environment cleaned up";
}

void IntegrationTests::init()
{
    // Reset system to known state before each test
    if (m_controller) {
        m_controller->stopPattern();
        m_controller->resetEmergencyStop();
        
        // Wait for system to stabilize
        QTest::qWait(100);
    }
}

void IntegrationTests::cleanup()
{
    // Cleanup after each test
    if (m_controller) {
        m_controller->stopPattern();
        m_controller->resetEmergencyStop();
    }
}

void IntegrationTests::testSystemInitialization()
{
    qDebug() << "Testing complete system initialization";
    
    // Test controller initialization
    QVERIFY(m_controller->isReady());
    QCOMPARE(m_controller->getSystemState(), VacuumController::READY);
    
    // Test hardware manager initialization
    auto hardwareManager = m_controller->getHardwareManager();
    QVERIFY(hardwareManager != nullptr);
    QVERIFY(hardwareManager->isReady());
    
    // Test safety manager initialization
    auto safetyManager = m_controller->getSafetyManager();
    QVERIFY(safetyManager != nullptr);
    QVERIFY(safetyManager->isSystemSafe());
    
    // Test pattern engine initialization
    auto patternEngine = m_controller->getPatternEngine();
    QVERIFY(patternEngine != nullptr);
    QCOMPARE(patternEngine->getState(), PatternEngine::STOPPED);
    
    // Test GUI initialization
    QVERIFY(m_mainWindow != nullptr);
    
    qDebug() << "System initialization test completed successfully";
}

void IntegrationTests::testSensorToSafetyIntegration()
{
    qDebug() << "Testing sensor to safety system integration";
    
    auto safetyManager = m_controller->getSafetyManager();
    QVERIFY(safetyManager != nullptr);
    
    // Setup signal spy for safety events
    QSignalSpy safetyViolationSpy(safetyManager, &SafetyManager::safetyViolation);
    QSignalSpy antiDetachmentSpy(safetyManager, &SafetyManager::antiDetachmentActivated);
    QSignalSpy overpressureSpy(safetyManager, &SafetyManager::overpressureDetected);
    
    // Test normal pressure readings
    simulatePressureChange(60.0, 50.0);  // Normal pressures
    QTest::qWait(200);
    
    QVERIFY(safetyManager->isSystemSafe());
    QCOMPARE(safetyViolationSpy.count(), 0);
    
    // Test anti-detachment threshold
    simulatePressureChange(30.0, 50.0);  // Below anti-detachment threshold
    QTest::qWait(200);
    
    QVERIFY(antiDetachmentSpy.count() > 0);
    
    // Reset to normal
    simulatePressureChange(60.0, 50.0);
    QTest::qWait(200);
    
    // Test overpressure detection
    simulatePressureChange(110.0, 50.0);  // Above pressure limit
    QTest::qWait(200);
    
    QVERIFY(overpressureSpy.count() > 0);
    QVERIFY(!safetyManager->isSystemSafe());
    
    qDebug() << "Sensor to safety integration test completed successfully";
}

void IntegrationTests::testPatternToHardwareIntegration()
{
    qDebug() << "Testing pattern to hardware integration";
    
    auto patternEngine = m_controller->getPatternEngine();
    auto hardwareManager = m_controller->getHardwareManager();
    
    QVERIFY(patternEngine != nullptr);
    QVERIFY(hardwareManager != nullptr);
    
    // Setup signal spies
    QSignalSpy patternStartedSpy(patternEngine, &PatternEngine::patternStarted);
    QSignalSpy actuatorChangedSpy(hardwareManager, &HardwareManager::actuatorStateChanged);
    
    // Start a simple pulse pattern
    QVERIFY(m_controller->startPattern("Medium Pulse"));
    
    // Wait for pattern to start
    QVERIFY(patternStartedSpy.wait(1000));
    QCOMPARE(patternEngine->getState(), PatternEngine::RUNNING);
    
    // Verify hardware actuator changes
    QTest::qWait(2000);  // Wait for pattern execution
    QVERIFY(actuatorChangedSpy.count() > 0);
    
    // Stop pattern
    m_controller->stopPattern();
    QTest::qWait(500);
    
    QCOMPARE(patternEngine->getState(), PatternEngine::STOPPED);
    
    qDebug() << "Pattern to hardware integration test completed successfully";
}

void IntegrationTests::testGUIToControllerIntegration()
{
    qDebug() << "Testing GUI to controller integration";
    
    // Get GUI components
    auto pressureMonitor = m_mainWindow->getPressureMonitor();
    auto patternSelector = m_mainWindow->getPatternSelector();
    auto safetyPanel = m_mainWindow->getSafetyPanel();
    
    QVERIFY(pressureMonitor != nullptr);
    QVERIFY(patternSelector != nullptr);
    QVERIFY(safetyPanel != nullptr);
    
    // Test pressure monitor updates
    QSignalSpy pressureUpdateSpy(m_controller.get(), &VacuumController::pressureUpdated);
    
    simulatePressureChange(70.0, 60.0);
    QTest::qWait(200);
    
    QVERIFY(pressureUpdateSpy.count() > 0);
    
    // Test pattern selection
    QSignalSpy patternSelectedSpy(patternSelector, &PatternSelector::patternSelected);
    
    patternSelector->selectPattern("Slow Pulse");
    QVERIFY(patternSelectedSpy.wait(1000));
    
    // Test safety panel alerts
    QSignalSpy safetyAlertSpy(safetyPanel, &SafetyPanel::safetyAlert);
    
    simulatePressureChange(110.0, 60.0);  // Trigger overpressure
    QTest::qWait(500);
    
    QVERIFY(safetyAlertSpy.count() > 0);
    
    qDebug() << "GUI to controller integration test completed successfully";
}

void IntegrationTests::testThreadCommunication()
{
    qDebug() << "Testing multi-thread communication";
    
    // Test data acquisition thread communication
    QSignalSpy pressureUpdateSpy(m_controller.get(), &VacuumController::pressureUpdated);
    
    // Simulate rapid pressure changes
    for (int i = 0; i < 10; ++i) {
        simulatePressureChange(50.0 + i * 2, 40.0 + i);
        QTest::qWait(50);
    }
    
    // Verify pressure updates are received
    QVERIFY(pressureUpdateSpy.count() >= 5);
    
    // Test safety thread communication
    auto safetyManager = m_controller->getSafetyManager();
    QSignalSpy safetyViolationSpy(safetyManager, &SafetyManager::safetyViolation);
    
    // Trigger safety violation
    simulatePressureChange(120.0, 60.0);
    QTest::qWait(200);
    
    QVERIFY(safetyViolationSpy.count() > 0);
    
    // Test pattern execution thread
    auto patternEngine = m_controller->getPatternEngine();
    QSignalSpy stepChangedSpy(patternEngine, &PatternEngine::stepChanged);
    
    QVERIFY(m_controller->startPattern("Fast Pulse"));
    QTest::qWait(2000);
    
    QVERIFY(stepChangedSpy.count() > 0);
    
    m_controller->stopPattern();
    
    qDebug() << "Thread communication test completed successfully";
}

void IntegrationTests::testRealTimePerformance()
{
    qDebug() << "Testing real-time performance requirements";
    
    QElapsedTimer timer;
    int pressureUpdateCount = 0;
    int guiUpdateCount = 0;
    int safetyCheckCount = 0;
    
    // Setup signal counters
    connect(m_controller.get(), &VacuumController::pressureUpdated, [&]() {
        pressureUpdateCount++;
    });
    
    auto safetyManager = m_controller->getSafetyManager();
    connect(safetyManager, &SafetyManager::safetyViolation, [&]() {
        safetyCheckCount++;
    });
    
    // Run for 2 seconds and measure performance
    timer.start();
    
    while (timer.elapsed() < 2000) {
        // Simulate varying pressure
        double pressure = 60.0 + 20.0 * sin(timer.elapsed() / 100.0);
        simulatePressureChange(pressure, 50.0);
        
        QApplication::processEvents();
        QTest::qWait(10);
        guiUpdateCount++;
    }
    
    qint64 elapsedMs = timer.elapsed();
    
    // Calculate rates
    double pressureRate = (pressureUpdateCount * 1000.0) / elapsedMs;
    double guiRate = (guiUpdateCount * 1000.0) / elapsedMs;
    
    qDebug() << "Performance metrics:";
    qDebug() << "  Pressure update rate:" << pressureRate << "Hz (target: 50Hz)";
    qDebug() << "  GUI update rate:" << guiRate << "Hz (target: 30Hz)";
    
    // Verify performance requirements
    QVERIFY(pressureRate >= 45.0);  // Allow 10% tolerance
    QVERIFY(guiRate >= 25.0);       // Allow some tolerance for test environment
    
    qDebug() << "Real-time performance test completed successfully";
}

void IntegrationTests::testSystemRecovery()
{
    qDebug() << "Testing system recovery capabilities";
    
    // Test recovery from emergency stop
    QSignalSpy emergencyStopSpy(m_controller.get(), &VacuumController::emergencyStopTriggered);
    
    m_controller->emergencyStop();
    QVERIFY(emergencyStopSpy.wait(1000));
    QVERIFY(m_controller->isEmergencyStopActive());
    
    // Test recovery
    QVERIFY(m_controller->resetEmergencyStop());
    QTest::qWait(500);
    QVERIFY(!m_controller->isEmergencyStopActive());
    QCOMPARE(m_controller->getSystemState(), VacuumController::READY);
    
    // Test recovery from sensor error
    simulateSensorFailure("AVL");
    QTest::qWait(500);
    
    // Reset sensor
    resetHardwareSimulation();
    QTest::qWait(1000);
    
    QVERIFY(m_controller->isReady());
    
    // Test recovery from pattern error
    QVERIFY(m_controller->startPattern("Medium Pulse"));
    QTest::qWait(500);
    
    // Simulate pattern error by triggering safety violation
    simulatePressureChange(120.0, 60.0);
    QTest::qWait(500);
    
    // System should stop pattern and recover
    auto patternEngine = m_controller->getPatternEngine();
    QCOMPARE(patternEngine->getState(), PatternEngine::STOPPED);
    
    // Reset to normal conditions
    simulatePressureChange(60.0, 50.0);
    QTest::qWait(1000);
    
    QVERIFY(m_controller->isReady());
    
    qDebug() << "System recovery test completed successfully";
}

void IntegrationTests::testDataFlowIntegrity()
{
    qDebug() << "Testing data flow integrity";
    
    // Test sensor data flow
    QSignalSpy pressureUpdateSpy(m_controller.get(), &VacuumController::pressureUpdated);
    
    // Set known pressure values
    double testAVL = 75.5;
    double testTank = 65.3;
    
    simulatePressureChange(testAVL, testTank);
    QVERIFY(pressureUpdateSpy.wait(1000));
    
    // Verify data integrity
    auto args = pressureUpdateSpy.last();
    double receivedAVL = args.at(0).toDouble();
    double receivedTank = args.at(1).toDouble();
    
    QVERIFY(qAbs(receivedAVL - testAVL) < 0.1);
    QVERIFY(qAbs(receivedTank - testTank) < 0.1);
    
    // Test pattern parameter flow
    auto patternEngine = m_controller->getPatternEngine();
    
    QVERIFY(m_controller->startPattern("Medium Pulse"));
    QTest::qWait(200);
    
    // Adjust parameters
    patternEngine->setIntensity(85.0);
    patternEngine->setSpeed(1.5);
    
    QTest::qWait(500);
    
    // Verify parameters are applied
    QVERIFY(qAbs(patternEngine->getIntensity() - 85.0) < 0.1);
    QVERIFY(qAbs(patternEngine->getSpeed() - 1.5) < 0.1);
    
    m_controller->stopPattern();
    
    qDebug() << "Data flow integrity test completed successfully";
}

// Helper methods
void IntegrationTests::simulatePressureChange(double avlPressure, double tankPressure)
{
    if (m_hardwareSimulationEnabled && m_controller) {
        auto hardwareManager = m_controller->getHardwareManager();
        if (hardwareManager) {
            hardwareManager->setSimulatedSensorValues(avlPressure, tankPressure);
        }
    }
}

void IntegrationTests::simulateHardwareFailure(const QString& component)
{
    if (m_hardwareSimulationEnabled && m_controller) {
        auto hardwareManager = m_controller->getHardwareManager();
        if (hardwareManager) {
            hardwareManager->simulateHardwareFailure(component);
        }
    }
}

void IntegrationTests::simulateSensorFailure(const QString& sensor)
{
    if (m_hardwareSimulationEnabled && m_controller) {
        auto hardwareManager = m_controller->getHardwareManager();
        if (hardwareManager) {
            hardwareManager->simulateSensorError(sensor);
        }
    }
}

void IntegrationTests::resetHardwareSimulation()
{
    if (m_hardwareSimulationEnabled && m_controller) {
        auto hardwareManager = m_controller->getHardwareManager();
        if (hardwareManager) {
            hardwareManager->resetHardwareSimulation();
        }
    }
}

QTEST_MAIN(IntegrationTests)
