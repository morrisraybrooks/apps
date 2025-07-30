#ifndef SAFETYSYSTEMTESTS_H
#define SAFETYSYSTEMTESTS_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <memory>

// Forward declarations
class VacuumController;
class SafetyManager;
class AntiDetachmentMonitor;
class EmergencyStop;
class HardwareManager;

/**
 * @brief Comprehensive safety system test suite
 * 
 * This test suite validates all safety-critical functionality:
 * - Anti-detachment monitoring and response
 * - Emergency stop functionality
 * - Overpressure protection
 * - Sensor failure detection
 * - Hardware failure response
 * - Safety system integration
 * - Fail-safe behavior validation
 */
class SafetySystemTests : public QObject
{
    Q_OBJECT

private slots:
    // Test setup and cleanup
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Anti-detachment monitor tests
    void testAntiDetachmentInitialization();
    void testAntiDetachmentThresholdSetting();
    void testAntiDetachmentDetection();
    void testAntiDetachmentResponse();
    void testAntiDetachmentRecovery();
    void testAntiDetachmentHysteresis();
    void testAntiDetachmentSelfTest();
    void testAntiDetachmentErrorHandling();
    
    // Emergency stop tests
    void testEmergencyStopInitialization();
    void testSoftwareEmergencyStop();
    void testHardwareEmergencyStop();
    void testEmergencyStopReset();
    void testEmergencyStopValidation();
    void testEmergencyStopFailsafe();
    
    // Overpressure protection tests
    void testOverpressureDetection();
    void testOverpressureResponse();
    void testOverpressureRecovery();
    void testOverpressureThresholds();
    void testOverpressureFailsafe();
    
    // Sensor failure tests
    void testSensorDisconnection();
    void testSensorOutOfRange();
    void testSensorNoise();
    void testSensorCalibrationFailure();
    void testMultipleSensorFailures();
    
    // Hardware failure tests
    void testActuatorFailure();
    void testPumpFailure();
    void testValveFailure();
    void testPowerFailure();
    void testCommunicationFailure();
    
    // Integration tests
    void testSafetySystemIntegration();
    void testSafetyToPatternIntegration();
    void testSafetyToGUIIntegration();
    void testMultipleSimultaneousFailures();
    
    // Performance tests
    void testSafetyResponseTime();
    void testSafetySystemLoad();
    void testSafetyMemoryUsage();
    void testSafetyConcurrency();
    
    // Stress tests
    void testContinuousOperation();
    void testRapidStateChanges();
    void testHighFrequencyAlerts();
    void testSystemRecovery();

private:
    // Test utilities
    void simulatePressureChange(double avlPressure, double tankPressure);
    void simulateHardwareFailure(const QString& component);
    void simulateSensorFailure(const QString& sensor);
    void waitForSafetyResponse(int timeoutMs = 5000);
    bool verifySafeState();
    void resetSystemToSafeState();
    
    // Verification helpers
    bool verifyAntiDetachmentActive();
    bool verifyEmergencyStopActive();
    bool verifyOverpressureProtection();
    bool verifySystemShutdown();
    bool verifyHardwareSafeState();
    
    // Test data generation
    void generatePressureTestData();
    void generateFailureScenarios();
    void generateStressTestData();
    
    // Mock objects
    std::unique_ptr<VacuumController> m_controller;
    std::unique_ptr<HardwareManager> m_mockHardware;
    std::unique_ptr<SafetyManager> m_safetyManager;
    std::unique_ptr<AntiDetachmentMonitor> m_antiDetachmentMonitor;
    std::unique_ptr<EmergencyStop> m_emergencyStop;
    
    // Test state
    bool m_hardwareSimulationEnabled;
    QMap<QString, QVariant> m_testParameters;
    QStringList m_testErrors;
    
    // Signal spies for monitoring
    std::unique_ptr<QSignalSpy> m_antiDetachmentSpy;
    std::unique_ptr<QSignalSpy> m_emergencyStopSpy;
    std::unique_ptr<QSignalSpy> m_overpressureSpy;
    std::unique_ptr<QSignalSpy> m_sensorErrorSpy;
    std::unique_ptr<QSignalSpy> m_hardwareErrorSpy;
    
    // Test constants
    static const double TEST_ANTI_DETACHMENT_THRESHOLD;
    static const double TEST_OVERPRESSURE_THRESHOLD;
    static const double TEST_PRESSURE_TOLERANCE;
    static const int TEST_RESPONSE_TIMEOUT_MS;
    static const int TEST_SAFETY_CHECK_INTERVAL_MS;
};

#endif // SAFETYSYSTEMTESTS_H
