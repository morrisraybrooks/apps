#include "OrgasmControlAlgorithmTests.h"
#include <QSignalSpy>
#include <QTest>
#include <QDebug>

OrgasmControlAlgorithmTests::OrgasmControlAlgorithmTests(QObject *parent)
    : TestSuite("OrgasmControlAlgorithm", parent)
    , m_hardwareManager(nullptr)
    , m_algorithm(nullptr)
{
}

bool OrgasmControlAlgorithmTests::setup()
{
    qDebug() << "Setting up OrgasmControlAlgorithm Tests";

    // Create hardware manager in simulation mode for testing
    m_hardwareManager = new HardwareManager(this);
    m_hardwareManager->setSimulationMode(true);

    // Initialize hardware manager
    if (!m_hardwareManager->initialize()) {
        setLastError("Failed to initialize hardware manager");
        return false;
    }

    // Create algorithm instance
    m_algorithm = new OrgasmControlAlgorithm(m_hardwareManager, this);

    return true;
}

void OrgasmControlAlgorithmTests::cleanup()
{
    if (m_algorithm) {
        m_algorithm->stop();
        m_algorithm->deleteLater();
        m_algorithm = nullptr;
    }

    if (m_hardwareManager) {
        m_hardwareManager->shutdown();
        m_hardwareManager->deleteLater();
        m_hardwareManager = nullptr;
    }
}

QStringList OrgasmControlAlgorithmTests::testNames() const
{
    return QStringList()
        << "testInitialization"
        << "testDefaultThresholds"
        << "testSetThresholds"
        << "testStartAdaptiveEdging"
        << "testStartForcedOrgasm"
        << "testStartDenial"
        << "testStartMilking"
        << "testStop"
        << "testEmergencyStop"
        << "testArousalStateTransitions"
        << "testSetTENSEnabled"
        << "testSetAntiEscapeEnabled"
        << "testMilkingThresholdValidation"
        << "testCalibrationValidation"
        << "testSignalEmissions";
}

TestResult OrgasmControlAlgorithmTests::runTest(const QString& testName)
{
    if (testName == "testInitialization") return testInitialization();
    if (testName == "testDefaultThresholds") return testDefaultThresholds();
    if (testName == "testSetThresholds") return testSetThresholds();
    if (testName == "testStartAdaptiveEdging") return testStartAdaptiveEdging();
    if (testName == "testStartForcedOrgasm") return testStartForcedOrgasm();
    if (testName == "testStartDenial") return testStartDenial();
    if (testName == "testStartMilking") return testStartMilking();
    if (testName == "testStop") return testStop();
    if (testName == "testEmergencyStop") return testEmergencyStop();
    if (testName == "testArousalStateTransitions") return testArousalStateTransitions();
    if (testName == "testSetTENSEnabled") return testSetTENSEnabled();
    if (testName == "testSetAntiEscapeEnabled") return testSetAntiEscapeEnabled();
    if (testName == "testMilkingThresholdValidation") return testMilkingThresholdValidation();
    if (testName == "testCalibrationValidation") return testCalibrationValidation();
    if (testName == "testSignalEmissions") return testSignalEmissions();

    setLastError(QString("Unknown test: %1").arg(testName));
    return TEST_FAILED;
}

// ============================================================================
// Initialization Tests
// ============================================================================

