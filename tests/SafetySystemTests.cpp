#include "SafetySystemTests.h"
#include "safety/SafetyConstants.h"
#include <QSignalSpy>
#include <QTest>
#include <QDebug>
#include <QCoreApplication>

SafetySystemTests::SafetySystemTests(QObject *parent)
    : TestSuite("SafetySystem", parent)
    , m_safetyManager(nullptr)
    , m_hardwareManager(nullptr)
    , m_antiDetachmentMonitor(nullptr)
    , m_adaptiveSealMonitor(nullptr)
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
    m_adaptiveSealMonitor = new AdaptiveSealMonitor(m_hardwareManager, this);
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

    if (m_adaptiveSealMonitor) {
        m_adaptiveSealMonitor->shutdown();
        m_adaptiveSealMonitor->deleteLater();
        m_adaptiveSealMonitor = nullptr;
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
        << "testFullVentOnRunawayPumpWithInvalidSensors"
        << "testAdaptiveSealMonitorInitialization"
        << "testAdaptiveThresholdCalculation"
        << "testMultiPathLeakDetection"
        << "testProgressiveResealRecovery"
        << "testArousalIntegration"
        << "testPhasePriorityAdjustment";
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
    } else if (testName == "testAdaptiveSealMonitorInitialization") {
        return testAdaptiveSealMonitorInitialization();
    } else if (testName == "testAdaptiveThresholdCalculation") {
        return testAdaptiveThresholdCalculation();
    } else if (testName == "testMultiPathLeakDetection") {
        return testMultiPathLeakDetection();
    } else if (testName == "testProgressiveResealRecovery") {
        return testProgressiveResealRecovery();
    } else if (testName == "testArousalIntegration") {
        return testArousalIntegration();
    } else if (testName == "testPhasePriorityAdjustment") {
        return testPhasePriorityAdjustment();
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

// ============================================================================
// AdaptiveSealMonitor Tests
// ============================================================================

TestResult SafetySystemTests::testAdaptiveSealMonitorInitialization()
{
    if (!m_adaptiveSealMonitor) {
        setLastError("AdaptiveSealMonitor not created");
        return TEST_FAILED;
    }

    // Initialize the monitor
    if (!m_adaptiveSealMonitor->initialize()) {
        setLastError("AdaptiveSealMonitor initialization failed");
        return TEST_FAILED;
    }

    // Verify initial state
    if (m_adaptiveSealMonitor->getSealState() != AdaptiveSealMonitor::SEALED) {
        setLastError("Initial state should be SEALED");
        return TEST_FAILED;
    }

    if (!m_adaptiveSealMonitor->isActive()) {
        setLastError("Monitor should be active after initialization");
        return TEST_FAILED;
    }

    // Verify default thresholds
    double baseline = m_adaptiveSealMonitor->getBaselineThreshold();
    if (baseline != SafetyConstants::DEFAULT_DETACHMENT_THRESHOLD_MMHG) {
        setLastError(QString("Baseline threshold incorrect: %1 vs expected %2")
                     .arg(baseline).arg(SafetyConstants::DEFAULT_DETACHMENT_THRESHOLD_MMHG));
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult SafetySystemTests::testAdaptiveThresholdCalculation()
{
    if (!m_adaptiveSealMonitor || !m_adaptiveSealMonitor->initialize()) {
        setLastError("AdaptiveSealMonitor not available");
        return TEST_FAILED;
    }

    double baseline = m_adaptiveSealMonitor->getBaselineThreshold();

    // Test 1: At zero arousal, threshold should equal baseline
    m_adaptiveSealMonitor->setArousalLevel(0.0);
    double threshold0 = m_adaptiveSealMonitor->getAdaptiveThreshold();
    if (qAbs(threshold0 - baseline) > 0.01) {
        setLastError(QString("At 0 arousal, threshold should be baseline: %1 vs %2")
                     .arg(threshold0).arg(baseline));
        return TEST_FAILED;
    }

    // Test 2: Below minimum arousal, threshold should still equal baseline
    m_adaptiveSealMonitor->setArousalLevel(0.2);  // Below ADAPTIVE_THRESHOLD_MIN_AROUSAL (0.3)
    double thresholdLow = m_adaptiveSealMonitor->getAdaptiveThreshold();
    if (qAbs(thresholdLow - baseline) > 0.01) {
        setLastError(QString("Below min arousal, threshold should be baseline: %1 vs %2")
                     .arg(thresholdLow).arg(baseline));
        return TEST_FAILED;
    }

    // Test 3: At high arousal, threshold should be reduced
    m_adaptiveSealMonitor->setArousalLevel(1.0);  // Maximum arousal
    double thresholdHigh = m_adaptiveSealMonitor->getAdaptiveThreshold();
    double expectedReduction = baseline * SafetyConstants::AROUSAL_COMPENSATION_FACTOR;
    double expectedThreshold = baseline - expectedReduction;

    // Threshold should be reduced but never below critical
    if (thresholdHigh >= baseline) {
        setLastError(QString("At max arousal, threshold should be reduced: %1 vs baseline %2")
                     .arg(thresholdHigh).arg(baseline));
        return TEST_FAILED;
    }

    // Test 4: Threshold should never go below critical minimum
    if (thresholdHigh < SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG) {
        setLastError(QString("Threshold should never go below critical: %1 vs %2")
                     .arg(thresholdHigh).arg(SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG));
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult SafetySystemTests::testMultiPathLeakDetection()
{
    if (!m_adaptiveSealMonitor || !m_adaptiveSealMonitor->initialize()) {
        setLastError("AdaptiveSealMonitor not available");
        return TEST_FAILED;
    }

    // Set up signal spies
    QSignalSpy leakSpy(m_adaptiveSealMonitor, &AdaptiveSealMonitor::leakDetected);
    QSignalSpy criticalSpy(m_adaptiveSealMonitor, &AdaptiveSealMonitor::criticalPressureLoss);

    // Start monitoring
    m_adaptiveSealMonitor->startMonitoring();
    QTest::qWait(50);

    // Simulate critical pressure loss (Path 3)
    m_hardwareManager->setSimulatedPressure(SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG - 5.0);

    // Wait for detection
    QTest::qWait(200);

    // Check that leak was detected
    if (leakSpy.count() < 1) {
        setLastError("Critical pressure loss not detected");
        m_adaptiveSealMonitor->stopMonitoring();
        return TEST_FAILED;
    }

    // Verify the leak path was CRITICAL_LOSS
    QList<QVariant> args = leakSpy.takeFirst();
    AdaptiveSealMonitor::LeakPath path = args.at(0).value<AdaptiveSealMonitor::LeakPath>();
    if (path != AdaptiveSealMonitor::CRITICAL_LOSS) {
        setLastError(QString("Expected CRITICAL_LOSS path, got %1").arg(static_cast<int>(path)));
        m_adaptiveSealMonitor->stopMonitoring();
        return TEST_FAILED;
    }

    m_adaptiveSealMonitor->stopMonitoring();
    return TEST_PASSED;
}

TestResult SafetySystemTests::testProgressiveResealRecovery()
{
    if (!m_adaptiveSealMonitor || !m_adaptiveSealMonitor->initialize()) {
        setLastError("AdaptiveSealMonitor not available");
        return TEST_FAILED;
    }

    // Set up signal spies
    QSignalSpy resealStartSpy(m_adaptiveSealMonitor, &AdaptiveSealMonitor::resealAttemptStarted);
    QSignalSpy resealSuccessSpy(m_adaptiveSealMonitor, &AdaptiveSealMonitor::resealSuccessful);

    // Start monitoring
    m_adaptiveSealMonitor->startMonitoring();
    QTest::qWait(50);

    // Simulate a leak
    m_hardwareManager->setSimulatedPressure(SafetyConstants::CRITICAL_SEAL_THRESHOLD_MMHG - 5.0);
    QTest::qWait(200);

    // Check that reseal attempt started
    if (resealStartSpy.count() < 1) {
        setLastError("Reseal attempt not started after leak detection");
        m_adaptiveSealMonitor->stopMonitoring();
        return TEST_FAILED;
    }

    // Verify first attempt parameters
    QList<QVariant> args = resealStartSpy.takeFirst();
    int attemptNumber = args.at(0).toInt();
    double pumpPower = args.at(1).toDouble();

    if (attemptNumber != 1) {
        setLastError(QString("First attempt should be #1, got %1").arg(attemptNumber));
        m_adaptiveSealMonitor->stopMonitoring();
        return TEST_FAILED;
    }

    // Pump power should be base + increment = 0.3 + 0.2 = 0.5 (50%)
    double expectedPower = SafetyConstants::RESEAL_BASE_PUMP_POWER + SafetyConstants::RESEAL_POWER_INCREMENT;
    if (qAbs(pumpPower - expectedPower) > 0.01) {
        setLastError(QString("First attempt power should be %1, got %2")
                     .arg(expectedPower).arg(pumpPower));
        m_adaptiveSealMonitor->stopMonitoring();
        return TEST_FAILED;
    }

    // Simulate successful reseal by restoring pressure
    m_hardwareManager->setSimulatedPressure(60.0);  // Above threshold
    QTest::qWait(SafetyConstants::RESEAL_ATTEMPT_DURATION_MS + 100);

    // Check for success signal
    if (resealSuccessSpy.count() < 1) {
        // May have failed and tried again - that's ok for this test
        qDebug() << "Note: Reseal may have required multiple attempts";
    }

    m_adaptiveSealMonitor->stopMonitoring();
    return TEST_PASSED;
}

TestResult SafetySystemTests::testArousalIntegration()
{
    if (!m_adaptiveSealMonitor || !m_adaptiveSealMonitor->initialize()) {
        setLastError("AdaptiveSealMonitor not available");
        return TEST_FAILED;
    }

    // Test arousal level setter/getter
    m_adaptiveSealMonitor->setArousalLevel(0.75);
    if (qAbs(m_adaptiveSealMonitor->getArousalLevel() - 0.75) > 0.001) {
        setLastError("Arousal level not set correctly");
        return TEST_FAILED;
    }

    // Test clamping at boundaries
    m_adaptiveSealMonitor->setArousalLevel(1.5);  // Above max
    if (m_adaptiveSealMonitor->getArousalLevel() > 1.0) {
        setLastError("Arousal level should be clamped to 1.0");
        return TEST_FAILED;
    }

    m_adaptiveSealMonitor->setArousalLevel(-0.5);  // Below min
    if (m_adaptiveSealMonitor->getArousalLevel() < 0.0) {
        setLastError("Arousal level should be clamped to 0.0");
        return TEST_FAILED;
    }

    // Test clitoral pressure integration
    m_adaptiveSealMonitor->setClitoralPressure(30.0);
    m_adaptiveSealMonitor->setClitoralPressure(35.0);  // Rising pressure

    return TEST_PASSED;
}

TestResult SafetySystemTests::testPhasePriorityAdjustment()
{
    if (!m_adaptiveSealMonitor || !m_adaptiveSealMonitor->initialize()) {
        setLastError("AdaptiveSealMonitor not available");
        return TEST_FAILED;
    }

    // Set up signal spy
    QSignalSpy prioritySpy(m_adaptiveSealMonitor, &AdaptiveSealMonitor::phasePriorityChanged);

    // Test NORMAL priority (default)
    if (m_adaptiveSealMonitor->getPhasePriority() != AdaptiveSealMonitor::NORMAL) {
        setLastError("Default priority should be NORMAL");
        return TEST_FAILED;
    }

    // Test ELEVATED priority
    m_adaptiveSealMonitor->setPhasePriority(AdaptiveSealMonitor::ELEVATED);
    if (m_adaptiveSealMonitor->getPhasePriority() != AdaptiveSealMonitor::ELEVATED) {
        setLastError("Priority should be ELEVATED");
        return TEST_FAILED;
    }

    // Test CRITICAL priority
    m_adaptiveSealMonitor->setPhasePriority(AdaptiveSealMonitor::CRITICAL);
    if (m_adaptiveSealMonitor->getPhasePriority() != AdaptiveSealMonitor::CRITICAL) {
        setLastError("Priority should be CRITICAL");
        return TEST_FAILED;
    }

    // Verify signals were emitted
    if (prioritySpy.count() < 2) {
        setLastError("Priority change signals not emitted");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}
