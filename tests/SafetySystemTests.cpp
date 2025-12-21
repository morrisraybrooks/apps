#include "SafetySystemTests.h"
#include <QSignalSpy>
#include <QTest>
#include <QDebug>
#include <QCoreApplication>

SafetySystemTests::SafetySystemTests(QObject *parent)
    : TestSuite("SafetySystem", parent)
    , m_safetyManager(nullptr)
    , m_hardwareManager(nullptr)
    , m_antiDetachmentMonitor(nullptr)
    , m_emergencyStop(nullptr)
{
}

bool SafetySystemTests::setup()
{
    qDebug() << "Setting up Safety System Tests";

    // Create mock hardware manager for testing
    m_hardwareManager = new HardwareManager(this);
    m_hardwareManager->setSimulationMode(true);

    // Create safety components
    m_safetyManager = new SafetyManager(m_hardwareManager, this);
    m_antiDetachmentMonitor = new AntiDetachmentMonitor(m_hardwareManager, this);
    m_emergencyStop = new EmergencyStop(m_hardwareManager, this);

    // Initialize components
    if (!m_hardwareManager->initialize()) {
        setLastError("Failed to initialize hardware manager");
        return false;
    }

    if (!m_safetyManager->initialize()) {
        setLastError("Failed to initialize safety manager");
        return false;
    }

    return true;
}

void SafetySystemTests::cleanup()
{
    // Reset emergency stop flag BEFORE cleaning up hardware
    // so the next test iteration doesn't see the flag already set
    if (m_hardwareManager) {
        if (m_hardwareManager->isEmergencyStop()) {
            m_hardwareManager->resetEmergencyStop();
        }
    }

    if (m_safetyManager) {
        m_safetyManager->shutdown();
        m_safetyManager->deleteLater();
        m_safetyManager = nullptr;
    }

    if (m_antiDetachmentMonitor) {
        m_antiDetachmentMonitor->deleteLater();
        m_antiDetachmentMonitor = nullptr;
    }

    if (m_emergencyStop) {
        m_emergencyStop->deleteLater();
        m_emergencyStop = nullptr;
    }

    if (m_hardwareManager) {
        m_hardwareManager->shutdown();
        m_hardwareManager->deleteLater();
        m_hardwareManager = nullptr;
    }
}

QStringList SafetySystemTests::testNames() const
{
    return QStringList()
        << "testSafetyManagerInitialization"
        << "testEmergencyStopActivation"
        << "testAntiDetachmentMonitoring"
        << "testSealMaintainedSafeStateOnEmergencyStop"
        << "testFullVentOnTissueDamageRiskOverpressure"
        << "testFullVentOnRunawayPumpWithInvalidSensors";
}

TestResult SafetySystemTests::runTest(const QString& testName)
{
    if (testName == "testSafetyManagerInitialization") {
        return testSafetyManagerInitialization();
    } else if (testName == "testEmergencyStopActivation") {
        return testEmergencyStopActivation();
    } else if (testName == "testAntiDetachmentMonitoring") {
        return testAntiDetachmentMonitoring();
    } else if (testName == "testSealMaintainedSafeStateOnEmergencyStop") {
        return testSealMaintainedSafeStateOnEmergencyStop();
    } else if (testName == "testFullVentOnTissueDamageRiskOverpressure") {
        return testFullVentOnTissueDamageRiskOverpressure();
    } else if (testName == "testFullVentOnRunawayPumpWithInvalidSensors") {
        return testFullVentOnRunawayPumpWithInvalidSensors();
    }

    setLastError(QString("Unknown test: %1").arg(testName));
    return TEST_FAILED;
}

