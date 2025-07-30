#include "ActuatorControl.h"
#include <QDebug>
#include <QMutexLocker>
#include <wiringPi.h>
#include <softPwm.h>
#include <stdexcept>
#include <cmath>

// Constants
const double ActuatorControl::MAX_PUMP_SPEED = 100.0;
const double ActuatorControl::MIN_PUMP_SPEED = 5.0;

ActuatorControl::ActuatorControl(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_emergencyStop(false)
    , m_pumpEnabled(false)
    , m_pumpSpeed(0.0)
    , m_pwmValue(0)
    , m_sol1State(false)
    , m_sol2State(false)
    , m_sol3State(false)
    , m_pwmTimer(new QTimer(this))
{
    // Set up PWM update timer
    m_pwmTimer->setInterval(20);  // 50Hz PWM update
    connect(m_pwmTimer, &QTimer::timeout, this, &ActuatorControl::updatePWM);
}

ActuatorControl::~ActuatorControl()
{
    shutdown();
}

bool ActuatorControl::initialize()
{
    try {
        qDebug() << "Initializing Actuator Control...";
        
        // Initialize GPIO
        if (!initializeGPIO()) {
            throw std::runtime_error("Failed to initialize GPIO");
        }
        
        // Initialize PWM
        if (!initializePWM()) {
            throw std::runtime_error("Failed to initialize PWM");
        }
        
        // Set all actuators to safe initial state
        safeShutdownAll();
        
        // Start PWM timer
        m_pwmTimer->start();
        
        m_initialized = true;
        qDebug() << "Actuator Control initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Actuator initialization failed: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void ActuatorControl::shutdown()
{
    if (!m_initialized) return;
    
    qDebug() << "Shutting down Actuator Control...";
    
    // Stop PWM timer
    m_pwmTimer->stop();
    
    // Safe shutdown all actuators
    safeShutdownAll();
    
    m_initialized = false;
    qDebug() << "Actuator Control shutdown complete";
}

void ActuatorControl::setPumpEnabled(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_emergencyStop && enabled) {
        qWarning() << "Cannot enable pump: Emergency stop active";
        emit actuatorError("Pump", "Cannot enable during emergency stop");
        return;
    }
    
    if (m_pumpEnabled != enabled) {
        m_pumpEnabled = enabled;
        
        // Set enable pin
        setGPIOOutput(GPIO_PUMP_ENABLE, enabled);
        
        if (!enabled) {
            m_pumpSpeed = 0.0;
            m_pwmValue = 0;
        }
        
        emit pumpStateChanged(m_pumpEnabled, m_pumpSpeed);
        qDebug() << "Pump" << (enabled ? "enabled" : "disabled");
    }
}

void ActuatorControl::setPumpSpeed(double speedPercent)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_emergencyStop) {
        qWarning() << "Cannot set pump speed: Emergency stop active";
        return;
    }
    
    // Clamp speed to valid range
    speedPercent = std::max(0.0, std::min(MAX_PUMP_SPEED, speedPercent));
    
    // Apply minimum speed threshold
    if (speedPercent > 0.0 && speedPercent < MIN_PUMP_SPEED) {
        speedPercent = MIN_PUMP_SPEED;
    }
    
    if (std::abs(m_pumpSpeed - speedPercent) > 0.1) {  // Avoid unnecessary updates
        m_pumpSpeed = speedPercent;
        
        // Convert percentage to PWM value
        m_pwmValue = static_cast<int>((speedPercent / 100.0) * PWM_RANGE);
        
        emit pumpStateChanged(m_pumpEnabled, m_pumpSpeed);
        qDebug() << QString("Pump speed set to %1% (PWM: %2)").arg(speedPercent, 0, 'f', 1).arg(m_pwmValue);
    }
}

void ActuatorControl::setSOL1(bool open)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_emergencyStop && open) {
        qWarning() << "Cannot open SOL1: Emergency stop active";
        return;
    }
    
    if (m_sol1State != open) {
        m_sol1State = open;
        setGPIOOutput(GPIO_SOL1, open);
        emit valveStateChanged(1, open);
        qDebug() << "SOL1 (AVL)" << (open ? "opened" : "closed");
    }
}

void ActuatorControl::setSOL2(bool open)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_sol2State != open) {
        m_sol2State = open;
        setGPIOOutput(GPIO_SOL2, open);
        emit valveStateChanged(2, open);
        qDebug() << "SOL2 (AVL vent)" << (open ? "opened" : "closed");
    }
}

void ActuatorControl::setSOL3(bool open)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_sol3State != open) {
        m_sol3State = open;
        setGPIOOutput(GPIO_SOL3, open);
        emit valveStateChanged(3, open);
        qDebug() << "SOL3 (Tank vent)" << (open ? "opened" : "closed");
    }
}

