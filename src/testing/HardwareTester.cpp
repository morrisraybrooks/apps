#include "HardwareTester.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/SensorInterface.h"
#include "../hardware/ActuatorControl.h"
#include "../safety/SafetyManager.h"
#include "../core/StatisticsUtils.h"
#include <QDebug>
#include <QThread>
#include <QJsonDocument>
#include <QFile>
#include <QCoreApplication>
#include <cmath>
#include <algorithm>

// Constants
const int HardwareTester::DEFAULT_TEST_TIMEOUT = 30000; // 30 seconds
const int HardwareTester::DEFAULT_SENSOR_SAMPLES = 10;
const int HardwareTester::DEFAULT_ACTUATOR_CYCLES = 5;
const double HardwareTester::DEFAULT_SENSOR_TOLERANCE = 5.0; // 5% tolerance
const double HardwareTester::DEFAULT_ACTUATOR_TOLERANCE = 2.0; // 2% tolerance

HardwareTester::HardwareTester(HardwareManager* hardware, SafetyManager* safety, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_safetyManager(safety)
    , m_sensorInterface(nullptr)
    , m_actuatorControl(nullptr)
    , m_testRunning(false)
    , m_progress(0)
    , m_currentTestIndex(0)
    , m_totalTests(0)
    , m_testTimeout(DEFAULT_TEST_TIMEOUT)
    , m_verboseOutput(true)
    , m_continueOnFailure(false)
    , m_testTimer(new QTimer(this))
    , m_timeoutTimer(new QTimer(this))
    , m_outputStream(nullptr)
{
    if (m_hardware) {
        m_sensorInterface = m_hardware->getSensorInterface();
        m_actuatorControl = m_hardware->getActuatorControl();
    }
    
    initializeHardwareTester();
}

HardwareTester::~HardwareTester()
{
    if (m_testRunning) {
        cancelTests();
    }
    
    if (m_outputStream) {
        delete m_outputStream;
    }
}

void HardwareTester::initializeHardwareTester()
{
    setupTestCases();
    
    // Setup output stream if file specified
    if (!m_outputFile.isEmpty()) {
        QFile* file = new QFile(m_outputFile);
        if (file->open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_outputStream = new QTextStream(file);
        } else {
            qWarning() << "Failed to open output file:" << m_outputFile;
            delete file;
        }
    }
}

void HardwareTester::setupTestCases()
{
    m_testCases.clear();
    
    // Sensor tests
    TestCase avlTest;
    avlTest.name = "AVL Sensor Test";
    avlTest.description = "Test AVL pressure sensor functionality and accuracy";
    avlTest.type = SENSOR_TEST;
    m_testCases.append(avlTest);
    
    TestCase tankTest;
    tankTest.name = "Tank Sensor Test";
    tankTest.description = "Test tank pressure sensor functionality and accuracy";
    tankTest.type = SENSOR_TEST;
    m_testCases.append(tankTest);
    
    // Actuator tests
    TestCase pumpTest;
    pumpTest.name = "Pump Control Test";
    pumpTest.description = "Test vacuum pump control and speed regulation";
    pumpTest.type = ACTUATOR_TEST;
    m_testCases.append(pumpTest);
    
    TestCase sol1Test;
    sol1Test.name = "SOL1 Valve Test";
    sol1Test.description = "Test SOL1 (AVL) valve operation";
    sol1Test.type = ACTUATOR_TEST;
    m_testCases.append(sol1Test);
    
    TestCase sol2Test;
    sol2Test.name = "SOL2 Valve Test";
    sol2Test.description = "Test SOL2 (AVL vent) valve operation";
    sol2Test.type = ACTUATOR_TEST;
    m_testCases.append(sol2Test);
    
    TestCase sol3Test;
    sol3Test.name = "SOL3 Valve Test";
    sol3Test.description = "Test SOL3 (tank vent) valve operation";
    sol3Test.type = ACTUATOR_TEST;
    m_testCases.append(sol3Test);
    
    // Safety tests
    TestCase emergencyTest;
    emergencyTest.name = "Emergency Stop Test";
    emergencyTest.description = "Test emergency stop functionality";
    emergencyTest.type = SAFETY_TEST;
    m_testCases.append(emergencyTest);
    
    // Communication tests
    TestCase spiTest;
    spiTest.name = "SPI Communication Test";
    spiTest.description = "Test SPI communication with MCP3008 ADC";
    spiTest.type = COMMUNICATION_TEST;
    m_testCases.append(spiTest);
    
    TestCase gpioTest;
    gpioTest.name = "GPIO Test";
    gpioTest.description = "Test GPIO pin functionality";
    gpioTest.type = COMMUNICATION_TEST;
    m_testCases.append(gpioTest);
    
    m_totalTests = m_testCases.size();
}

bool HardwareTester::runSensorTests()
{
    if (m_testRunning) return false;
    
    logTestMessage("Starting sensor tests...");
    
    m_testRunning = true;
    m_currentTestSuite.name = "Sensor Tests";
    m_currentTestSuite.description = "Comprehensive sensor functionality tests";
    m_currentTestSuite.startTime = QDateTime::currentDateTime();
    
    emit testStarted("Sensor Tests");
    
    bool allPassed = true;
    
    // Run sensor-specific tests
    for (TestCase& testCase : m_testCases) {
        if (testCase.type == SENSOR_TEST) {
            if (!executeTestCase(testCase)) {
                allPassed = false;
                if (!m_continueOnFailure) break;
            }
        }
    }
    
    m_currentTestSuite.endTime = QDateTime::currentDateTime();
    m_testRunning = false;
    
    emit testSuiteCompleted(m_currentTestSuite);
    
    return allPassed;
}

bool HardwareTester::runActuatorTests()
{
    if (m_testRunning) return false;
    
    logTestMessage("Starting actuator tests...");
    
    m_testRunning = true;
    m_currentTestSuite.name = "Actuator Tests";
    m_currentTestSuite.description = "Comprehensive actuator functionality tests";
    m_currentTestSuite.startTime = QDateTime::currentDateTime();
    
    emit testStarted("Actuator Tests");
    
    bool allPassed = true;
    
    // Run actuator-specific tests
    for (TestCase& testCase : m_testCases) {
        if (testCase.type == ACTUATOR_TEST) {
            if (!executeTestCase(testCase)) {
                allPassed = false;
                if (!m_continueOnFailure) break;
            }
        }
    }
    
    m_currentTestSuite.endTime = QDateTime::currentDateTime();
    m_testRunning = false;
    
    emit testSuiteCompleted(m_currentTestSuite);
    
    return allPassed;
}

bool HardwareTester::runComprehensiveTests()
{
    if (m_testRunning) return false;
    
    logTestMessage("Starting comprehensive hardware tests...");
    
    m_testRunning = true;
    m_currentTestSuite.name = "Comprehensive Tests";
    m_currentTestSuite.description = "Complete hardware validation test suite";
    m_currentTestSuite.startTime = QDateTime::currentDateTime();
    m_currentTestIndex = 0;
    
    emit testStarted("Comprehensive Tests");
    
    bool allPassed = true;
    
    // Run all test cases
    for (TestCase& testCase : m_testCases) {
        if (!executeTestCase(testCase)) {
            allPassed = false;
            if (!m_continueOnFailure) break;
        }
        m_currentTestIndex++;
        updateTestProgress();
    }
    
    m_currentTestSuite.endTime = QDateTime::currentDateTime();
    m_lastTestSuite = m_currentTestSuite;
    m_testRunning = false;
    
    emit testSuiteCompleted(m_currentTestSuite);
    
    return allPassed;
}

bool HardwareTester::executeTestCase(TestCase& testCase)
{
    startTestCase(testCase);
    
    bool result = false;
    
    try {
        // Execute specific test based on name
        if (testCase.name == "AVL Sensor Test") {
            result = performAVLSensorTest(testCase);
        } else if (testCase.name == "Tank Sensor Test") {
            result = performTankSensorTest(testCase);
        } else if (testCase.name == "Pump Control Test") {
            result = performPumpTest(testCase);
        } else if (testCase.name == "SOL1 Valve Test") {
            result = performValveTest(testCase, "SOL1");
        } else if (testCase.name == "SOL2 Valve Test") {
            result = performValveTest(testCase, "SOL2");
        } else if (testCase.name == "SOL3 Valve Test") {
            result = performValveTest(testCase, "SOL3");
        } else if (testCase.name == "Emergency Stop Test") {
            result = performEmergencyStopTest(testCase);
        } else if (testCase.name == "SPI Communication Test") {
            result = performSPITest(testCase);
        } else if (testCase.name == "GPIO Test") {
            result = performGPIOTest(testCase);
        } else {
            completeTestCase(testCase, TEST_SKIPPED, "Test not implemented");
            return false;
        }
        
        TestResult testResult = result ? TEST_PASSED : TEST_FAILED;
        completeTestCase(testCase, testResult);
        
    } catch (const std::exception& e) {
        completeTestCase(testCase, TEST_FAILED, QString("Exception: %1").arg(e.what()));
        result = false;
    }
    
    return result;
}

void HardwareTester::startTestCase(TestCase& testCase)
{
    testCase.timestamp = QDateTime::currentDateTime();
    testCase.result = TEST_IN_PROGRESS;
    
    m_currentTest = testCase.name;
    m_testStartTime = QDateTime::currentDateTime();
    
    // Start timeout timer
    m_timeoutTimer->start(m_testTimeout);
    
    logTestMessage(QString("Starting test: %1").arg(testCase.name));
    emit testStarted(testCase.name);
}

void HardwareTester::completeTestCase(TestCase& testCase, TestResult result, const QString& details)
{
    m_timeoutTimer->stop();
    
    testCase.result = result;
    testCase.details = details;
    testCase.duration = m_testStartTime.msecsTo(QDateTime::currentDateTime());
    
    // Update test suite statistics
    switch (result) {
    case TEST_PASSED:
        m_currentTestSuite.passed++;
        break;
    case TEST_FAILED:
        m_currentTestSuite.failed++;
        break;
    case TEST_WARNING:
        m_currentTestSuite.warnings++;
        break;
    case TEST_SKIPPED:
        m_currentTestSuite.skipped++;
        break;
    default:
        break;
    }
    
    m_currentTestSuite.totalDuration += testCase.duration;
    m_currentTestSuite.testCases.append(testCase);
    
    logTestResult(testCase);
    emit testCompleted(testCase.name, result);
}

void HardwareTester::updateTestProgress()
{
    if (m_totalTests > 0) {
        m_progress = (m_currentTestIndex * 100) / m_totalTests;
        emit testProgress(m_progress, QString("Running test %1 of %2").arg(m_currentTestIndex + 1).arg(m_totalTests));
    }
}

// Test implementation methods
bool HardwareTester::performAVLSensorTest(TestCase& testCase)
{
    if (!m_sensorInterface) {
        testCase.details = "Sensor interface not available";
        return false;
    }

    QList<double> readings;

    // Take multiple readings
    for (int i = 0; i < DEFAULT_SENSOR_SAMPLES; ++i) {
        double reading = m_sensorInterface->readAVLPressure();
        if (reading < 0) {
            testCase.details = QString("Invalid reading %1 at sample %2").arg(reading).arg(i + 1);
            return false;
        }
        readings.append(reading);
        QThread::msleep(100); // Small delay between readings
    }

    // Calculate statistics using shared utility
    StatisticsUtils::Stats stats = StatisticsUtils::calculate(readings);

    // Store test data
    testCase.data["mean_reading"] = stats.mean;
    testCase.data["std_deviation"] = stats.stdDev;
    testCase.data["coefficient_of_variation"] = stats.coefficientOfVariation;
    testCase.data["sample_count"] = stats.sampleCount;

    // Check if readings are stable (CV < tolerance)
    if (stats.coefficientOfVariation > DEFAULT_SENSOR_TOLERANCE) {
        testCase.details = QString("Sensor readings unstable: CV = %1%").arg(stats.coefficientOfVariation, 0, 'f', 2);
        return false;
    }

    testCase.details = QString("AVL sensor stable: mean = %1 mmHg, CV = %2%")
                      .arg(stats.mean, 0, 'f', 2).arg(stats.coefficientOfVariation, 0, 'f', 2);
    return true;
}

bool HardwareTester::performTankSensorTest(TestCase& testCase)
{
    if (!m_sensorInterface) {
        testCase.details = "Sensor interface not available";
        return false;
    }

    QList<double> readings;

    // Take multiple readings
    for (int i = 0; i < DEFAULT_SENSOR_SAMPLES; ++i) {
        double reading = m_sensorInterface->readTankPressure();
        if (reading < 0) {
            testCase.details = QString("Invalid reading %1 at sample %2").arg(reading).arg(i + 1);
            return false;
        }
        readings.append(reading);
        QThread::msleep(100);
    }

    // Calculate statistics using shared utility
    StatisticsUtils::Stats stats = StatisticsUtils::calculate(readings);

    // Store test data
    testCase.data["mean_reading"] = stats.mean;
    testCase.data["std_deviation"] = stats.stdDev;
    testCase.data["coefficient_of_variation"] = stats.coefficientOfVariation;
    testCase.data["sample_count"] = stats.sampleCount;

    // Check stability
    if (stats.coefficientOfVariation > DEFAULT_SENSOR_TOLERANCE) {
        testCase.details = QString("Sensor readings unstable: CV = %1%").arg(stats.coefficientOfVariation, 0, 'f', 2);
        return false;
    }

    testCase.details = QString("Tank sensor stable: mean = %1 mmHg, CV = %2%")
                      .arg(stats.mean, 0, 'f', 2).arg(stats.coefficientOfVariation, 0, 'f', 2);
    return true;
}

bool HardwareTester::performPumpTest(TestCase& testCase)
{
    if (!m_actuatorControl) {
        testCase.details = "Actuator control not available";
        return false;
    }

    try {
        // Test pump enable/disable
        m_actuatorControl->setPumpEnabled(true);
        QThread::msleep(500);

        if (!m_actuatorControl->isPumpEnabled()) {
            testCase.details = "Failed to enable pump";
            return false;
        }

        // Test speed control
        QList<double> testSpeeds = {25.0, 50.0, 75.0, 100.0};
        QList<double> actualSpeeds;

        for (double targetSpeed : testSpeeds) {
            m_actuatorControl->setPumpSpeed(targetSpeed);
            QThread::msleep(1000); // Allow time for speed change

            double actualSpeed = m_actuatorControl->getPumpSpeed();
            actualSpeeds.append(actualSpeed);

            double error = std::abs(actualSpeed - targetSpeed) / targetSpeed * 100.0;
            if (error > DEFAULT_ACTUATOR_TOLERANCE) {
                testCase.details = QString("Speed control error: target=%1%, actual=%2%, error=%3%")
                                  .arg(targetSpeed).arg(actualSpeed).arg(error, 0, 'f', 1);
                m_actuatorControl->setPumpEnabled(false);
                return false;
            }
        }

        // Stop pump
        m_actuatorControl->setPumpEnabled(false);
        QThread::msleep(500);

        if (m_actuatorControl->isPumpEnabled()) {
            testCase.details = "Failed to disable pump";
            return false;
        }

        // Store test data
        testCase.data["test_speeds"] = QJsonArray::fromVariantList(QVariantList(testSpeeds.begin(), testSpeeds.end()));
        testCase.data["actual_speeds"] = QJsonArray::fromVariantList(QVariantList(actualSpeeds.begin(), actualSpeeds.end()));

        testCase.details = "Pump control test passed - all speeds within tolerance";
        return true;

    } catch (const std::exception& e) {
        testCase.details = QString("Pump test exception: %1").arg(e.what());
        m_actuatorControl->setPumpEnabled(false); // Ensure pump is stopped
        return false;
    }
}

bool HardwareTester::performValveTest(TestCase& testCase, const QString& valveName)
{
    if (!m_actuatorControl) {
        testCase.details = "Actuator control not available";
        return false;
    }

    try {
        bool initialState, finalState;

        // Test valve operation
        if (valveName == "SOL1") {
            // Test open
            m_actuatorControl->setSOL1(true);
            QThread::msleep(500);
            initialState = m_actuatorControl->getSOL1State();

            // Test close
            m_actuatorControl->setSOL1(false);
            QThread::msleep(500);
            finalState = m_actuatorControl->getSOL1State();

        } else if (valveName == "SOL2") {
            m_actuatorControl->setSOL2(true);
            QThread::msleep(500);
            initialState = m_actuatorControl->getSOL2State();

            m_actuatorControl->setSOL2(false);
            QThread::msleep(500);
            finalState = m_actuatorControl->getSOL2State();

        } else if (valveName == "SOL3") {
            m_actuatorControl->setSOL3(true);
            QThread::msleep(500);
            initialState = m_actuatorControl->getSOL3State();

            m_actuatorControl->setSOL3(false);
            QThread::msleep(500);
            finalState = m_actuatorControl->getSOL3State();

        } else {
            testCase.details = "Unknown valve name";
            return false;
        }

        // Check results
        if (!initialState || finalState) {
            testCase.details = QString("Valve %1 operation failed: open=%2, close=%3")
                              .arg(valveName).arg(initialState).arg(!finalState);
            return false;
        }

        testCase.data["valve_name"] = valveName;
        testCase.data["open_test"] = initialState;
        testCase.data["close_test"] = !finalState;

        testCase.details = QString("Valve %1 operation test passed").arg(valveName);
        return true;

    } catch (const std::exception& e) {
        testCase.details = QString("Valve test exception: %1").arg(e.what());
        return false;
    }
}

bool HardwareTester::performEmergencyStopTest(TestCase& testCase)
{
    if (!m_safetyManager) {
        testCase.details = "Safety manager not available";
        return false;
    }

    try {
        // Test emergency stop functionality
        bool initialSafetyCheck = m_safetyManager->performSafetyCheck();

        // Trigger emergency stop
        m_safetyManager->triggerEmergencyStop("Hardware test");
        QThread::msleep(500);

        // Check if emergency stop is active
        auto safetyState = m_safetyManager->getSafetyState();
        if (safetyState != SafetyManager::EMERGENCY_STOP) {
            testCase.details = "Emergency stop not activated";
            return false;
        }

        // Test reset (if available)
        // Note: In a real system, this might require manual intervention

        testCase.data["initial_safety_check"] = initialSafetyCheck;
        testCase.data["emergency_stop_activated"] = (safetyState == SafetyManager::EMERGENCY_STOP);

        testCase.details = "Emergency stop test passed";
        return true;

    } catch (const std::exception& e) {
        testCase.details = QString("Emergency stop test exception: %1").arg(e.what());
        return false;
    }
}

bool HardwareTester::performSPITest(TestCase& testCase)
{
    if (!m_sensorInterface) {
        testCase.details = "Sensor interface not available";
        return false;
    }

    try {
        // Test SPI communication by reading from both channels
        double avlReading = m_sensorInterface->readAVLPressure();
        double tankReading = m_sensorInterface->readTankPressure();

        // Check if readings are valid (not error values)
        if (avlReading < 0 || tankReading < 0) {
            testCase.details = QString("SPI communication failed: AVL=%1, Tank=%2")
                              .arg(avlReading).arg(tankReading);
            return false;
        }

        testCase.data["avl_reading"] = avlReading;
        testCase.data["tank_reading"] = tankReading;

        testCase.details = QString("SPI communication test passed: AVL=%1, Tank=%2")
                          .arg(avlReading, 0, 'f', 2).arg(tankReading, 0, 'f', 2);
        return true;

    } catch (const std::exception& e) {
        testCase.details = QString("SPI test exception: %1").arg(e.what());
        return false;
    }
}

bool HardwareTester::performGPIOTest(TestCase& testCase)
{
    if (!m_actuatorControl) {
        testCase.details = "Actuator control not available";
        return false;
    }

    try {
        // Test GPIO by cycling valve states
        int successfulOperations = 0;
        int totalOperations = 6; // 3 valves × 2 operations each

        // Test SOL1
        m_actuatorControl->setSOL1(true);
        QThread::msleep(100);
        if (m_actuatorControl->getSOL1State()) successfulOperations++;

        m_actuatorControl->setSOL1(false);
        QThread::msleep(100);
        if (!m_actuatorControl->getSOL1State()) successfulOperations++;

        // Test SOL2
        m_actuatorControl->setSOL2(true);
        QThread::msleep(100);
        if (m_actuatorControl->getSOL2State()) successfulOperations++;

        m_actuatorControl->setSOL2(false);
        QThread::msleep(100);
        if (!m_actuatorControl->getSOL2State()) successfulOperations++;

        // Test SOL3
        m_actuatorControl->setSOL3(true);
        QThread::msleep(100);
        if (m_actuatorControl->getSOL3State()) successfulOperations++;

        m_actuatorControl->setSOL3(false);
        QThread::msleep(100);
        if (!m_actuatorControl->getSOL3State()) successfulOperations++;

        testCase.data["successful_operations"] = successfulOperations;
        testCase.data["total_operations"] = totalOperations;
        testCase.data["success_rate"] = (double)successfulOperations / totalOperations * 100.0;

        if (successfulOperations == totalOperations) {
            testCase.details = "GPIO test passed - all operations successful";
            return true;
        } else {
            testCase.details = QString("GPIO test failed: %1/%2 operations successful")
                              .arg(successfulOperations).arg(totalOperations);
            return false;
        }

    } catch (const std::exception& e) {
        testCase.details = QString("GPIO test exception: %1").arg(e.what());
        return false;
    }
}

// Utility methods
void HardwareTester::cancelTests()
{
    if (!m_testRunning) return;

    m_testRunning = false;
    m_timeoutTimer->stop();

    logTestMessage("Tests cancelled by user");
}

void HardwareTester::onTestTimeout()
{
    if (m_testRunning) {
        logTestMessage(QString("Test timeout: %1").arg(m_currentTest));

        // Find current test case and mark as failed
        for (TestCase& testCase : m_testCases) {
            if (testCase.name == m_currentTest && testCase.result == TEST_IN_PROGRESS) {
                completeTestCase(testCase, TEST_FAILED, "Test timeout");
                break;
            }
        }
    }
}

QList<HardwareTester::TestCase> HardwareTester::getFailedTests() const
{
    QList<TestCase> failedTests;

    for (const TestCase& testCase : m_lastTestSuite.testCases) {
        if (testCase.result == TEST_FAILED) {
            failedTests.append(testCase);
        }
    }

    return failedTests;
}

QJsonObject HardwareTester::getTestReport() const
{
    return testSuiteToJson(m_lastTestSuite);
}

bool HardwareTester::saveTestReport(const QString& filename) const
{
    QJsonObject report = getTestReport();
    QJsonDocument doc(report);

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }

    return false;
}

