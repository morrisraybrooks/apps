#ifndef MOCKACTUATORCONTROL_H
#define MOCKACTUATORCONTROL_H

#include <QObject>
#include <QMap>
#include <QMutex>

/**
 * @brief Mock actuator control for testing
 * 
 * Provides a complete mock implementation of the actuator control interface
 * for testing purposes. Simulates pump and solenoid control without
 * requiring actual hardware.
 */
class MockActuatorControl : public QObject
{
    Q_OBJECT

public:
    explicit MockActuatorControl(QObject *parent = nullptr);
    ~MockActuatorControl();

    // Initialization and shutdown
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // Pump control
    bool setPump(bool enable);
    bool getPumpState() const;
    bool setPumpPWM(int percentage);
    int getPumpPWM() const;
    
    // Solenoid control
    bool setSolenoid(int solenoidNumber, bool open);
    bool getSolenoidState(int solenoidNumber) const;
    bool setAllSolenoids(bool sol1, bool sol2, bool sol3);
    QMap<int, bool> getAllSolenoidStates() const;
    
    // Emergency control
    void emergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStop() const;
    
    // System operations
    bool performSelfTest();
    void resetToSafeState();
    bool testActuator(int actuatorId);

signals:
    // Actuator state change signals
    void pumpStateChanged(bool enabled);
    void pumpPWMChanged(int percentage);
    void solenoidStateChanged(int solenoidNumber, bool open);
    void emergencyStopTriggered();
    void emergencyStopReset();

private:
    // System state
    bool m_initialized;
    bool m_emergencyStop;
    
    // Pump state
    bool m_pumpState;
    int m_pumpPWM;
    
    // Solenoid states
    QMap<int, bool> m_solenoidStates;
    
    // Thread safety
    mutable QMutex m_mutex;
};

#endif // MOCKACTUATORCONTROL_H
