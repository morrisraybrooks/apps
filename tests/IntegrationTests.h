#ifndef INTEGRATIONTESTS_H
#define INTEGRATIONTESTS_H

#include <QObject>
#include <QTest>
#include <QElapsedTimer>
#include <memory>

// Forward declarations
class VacuumController;
class MainWindow;

/**
 * @brief Comprehensive integration tests for the vacuum controller system
 * 
 * These tests validate the integration between all major system components:
 * - GUI to controller communication
 * - Hardware to software integration
 * - Real-time performance validation
 * - Multi-threaded system coordination
 * - Safety system integration
 * - Data flow integrity
 * - System recovery capabilities
 */
class IntegrationTests : public QObject
{
    Q_OBJECT

private slots:
    // Test setup and cleanup
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Core integration tests
    void testSystemInitialization();
    void testSensorToSafetyIntegration();
    void testPatternToHardwareIntegration();
    void testGUIToControllerIntegration();
    void testThreadCommunication();
    
    // Performance and timing tests
    void testRealTimePerformance();
    void testDataAcquisitionTiming();
    void testGUIResponsiveness();
    void testSafetyResponseTime();
    
    // System robustness tests
    void testSystemRecovery();
    void testErrorHandlingIntegration();
    void testFailsafeOperation();
    void testDataFlowIntegrity();
    
    // Load and stress tests
    void testHighFrequencyOperations();
    void testConcurrentOperations();
    void testMemoryStability();
    void testLongRunningOperation();

private:
    // Test utilities
    void simulatePressureChange(double avlPressure, double tankPressure);
    void simulateHardwareFailure(const QString& component);
    void simulateSensorFailure(const QString& sensor);
    void resetHardwareSimulation();
    
    void waitForSystemStabilization(int timeoutMs = 5000);
    bool verifySystemState(const QString& expectedState);
    bool verifyDataIntegrity();
    
    // Test objects
    std::unique_ptr<VacuumController> m_controller;
    std::unique_ptr<MainWindow> m_mainWindow;
    
    // Test state
    bool m_hardwareSimulationEnabled;
    QElapsedTimer m_testTimer;
    
    // Performance tracking
    struct PerformanceMetrics {
        double dataAcquisitionRate = 0.0;
        double guiUpdateRate = 0.0;
        double safetyCheckRate = 0.0;
        qint64 averageResponseTime = 0;
        qint64 maxResponseTime = 0;
        
        PerformanceMetrics() = default;
    };
    
    PerformanceMetrics m_performanceMetrics;
    
    // Test constants
    static const int DEFAULT_TEST_TIMEOUT = 10000;
    static const double PRESSURE_TOLERANCE = 0.1;
    static const double TIMING_TOLERANCE = 0.1;
    static const int STABILIZATION_TIME = 1000;
};

#endif // INTEGRATIONTESTS_H
