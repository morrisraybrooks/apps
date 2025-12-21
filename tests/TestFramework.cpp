#include "TestFramework.h"
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>
#include <QElapsedTimer>

TestFramework::TestFramework(QObject *parent)
    : QObject(parent)
    , m_testCount(0)
    , m_passedCount(0)
    , m_failedCount(0)
    , m_skippedCount(0)
    , m_currentSuite(nullptr)
    , m_verbose(false)
    , m_stopOnFailure(false)
    , m_timeoutMs(30000) // 30 seconds default timeout
{
    qDebug() << "Test Framework initialized";
}

TestFramework::~TestFramework()
{
    cleanup();
}

void TestFramework::addTestSuite(TestSuite* suite)
{
    if (!suite) return;
    
    m_testSuites.append(suite);
    suite->setParent(this);
    
    qDebug() << "Added test suite:" << suite->name();
}

bool TestFramework::runAllTests()
{
    qDebug() << "=== Starting Test Execution ===";
    
    m_testCount = 0;
    m_passedCount = 0;
    m_failedCount = 0;
    m_skippedCount = 0;
    
    QElapsedTimer totalTimer;
    totalTimer.start();
    
    bool allPassed = true;
    
    for (TestSuite* suite : m_testSuites) {
        if (!runTestSuite(suite)) {
            allPassed = false;
            if (m_stopOnFailure) {
                break;
            }
        }
    }
    
    qint64 totalTime = totalTimer.elapsed();
    
    // Print summary
    printSummary(totalTime);
    
    return allPassed;
}

bool TestFramework::runTestSuite(TestSuite* suite)
{
    if (!suite) return false;
    
    m_currentSuite = suite;
    
    qDebug() << QString("\n--- Running Test Suite: %1 ---").arg(suite->name());
    
    QElapsedTimer suiteTimer;
    suiteTimer.start();
    
    bool suiteResult = true;
    
    // Setup suite
    if (!suite->setup()) {
        qWarning() << "Suite setup failed:" << suite->name();
        return false;
    }
    
    // Run all tests in the suite
    const QStringList testNames = suite->testNames();
    for (const QString& testName : testNames) {
        if (!runSingleTest(suite, testName)) {
            suiteResult = false;
            if (m_stopOnFailure) {
                break;
            }
        }
    }
    
    // Cleanup suite
    suite->cleanup();
    
    qint64 suiteTime = suiteTimer.elapsed();
    qDebug() << QString("Suite '%1' completed in %2ms").arg(suite->name()).arg(suiteTime);
    
    m_currentSuite = nullptr;
    return suiteResult;
}

bool TestFramework::runSingleTest(TestSuite* suite, const QString& testName)
{
    if (!suite) return false;
    
    m_testCount++;
    
    if (m_verbose) {
        qDebug() << QString("  Running: %1").arg(testName);
    }
    
    QElapsedTimer testTimer;
    testTimer.start();
    
    TestResult result = TEST_FAILED;
    QString errorMessage;
    
    try {
        // Setup test
        if (!suite->setupTest(testName)) {
            result = TEST_SKIPPED;
            errorMessage = "Test setup failed";
        } else {
            // Run the test with timeout
            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            timeoutTimer.start(m_timeoutMs);
            
            bool testCompleted = false;
            connect(&timeoutTimer, &QTimer::timeout, [&]() {
                if (!testCompleted) {
                    result = TEST_FAILED;
                    errorMessage = "Test timeout";
                }
            });
            
            // Execute the actual test
            result = suite->runTest(testName);
            testCompleted = true;
            timeoutTimer.stop();
            
            if (result == TEST_FAILED) {
                errorMessage = suite->lastError();
            }
        }
        
        // Cleanup test
        suite->cleanupTest(testName);
        
    } catch (const std::exception& e) {
        result = TEST_FAILED;
        errorMessage = QString("Exception: %1").arg(e.what());
    } catch (...) {
        result = TEST_FAILED;
        errorMessage = "Unknown exception";
    }
    
    qint64 testTime = testTimer.elapsed();
    
    // Update counters
    switch (result) {
    case TEST_PASSED:
        m_passedCount++;
        if (m_verbose) {
            qDebug() << QString("    PASS: %1 (%2ms)").arg(testName).arg(testTime);
        }
        break;
    case TEST_FAILED:
        m_failedCount++;
        qDebug() << QString("    FAIL: %1 (%2ms) - %3").arg(testName).arg(testTime).arg(errorMessage);
        break;
    case TEST_SKIPPED:
        m_skippedCount++;
        if (m_verbose) {
            qDebug() << QString("    SKIP: %1 - %2").arg(testName).arg(errorMessage);
        }
        break;
    }
    
    // Record test result
    TestRecord record;
    record.suiteName = suite->name();
    record.testName = testName;
    record.result = result;
    record.duration = testTime;
    record.errorMessage = errorMessage;
    record.timestamp = QDateTime::currentDateTime();
    
    m_testResults.append(record);
    
    return (result == TEST_PASSED);
}

