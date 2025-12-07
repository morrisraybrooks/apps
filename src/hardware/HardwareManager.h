#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include <QMutex>
#include <memory>

// Forward declarations
class SensorInterface;
class ActuatorControl;
class MCP3008;
class TENSController;
class FluidSensor;
class MotionSensor;

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
    double readAVLPressure();         // Outer V-seal chamber pressure (Sensor 1)
    double readTankPressure();        // Tank vacuum pressure (Sensor 2)
    double readClitoralPressure();    // Clitoral cylinder pressure (Sensor 3)

    // Actuator controls
    void setPumpSpeed(double speedPercent);  // 0-100%
    void setPumpEnabled(bool enabled);

    // Solenoid valve controls (as per specification)
    // Outer V-seal chamber (sustained vacuum for engorgement)
    void setSOL1(bool open);  // Outer chamber vacuum valve
    void setSOL2(bool open);  // Outer chamber vent valve
    void setSOL3(bool open);  // Tank vent valve
    // Clitoral cylinder (high-frequency oscillation for air-pulse)
    void setSOL4(bool open);  // Clitoral cylinder vacuum valve
    void setSOL5(bool open);  // Clitoral cylinder vent valve

    // System status
    bool isPumpEnabled() const { return m_pumpEnabled; }
    double getPumpSpeed() const { return m_pumpSpeed; }
    bool getSOL1State() const { return m_sol1State; }
    bool getSOL2State() const { return m_sol2State; }
    bool getSOL3State() const { return m_sol3State; }
    bool getSOL4State() const { return m_sol4State; }
    bool getSOL5State() const { return m_sol5State; }

    SensorInterface* getSensorInterface() const { return m_sensorInterface.get(); }
    ActuatorControl* getActuatorControl() const { return m_actuatorControl.get(); }
    TENSController* getTENSController() const { return m_tensController.get(); }
    FluidSensor* getFluidSensor() const { return m_fluidSensor.get(); }
    MotionSensor* getMotionSensor() const { return m_motionSensor.get(); }

    // Fluid sensor readings
    double readFluidVolumeMl();           // Current volume in reservoir
    double readFluidFlowRate();           // mL/min flow rate
    double readCumulativeFluidMl();       // Session total

    // TENS control (integrated with clitoral cup electrodes)
    void setTENSEnabled(bool enabled);
    void setTENSFrequency(double hz);
    void setTENSPulseWidth(int microseconds);
    void setTENSAmplitude(double percent);
    bool isTENSRunning() const;
    bool isTENSFault() const;

    // Emergency controls
    void emergencyStop();
    bool resetEmergencyStop();
    bool isEmergencyStop() const { return m_emergencyStop; }
    
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
    std::unique_ptr<TENSController> m_tensController;
    std::unique_ptr<FluidSensor> m_fluidSensor;
    std::unique_ptr<MotionSensor> m_motionSensor;

    // System state
    bool m_initialized;
    bool m_emergencyStop;
    mutable QMutex m_stateMutex;
    
    // Actuator states
    bool m_pumpEnabled;
    double m_pumpSpeed;  // 0-100%
    // Outer V-seal chamber
    bool m_sol1State;    // SOL1 (GPIO 17): Outer chamber vacuum
    bool m_sol2State;    // SOL2 (GPIO 27): Outer chamber vent valve
    bool m_sol3State;    // SOL3 (GPIO 22): Tank vent valve
    // Clitoral cylinder
    bool m_sol4State;    // SOL4 (GPIO 23): Clitoral cylinder vacuum
    bool m_sol5State;    // SOL5 (GPIO 24): Clitoral cylinder vent valve

    // Error tracking
    QString m_lastError;

    // Simulation mode
    bool m_simulationMode;
    double m_simulatedAVLPressure;
    double m_simulatedTankPressure;
    double m_simulatedClitoralPressure;
    QStringList m_simulatedFailures;
    
    // GPIO pin definitions (as per specification)
    // Outer V-seal chamber (sustained vacuum for engorgement)
    static const int GPIO_SOL1 = 17;    // Outer chamber vacuum valve
    static const int GPIO_SOL2 = 27;    // Outer chamber vent valve
    static const int GPIO_SOL3 = 22;    // Tank vent valve
    // Clitoral cylinder (high-frequency oscillation for air-pulse)
    static const int GPIO_SOL4 = 23;    // Clitoral cylinder vacuum valve
    static const int GPIO_SOL5 = 24;    // Clitoral cylinder vent valve
    // Pump control
    static const int GPIO_PUMP_ENABLE = 25;  // L293D Enable pin
    static const int GPIO_PUMP_PWM = 18;     // PWM for pump speed

    // SPI pin definitions for MCP3008
    static const int SPI_SCK = 11;   // GPIO 11 (SCK)
    static const int SPI_MOSI = 10;  // GPIO 10 (MOSI)
    static const int SPI_MISO = 9;   // GPIO 9 (MISO)
    static const int SPI_CS = 8;     // GPIO 8 (CS)

    // ADC channel assignments
    static const int ADC_CHANNEL_AVL = 0;       // Channel 0: Outer V-seal chamber sensor
    static const int ADC_CHANNEL_TANK = 1;      // Channel 1: Tank vacuum sensor
    static const int ADC_CHANNEL_CLITORAL = 2;  // Channel 2: Clitoral cylinder sensor

    // TENS GPIO pin definitions (integrated into clitoral cup)
    static const int GPIO_TENS_ENABLE = 5;   // Master enable for TENS output
    static const int GPIO_TENS_PHASE = 6;    // Polarity control (H=positive, L=negative)
    static const int GPIO_TENS_PWM = 12;     // Amplitude PWM (hardware PWM1)
    static const int GPIO_TENS_FAULT = 16;   // Fault input from driver
};

#endif // HARDWAREMANAGER_H