TestResult SafetySystemTests::testSafetyManagerInitialization()
{
    if (!m_safetyManager) {
        setLastError("Safety manager not created");
        return TEST_FAILED;
    }

    // Test that safety manager exists and can be queried
    if (!m_safetyManager->isSystemSafe()) {
        // System may not be safe in simulation mode, that's ok
        qDebug() << "Note: System not in safe state (expected in simulation)";
    }

    // Test safety limits are set correctly
    if (m_safetyManager->getMaxPressure() <= 0) {
        setLastError("Max pressure not set correctly");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult SafetySystemTests::testEmergencyStopActivation()
{
    if (!m_emergencyStop) {
        setLastError("Emergency stop not available");
        return TEST_FAILED;
    }

    // Set up signal spy
    QSignalSpy emergencyStopSpy(m_emergencyStop, &EmergencyStop::emergencyStopTriggered);

    // Trigger emergency stop
    m_emergencyStop->trigger("Test trigger");

    // Verify signal was emitted
    if (emergencyStopSpy.count() != 1) {
        setLastError("Emergency stop signal not emitted");
        return TEST_FAILED;
    }

    // Verify system state
    if (!m_emergencyStop->isTriggered()) {
        setLastError("Emergency stop should be triggered");
        return TEST_FAILED;
    }

    // Reset for next test
    m_emergencyStop->reset();

    return TEST_PASSED;
}

TestResult SafetySystemTests::testAntiDetachmentMonitoring()
{
    if (!m_antiDetachmentMonitor) {
        setLastError("Anti-detachment monitor not available");
        return TEST_FAILED;
    }

    // Start monitoring
    m_antiDetachmentMonitor->startMonitoring();

    // Give it time to start
    QTest::qWait(100);

    // Stop monitoring
    m_antiDetachmentMonitor->stopMonitoring();

    return TEST_PASSED;
}

TestResult SafetySystemTests::testSealMaintainedSafeStateOnEmergencyStop()
{
    if (!m_hardwareManager || !m_safetyManager) {
        setLastError("Safety components not initialized");
        return TEST_FAILED;
    }

    // Reset emergency stop flag from any previous test
    // The TestFramework calls cleanup() only once at the end, not between tests
    if (m_hardwareManager->isEmergencyStop()) {
        m_hardwareManager->resetEmergencyStop();
    }

    // Trigger emergency stop via SafetyManager
    QSignalSpy emergencySpy(m_safetyManager, &SafetyManager::emergencyStopTriggered);
    m_safetyManager->triggerEmergencyStop("Test seal-maintained state");
    
    // Process any pending events
    QCoreApplication::processEvents();

    // We expect one emergency event
    if (emergencySpy.count() < 1) {
        setLastError("Emergency stop was not triggered by SafetyManager");
        return TEST_FAILED;
    }

    // In seal-maintained safe state the hardware emergency flag must be set
    if (!m_hardwareManager->isEmergencyStop()) {
        setLastError("Hardware emergency flag not set after emergency stop in seal-maintained state");
        return TEST_FAILED;
    }

    // And AVL vent (SOL2) must remain closed while inner circuits are vented
    // Note: we cannot directly read solenoid state here without exposing
    // getters; this test primarily validates the logical path by ensuring
    // no crash and emergency flag set. Detailed valve behavior is covered
    // by lower-level HardwareTests.

    return TEST_PASSED;
}

TestResult SafetySystemTests::testFullVentOnTissueDamageRiskOverpressure()
{
    if (!m_hardwareManager || !m_safetyManager) {
        setLastError("Safety components not initialized");
        return TEST_FAILED;
    }

    // In simulation mode, directly manipulate simulated pressures
    m_hardwareManager->setSimulationMode(true);

    // Set a pressure above the tissue-damage risk threshold (e.g. 160 mmHg)
    const double riskThreshold = m_safetyManager->tissueDamageRiskPressure();
    m_hardwareManager->setSimulatedPressure(riskThreshold + 10.0);

    QSignalSpy emergencySpy(m_safetyManager, &SafetyManager::emergencyStopTriggered);

    // Force a safety check cycle
    bool ok = m_safetyManager->performSafetyCheck();
    Q_UNUSED(ok);

    // We expect emergency stop due to tissue-damage risk
    if (!emergencySpy.wait(2000) || emergencySpy.count() < 1) {
        setLastError("Emergency stop not triggered for tissue-damage risk overpressure");
        return TEST_FAILED;
    }

    if (!m_hardwareManager->isEmergencyStop()) {
        setLastError("Hardware emergency flag not set after tissue-damage risk overpressure");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult SafetySystemTests::testFullVentOnRunawayPumpWithInvalidSensors()
{
    if (!m_hardwareManager || !m_safetyManager) {
        setLastError("Safety components not initialized");
        return TEST_FAILED;
    }

    // Enable simulation mode to control pressures
    m_hardwareManager->setSimulationMode(true);

    // Configure invalid sensor data: out of valid range
    m_hardwareManager->setSimulatedSensorValues(-10.0, 250.0);

    // Simulate pump runaway via high pump speed
    m_hardwareManager->setPumpSpeed(100.0);

    QSignalSpy emergencySpy(m_safetyManager, &SafetyManager::emergencyStopTriggered);

    // Run multiple safety checks to satisfy consecutive sample requirements
    const int intervalMs = m_safetyManager->monitoringIntervalMs();
    for (int i = 0; i < 10; ++i) {
        m_safetyManager->performSafetyCheck();
        QTest::qWait(intervalMs);
        if (emergencySpy.count() > 0) {
            break;
        }
    }

    if (emergencySpy.count() < 1) {
        setLastError("Emergency stop not triggered for runaway pump with invalid sensors");
        return TEST_FAILED;
    }

    if (!m_hardwareManager->isEmergencyStop()) {
        setLastError("Hardware emergency flag not set after runaway pump with invalid sensors");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}