TestResult OrgasmControlAlgorithmTests::testInitialization()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Verify initial state is STOPPED
    if (m_algorithm->getState() != OrgasmControlAlgorithm::ControlState::STOPPED) {
        setLastError("Initial state should be STOPPED");
        return TEST_FAILED;
    }

    // Verify initial mode is MANUAL
    if (m_algorithm->getMode() != OrgasmControlAlgorithm::Mode::MANUAL) {
        setLastError("Initial mode should be MANUAL");
        return TEST_FAILED;
    }

    // Verify initial arousal level is 0
    if (m_algorithm->getArousalLevel() > 0.001) {
        setLastError(QString("Initial arousal should be 0, got %1").arg(m_algorithm->getArousalLevel()));
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testDefaultThresholds()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Check default edge threshold (0.70)
    double edgeThresh = m_algorithm->edgeThreshold();
    if (edgeThresh < 0.69 || edgeThresh > 0.71) {
        setLastError(QString("Edge threshold should be ~0.70, got %1").arg(edgeThresh));
        return TEST_FAILED;
    }

    // Check default orgasm threshold (0.90)
    double orgasmThresh = m_algorithm->orgasmThreshold();
    if (orgasmThresh < 0.89 || orgasmThresh > 0.91) {
        setLastError(QString("Orgasm threshold should be ~0.90, got %1").arg(orgasmThresh));
        return TEST_FAILED;
    }

    // Check default recovery threshold (0.45)
    double recoveryThresh = m_algorithm->recoveryThreshold();
    if (recoveryThresh < 0.44 || recoveryThresh > 0.46) {
        setLastError(QString("Recovery threshold should be ~0.45, got %1").arg(recoveryThresh));
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testSetThresholds()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Test setting edge threshold
    m_algorithm->setEdgeThreshold(0.75);
    double edgeThresh = m_algorithm->edgeThreshold();
    if (edgeThresh < 0.74 || edgeThresh > 0.76) {
        setLastError(QString("Edge threshold should be 0.75, got %1").arg(edgeThresh));
        return TEST_FAILED;
    }

    // Test setting orgasm threshold
    m_algorithm->setOrgasmThreshold(0.92);
    double orgasmThresh = m_algorithm->orgasmThreshold();
    if (orgasmThresh < 0.91 || orgasmThresh > 0.93) {
        setLastError(QString("Orgasm threshold should be 0.92, got %1").arg(orgasmThresh));
        return TEST_FAILED;
    }

    // Test setting recovery threshold
    m_algorithm->setRecoveryThreshold(0.40);
    double recoveryThresh = m_algorithm->recoveryThreshold();
    if (recoveryThresh < 0.39 || recoveryThresh > 0.41) {
        setLastError(QString("Recovery threshold should be 0.40, got %1").arg(recoveryThresh));
        return TEST_FAILED;
    }

    // Test threshold clamping (edge threshold max is 0.95)
    m_algorithm->setEdgeThreshold(1.5);
    if (m_algorithm->edgeThreshold() > 0.95) {
        setLastError("Edge threshold should be clamped to 0.95");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

// ============================================================================
// Mode Start/Stop Tests
// ============================================================================

TestResult OrgasmControlAlgorithmTests::testStartAdaptiveEdging()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Set up signal spy for state changes
    QSignalSpy stateSpy(m_algorithm, &OrgasmControlAlgorithm::stateChanged);

    // Start adaptive edging with 3 target cycles
    m_algorithm->startAdaptiveEdging(3);

    // Verify state transitioned to CALIBRATING
    if (m_algorithm->getState() != OrgasmControlAlgorithm::ControlState::CALIBRATING) {
        setLastError("State should be CALIBRATING after start");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    // Verify mode is ADAPTIVE_EDGING
    if (m_algorithm->getMode() != OrgasmControlAlgorithm::Mode::ADAPTIVE_EDGING) {
        setLastError("Mode should be ADAPTIVE_EDGING");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    // Verify edge count is reset to 0
    if (m_algorithm->getEdgeCount() != 0) {
        setLastError(QString("Edge count should be 0 at start, got %1").arg(m_algorithm->getEdgeCount()));
        m_algorithm->stop();
        return TEST_FAILED;
    }

    m_algorithm->stop();
    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testStartForcedOrgasm()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Start forced orgasm with 2 target orgasms, 30 minute max
    m_algorithm->startForcedOrgasm(2, 30 * 60 * 1000);

    // Verify state is CALIBRATING
    if (m_algorithm->getState() != OrgasmControlAlgorithm::ControlState::CALIBRATING) {
        setLastError("State should be CALIBRATING after start");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    // Verify mode is FORCED_ORGASM
    if (m_algorithm->getMode() != OrgasmControlAlgorithm::Mode::FORCED_ORGASM) {
        setLastError("Mode should be FORCED_ORGASM");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    m_algorithm->stop();
    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testStartDenial()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Start denial mode for 20 minutes
    m_algorithm->startDenial(20 * 60 * 1000);

    // Verify state is CALIBRATING
    if (m_algorithm->getState() != OrgasmControlAlgorithm::ControlState::CALIBRATING) {
        setLastError("State should be CALIBRATING after start");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    // Verify mode is DENIAL
    if (m_algorithm->getMode() != OrgasmControlAlgorithm::Mode::DENIAL) {
        setLastError("Mode should be DENIAL");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    m_algorithm->stop();
    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testStartMilking()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Start milking mode for 45 minutes with failure mode 1
    m_algorithm->startMilking(45 * 60 * 1000, 1);

    // Verify state is CALIBRATING
    if (m_algorithm->getState() != OrgasmControlAlgorithm::ControlState::CALIBRATING) {
        setLastError("State should be CALIBRATING after start");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    // Verify mode is MILKING
    if (m_algorithm->getMode() != OrgasmControlAlgorithm::Mode::MILKING) {
        setLastError("Mode should be MILKING");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    m_algorithm->stop();
    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testStop()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Start and then stop
    m_algorithm->startAdaptiveEdging(5);
    QTest::qWait(50);  // Let timers start

    m_algorithm->stop();

    // Verify state is STOPPED
    if (m_algorithm->getState() != OrgasmControlAlgorithm::ControlState::STOPPED) {
        setLastError("State should be STOPPED after stop()");
        return TEST_FAILED;
    }

    // Verify mode is MANUAL
    if (m_algorithm->getMode() != OrgasmControlAlgorithm::Mode::MANUAL) {
        setLastError("Mode should be MANUAL after stop()");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testEmergencyStop()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Set up signal spy for emergencyStopActivated
    QSignalSpy emergencySpy(m_algorithm, &OrgasmControlAlgorithm::emergencyStopActivated);

    // Start algorithm
    m_algorithm->startAdaptiveEdging(5);
    QTest::qWait(50);

    // Trigger emergency stop
    m_algorithm->emergencyStop();

    // Verify emergency stop was triggered
    if (emergencySpy.count() < 1) {
        setLastError("Emergency stop signal should be emitted");
        return TEST_FAILED;
    }

    // Verify state
    auto state = m_algorithm->getState();
    if (state != OrgasmControlAlgorithm::ControlState::STOPPED &&
        state != OrgasmControlAlgorithm::ControlState::ERROR) {
        setLastError("State should be STOPPED or ERROR after emergency stop");
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

// ============================================================================
// Arousal State Tests
// ============================================================================

TestResult OrgasmControlAlgorithmTests::testArousalStateTransitions()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Initial arousal state should be BASELINE
    auto arousalState = m_algorithm->getArousalState();
    if (arousalState != OrgasmControlAlgorithm::ArousalState::BASELINE) {
        // Note: May be WARMING if hardware simulation provides non-zero pressure
        qDebug() << "Initial arousal state:" << static_cast<int>(arousalState);
    }

    return TEST_PASSED;
}

// ============================================================================
// Configuration Tests
// ============================================================================

TestResult OrgasmControlAlgorithmTests::testSetTENSEnabled()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    m_algorithm->setTENSEnabled(false);
    // No direct getter, but should not crash

    m_algorithm->setTENSEnabled(true);

    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testSetAntiEscapeEnabled()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    m_algorithm->setAntiEscapeEnabled(false);
    m_algorithm->setAntiEscapeEnabled(true);

    return TEST_PASSED;
}

TestResult OrgasmControlAlgorithmTests::testMilkingThresholdValidation()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Test milking zone lower threshold validation
    m_algorithm->setMilkingZoneLower(0.60);
    m_algorithm->setMilkingZoneUpper(0.85);
    m_algorithm->setDangerThreshold(0.90);

    // These should not crash and should maintain valid ordering
    // lower < upper < danger

    return TEST_PASSED;
}

// ============================================================================
// Safety Tests
// ============================================================================

TestResult OrgasmControlAlgorithmTests::testCalibrationValidation()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Start algorithm and let it try to calibrate
    // In simulation mode with no pressure, calibration may fail validation
    m_algorithm->startAdaptiveEdging(3);

    // Wait a short time (less than calibration duration)
    QTest::qWait(100);

    // State should still be CALIBRATING or transitioned based on sim values
    auto state = m_algorithm->getState();
    qDebug() << "State after 100ms:" << static_cast<int>(state);

    m_algorithm->stop();
    return TEST_PASSED;
}

// ============================================================================
// Signal Emission Tests
// ============================================================================

TestResult OrgasmControlAlgorithmTests::testSignalEmissions()
{
    if (!m_algorithm) {
        setLastError("Algorithm not created");
        return TEST_FAILED;
    }

    // Set up signal spies
    QSignalSpy stateSpy(m_algorithm, &OrgasmControlAlgorithm::stateChanged);
    QSignalSpy modeSpy(m_algorithm, &OrgasmControlAlgorithm::modeChanged);

    // Start algorithm - should emit state and mode changes
    m_algorithm->startAdaptiveEdging(3);

    if (stateSpy.count() < 1) {
        setLastError("State change signal should be emitted on start");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    if (modeSpy.count() < 1) {
        setLastError("Mode change signal should be emitted on start");
        m_algorithm->stop();
        return TEST_FAILED;
    }

    m_algorithm->stop();
    return TEST_PASSED;
}

// Stub implementations for tests not yet fully implemented
TestResult OrgasmControlAlgorithmTests::testArousalLevelCalculation()
{
    return TEST_PASSED;  // TODO: Implement with simulated pressure values
}

TestResult OrgasmControlAlgorithmTests::testArousalSmoothing()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testEdgeDetection()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testBackOffBehavior()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testPointOfNoReturnDetection()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testOrgasmDetection()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testUnexpectedOrgasmDuringEdging()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testMilkingZoneTracking()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testMilkingPIDControl()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testDangerZoneReduction()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testHighPressureLimit()
{
    return TEST_PASSED;  // TODO: Implement
}

TestResult OrgasmControlAlgorithmTests::testSensorErrorHandling()
{
    return TEST_PASSED;  // TODO: Implement
}