void TestFramework::printSummary(qint64 totalTime)
{
    qDebug() << "\n=== Test Summary ===";
    qDebug() << QString("Total Tests: %1").arg(m_testCount);
    qDebug() << QString("Passed: %1").arg(m_passedCount);
    qDebug() << QString("Failed: %1").arg(m_failedCount);
    qDebug() << QString("Skipped: %1").arg(m_skippedCount);
    qDebug() << QString("Total Time: %1ms").arg(totalTime);
    
    if (m_failedCount > 0) {
        qDebug() << "\n=== Failed Tests ===";
        for (const TestRecord& record : m_testResults) {
            if (record.result == TEST_FAILED) {
                qDebug() << QString("%1::%2 - %3")
                            .arg(record.suiteName)
                            .arg(record.testName)
                            .arg(record.errorMessage);
            }
        }
    }
    
    double successRate = m_testCount > 0 ? (double(m_passedCount) / m_testCount) * 100.0 : 0.0;
    qDebug() << QString("Success Rate: %1%").arg(successRate, 0, 'f', 1);
}

void TestFramework::cleanup()
{
    for (TestSuite* suite : m_testSuites) {
        suite->cleanup();
    }
    m_testSuites.clear();
    m_testResults.clear();
}

bool TestFramework::exportResults(const QString& filePath, ExportFormat format)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }
    
    QTextStream stream(&file);
    
    switch (format) {
    case XML_FORMAT:
        return exportToXML(stream);
    case JSON_FORMAT:
        return exportToJSON(stream);
    case CSV_FORMAT:
        return exportToCSV(stream);
    default:
        qWarning() << "Unsupported export format";
        return false;
    }
}

bool TestFramework::exportToXML(QTextStream& stream)
{
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<testResults>\n";
    stream << QString("  <summary tests=\"%1\" passed=\"%2\" failed=\"%3\" skipped=\"%4\"/>\n")
              .arg(m_testCount).arg(m_passedCount).arg(m_failedCount).arg(m_skippedCount);
    
    for (const TestRecord& record : m_testResults) {
        stream << QString("  <test suite=\"%1\" name=\"%2\" result=\"%3\" duration=\"%4\"")
                  .arg(record.suiteName).arg(record.testName)
                  .arg(testResultToString(record.result)).arg(record.duration);
        
        if (!record.errorMessage.isEmpty()) {
            stream << QString(" error=\"%1\"").arg(record.errorMessage.toHtmlEscaped());
        }
        
        stream << "/>\n";
    }
    
    stream << "</testResults>\n";
    return true;
}

bool TestFramework::exportToJSON(QTextStream& stream)
{
    QJsonObject root;
    
    QJsonObject summary;
    summary["total"] = m_testCount;
    summary["passed"] = m_passedCount;
    summary["failed"] = m_failedCount;
    summary["skipped"] = m_skippedCount;
    root["summary"] = summary;
    
    QJsonArray tests;
    for (const TestRecord& record : m_testResults) {
        QJsonObject test;
        test["suite"] = record.suiteName;
        test["name"] = record.testName;
        test["result"] = testResultToString(record.result);
        test["duration"] = record.duration;
        test["timestamp"] = record.timestamp.toString(Qt::ISODate);
        
        if (!record.errorMessage.isEmpty()) {
            test["error"] = record.errorMessage;
        }
        
        tests.append(test);
    }
    root["tests"] = tests;
    
    QJsonDocument doc(root);
    stream << doc.toJson();
    return true;
}

bool TestFramework::exportToCSV(QTextStream& stream)
{
    // Write header
    stream << "Suite,Test,Result,Duration,Timestamp,Error\n";
    
    // Write test records
    for (const TestRecord& record : m_testResults) {
        stream << QString("%1,%2,%3,%4,%5,\"%6\"\n")
                  .arg(record.suiteName)
                  .arg(record.testName)
                  .arg(testResultToString(record.result))
                  .arg(record.duration)
                  .arg(record.timestamp.toString(Qt::ISODate))
                  .arg(QString(record.errorMessage).replace("\"", "\"\""));
    }
    
    return true;
}

QString TestFramework::testResultToString(TestResult result)
{
    switch (result) {
    case TEST_PASSED: return "PASSED";
    case TEST_FAILED: return "FAILED";
    case TEST_SKIPPED: return "SKIPPED";
    default: return "UNKNOWN";
    }
}

// TestSuite base class implementation
TestSuite::TestSuite(const QString& name, QObject* parent)
    : QObject(parent)
    , m_name(name)
{
}

TestSuite::~TestSuite()
{
}

bool TestSuite::setup()
{
    // Default implementation - can be overridden
    return true;
}

void TestSuite::cleanup()
{
    // Default implementation - can be overridden
}

bool TestSuite::setupTest(const QString& testName)
{
    Q_UNUSED(testName)
    // Default implementation - can be overridden
    return true;
}

void TestSuite::cleanupTest(const QString& testName)
{
    Q_UNUSED(testName)
    // Default implementation - can be overridden
}

QString TestSuite::lastError() const
{
    return m_lastError;
}

void TestSuite::setLastError(const QString& error)
{
    m_lastError = error;
}
