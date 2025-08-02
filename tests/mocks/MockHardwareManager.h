#ifndef MOCKHARDWAREMANAGER_H
#define MOCKHARDWAREMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>

/**
 * @brief Mock hardware manager for testing
 * 
 * Provides a complete mock implementation of the hardware interface
 * for testing purposes. Simulates all hardware operations without
 * requiring actual GPIO/SPI hardware.
 */
class MockHardwareManager : public QObject
{
    Q_OBJECT

public:
    explicit MockHardwareManager(QObject *parent = nullptr);
    ~MockHardwareManager();

    // Initialization and shutdown
    bool initialize();
    void shutdown();
    
    // Pump control
    bool setPump(bool enable);
    bool getPumpState() const;
    bool setPumpPWM(int percentage);
    int getPumpPWM() const;
    
    // Solenoid control
    bool setSolenoid(int solenoidNumber, bool open);
    bool getSolenoidState(int solenoidNumber) const;
    bool setAllSolenoids(bool sol1, bool sol2, bool sol3);
    
    // Sensor reading
    double readPressureSensor(int sensorNumber);
    void setPressureSensorValue(int sensorNumber, double pressure);
    
    // Emergency stop
    bool isEmergencyStop() const;
    void triggerEmergencyStop();
    void resetEmergencyStop();
    
    // Error simulation
    void simulateSensorError(int sensorNumber, bool hasError);
    bool isSensorError(int sensorNumber) const;
    
    // Status queries
    bool isGPIOInitialized() const;
    bool isSPIInitialized() const;
    
    // System operations
    bool performSelfTest();
    void resetToSafeState();

signals:
    // Hardware state change signals
    void pumpStateChanged(bool enabled);
    void pumpPWMChanged(int percentage);
    void solenoidStateChanged(int solenoidNumber, bool open);
    void pressureChanged(int sensorNumber, double pressure);
    void emergencyStopTriggered();
    void emergencyStopReset();
    void sensorErrorChanged(int sensorNumber, bool hasError);

private:
    // Hardware state
    bool m_pumpState;
    int m_pumpPWM;
    QMap<int, bool> m_solenoidStates;
    bool m_emergencyStop;
    
    // Sensor data
    double m_pressure1;
    double m_pressure2;
    bool m_sensorError1;
    bool m_sensorError2;
    
    // System state
    bool m_gpioInitialized;
    bool m_spiInitialized;
    
    // Thread safety
    mutable QMutex m_mutex;
};

#endif // MOCKHARDWAREMANAGER_H
