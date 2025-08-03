#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include <QMutex>
#include <memory>

// Forward declarations
class SensorInterface;
class ActuatorControl;
class MCP3008;

/**
 * @brief Hardware abstraction layer for the vacuum controller
 * 
 * This class provides a unified interface to all hardware components
 * including sensors, actuators, and communication interfaces.
 * It handles initialization, error checking, and safe shutdown.
 */
class HardwareManager : public QObject
{
    Q_OBJECT

public:
    explicit HardwareManager(QObject *parent = nullptr);
    ~HardwareManager();

    // Initialization and shutdown
    bool initialize();
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Sensor readings (pressure in mmHg)
    double readAVLPressure();      // Applied Vacuum Line pressure
    double readTankPressure();     // Tank vacuum pressure
    
    // Actuator controls
    void setPumpSpeed(double speedPercent);  // 0-100%
    void setPumpEnabled(bool enabled);
    
    // Solenoid valve controls (as per specification)
    void setSOL1(bool open);  // AVL (Applied Vacuum Line)
    void setSOL2(bool open);  // AVL vent valve
    void setSOL3(bool open);  // Tank vent valve
    
    // System status
    bool isPumpEnabled() const { return m_pumpEnabled; }
    double getPumpSpeed() const { return m_pumpSpeed; }
    bool getSOL1State() const { return m_sol1State; }
    bool getSOL2State() const { return m_sol2State; }
    bool getSOL3State() const { return m_sol3State; }

    SensorInterface* getSensorInterface() const { return m_sensorInterface.get(); }
    ActuatorControl* getActuatorControl() const { return m_actuatorControl.get(); }
    
    // Emergency controls
    void emergencyStop();
    bool resetEmergencyStop();
    
    // Hardware diagnostics
    bool performSelfTest();
    QString getLastError() const { return m_lastError; }

    // Simulation mode for testing
    void setSimulationMode(bool enabled);
    bool isSimulationMode() const { return m_simulationMode; }
    void setSimulatedPressure(double pressure);
    void setSimulatedSensorValues(double avlPressure, double tankPressure);
    void simulateHardwareFailure(const QString& component);
    void simulateSensorError(const QString& sensor);
    void resetHardwareSimulation();

Q_SIGNALS:
    void hardwareError(const QString& error);
    void sensorError(const QString& sensor, const QString& error);
    void actuatorError(const QString& actuator, const QString& error);

private Q_SLOTS:
    void handleSensorError(const QString& error);
    void handleActuatorError(const QString& error);

private:
    void initializeGPIO();
    void initializeSPI();
    bool validateHardware();
    void safeShutdown();

    // Hardware interfaces
    std::unique_ptr<SensorInterface> m_sensorInterface;
    std::unique_ptr<ActuatorControl> m_actuatorControl;
    std::unique_ptr<MCP3008> m_adc;
    
    // System state
    bool m_initialized;
    bool m_emergencyStop;
    mutable QMutex m_stateMutex;
    
    // Actuator states
    bool m_pumpEnabled;
    double m_pumpSpeed;  // 0-100%
    bool m_sol1State;    // SOL1 (GPIO 17): AVL
    bool m_sol2State;    // SOL2 (GPIO 27): AVL vent valve
    bool m_sol3State;    // SOL3 (GPIO 22): Tank vent valve
    
    // Error tracking
    QString m_lastError;

    // Simulation mode
    bool m_simulationMode;
    double m_simulatedAVLPressure;
    double m_simulatedTankPressure;
    QStringList m_simulatedFailures;
    
    // GPIO pin definitions (as per specification)
    static const int GPIO_SOL1 = 17;    // AVL (Applied Vacuum Line)
    static const int GPIO_SOL2 = 27;    // AVL vent valve
    static const int GPIO_SOL3 = 22;    // Tank vent valve
    static const int GPIO_PUMP_ENABLE = 25;  // L293D Enable pin
    static const int GPIO_PUMP_PWM = 18;     // PWM for pump speed
    
    // SPI pin definitions for MCP3008
    static const int SPI_SCK = 11;   // GPIO 11 (SCK)
    static const int SPI_MOSI = 10;  // GPIO 10 (MOSI)
    static const int SPI_MISO = 9;   // GPIO 9 (MISO)
    static const int SPI_CS = 8;     // GPIO 8 (CS)
    
    // ADC channel assignments
    static const int ADC_CHANNEL_AVL = 0;   // Channel 0: AVL-Line sensor
    static const int ADC_CHANNEL_TANK = 1;  // Channel 1: Tank vacuum sensor
};

#endif // HARDWAREMANAGER_H
