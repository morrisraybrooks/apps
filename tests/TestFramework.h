#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H

#include <QObject>
#include <QTest>
#include <QTimer>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QList>

// Forward declarations
class TestSuite;

/**
 * @brief Test result enumeration
 */
enum TestResult {
    TEST_PASSED,
    TEST_FAILED,
    TEST_SKIPPED
};

/**
 * @brief Export format enumeration
 */
enum ExportFormat {
    XML_FORMAT,
    JSON_FORMAT,
    CSV_FORMAT
};

/**
 * @brief Test record structure
 */
struct TestRecord {
    QString suiteName;
    QString testName;
    TestResult result;
    qint64 duration;
    QString errorMessage;
    QDateTime timestamp;
};

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
    explicit TestFramework(QObject *parent = nullptr);
    ~TestFramework();

    // Test suite management
    void addTestSuite(TestSuite* suite);

    // Test execution
    bool runAllTests();
    bool runTestSuite(TestSuite* suite);
    bool runSingleTest(TestSuite* suite, const QString& testName);

    // Configuration
    void setVerbose(bool verbose) { m_verbose = verbose; }
    void setStopOnFailure(bool stop) { m_stopOnFailure = stop; }
    void setTimeout(int timeoutMs) { m_timeoutMs = timeoutMs; }

    // Results
    int testCount() const { return m_testCount; }
    int passedCount() const { return m_passedCount; }
    int failedCount() const { return m_failedCount; }
    int skippedCount() const { return m_skippedCount; }
    QList<TestRecord> testResults() const { return m_testResults; }

    // Test suite access
    const QList<TestSuite*>& testSuites() const { return m_testSuites; }
    int testSuiteCount() const { return m_testSuites.size(); }

    // Export
    bool exportResults(const QString& filePath, ExportFormat format);

private:
    void printSummary(qint64 totalTime);
    void cleanup();
    bool exportToXML(QTextStream& stream);
    bool exportToJSON(QTextStream& stream);
    bool exportToCSV(QTextStream& stream);
    QString testResultToString(TestResult result);

    // Test suites
    QList<TestSuite*> m_testSuites;
    TestSuite* m_currentSuite;

    // Counters
    int m_testCount;
    int m_passedCount;
    int m_failedCount;
    int m_skippedCount;

    // Configuration
    bool m_verbose;
    bool m_stopOnFailure;
    int m_timeoutMs;

    // Results
    QList<TestRecord> m_testResults;
};

/**
 * @brief Base class for test suites
 */
class TestSuite : public QObject
{
    Q_OBJECT

public:
    explicit TestSuite(const QString& name, QObject* parent = nullptr);
    virtual ~TestSuite();

    QString name() const { return m_name; }

    // Override these in derived classes
    virtual bool setup();
    virtual void cleanup();
    virtual bool setupTest(const QString& testName);
    virtual void cleanupTest(const QString& testName);
    virtual QStringList testNames() const = 0;
    virtual TestResult runTest(const QString& testName) = 0;

    QString lastError() const;
    void setLastError(const QString& error);

protected:
    QString m_name;
    QString m_lastError;
};

#endif // TESTFRAMEWORK_H
