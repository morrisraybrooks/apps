#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QScreen>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>
#include <cstdlib>

#include "VacuumController.h"
#include "gui/MainWindow.h"
#include "safety/SafetyManager.h"
#include "testing/HardwareTester.h"

// Forward declarations
int runHardwareTests(QApplication& app, const QCommandLineParser& parser);
int runGUIApplication(QApplication& app);

int main(int argc, char *argv[])
{
    // Configure Qt platform for optimal display on Raspberry Pi
    // Priority: Wayland > EGLFS > Auto-detect

    // Check if platform is specified via command line or environment
    QString platform = qgetenv("QT_QPA_PLATFORM");
    if (platform.isEmpty()) {
        // Default to Wayland for modern systems
        qputenv("QT_QPA_PLATFORM", "wayland");
        platform = "wayland";
    }

    // Configure platform-specific settings
    if (platform == "wayland") {
        qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
        qputenv("QT_SCALE_FACTOR", "1.5");  // High-DPI for 50-inch display
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        qputenv("QT_FONT_DPI", "120");
        qputenv("QT_IM_MODULE", "qtvirtualkeyboard");
    } else if (platform == "eglfs") {
        qputenv("QT_QPA_EGLFS_ALWAYS_SET_MODE", "1");
        qputenv("QT_QPA_EGLFS_HIDECURSOR", "1");  // Hide cursor for touch-only
        qputenv("QT_SCALE_FACTOR", "1.5");
        qputenv("QT_FONT_DPI", "120");
    }

    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Vacuum Controller");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Medical Devices Inc");

    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Vacuum Controller System - Hardware control and testing application");
    parser.addHelpOption();
    parser.addVersionOption();

    // Testing options
    QCommandLineOption testSensorsOption("test-sensors", "Run sensor hardware tests");
    parser.addOption(testSensorsOption);

    QCommandLineOption testActuatorsOption("test-actuators", "Run actuator hardware tests");
    parser.addOption(testActuatorsOption);

    QCommandLineOption testAllOption("test-all", "Run comprehensive hardware tests");
    parser.addOption(testAllOption);

    QCommandLineOption verboseOption("verbose", "Enable verbose test output");
    parser.addOption(verboseOption);

    QCommandLineOption outputFileOption("output", "Save test results to file", "filename");
    parser.addOption(outputFileOption);

    QCommandLineOption timeoutOption("timeout", "Set test timeout in seconds (default: 30)", "seconds");
    parser.addOption(timeoutOption);

    QCommandLineOption continueOnFailureOption("continue-on-failure", "Continue testing after failures");
    parser.addOption(continueOnFailureOption);

    // Parse command line arguments
    parser.process(app);

    // Check if running in test mode
    if (parser.isSet(testSensorsOption) || parser.isSet(testActuatorsOption) || parser.isSet(testAllOption)) {
        return runHardwareTests(app, parser);
    }

    // Configure for large display (50-inch HDMI)
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        std::cout << "Display resolution: " << screenGeometry.width() 
                  << "x" << screenGeometry.height() << std::endl;
    }
    
    // Set application style for touch interface
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Apply custom stylesheet for medical device UI
    QString styleSheet = R"(
        QMainWindow {
            background-color: #f0f0f0;
            font-size: 16pt;
        }
        QPushButton {
            background-color: #4CAF50;
            border: 2px solid #45a049;
            color: white;
            padding: 15px 32px;
            text-align: center;
            font-size: 18pt;
            margin: 4px 2px;
            border-radius: 8px;
            min-height: 60px;
            min-width: 120px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        .emergency-button {
            background-color: #f44336;
            border: 2px solid #da190b;
        }
        .emergency-button:hover {
            background-color: #da190b;
        }
        QLabel {
            font-size: 14pt;
            color: #333333;
        }
        .status-label {
            font-size: 16pt;
            font-weight: bold;
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 5px;
            background-color: white;
        }
        .pressure-display {
            font-size: 24pt;
            font-weight: bold;
            color: #2196F3;
            background-color: #e3f2fd;
            border: 3px solid #2196F3;
            border-radius: 10px;
            padding: 20px;
            text-align: center;
        }
    )";
    app.setStyleSheet(styleSheet);
    
    try {
        // Initialize the vacuum controller system
        VacuumController controller;

        // Initialize the controller - allow GUI to start even if hardware fails
        std::cout << "Starting controller initialization..." << std::endl;
        bool hardwareReady = controller.initialize();
        if (!hardwareReady) {
            std::cout << "Hardware initialization failed - starting in GUI-only mode" << std::endl;
            // Don't exit - allow GUI to start for debugging/testing
        } else {
            std::cout << "Controller initialization completed successfully!" << std::endl;
        }

        // Create and show main window
        std::cout << "Creating MainWindow..." << std::endl;
        MainWindow window(&controller);
        std::cout << "MainWindow created successfully!" << std::endl;

        // Show window with title bar (not fullscreen)
        std::cout << "Showing MainWindow..." << std::endl;
        window.show();
        std::cout << "MainWindow shown successfully!" << std::endl;

        // Start monitoring threads after GUI is ready (only if hardware is ready)
        if (hardwareReady) {
            std::cout << "Starting monitoring threads..." << std::endl;
            controller.startMonitoringThreads();
            std::cout << "Monitoring threads started!" << std::endl;
        } else {
            std::cout << "Skipping monitoring threads - hardware not ready" << std::endl;
        }

        // Enable touch events
        std::cout << "Enabling touch events..." << std::endl;
        window.setAttribute(Qt::WA_AcceptTouchEvents, true);
        std::cout << "Touch events enabled!" << std::endl;

        std::cout << "Vacuum Controller GUI started successfully" << std::endl;
        
        return runGUIApplication(app);

    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Startup Error",
                            QString("Failed to initialize vacuum controller: %1").arg(e.what()));
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

