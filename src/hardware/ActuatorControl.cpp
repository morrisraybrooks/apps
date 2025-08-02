#include "ActuatorControl.h"
#include <QDebug>
#include <QMutexLocker>
#include <gpiod.h>
#include <stdexcept>
#include <cmath>
#include <algorithm>

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
    , m_gpioChip(nullptr)
    , m_sol1Line(nullptr)
    , m_sol2Line(nullptr)
    , m_sol3Line(nullptr)
    , m_pumpEnableLine(nullptr)
    , m_pumpPwmLine(nullptr)
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

    // Release GPIO lines
    if (m_sol1Line) {
        gpiod_line_release(m_sol1Line);
        m_sol1Line = nullptr;
    }
    if (m_sol2Line) {
        gpiod_line_release(m_sol2Line);
        m_sol2Line = nullptr;
    }
    if (m_sol3Line) {
        gpiod_line_release(m_sol3Line);
        m_sol3Line = nullptr;
    }
    if (m_pumpEnableLine) {
        gpiod_line_release(m_pumpEnableLine);
        m_pumpEnableLine = nullptr;
    }
    if (m_pumpPwmLine) {
        gpiod_line_release(m_pumpPwmLine);
        m_pumpPwmLine = nullptr;
    }

    // Close GPIO chip
    if (m_gpioChip) {
        gpiod_chip_close(m_gpioChip);
        m_gpioChip = nullptr;
    }

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
    if (m_initialized && m_pumpEnabled && !m_emergencyStop && m_pumpPwmLine) {
        // Simple software PWM: toggle GPIO based on PWM value
        // This is a basic implementation - for production, consider hardware PWM
        static int pwmCounter = 0;
        pwmCounter = (pwmCounter + 1) % PWM_RANGE;

        bool pwmState = (pwmCounter < m_pwmValue);
        gpiod_line_set_value(m_pumpPwmLine, pwmState ? 1 : 0);
    }
}

bool ActuatorControl::initializeGPIO()
{
    try {
        // Open GPIO chip (usually gpiochip0 on Raspberry Pi)
        m_gpioChip = gpiod_chip_open_by_name("gpiochip0");
        if (!m_gpioChip) {
            throw std::runtime_error("Failed to open GPIO chip");
        }

        // Get GPIO lines for solenoid valves
        m_sol1Line = gpiod_chip_get_line(m_gpioChip, GPIO_SOL1);
        m_sol2Line = gpiod_chip_get_line(m_gpioChip, GPIO_SOL2);
        m_sol3Line = gpiod_chip_get_line(m_gpioChip, GPIO_SOL3);
        m_pumpEnableLine = gpiod_chip_get_line(m_gpioChip, GPIO_PUMP_ENABLE);
        m_pumpPwmLine = gpiod_chip_get_line(m_gpioChip, GPIO_PUMP_PWM);

        if (!m_sol1Line || !m_sol2Line || !m_sol3Line || !m_pumpEnableLine || !m_pumpPwmLine) {
            throw std::runtime_error("Failed to get GPIO lines");
        }

        // Request lines as outputs with initial low state
        if (gpiod_line_request_output(m_sol1Line, "VacuumController-SOL1", 0) < 0 ||
            gpiod_line_request_output(m_sol2Line, "VacuumController-SOL2", 0) < 0 ||
            gpiod_line_request_output(m_sol3Line, "VacuumController-SOL3", 0) < 0 ||
            gpiod_line_request_output(m_pumpEnableLine, "VacuumController-PumpEnable", 0) < 0 ||
            gpiod_line_request_output(m_pumpPwmLine, "VacuumController-PumpPWM", 0) < 0) {
            throw std::runtime_error("Failed to request GPIO lines as outputs");
        }

        qDebug() << "GPIO pins initialized using libgpiod";
        return true;

    } catch (const std::exception& e) {
        qCritical() << "GPIO initialization failed:" << e.what();
        return false;
    }
}

bool ActuatorControl::initializePWM()
{
    try {
        // PWM is handled through the GPIO line we already set up
        // For now, we'll use software PWM via the timer
        // Hardware PWM could be implemented later using /sys/class/pwm

        qDebug() << QString("PWM initialized on GPIO %1 (software PWM)")
                    .arg(GPIO_PUMP_PWM);
        return true;

    } catch (const std::exception& e) {
        qCritical() << "PWM initialization failed:" << e.what();
        return false;
    }
}

void ActuatorControl::setGPIOOutput(int pin, bool state)
{
    struct gpiod_line* line = nullptr;

    // Map pin number to GPIO line
    switch (pin) {
        case GPIO_SOL1: line = m_sol1Line; break;
        case GPIO_SOL2: line = m_sol2Line; break;
        case GPIO_SOL3: line = m_sol3Line; break;
        case GPIO_PUMP_ENABLE: line = m_pumpEnableLine; break;
        case GPIO_PUMP_PWM: line = m_pumpPwmLine; break;
        default:
            qWarning() << "Unknown GPIO pin:" << pin;
            return;
    }

    if (line && gpiod_line_set_value(line, state ? 1 : 0) < 0) {
        qWarning() << "Failed to set GPIO pin" << pin << "to" << state;
    }
}

bool ActuatorControl::getGPIOState(int pin)
{
    struct gpiod_line* line = nullptr;

    // Map pin number to GPIO line
    switch (pin) {
        case GPIO_SOL1: line = m_sol1Line; break;
        case GPIO_SOL2: line = m_sol2Line; break;
        case GPIO_SOL3: line = m_sol3Line; break;
        case GPIO_PUMP_ENABLE: line = m_pumpEnableLine; break;
        case GPIO_PUMP_PWM: line = m_pumpPwmLine; break;
        default:
            qWarning() << "Unknown GPIO pin:" << pin;
            return false;
    }

    if (line) {
        int value = gpiod_line_get_value(line);
        return value > 0;
    }
    return false;
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
        setGPIOOutput(GPIO_PUMP_PWM, false);
    }
    
    qDebug() << "All actuators set to safe state";
}
