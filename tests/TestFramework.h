#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H

#include <QObject>
#include <QTest>
#include <QTimer>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <memory>

// Forward declarations
class VacuumController;
class HardwareManager;
class SafetyManager;
class PatternEngine;

/**
 * @brief Comprehensive testing framework for vacuum controller
 * 
 * This framework provides:
 * - Unit tests for all components
 * - Integration tests for system interactions
 * - Safety system validation tests
 * - Performance and stress tests
 * - Hardware simulation for testing
 * - Automated test execution
 * - Test result reporting
 * - Continuous integration support
 */
class TestFramework : public QObject
{
    Q_OBJECT

public:
    enum TestType {
        UNIT_TEST,          // Individual component tests
        INTEGRATION_TEST,   // Component interaction tests
        SAFETY_TEST,        // Safety system tests
        PERFORMANCE_TEST,   // Performance and stress tests
        HARDWARE_TEST,      // Hardware interface tests
        GUI_TEST,           // GUI functionality tests
        PATTERN_TEST,       // Pattern execution tests
        REGRESSION_TEST     // Regression tests
    };

    enum TestResult {
        PASSED,
        FAILED,
        SKIPPED,
        ERROR
    };

    struct TestCase {
        QString name;
        QString description;
        TestType type;
        QString category;
        int timeoutMs;
        bool enabled;
        QJsonObject parameters;
        
        TestCase() : type(UNIT_TEST), timeoutMs(30000), enabled(true) {}
        TestCase(const QString& n, const QString& desc, TestType t, const QString& cat = QString())
            : name(n), description(desc), type(t), category(cat), timeoutMs(30000), enabled(true) {}
    };

    struct TestResult_s {
        QString testName;
        TestResult result;
        QString message;
        qint64 executionTimeMs;
        QJsonObject details;
        QDateTime timestamp;
        
        TestResult_s() : result(FAILED), executionTimeMs(0), timestamp(QDateTime::currentDateTime()) {}
    };

    explicit TestFramework(QObject *parent = nullptr);
    ~TestFramework();

    // Test execution
    bool runAllTests();
    bool runTestsByType(TestType type);
    bool runTestsByCategory(const QString& category);
    bool runSingleTest(const QString& testName);
    bool runTestSuite(const QStringList& testNames);
    
    // Test management
    void addTestCase(const TestCase& testCase);
    void removeTestCase(const QString& testName);
    void enableTestCase(const QString& testName, bool enabled);
    QList<TestCase> getTestCases() const;
    QList<TestCase> getTestCasesByType(TestType type) const;
    
    // Test results
    QList<TestResult_s> getTestResults() const;
    QList<TestResult_s> getFailedTests() const;
    TestResult_s getTestResult(const QString& testName) const;
    QJsonObject getTestSummary() const;
    
    // Hardware simulation
    void enableHardwareSimulation(bool enabled);
    void setSimulatedSensorValues(double avlPressure, double tankPressure);
    void simulateHardwareFailure(const QString& component);
    void simulateSensorError(const QString& sensor);
    void resetHardwareSimulation();
    
    // Test configuration
    void setTestTimeout(int timeoutMs);
    void setVerboseOutput(bool verbose);
    void setStopOnFirstFailure(bool stopOnFailure);
    void loadTestConfiguration(const QString& configPath);
    void saveTestConfiguration(const QString& configPath);

public slots:
    void abortTests();
    void pauseTests();
    void resumeTests();

signals:
    void testStarted(const QString& testName);
    void testCompleted(const QString& testName, TestResult result);
    void testSuiteStarted(int totalTests);
    void testSuiteCompleted(int passed, int failed, int skipped);
    void testProgress(int currentTest, int totalTests);

private slots:
    void onTestTimeout();

private:
    void initializeTestFramework();
    void setupTestCases();
    void setupHardwareSimulation();
    
    // Test execution helpers
    TestResult_s executeTest(const TestCase& testCase);
    bool setupTestEnvironment(const TestCase& testCase);
    void cleanupTestEnvironment();
    void recordTestResult(const TestResult_s& result);
    
