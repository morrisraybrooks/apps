#ifndef HARDWARETESTER_H
#define HARDWARETESTER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTextStream>

// Forward declarations
class HardwareManager;
class SensorInterface;
class ActuatorControl;
class SafetyManager;

/**
 * @brief Comprehensive hardware testing system for vacuum controller
 * 
 * This class provides automated testing capabilities for all hardware components
 * including sensors, actuators, and safety systems. It supports both command-line
 * and programmatic testing interfaces.
 */
class HardwareTester : public QObject
{
    Q_OBJECT

public:
    enum TestType {
        SENSOR_TEST,
        ACTUATOR_TEST,
        SAFETY_TEST,
        COMMUNICATION_TEST,
        PERFORMANCE_TEST,
        COMPREHENSIVE_TEST
    };

    enum TestResult {
        TEST_PASSED,
        TEST_FAILED,
        TEST_WARNING,
        TEST_SKIPPED,
        TEST_IN_PROGRESS
    };

    struct TestCase {
        QString name;
        QString description;
        TestType type;
        TestResult result;
        QString details;
        QDateTime timestamp;
        double duration;
        QJsonObject data;
        
        TestCase() : type(SENSOR_TEST), result(TEST_SKIPPED), duration(0.0) {}
    };

    struct TestSuite {
        QString name;
        QString description;
        QList<TestCase> testCases;
        int passed;
        int failed;
        int warnings;
        int skipped;
        double totalDuration;
        QDateTime startTime;
        QDateTime endTime;
        
        TestSuite() : passed(0), failed(0), warnings(0), skipped(0), totalDuration(0.0) {}
    };

    explicit HardwareTester(HardwareManager* hardware, SafetyManager* safety, QObject *parent = nullptr);
    ~HardwareTester();

    // Test execution
    bool runSensorTests();
    bool runActuatorTests();
    bool runSafetyTests();
    bool runCommunicationTests();
    bool runPerformanceTests();
    bool runComprehensiveTests();
    
    // Individual component tests
    bool testAVLSensor();
    bool testTankSensor();
    bool testPumpControl();
    bool testSOL1Valve();
    bool testSOL2Valve();
    bool testSOL3Valve();
    bool testEmergencyStop();
    bool testSPICommunication();
    bool testGPIOPins();
    
    // Test configuration
    void setTestTimeout(int timeoutMs) { m_testTimeout = timeoutMs; }
    void setVerboseOutput(bool verbose) { m_verboseOutput = verbose; }
    void setOutputFile(const QString& filename) { m_outputFile = filename; }
    void setContinueOnFailure(bool continueOnFailure) { m_continueOnFailure = continueOnFailure; }
    
    // Test results
    TestSuite getLastTestSuite() const { return m_lastTestSuite; }
    QList<TestCase> getFailedTests() const;
    QJsonObject getTestReport() const;
    bool saveTestReport(const QString& filename) const;
    
    // Test status
    bool isTestRunning() const { return m_testRunning; }
    QString getCurrentTest() const { return m_currentTest; }
    int getProgress() const { return m_progress; }

public Q_SLOTS:
    void cancelTests();

Q_SIGNALS:
    void testStarted(const QString& testName);
    void testCompleted(const QString& testName, TestResult result);
    void testProgress(int percentage, const QString& status);
    void testSuiteCompleted(const TestSuite& suite);
    void testMessage(const QString& message);

private Q_SLOTS:
    void onTestTimeout();

private:
    void initializeHardwareTester();
    void setupTestCases();
    
    // Test execution helpers
    bool executeTestCase(TestCase& testCase);
    void startTestCase(TestCase& testCase);
    void completeTestCase(TestCase& testCase, TestResult result, const QString& details = QString());
    void updateTestProgress();
    
    // Sensor test implementations
    bool performAVLSensorTest(TestCase& testCase);
    bool performTankSensorTest(TestCase& testCase);
    bool performSensorAccuracyTest(TestCase& testCase);
    bool performSensorStabilityTest(TestCase& testCase);
    
    // Actuator test implementations
    bool performPumpTest(TestCase& testCase);
    bool performValveTest(TestCase& testCase, const QString& valveName);
    bool performActuatorResponseTest(TestCase& testCase);
    bool performActuatorEnduranceTest(TestCase& testCase);
    
    // Safety test implementations
    bool performEmergencyStopTest(TestCase& testCase);
    bool performOverpressureTest(TestCase& testCase);
    bool performSensorFailureTest(TestCase& testCase);
    bool performSafetySystemTest(TestCase& testCase);
    
    // Communication test implementations
    bool performSPITest(TestCase& testCase);
    bool performGPIOTest(TestCase& testCase);
    bool performI2CTest(TestCase& testCase);
    bool performCommunicationLatencyTest(TestCase& testCase);
    
    // Performance test implementations
    bool performThroughputTest(TestCase& testCase);
    bool performLatencyTest(TestCase& testCase);
    bool performMemoryUsageTest(TestCase& testCase);
    bool performCPUUsageTest(TestCase& testCase);
    
    // Utility methods
    void logTestMessage(const QString& message);
    void logTestResult(const TestCase& testCase);
    QString formatTestDuration(double durationMs) const;
    QString formatTestResult(TestResult result) const;
    QJsonObject testCaseToJson(const TestCase& testCase) const;
    QJsonObject testSuiteToJson(const TestSuite& suite) const;
    
    // Hardware interfaces
    HardwareManager* m_hardware;
    SensorInterface* m_sensorInterface;
    ActuatorControl* m_actuatorControl;
    SafetyManager* m_safetyManager;
    
    // Test state
    bool m_testRunning;
    QString m_currentTest;
    int m_progress;
    int m_currentTestIndex;
    int m_totalTests;
    QDateTime m_testStartTime;
    
    // Test configuration
    int m_testTimeout;
    bool m_verboseOutput;
    QString m_outputFile;
    bool m_continueOnFailure;
    
    // Test data
    TestSuite m_currentTestSuite;
    TestSuite m_lastTestSuite;
    QList<TestCase> m_testCases;
    
    // Timing
    QTimer* m_testTimer;
    QTimer* m_timeoutTimer;
    
    // Output
    QTextStream* m_outputStream;
    
    // Constants
    static const int DEFAULT_TEST_TIMEOUT;
    static const int DEFAULT_SENSOR_SAMPLES;
    static const int DEFAULT_ACTUATOR_CYCLES;
    static const double DEFAULT_SENSOR_TOLERANCE;
    static const double DEFAULT_ACTUATOR_TOLERANCE;
};

#endif // HARDWARETESTER_H
