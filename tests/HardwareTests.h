#ifndef HARDWARETESTS_H
#define HARDWARETESTS_H

#include <QObject>
#include <QTest>
#include <QElapsedTimer>

/**
 * @brief Hardware testing class for vacuum controller
 * 
 * Tests all hardware-related functionality including:
 * - GPIO initialization and control
 * - Solenoid valve operations
 * - Pump control and PWM
 * - Sensor readings and calibration
 * - Emergency stop functionality
 * - SPI communication with ADC
 * - Hardware response timings
 * - Concurrent operations
 */
class HardwareTests : public QObject
{
    Q_OBJECT

public:
    explicit HardwareTests(QObject *parent = nullptr);

private slots:
    // Test framework setup/teardown
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // GPIO and pin control tests
    void testGPIOInitialization();
    
    // Solenoid control tests
    void testSolenoidControl();
    
    // Pump control tests
    void testPumpControl();
    
    // Sensor reading tests
    void testSensorReading();
    
    // Emergency stop tests
    void testEmergencyStop();
    
    // SPI communication tests
    void testSPICommunication();
    
    // Performance and timing tests
    void testHardwareTimings();
    
    // Concurrent operation tests
    void testConcurrentOperations();

private:
    // Helper methods for test setup
    void setupTestHardware();
    void resetHardwareToSafeState();
    bool verifyHardwareConnections();
    
    // Test data
    static constexpr int SOL1_PIN = 17;
    static constexpr int SOL2_PIN = 27;
    static constexpr int SOL3_PIN = 22;
    static constexpr int PUMP_PIN = 25;
    static constexpr int EMERGENCY_STOP_PIN = 21;
    
    static constexpr double MAX_PRESSURE = 100.0;  // mmHg
    static constexpr double MIN_PRESSURE = -100.0; // mmHg
    
    static constexpr int MAX_RESPONSE_TIME_MS = 50;
    static constexpr int MAX_SENSOR_READ_TIME_MS = 5;
};

#endif // HARDWARETESTS_H