void HardwareTester::logTestMessage(const QString& message)
{
    QString timestampedMessage = QString("[%1] %2")
                                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                                .arg(message);

    if (m_verboseOutput) {
        qDebug() << timestampedMessage;
    }

    if (m_outputStream) {
        *m_outputStream << timestampedMessage << Qt::endl;
        m_outputStream->flush();
    }

    emit testMessage(timestampedMessage);
}

void HardwareTester::logTestResult(const TestCase& testCase)
{
    QString result = formatTestResult(testCase.result);
    QString duration = formatTestDuration(testCase.duration);

    QString message = QString("Test: %1 - %2 (%3)")
                     .arg(testCase.name)
                     .arg(result)
                     .arg(duration);

    if (!testCase.details.isEmpty()) {
        message += QString(" - %1").arg(testCase.details);
    }

    logTestMessage(message);
}

QString HardwareTester::formatTestDuration(double durationMs) const
{
    if (durationMs < 1000) {
        return QString("%1ms").arg(durationMs, 0, 'f', 0);
    } else {
        return QString("%1s").arg(durationMs / 1000.0, 0, 'f', 2);
    }
}

QString HardwareTester::formatTestResult(TestResult result) const
{
    switch (result) {
    case TEST_PASSED: return "PASSED";
    case TEST_FAILED: return "FAILED";
    case TEST_WARNING: return "WARNING";
    case TEST_SKIPPED: return "SKIPPED";
    case TEST_IN_PROGRESS: return "IN PROGRESS";
    default: return "UNKNOWN";
    }
}

