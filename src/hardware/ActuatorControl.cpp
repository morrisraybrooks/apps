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
    , m_sol1Request(nullptr)
    , m_sol2Request(nullptr)
    , m_sol3Request(nullptr)
    , m_pumpEnableRequest(nullptr)
    , m_pumpPwmRequest(nullptr)
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

    // Release GPIO line requests (libgpiod v2.x)
    if (m_sol1Request) {
        gpiod_line_request_release(m_sol1Request);
        m_sol1Request = nullptr;
    }
    if (m_sol2Request) {
        gpiod_line_request_release(m_sol2Request);
        m_sol2Request = nullptr;
    }
    if (m_sol3Request) {
        gpiod_line_request_release(m_sol3Request);
        m_sol3Request = nullptr;
    }
    if (m_pumpEnableRequest) {
        gpiod_line_request_release(m_pumpEnableRequest);
        m_pumpEnableRequest = nullptr;
    }
    if (m_pumpPwmRequest) {
        gpiod_line_request_release(m_pumpPwmRequest);
        m_pumpPwmRequest = nullptr;
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
    if (m_initialized && m_pumpEnabled && !m_emergencyStop && m_pumpPwmRequest) {
        // Simple software PWM: toggle GPIO based on PWM value
        // This is a basic implementation - for production, consider hardware PWM
        static int pwmCounter = 0;
        pwmCounter = (pwmCounter + 1) % PWM_RANGE;

        bool pwmState = (pwmCounter < m_pwmValue);
        if (m_pumpPwmRequest) {
            enum gpiod_line_value value = pwmState ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
            gpiod_line_request_set_value(m_pumpPwmRequest, GPIO_PUMP_PWM, value);
        }
    }
}

bool ActuatorControl::initializeGPIO()
{
    try {
        // Open GPIO chip (usually gpiochip0 on Raspberry Pi) - libgpiod v2.x
        m_gpioChip = gpiod_chip_open("/dev/gpiochip0");
        if (!m_gpioChip) {
            throw std::runtime_error("Failed to open GPIO chip");
        }

        // Create individual requests for each line (libgpiod v2.x approach)
        // Helper function to create a single line request
        auto createLineRequest = [this](unsigned int offset, const char* consumer) -> struct gpiod_line_request* {
            struct gpiod_request_config *req_cfg = gpiod_request_config_new();
            if (!req_cfg) return nullptr;

            gpiod_request_config_set_consumer(req_cfg, consumer);

            struct gpiod_line_settings *settings = gpiod_line_settings_new();
            if (!settings) {
                gpiod_request_config_free(req_cfg);
                return nullptr;
            }

            gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
            gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

            struct gpiod_line_config *line_cfg = gpiod_line_config_new();
            if (!line_cfg) {
                gpiod_line_settings_free(settings);
                gpiod_request_config_free(req_cfg);
                return nullptr;
            }

            gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

            struct gpiod_line_request *request = gpiod_chip_request_lines(m_gpioChip, req_cfg, line_cfg);

            gpiod_line_config_free(line_cfg);
            gpiod_line_settings_free(settings);
            gpiod_request_config_free(req_cfg);

            return request;
        };

        // Create requests for each GPIO line
        m_sol1Request = createLineRequest(GPIO_SOL1, "VacuumController-SOL1");
        m_sol2Request = createLineRequest(GPIO_SOL2, "VacuumController-SOL2");
        m_sol3Request = createLineRequest(GPIO_SOL3, "VacuumController-SOL3");
        m_pumpEnableRequest = createLineRequest(GPIO_PUMP_ENABLE, "VacuumController-PumpEnable");
        m_pumpPwmRequest = createLineRequest(GPIO_PUMP_PWM, "VacuumController-PumpPWM");

        if (!m_sol1Request || !m_sol2Request || !m_sol3Request || !m_pumpEnableRequest || !m_pumpPwmRequest) {
            throw std::runtime_error("Failed to create individual line requests");
        }

        qDebug() << "GPIO pins initialized using libgpiod v2.2.1";
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
    struct gpiod_line_request* request = nullptr;

    // Map pin number to GPIO line request (libgpiod v2.x)
    switch (pin) {
        case GPIO_SOL1: request = m_sol1Request; break;
        case GPIO_SOL2: request = m_sol2Request; break;
        case GPIO_SOL3: request = m_sol3Request; break;
        case GPIO_PUMP_ENABLE: request = m_pumpEnableRequest; break;
        case GPIO_PUMP_PWM: request = m_pumpPwmRequest; break;
        default:
            qWarning() << "Unknown GPIO pin:" << pin;
            return;
    }

    if (request) {
        enum gpiod_line_value value = state ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
        if (gpiod_line_request_set_value(request, pin, value) < 0) {
            qWarning() << "Failed to set GPIO pin" << pin << "to" << state;
        }
    }
}

bool ActuatorControl::getGPIOState(int pin)
{
    struct gpiod_line_request* request = nullptr;

    // Map pin number to GPIO line request (libgpiod v2.x)
    switch (pin) {
        case GPIO_SOL1: request = m_sol1Request; break;
        case GPIO_SOL2: request = m_sol2Request; break;
        case GPIO_SOL3: request = m_sol3Request; break;
        case GPIO_PUMP_ENABLE: request = m_pumpEnableRequest; break;
        case GPIO_PUMP_PWM: request = m_pumpPwmRequest; break;
        default:
            qWarning() << "Unknown GPIO pin:" << pin;
            return false;
    }

    if (request) {
        enum gpiod_line_value value = gpiod_line_request_get_value(request, pin);
        return value == GPIOD_LINE_VALUE_ACTIVE;
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
