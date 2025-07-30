#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

/**
 * @brief Automated test execution and reporting system
 * 
 * This system provides:
 * - Command-line test execution
 * - Automated test suite running
 * - Test result reporting (XML, JSON, HTML)
 * - Continuous integration support
 * - Test filtering and selection
 * - Parallel test execution
 * - Test result comparison
 * - Performance benchmarking
 */
class TestRunner : public QObject
{
    Q_OBJECT

public:
    enum OutputFormat {
        CONSOLE,        // Console output
        XML_JUNIT,      // JUnit XML format
        JSON_FORMAT,    // JSON format
        HTML_REPORT,    // HTML report
        CSV_FORMAT      // CSV format
    };

    struct TestRunConfiguration {
        QStringList testSuites;
        QStringList testCases;
        QStringList excludedTests;
        QString outputPath;
        OutputFormat outputFormat;
        bool verboseOutput;
        bool stopOnFirstFailure;
        bool parallelExecution;
        int maxParallelTests;
        int testTimeout;
        bool generateReport;
        QString reportTemplate;
        
        TestRunConfiguration() : outputFormat(CONSOLE), verboseOutput(false),
                               stopOnFirstFailure(false), parallelExecution(false),
                               maxParallelTests(4), testTimeout(30000), generateReport(true) {}
    };

    struct TestRunResult {
        QString suiteName;
        QString testName;
        QString result;
        QString message;
        qint64 executionTime;
        QDateTime timestamp;
        QJsonObject details;
        
        TestRunResult() : executionTime(0), timestamp(QDateTime::currentDateTime()) {}
    };

    explicit TestRunner(QObject *parent = nullptr);
    ~TestRunner();

    // Test execution
    int runTests(const TestRunConfiguration& config);
    int runAllTests();
    int runTestSuite(const QString& suiteName);
    int runSpecificTests(const QStringList& testNames);
    
    // Configuration
    void loadConfiguration(const QString& configPath);
    void saveConfiguration(const QString& configPath, const TestRunConfiguration& config);
    TestRunConfiguration getDefaultConfiguration();
    
    // Command line interface
    void setupCommandLineParser();
    TestRunConfiguration parseCommandLine(const QStringList& arguments);
    void printUsage();
    
    // Report generation
    bool generateXMLReport(const QString& outputPath, const QList<TestRunResult>& results);
    bool generateJSONReport(const QString& outputPath, const QList<TestRunResult>& results);
    bool generateHTMLReport(const QString& outputPath, const QList<TestRunResult>& results);
    bool generateCSVReport(const QString& outputPath, const QList<TestRunResult>& results);
    
    // Test discovery
    QStringList discoverTestSuites();
    QStringList discoverTestCases(const QString& suiteName);
    QJsonObject getTestMetadata(const QString& testName);
    
    // Continuous integration support
    void setCIMode(bool enabled);
    bool isCIMode() const { return m_ciMode; }
    int getExitCode() const { return m_exitCode; }
    
    // Performance tracking
    void enablePerformanceTracking(bool enabled);
    QJsonObject getPerformanceMetrics();
    void compareWithBaseline(const QString& baselinePath);

public slots:
    void abortTestRun();

signals:
    void testRunStarted(int totalTests);
    void testRunCompleted(int passed, int failed, int skipped);
    void testSuiteStarted(const QString& suiteName);
    void testSuiteCompleted(const QString& suiteName, int passed, int failed);
    void testStarted(const QString& testName);
    void testCompleted(const QString& testName, const QString& result);
    void testProgress(int currentTest, int totalTests);

private slots:
    void onTestTimeout();

private:
    void initializeTestRunner();
    void setupTestEnvironment();
    void cleanupTestEnvironment();
    
    // Test execution helpers
    QList<TestRunResult> executeTestSuite(const QString& suiteName, const TestRunConfiguration& config);
    TestRunResult executeTest(const QString& testName, const TestRunConfiguration& config);
    bool shouldRunTest(const QString& testName, const TestRunConfiguration& config);
    
    // Report generation helpers
    QString generateXMLTestCase(const TestRunResult& result);
    QString generateHTMLTestRow(const TestRunResult& result);
    QJsonObject resultToJson(const TestRunResult& result);
    QString formatDuration(qint64 milliseconds);
    QString formatTimestamp(const QDateTime& timestamp);
    
    // Test discovery helpers
    void scanForTestSuites();
    void loadTestMetadata();
    
    // Performance tracking
    void recordPerformanceMetric(const QString& metric, double value);
    void updatePerformanceBaseline();
    
    // CI integration
    void setupCIEnvironment();
    void reportCIResults();
    void setCIExitCode(int code);
    
    // Configuration and state
    TestRunConfiguration m_currentConfig;
    QList<TestRunResult> m_testResults;
    QMap<QString, QStringList> m_testSuites;
    QMap<QString, QJsonObject> m_testMetadata;
    
    // Execution state
    bool m_testRunning;
    bool m_abortRequested;
    int m_currentTestIndex;
    int m_totalTests;
    QTimer* m_timeoutTimer;
    
    // Command line parser
    QCommandLineParser m_parser;
    
    // CI mode
    bool m_ciMode;
    int m_exitCode;
    
    // Performance tracking
    bool m_performanceTrackingEnabled;
    QJsonObject m_performanceMetrics;
    QString m_baselinePath;
    
    // Output and logging
    QTextStream m_outputStream;
    QString m_logFilePath;
    bool m_verboseOutput;
    
    // Constants
    static const QString DEFAULT_CONFIG_PATH;
    static const QString DEFAULT_OUTPUT_PATH;
    static const QString DEFAULT_BASELINE_PATH;
    static const int DEFAULT_TEST_TIMEOUT = 30000;
    static const int DEFAULT_MAX_PARALLEL_TESTS = 4;
    
    // HTML report template
    static const QString HTML_REPORT_TEMPLATE;
    static const QString HTML_TEST_ROW_TEMPLATE;
    
    // XML report templates
    static const QString XML_TESTSUITE_TEMPLATE;
    static const QString XML_TESTCASE_TEMPLATE;
};

// Main function for standalone test runner
int runTestRunner(int argc, char *argv[]);

#endif // TESTRUNNER_H