QJsonObject HardwareTester::testCaseToJson(const TestCase& testCase) const
{
    QJsonObject obj;
    obj["name"] = testCase.name;
    obj["description"] = testCase.description;
    obj["type"] = static_cast<int>(testCase.type);
    obj["result"] = static_cast<int>(testCase.result);
    obj["details"] = testCase.details;
    obj["timestamp"] = testCase.timestamp.toString(Qt::ISODate);
    obj["duration_ms"] = testCase.duration;
    obj["data"] = testCase.data;

    return obj;
}

QJsonObject HardwareTester::testSuiteToJson(const TestSuite& suite) const
{
    QJsonObject obj;
    obj["name"] = suite.name;
    obj["description"] = suite.description;
    obj["start_time"] = suite.startTime.toString(Qt::ISODate);
    obj["end_time"] = suite.endTime.toString(Qt::ISODate);
    obj["total_duration_ms"] = suite.totalDuration;
    obj["passed"] = suite.passed;
    obj["failed"] = suite.failed;
    obj["warnings"] = suite.warnings;
    obj["skipped"] = suite.skipped;

    QJsonArray testCases;
    for (const TestCase& testCase : suite.testCases) {
        testCases.append(testCaseToJson(testCase));
    }
    obj["test_cases"] = testCases;

    return obj;
}