int runGUIApplication(QApplication& app)
{
    return app.exec();
}

int runHardwareTests(QApplication& app, const QCommandLineParser& parser)
{
    std::cout << "=== Vacuum Controller Hardware Testing ===" << std::endl;
    std::cout << "Initializing hardware..." << std::endl;

    try {
        // Initialize hardware manager
        VacuumController controller;
        if (!controller.initialize()) {
            std::cerr << "Error: Failed to initialize hardware" << std::endl;
            return -1;
        }

        // Create hardware tester
        HardwareTester tester(controller.getHardwareManager(), controller.getSafetyManager());

        // Configure tester based on command line options
        if (parser.isSet("verbose")) {
            tester.setVerboseOutput(true);
        }

        if (parser.isSet("output")) {
            tester.setOutputFile(parser.value("output"));
        }

        if (parser.isSet("timeout")) {
            bool ok;
            int timeout = parser.value("timeout").toInt(&ok);
            if (ok && timeout > 0) {
                tester.setTestTimeout(timeout * 1000); // Convert to milliseconds
            }
        }

        if (parser.isSet("continue-on-failure")) {
            tester.setContinueOnFailure(true);
        }

        // Run appropriate tests
        bool testResult = false;

        if (parser.isSet("test-sensors")) {
            std::cout << "Running sensor tests..." << std::endl;
            testResult = tester.runSensorTests();
        } else if (parser.isSet("test-actuators")) {
            std::cout << "Running actuator tests..." << std::endl;
            testResult = tester.runActuatorTests();
        } else if (parser.isSet("test-all")) {
            std::cout << "Running comprehensive tests..." << std::endl;
            testResult = tester.runComprehensiveTests();
        }

        // Print test summary
        HardwareTester::TestSuite suite = tester.getLastTestSuite();
        std::cout << std::endl << "=== Test Results ===" << std::endl;
        std::cout << "Total Tests: " << (suite.passed + suite.failed + suite.warnings + suite.skipped) << std::endl;
        std::cout << "Passed: " << suite.passed << std::endl;
        std::cout << "Failed: " << suite.failed << std::endl;
        std::cout << "Warnings: " << suite.warnings << std::endl;
        std::cout << "Skipped: " << suite.skipped << std::endl;
        std::cout << "Duration: " << (suite.totalDuration / 1000.0) << " seconds" << std::endl;

        // Save test report if output file specified
        if (parser.isSet("output")) {
            QString reportFile = parser.value("output") + "_report.json";
            if (tester.saveTestReport(reportFile)) {
                std::cout << "Test report saved to: " << reportFile.toStdString() << std::endl;
            }
        }

        // Print failed tests
        if (suite.failed > 0) {
            std::cout << std::endl << "=== Failed Tests ===" << std::endl;
            QList<HardwareTester::TestCase> failedTests = tester.getFailedTests();
            for (const HardwareTester::TestCase& test : failedTests) {
                std::cout << "- " << test.name.toStdString() << ": " << test.details.toStdString() << std::endl;
            }
        }

        std::cout << std::endl << "Testing complete." << std::endl;

        // Return appropriate exit code
        return testResult ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "Error during testing: " << e.what() << std::endl;
        return -1;
    }
}