void ActuatorControl::emergencyStop()
{
    QMutexLocker locker(&m_stateMutex);
    
    qWarning() << "ACTUATOR EMERGENCY STOP ACTIVATED";
    
    m_emergencyStop = true;
    
    // Immediately stop pump
    m_pumpEnabled = false;
    m_pumpSpeed = 0.0;
    m_pwmValue = 0;
    setGPIOOutput(GPIO_PUMP_ENABLE, false);
    
    // Open vent valves for safety
    m_sol2State = true;  // AVL vent valve open
    m_sol3State = true;  // Tank vent valve open
    setGPIOOutput(GPIO_SOL2, true);
    setGPIOOutput(GPIO_SOL3, true);
    
    // Close AVL valve
    m_sol1State = false;
    setGPIOOutput(GPIO_SOL1, false);
    
    emit emergencyStopActivated();
    emit pumpStateChanged(m_pumpEnabled, m_pumpSpeed);
    emit valveStateChanged(1, m_sol1State);
    emit valveStateChanged(2, m_sol2State);
    emit valveStateChanged(3, m_sol3State);
}

bool ActuatorControl::resetEmergencyStop()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (!m_emergencyStop) {
        return true;  // Already reset
    }
    
    // Perform safety checks before reset
    if (!performSelfTest()) {
        m_lastError = "Self-test failed during emergency stop reset";
        return false;
    }
    
    m_emergencyStop = false;
    qDebug() << "Actuator emergency stop reset";
    return true;
}

bool ActuatorControl::performSelfTest()
{
    if (!m_initialized) {
        m_lastError = "Actuator control not initialized";
        return false;
    }
    
    try {
        // Test GPIO pins by reading back their states
        bool sol1Test = (getGPIOState(GPIO_SOL1) == m_sol1State);
        bool sol2Test = (getGPIOState(GPIO_SOL2) == m_sol2State);
        bool sol3Test = (getGPIOState(GPIO_SOL3) == m_sol3State);
        bool pumpTest = (getGPIOState(GPIO_PUMP_ENABLE) == m_pumpEnabled);
        
        if (!sol1Test || !sol2Test || !sol3Test || !pumpTest) {
            throw std::runtime_error("GPIO state mismatch detected");
        }
        
        qDebug() << "Actuator self-test passed";
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Self-test failed: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void ActuatorControl::updatePWM()
{
    if (m_initialized && m_pumpEnabled && !m_emergencyStop) {
        // Update software PWM
        softPwmWrite(GPIO_PUMP_PWM, m_pwmValue);
    }
}

bool ActuatorControl::initializeGPIO()
{
    try {
        // Set up solenoid valve pins as outputs
        pinMode(GPIO_SOL1, OUTPUT);
        pinMode(GPIO_SOL2, OUTPUT);
        pinMode(GPIO_SOL3, OUTPUT);
        pinMode(GPIO_PUMP_ENABLE, OUTPUT);
        
        // Initialize all outputs to LOW (safe state)
        digitalWrite(GPIO_SOL1, LOW);
        digitalWrite(GPIO_SOL2, LOW);
        digitalWrite(GPIO_SOL3, LOW);
        digitalWrite(GPIO_PUMP_ENABLE, LOW);
        
        qDebug() << "GPIO pins initialized";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "GPIO initialization failed:" << e.what();
        return false;
    }
}

bool ActuatorControl::initializePWM()
{
    try {
        // Initialize software PWM for pump control
        if (softPwmCreate(GPIO_PUMP_PWM, 0, PWM_RANGE) != 0) {
            throw std::runtime_error("Failed to create software PWM");
        }
        
        qDebug() << QString("PWM initialized on GPIO %1 with range %2")
                    .arg(GPIO_PUMP_PWM).arg(PWM_RANGE);
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "PWM initialization failed:" << e.what();
        return false;
    }
}

void ActuatorControl::setGPIOOutput(int pin, bool state)
{
    digitalWrite(pin, state ? HIGH : LOW);
}

bool ActuatorControl::getGPIOState(int pin)
{
    return digitalRead(pin) == HIGH;
}

void ActuatorControl::safeShutdownAll()
{
    // Set all actuators to safe state
    m_pumpEnabled = false;
    m_pumpSpeed = 0.0;
    m_pwmValue = 0;
    m_sol1State = false;  // AVL valve closed
    m_sol2State = true;   // AVL vent valve open (safe)
    m_sol3State = true;   // Tank vent valve open (safe)
    
    // Apply to hardware
    setGPIOOutput(GPIO_PUMP_ENABLE, false);
    setGPIOOutput(GPIO_SOL1, false);
    setGPIOOutput(GPIO_SOL2, true);
    setGPIOOutput(GPIO_SOL3, true);
    
    if (m_initialized) {
        softPwmWrite(GPIO_PUMP_PWM, 0);
    }
    
    qDebug() << "All actuators set to safe state";
}
