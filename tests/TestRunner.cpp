#include "TestRunner.h"
#include "TestFramework.h"
#include "SafetySystemTests.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>

TestRunner::TestRunner(QObject *parent)
    : QObject(parent)
    , m_framework(new TestFramework(this))
    , m_exitCode(0)
{
}

TestRunner::~TestRunner()
{
}

int TestRunner::run(const QStringList& arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Vacuum Controller Test Runner");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add command line options
    QCommandLineOption verboseOption(QStringList() << "v" << "verbose", "Verbose output");
    parser.addOption(verboseOption);
    
    QCommandLineOption suiteOption(QStringList() << "s" << "suite", "Run specific test suite", "suite");
    parser.addOption(suiteOption);
    
    QCommandLineOption testOption(QStringList() << "t" << "test", "Run specific test", "test");
    parser.addOption(testOption);
    
    QCommandLineOption outputOption(QStringList() << "o" << "output", "Output file for results", "file");
    parser.addOption(outputOption);
    
    QCommandLineOption formatOption(QStringList() << "f" << "format", "Output format (xml, json, csv)", "format", "xml");
    parser.addOption(formatOption);
    
    QCommandLineOption stopOnFailureOption("stop-on-failure", "Stop execution on first failure");
    parser.addOption(stopOnFailureOption);
    
    QCommandLineOption timeoutOption("timeout", "Test timeout in milliseconds", "ms", "30000");
    parser.addOption(timeoutOption);
    
    // Parse arguments
    parser.process(arguments);
    
    // Configure framework
    m_framework->setVerbose(parser.isSet(verboseOption));
    m_framework->setStopOnFailure(parser.isSet(stopOnFailureOption));
    m_framework->setTimeout(parser.value(timeoutOption).toInt());
    
    // Register test suites
    registerTestSuites();
    
    // Run tests
    bool success = false;
    
    if (parser.isSet(suiteOption)) {
        QString suiteName = parser.value(suiteOption);
        if (parser.isSet(testOption)) {
            QString testName = parser.value(testOption);
            success = runSingleTest(suiteName, testName);
        } else {
            success = runTestSuite(suiteName);
        }
    } else {
        success = runAllTests();
    }
    
    // Export results if requested
    if (parser.isSet(outputOption)) {
        QString outputFile = parser.value(outputOption);
        QString format = parser.value(formatOption).toLower();
        
        TestFramework::ExportFormat exportFormat = TestFramework::XML_FORMAT;
        if (format == "json") {
            exportFormat = TestFramework::JSON_FORMAT;
        } else if (format == "csv") {
            exportFormat = TestFramework::CSV_FORMAT;
        }
        
        if (!m_framework->exportResults(outputFile, exportFormat)) {
            qWarning() << "Failed to export results to:" << outputFile;
        } else {
            qDebug() << "Results exported to:" << outputFile;
        }
    }
    
    m_exitCode = success ? 0 : 1;
    
    // Schedule application exit
    QTimer::singleShot(0, this, [this]() {
        QCoreApplication::exit(m_exitCode);
    });
    
    return m_exitCode;
}

void TestRunner::registerTestSuites()
{
    qDebug() << "Registering test suites...";
    
    // Register safety system tests
    SafetySystemTests* safetyTests = new SafetySystemTests(this);
    m_framework->addTestSuite(safetyTests);
    
    // TODO: Add other test suites as they are implemented
    // HardwareTests* hardwareTests = new HardwareTests(this);
    // m_framework->addTestSuite(hardwareTests);
    
    // PatternTests* patternTests = new PatternTests(this);
    // m_framework->addTestSuite(patternTests);
    
    // GUITests* guiTests = new GUITests(this);
    // m_framework->addTestSuite(guiTests);
    
    // PerformanceTests* performanceTests = new PerformanceTests(this);
    // m_framework->addTestSuite(performanceTests);
    
    // IntegrationTests* integrationTests = new IntegrationTests(this);
    // m_framework->addTestSuite(integrationTests);
    
    qDebug() << "Registered" << m_framework->testSuiteCount() << "test suites";
}

bool TestRunner::runAllTests()
{
    qDebug() << "Running all test suites...";
    return m_framework->runAllTests();
}

bool TestRunner::runTestSuite(const QString& suiteName)
{
    qDebug() << "Running test suite:" << suiteName;
    
    TestSuite* suite = findTestSuite(suiteName);
    if (!suite) {
        qWarning() << "Test suite not found:" << suiteName;
        return false;
    }
    
    return m_framework->runTestSuite(suite);
}

bool TestRunner::runSingleTest(const QString& suiteName, const QString& testName)
{
    qDebug() << "Running single test:" << suiteName << "::" << testName;
    
    TestSuite* suite = findTestSuite(suiteName);
    if (!suite) {
        qWarning() << "Test suite not found:" << suiteName;
        return false;
    }
    
    if (!suite->testNames().contains(testName)) {
        qWarning() << "Test not found:" << testName << "in suite:" << suiteName;
        return false;
    }
    
    return m_framework->runSingleTest(suite, testName);
}

TestSuite* TestRunner::findTestSuite(const QString& name)
{
    const QList<TestSuite*>& suites = m_framework->testSuites();
    for (TestSuite* suite : suites) {
        if (suite->name() == name) {
            return suite;
        }
    }
    return nullptr;
}

void TestRunner::printAvailableTests()
{
    qDebug() << "Available test suites:";
    
    const QList<TestSuite*>& suites = m_framework->testSuites();
    for (TestSuite* suite : suites) {
        qDebug() << "  " << suite->name();
        
        const QStringList tests = suite->testNames();
        for (const QString& test : tests) {
            qDebug() << "    " << test;
        }
    }
}

// Main function for standalone test runner
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("VacuumControllerTests");
    app.setApplicationVersion("1.0");
    
    TestRunner runner;
    
    // Handle special commands
    QStringList args = app.arguments();
    if (args.contains("--list-tests")) {
        runner.registerTestSuites();
        runner.printAvailableTests();
        return 0;
    }
    
    // Run tests
    int result = runner.run(args);
    
    // Start event loop
    return app.exec();
}
