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
#include "gui/styles/ModernMedicalStyle.h"
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

    // Configure platform-specific settings with enhanced high-DPI support
    if (platform == "wayland") {
        qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
        qputenv("QT_IM_MODULE", "qtvirtualkeyboard");
        // Scale factor will be determined automatically by ModernMedicalStyle
    } else if (platform == "eglfs") {
        qputenv("QT_QPA_EGLFS_ALWAYS_SET_MODE", "1");
        qputenv("QT_QPA_EGLFS_HIDECURSOR", "1");  // Hide cursor for touch-only
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    } else if (platform == "xcb") {
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
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

    // Initialize modern medical styling system
    ModernMedicalStyle::initialize(&app);

    // Configure for large display (50-inch HDMI and beyond)
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        qreal dpi = screen->logicalDotsPerInch();
        qreal physicalDpi = screen->physicalDotsPerInch();

        std::cout << "Display configuration:" << std::endl;
        std::cout << "  Resolution: " << screenGeometry.width()
                  << "x" << screenGeometry.height() << std::endl;
        std::cout << "  Logical DPI: " << dpi << std::endl;
        std::cout << "  Physical DPI: " << physicalDpi << std::endl;
        std::cout << "  Scale Factor: " << ModernMedicalStyle::getScaleFactor() << std::endl;

        // Apply enhanced high-DPI scaling for large medical displays
        if (screenGeometry.width() >= 3840 || screenGeometry.height() >= 2160) {
            std::cout << "  Detected 4K+ display - optimizing for ultra-high resolution" << std::endl;
        } else if (screenGeometry.width() >= 2560 || screenGeometry.height() >= 1440) {
            std::cout << "  Detected QHD+ display - optimizing for high resolution" << std::endl;
        } else if (screenGeometry.width() >= 1920 || screenGeometry.height() >= 1080) {
            std::cout << "  Detected Full HD+ display - optimizing for standard resolution" << std::endl;
        }
    }

    // Apply comprehensive modern medical device styling
    QString modernStyleSheet = QString(
        "QMainWindow {"
        "    background-color: %1;"
        "    font-family: %2;"
        "    font-size: %3pt;"
        "}"
        "%4"  // Primary button style
        "%5"  // Secondary button style
        "%6"  // Success button style
        "%7"  // Warning button style
        "%8"  // Danger button style
        "%9"  // Emergency button style
        "%10" // Label styles
        "%11" // GroupBox style
        "%12" // Frame style
        "%13" // Pressure display style
    ).arg(ModernMedicalStyle::Colors::BACKGROUND_LIGHT.name())
     .arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
     .arg(ModernMedicalStyle::Typography::getBody())
     .arg(ModernMedicalStyle::getButtonStyle("primary"))
     .arg(ModernMedicalStyle::getButtonStyle("secondary"))
     .arg(ModernMedicalStyle::getButtonStyle("success"))
     .arg(ModernMedicalStyle::getButtonStyle("warning"))
     .arg(ModernMedicalStyle::getButtonStyle("danger"))
     .arg(ModernMedicalStyle::getEmergencyButtonStyle())
     .arg(ModernMedicalStyle::getLabelStyle("body"))
     .arg(ModernMedicalStyle::getGroupBoxStyle())
     .arg(ModernMedicalStyle::getFrameStyle())
     .arg(ModernMedicalStyle::getPressureDisplayStyle());

    app.setStyleSheet(modernStyleSheet);
    
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
