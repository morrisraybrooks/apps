#include "SafetySystemTests.h"
#include "../src/safety/SafetyManager.h"
#include "../src/safety/AntiDetachmentMonitor.h"
#include "../src/safety/EmergencyStop.h"
#include "../src/hardware/HardwareManager.h"
#include <QSignalSpy>
#include <QTest>
#include <QDebug>

SafetySystemTests::SafetySystemTests(QObject *parent)
    : TestSuite("SafetySystem", parent)
    , m_safetyManager(nullptr)
    , m_hardwareManager(nullptr)
    , m_antiDetachmentMonitor(nullptr)
    , m_emergencyStop(nullptr)
{
}

SafetySystemTests::~SafetySystemTests()
{
    cleanup();
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
        << "testPressureLimitEnforcement"
        << "testEmergencyStopActivation"
        << "testAntiDetachmentMonitoring"
        << "testSafetyViolationHandling"
        << "testSafetySystemRecovery"
        << "testConcurrentSafetyChecks"
        << "testSafetyConfigurationValidation";
}

TestResult SafetySystemTests::runTest(const QString& testName)
{
    if (testName == "testSafetyManagerInitialization") {
        return testSafetyManagerInitialization();
    } else if (testName == "testPressureLimitEnforcement") {
        return testPressureLimitEnforcement();
    } else if (testName == "testEmergencyStopActivation") {
        return testEmergencyStopActivation();
    } else if (testName == "testAntiDetachmentMonitoring") {
        return testAntiDetachmentMonitoring();
    } else if (testName == "testSafetyViolationHandling") {
        return testSafetyViolationHandling();
    } else if (testName == "testSafetySystemRecovery") {
        return testSafetySystemRecovery();
    } else if (testName == "testConcurrentSafetyChecks") {
        return testConcurrentSafetyChecks();
    } else if (testName == "testSafetyConfigurationValidation") {
        return testSafetyConfigurationValidation();
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
    
    // Test initial state
    if (!m_safetyManager->isInitialized()) {
        setLastError("Safety manager not initialized");
        return TEST_FAILED;
    }
    
    if (!m_safetyManager->isSystemSafe()) {
        setLastError("System should be safe after initialization");
        return TEST_FAILED;
    }
    
    // Test safety limits are set correctly
    if (m_safetyManager->getMaxPressure() <= 0) {
        setLastError("Max pressure not set correctly");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testPressureLimitEnforcement()
{
    if (!m_safetyManager || !m_hardwareManager) {
        setLastError("Required components not available");
        return TEST_FAILED;
    }
    
    // Set up signal spy to monitor safety violations
    QSignalSpy violationSpy(m_safetyManager, &SafetyManager::safetyViolation);
    
    // Simulate pressure exceeding safe limits
    double maxPressure = m_safetyManager->getMaxPressure();
    double testPressure = maxPressure + 10.0; // Exceed by 10%
    
    // In simulation mode, we can set the pressure directly
    m_hardwareManager->setSimulatedPressure(testPressure);
    
    // Trigger safety check
    m_safetyManager->performSafetyCheck();
    
    // Wait for signal
    if (!violationSpy.wait(1000)) {
        setLastError("Safety violation signal not emitted for overpressure");
        return TEST_FAILED;
    }
    
    // Verify the system is no longer safe
    if (m_safetyManager->isSystemSafe()) {
        setLastError("System should not be safe after pressure violation");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testEmergencyStopActivation()
{
    if (!m_emergencyStop || !m_hardwareManager) {
        setLastError("Required components not available");
        return TEST_FAILED;
    }
    
    // Set up signal spy
    QSignalSpy emergencyStopSpy(m_emergencyStop, &EmergencyStop::emergencyStopActivated);
    
    // Activate emergency stop
    m_emergencyStop->activate();
    
    // Verify signal was emitted
    if (emergencyStopSpy.count() != 1) {
        setLastError("Emergency stop signal not emitted");
        return TEST_FAILED;
    }
    
    // Verify system state
    if (!m_emergencyStop->isActive()) {
        setLastError("Emergency stop should be active");
        return TEST_FAILED;
    }
    
    // Verify hardware is in safe state
    if (m_hardwareManager->isPumpEnabled()) {
        setLastError("Pump should be disabled during emergency stop");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testAntiDetachmentMonitoring()
{
    if (!m_antiDetachmentMonitor || !m_hardwareManager) {
        setLastError("Required components not available");
        return TEST_FAILED;
    }
    
    // Start monitoring
    m_antiDetachmentMonitor->startMonitoring();
    
    if (!m_antiDetachmentMonitor->isMonitoring()) {
        setLastError("Anti-detachment monitoring should be active");
        return TEST_FAILED;
    }
    
    // Set up signal spy
    QSignalSpy detachmentSpy(m_antiDetachmentMonitor, &AntiDetachmentMonitor::detachmentDetected);
    
    // Simulate detachment condition (rapid pressure drop)
    m_hardwareManager->setSimulatedPressure(50.0);
    QTest::qWait(100);
    m_hardwareManager->setSimulatedPressure(5.0); // Rapid drop
    
    // Wait for detection
    if (!detachmentSpy.wait(2000)) {
        setLastError("Detachment not detected");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testSafetyViolationHandling()
{
    if (!m_safetyManager) {
        setLastError("Safety manager not available");
        return TEST_FAILED;
    }
    
    // Set up signal spy
    QSignalSpy violationSpy(m_safetyManager, &SafetyManager::safetyViolation);
    QSignalSpy shutdownSpy(m_safetyManager, &SafetyManager::emergencyShutdown);
    
    // Simulate multiple safety violations
    for (int i = 0; i < 3; ++i) {
        m_safetyManager->reportSafetyViolation("Test violation " + QString::number(i));
        QTest::qWait(100);
    }
    
    // Verify violations were recorded
    if (violationSpy.count() < 3) {
        setLastError("Not all safety violations were recorded");
        return TEST_FAILED;
    }
    
    // Check if emergency shutdown was triggered
    if (shutdownSpy.count() == 0) {
        setLastError("Emergency shutdown should be triggered after multiple violations");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testSafetySystemRecovery()
{
    if (!m_safetyManager) {
        setLastError("Safety manager not available");
        return TEST_FAILED;
    }
    
    // Trigger a safety violation
    m_safetyManager->reportSafetyViolation("Test violation for recovery");
    
    // Verify system is not safe
    if (m_safetyManager->isSystemSafe()) {
        setLastError("System should not be safe after violation");
        return TEST_FAILED;
    }
    
    // Attempt recovery
    bool recoveryResult = m_safetyManager->attemptRecovery();
    
    if (!recoveryResult) {
        setLastError("Safety system recovery failed");
        return TEST_FAILED;
    }
    
    // Verify system is safe again
    if (!m_safetyManager->isSystemSafe()) {
        setLastError("System should be safe after recovery");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testConcurrentSafetyChecks()
{
    if (!m_safetyManager) {
        setLastError("Safety manager not available");
        return TEST_FAILED;
    }
    
    // Start multiple concurrent safety checks
    QList<QFuture<void>> futures;
    
    for (int i = 0; i < 5; ++i) {
        QFuture<void> future = QtConcurrent::run([this]() {
            for (int j = 0; j < 10; ++j) {
                m_safetyManager->performSafetyCheck();
                QThread::msleep(10);
            }
        });
        futures.append(future);
    }
    
    // Wait for all to complete
    for (auto& future : futures) {
        future.waitForFinished();
    }
    
    // Verify system is still functional
    if (!m_safetyManager->isInitialized()) {
        setLastError("Safety manager should still be initialized after concurrent checks");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}

TestResult SafetySystemTests::testSafetyConfigurationValidation()
{
    if (!m_safetyManager) {
        setLastError("Safety manager not available");
        return TEST_FAILED;
    }
    
    // Test invalid configuration
    SafetyConfiguration invalidConfig;
    invalidConfig.maxPressure = -10.0; // Invalid negative pressure
    invalidConfig.minPressure = 200.0; // Min > Max
    
    if (m_safetyManager->validateConfiguration(invalidConfig)) {
        setLastError("Invalid configuration should not be accepted");
        return TEST_FAILED;
    }
    
    // Test valid configuration
    SafetyConfiguration validConfig;
    validConfig.maxPressure = 100.0;
    validConfig.minPressure = 0.0;
    validConfig.maxPressureGradient = 50.0;
    
    if (!m_safetyManager->validateConfiguration(validConfig)) {
        setLastError("Valid configuration should be accepted");
        return TEST_FAILED;
    }
    
    return TEST_PASSED;
}