// Individual test methods for command line interface
bool HardwareTester::testAVLSensor()
{
    TestCase testCase;
    testCase.name = "AVL Sensor Test";
    testCase.description = "Test AVL pressure sensor";
    testCase.type = SENSOR_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testTankSensor()
{
    TestCase testCase;
    testCase.name = "Tank Sensor Test";
    testCase.description = "Test tank pressure sensor";
    testCase.type = SENSOR_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testPumpControl()
{
    TestCase testCase;
    testCase.name = "Pump Control Test";
    testCase.description = "Test pump control";
    testCase.type = ACTUATOR_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testSOL1Valve()
{
    TestCase testCase;
    testCase.name = "SOL1 Valve Test";
    testCase.description = "Test SOL1 valve";
    testCase.type = ACTUATOR_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testSOL2Valve()
{
    TestCase testCase;
    testCase.name = "SOL2 Valve Test";
    testCase.description = "Test SOL2 valve";
    testCase.type = ACTUATOR_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testSOL3Valve()
{
    TestCase testCase;
    testCase.name = "SOL3 Valve Test";
    testCase.description = "Test SOL3 valve";
    testCase.type = ACTUATOR_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testEmergencyStop()
{
    TestCase testCase;
    testCase.name = "Emergency Stop Test";
    testCase.description = "Test emergency stop";
    testCase.type = SAFETY_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testSPICommunication()
{
    TestCase testCase;
    testCase.name = "SPI Communication Test";
    testCase.description = "Test SPI communication";
    testCase.type = COMMUNICATION_TEST;

    return executeTestCase(testCase);
}

bool HardwareTester::testGPIOPins()
{
    TestCase testCase;
    testCase.name = "GPIO Test";
    testCase.description = "Test GPIO pins";
    testCase.type = COMMUNICATION_TEST;

    return executeTestCase(testCase);
}

// Placeholder implementations for other test types
bool HardwareTester::runSafetyTests()
{
    // Implementation similar to runSensorTests but for safety tests
    return true;
}

bool HardwareTester::runCommunicationTests()
{
    // Implementation similar to runSensorTests but for communication tests
    return true;
}

bool HardwareTester::runPerformanceTests()
{
    // Implementation for performance tests
    return true;
}
