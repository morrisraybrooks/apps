#ifndef ACTUATORCONTROL_H
#define ACTUATORCONTROL_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <memory>

// libgpiod v2.x C++ API
#include <gpiod.hpp>

/**
 * @brief Control interface for vacuum system actuators
 * 
 * This class manages all actuators in the vacuum system:
 * - Vacuum pump (PWM controlled via L293D motor driver)
 * - 3 solenoid valves for vacuum routing and venting
 * 
 * Provides safe control with proper initialization and emergency stop.
 */
class ActuatorControl : public QObject
{
    Q_OBJECT

public:
    explicit ActuatorControl(QObject *parent = nullptr);
    ~ActuatorControl();

    // Initialization
    bool initialize();
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Pump control
    void setPumpEnabled(bool enabled);
    void setPumpSpeed(double speedPercent);  // 0-100%
    bool isPumpEnabled() const { return m_pumpEnabled; }
    double getPumpSpeed() const { return m_pumpSpeed; }
    
    // Solenoid valve controls (as per specification)
    void setSOL1(bool open);  // GPIO 17: AVL (Applied Vacuum Line)
    void setSOL2(bool open);  // GPIO 27: AVL vent valve
    void setSOL3(bool open);  // GPIO 22: Tank vent valve
    
    // Valve state queries
    bool getSOL1State() const { return m_sol1State; }
    bool getSOL2State() const { return m_sol2State; }
    bool getSOL3State() const { return m_sol3State; }
    
    // Safety controls
    void emergencyStop();
    bool resetEmergencyStop();
    bool isEmergencyStopped() const { return m_emergencyStop; }
    
    // System diagnostics
    bool performSelfTest();
    QString getLastError() const { return m_lastError; }

    // PWM configuration
    void setPWMFrequency(int frequency);
    int getPWMFrequency() const { return m_pwmFrequency; }

Q_SIGNALS:
    void actuatorError(const QString& actuator, const QString& error);
    void emergencyStopActivated();
    void pumpStateChanged(bool enabled, double speed);
    void valveStateChanged(int valve, bool open);

private Q_SLOTS:
    void updatePWM();

private:
    bool initializeGPIO();
    bool initializePWM();
    void setGPIOOutput(int pin, bool state);
    bool getGPIOState(int pin);
    void safeShutdownAll();

    // System state
    bool m_initialized;
    bool m_emergencyStop;
    mutable QMutex m_stateMutex;
    
    // Pump state
    bool m_pumpEnabled;
    double m_pumpSpeed;  // 0-100%
    int m_pwmValue;      // Actual PWM value (0-1024)
    
    // Valve states
    bool m_sol1State;    // SOL1 (GPIO 17): AVL
    bool m_sol2State;    // SOL2 (GPIO 27): AVL vent valve
    bool m_sol3State;    // SOL3 (GPIO 22): Tank vent valve
    
    // PWM control
    QTimer* m_pwmTimer;
    int m_pwmFrequency;

    // GPIO control using libgpiod v2.x C++ API
    std::unique_ptr<gpiod::chip> m_gpioChip;
    std::unique_ptr<gpiod::line_request> m_outputRequest;

    // Error tracking
    QString m_lastError;
    
    // GPIO pin definitions (as per specification)
    static const int GPIO_SOL1 = 17;         // AVL (Applied Vacuum Line)
    static const int GPIO_SOL2 = 27;         // AVL vent valve
    static const int GPIO_SOL3 = 22;         // Tank vent valve
    static const int GPIO_PUMP_ENABLE = 25;  // L293D Enable pin
    static const int GPIO_PUMP_PWM = 18;     // PWM for pump speed control
    
    // PWM configuration
    static const int PWM_FREQUENCY = 5000;   // 5kHz as per specification
    static const int PWM_RANGE = 1024;       // PWM range (0-1024)
    
    // Safety limits
    static const double MAX_PUMP_SPEED;      // Maximum allowed pump speed (%)
    static const double MIN_PUMP_SPEED;      // Minimum meaningful pump speed (%)
};

#endif // ACTUATORCONTROL_H
