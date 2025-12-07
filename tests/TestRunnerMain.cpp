#include <QApplication>
#include <QTest>
#include <QDebug>
#include <iostream>

// Include all test classes
#include "SafetySystemTests.h"
#include "HardwareTests.h"
#include "PatternTests.h"
#include "GUITests.h"
#include "PerformanceTests.h"
#include "IntegrationTests.h"
#include "TestRunner.h"

/**
 * @brief Main entry point for the comprehensive test suite
 * 
 * This application runs all vacuum controller tests and generates
 * a comprehensive test report. It can run individual test suites
 * or the complete test battery.
 */

void printUsage(const QString& programName)
{
    std::cout << "Vacuum Controller Test Suite" << std::endl;
    std::cout << "Usage: " << programName.toStdString() << " [options] [test_suite]" << std::endl;
    std::cout << std::endl;
    std::cout << "Test Suites:" << std::endl;
    std::cout << "  all           - Run all tests (default)" << std::endl;
    std::cout << "  safety        - Safety system tests" << std::endl;
    std::cout << "  hardware      - Hardware interface tests" << std::endl;
    std::cout << "  patterns      - Pattern execution tests" << std::endl;
    std::cout << "  gui           - GUI and user interface tests" << std::endl;
    std::cout << "  performance   - Performance and benchmarking tests" << std::endl;
    std::cout << "  integration   - System integration tests" << std::endl;
    std::cout << "  runner        - Comprehensive test runner" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help    - Show this help message" << std::endl;
    std::cout << "  -v, --verbose - Verbose output" << std::endl;
    std::cout << "  -q, --quiet   - Quiet output" << std::endl;
    std::cout << std::endl;
}

int runTestSuite(const QString& suiteName, int argc, char* argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("VacuumControllerTests");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Morris Brooks");
    
    int result = 0;
    
    if (suiteName == "safety") {
        SafetySystemTests tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "hardware") {
        HardwareTests tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "patterns") {
        PatternTests tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "gui") {
        GUITests tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "performance") {
        PerformanceTests tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "integration") {
        IntegrationTests tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "runner") {
        TestRunner tests;
        result = QTest::qExec(&tests, argc, argv);
    }
    else if (suiteName == "all") {
        std::cout << "Running comprehensive test suite..." << std::endl;
        
        int totalResult = 0;
        
        // Run all test suites
        std::cout << "\n=== Safety System Tests ===" << std::endl;
        SafetySystemTests safetyTests;
        totalResult += QTest::qExec(&safetyTests, argc, argv);
        
        std::cout << "\n=== Hardware Tests ===" << std::endl;
        HardwareTests hardwareTests;
        totalResult += QTest::qExec(&hardwareTests, argc, argv);
        
        std::cout << "\n=== Pattern Tests ===" << std::endl;
        PatternTests patternTests;
        totalResult += QTest::qExec(&patternTests, argc, argv);
        
        std::cout << "\n=== GUI Tests ===" << std::endl;
        GUITests guiTests;
        totalResult += QTest::qExec(&guiTests, argc, argv);
        
        std::cout << "\n=== Performance Tests ===" << std::endl;
        PerformanceTests performanceTests;
        totalResult += QTest::qExec(&performanceTests, argc, argv);
        
        std::cout << "\n=== Integration Tests ===" << std::endl;
        IntegrationTests integrationTests;
        totalResult += QTest::qExec(&integrationTests, argc, argv);
        
        std::cout << "\n=== Comprehensive Test Runner ===" << std::endl;
        TestRunner testRunner;
        totalResult += QTest::qExec(&testRunner, argc, argv);
        
        result = totalResult;
        
        std::cout << "\n=== FINAL RESULTS ===" << std::endl;
        if (result == 0) {
            std::cout << "✅ ALL TESTS PASSED!" << std::endl;
        } else {
            std::cout << "❌ SOME TESTS FAILED (exit code: " << result << ")" << std::endl;
        }
    }
    else {
        std::cerr << "Unknown test suite: " << suiteName.toStdString() << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return result;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    QString testSuite = "all";
    bool showHelp = false;
    
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp = true;
            break;
        }
        else if (arg == "-v" || arg == "--verbose") {
            // Verbose mode is handled by QTest
            continue;
        }
        else if (arg == "-q" || arg == "--quiet") {
            // Quiet mode is handled by QTest
            continue;
        }
        else if (!arg.startsWith("-")) {
            // This is the test suite name
            testSuite = arg;
        }
    }
    
    if (showHelp) {
        printUsage(argv[0]);
        return 0;
    }
    
    std::cout << "Vacuum Controller Test Suite v1.0.0" << std::endl;
    std::cout << "Running test suite: " << testSuite.toStdString() << std::endl;
    std::cout << "libgpiod version: 2.2.1" << std::endl;
    std::cout << "Qt version: " << QT_VERSION_STR << std::endl;
    std::cout << std::endl;
    
    return runTestSuite(testSuite, argc, argv);
}
