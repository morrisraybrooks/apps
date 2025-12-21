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

#include "TestFramework.h"

/**
 * @brief Automated test execution and reporting system
 *
 * This system provides:
 * - Command-line test execution
 * - Automated test suite running
 * - Test result reporting (XML, JSON, HTML)
 * - Continuous integration support
 * - Test filtering and selection
 */
class TestRunner : public QObject
{
    Q_OBJECT

public:
    explicit TestRunner(QObject *parent = nullptr);
    ~TestRunner();

    // Main entry point
    int run(const QStringList& arguments);

    // Test suite management
    void registerTestSuites();

    // Test execution
    bool runAllTests();
    bool runTestSuite(const QString& suiteName);
    bool runSingleTest(const QString& suiteName, const QString& testName);

    // Utilities
    TestSuite* findTestSuite(const QString& name);
    void printAvailableTests();

    // Exit code
    int getExitCode() const { return m_exitCode; }

private:
    TestFramework* m_framework;
    int m_exitCode;
};

#endif // TESTRUNNER_H
