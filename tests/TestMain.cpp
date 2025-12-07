#include <QApplication>
#include <QTest>
#include <iostream>

/**
 * @brief Main entry point for individual test executables
 * 
 * This file provides the main() function for individual test executables.
 * Each test executable links against this to get a proper Qt test environment.
 */

// Forward declarations for test classes
class SafetySystemTests;
class HardwareTests;
class PatternTests;
class GUITests;
class PerformanceTests;
class IntegrationTests;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties for testing
    app.setApplicationName("VacuumControllerTests");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Morris Brooks");
    
    std::cout << "Starting Vacuum Controller Test Suite..." << std::endl;
    std::cout << "Qt Version: " << QT_VERSION_STR << std::endl;
    std::cout << "Test Arguments: ";
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl << std::endl;
    
    // The actual test execution will be handled by the specific test class
    // that links against this main function
    
    return 0;
}