    // Unit tests
    TestResult_s testHardwareManager();
    TestResult_s testSensorInterface();
    TestResult_s testActuatorControl();
    TestResult_s testSafetyManager();
    TestResult_s testAntiDetachmentMonitor();
    TestResult_s testEmergencyStop();
    TestResult_s testPatternEngine();
    TestResult_s testPatternDefinitions();
    TestResult_s testVacuumController();
    
    // Integration tests
    TestResult_s testSystemInitialization();
    TestResult_s testSensorToSafetyIntegration();
    TestResult_s testPatternToHardwareIntegration();
    TestResult_s testGUIToControllerIntegration();
    TestResult_s testThreadCommunication();
    
    // Safety tests
    TestResult_s testOverpressureProtection();
    TestResult_s testAntiDetachmentResponse();
    TestResult_s testEmergencyStopResponse();
    TestResult_s testSafetySystemFailover();
    TestResult_s testSensorFailureHandling();
    TestResult_s testHardwareFailureResponse();
    
    // Performance tests
    TestResult_s testDataAcquisitionPerformance();
    TestResult_s testGUIResponsiveness();
    TestResult_s testMemoryUsage();
    TestResult_s testThreadPerformance();
    TestResult_s testPatternExecutionTiming();
    
    // Hardware tests
    TestResult_s testGPIOControl();
    TestResult_s testSPICommunication();
    TestResult_s testSensorReadings();
    TestResult_s testActuatorResponse();
    TestResult_s testHardwareInitialization();
    
    // GUI tests
    TestResult_s testMainWindowCreation();
    TestResult_s testPressureMonitorDisplay();
    TestResult_s testPatternSelectorFunctionality();
    TestResult_s testSafetyPanelAlerts();
    TestResult_s testSettingsDialog();
    
    // Pattern tests
    TestResult_s testPatternValidation();
    TestResult_s testPatternExecution();
    TestResult_s testPatternParameterAdjustment();
    TestResult_s testCustomPatternCreation();
    TestResult_s testPatternSafety();
    
    // Test utilities
    bool waitForSignal(QObject* sender, const char* signal, int timeoutMs = 5000);
    bool compareDoubles(double a, double b, double tolerance = 0.001);
    void logTestMessage(const QString& message);
    QString formatTestResult(const TestResult_s& result);
    
    // Mock objects and simulation
    std::unique_ptr<VacuumController> m_mockController;
    std::unique_ptr<HardwareManager> m_mockHardware;
    bool m_hardwareSimulationEnabled;
    QJsonObject m_simulatedSensorValues;
    QStringList m_simulatedFailures;
    
    // Test management
    QList<TestCase> m_testCases;
    QList<TestResult_s> m_testResults;
    QMap<QString, TestCase> m_testCaseMap;
    
    // Execution state
    bool m_testsRunning;
    bool m_testsPaused;
    bool m_stopOnFirstFailure;
    bool m_verboseOutput;
    int m_currentTestIndex;
    int m_testTimeout;
    QTimer* m_timeoutTimer;
    QElapsedTimer m_executionTimer;
    
    // Test environment
    QString m_testDataPath;
    QString m_testConfigPath;
    QString m_testOutputPath;
    
    // Constants
    static const int DEFAULT_TEST_TIMEOUT = 30000;     // 30 seconds
    static const QString TEST_DATA_PATH;
    static const QString TEST_CONFIG_PATH;
    static const QString TEST_OUTPUT_PATH;
};

// Test macros for convenience
#define VACUUM_TEST_VERIFY(condition) \
    do { \
        if (!(condition)) { \
            return TestResult_s{testName, TestFramework::FAILED, \
                               QString("Verification failed: %1").arg(#condition), \
                               timer.elapsed()}; \
        } \
    } while (0)

#define VACUUM_TEST_COMPARE(actual, expected) \
    do { \
        if (!compareDoubles(actual, expected)) { \
            return TestResult_s{testName, TestFramework::FAILED, \
                               QString("Comparison failed: %1 != %2").arg(actual).arg(expected), \
                               timer.elapsed()}; \
        } \
    } while (0)

#define VACUUM_TEST_TIMEOUT(timeoutMs) \
    do { \
        QTimer::singleShot(timeoutMs, this, [&]() { \
            testTimedOut = true; \
        }); \
    } while (0)

#endif // TESTFRAMEWORK_H
