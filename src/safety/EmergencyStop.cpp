#include "EmergencyStop.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <gpiod.h>
#include <stdexcept>

// Define GPIO line offset
#define GPIO_EMERGENCY_BUTTON 21 // Example GPIO pin

EmergencyStop::EmergencyStop(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_active(false)
    , m_triggered(false)
    , m_triggerCount(0)
    , m_lastTriggerTime(0)
    , m_hardwareButtonEnabled(true)
    , m_lastButtonState(false)
    , m_buttonCheckTimer(new QTimer(this))
    , m_gpioChip(nullptr)
    , m_buttonRequest(nullptr)
{
    // Set up button checking timer
    m_buttonCheckTimer->setInterval(BUTTON_CHECK_INTERVAL_MS);
    connect(m_buttonCheckTimer, &QTimer::timeout, this, &EmergencyStop::checkHardwareButton);
}

EmergencyStop::~EmergencyStop()
{
    shutdown();
}

bool EmergencyStop::initialize()
{
    if (!m_hardware) {
        qCritical() << "Hardware manager not provided to EmergencyStop";
        return false;
    }
    
    try {
        // Initialize hardware button if enabled
        if (m_hardwareButtonEnabled) {
            initializeHardwareButton();
        }
        
        m_active = true;
        
        // Start button monitoring
        if (m_hardwareButtonEnabled) {
            m_buttonCheckTimer->start();
        }
        
        qDebug() << "Emergency stop system initialized";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "Emergency stop initialization failed:" << e.what();
        return false;
    }
}

void EmergencyStop::shutdown()
{
    if (!m_active) return;
    
    // Stop button monitoring
    m_buttonCheckTimer->stop();

    // Release GPIO resources (libgpiod v2.x)
    if (m_buttonRequest) {
        gpiod_line_request_release(m_buttonRequest);
        m_buttonRequest = nullptr;
    }
    if (m_gpioChip) {
        gpiod_chip_close(m_gpioChip);
        m_gpioChip = nullptr;
    }

    m_active = false;
    qDebug() << "Emergency stop system shutdown";
}

void EmergencyStop::trigger(const QString& reason)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_triggered) {
        qWarning() << "Emergency stop already triggered";
        return;
    }
    
    qCritical() << "EMERGENCY STOP TRIGGERED:" << reason;
    
    m_triggered = true;
    m_lastTriggerReason = reason;
    m_lastTriggerTime = QDateTime::currentMSecsSinceEpoch();
    m_triggerCount++;
    
    // Perform immediate emergency shutdown
    performEmergencyShutdown();
    
    emit emergencyStopTriggered(reason);
}

bool EmergencyStop::reset()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (!m_triggered) {
        return true;  // Already reset
    }
    
    // Validate reset conditions
    if (!validateResetConditions()) {
        qWarning() << "Emergency stop reset conditions not met";
        return false;
    }
    
    try {
        // Reset hardware to safe state
        if (m_hardware) {
            if (!m_hardware->resetEmergencyStop()) {
                qCritical() << "Hardware emergency stop reset failed";
                return false;
            }
        }
        
        m_triggered = false;
        m_lastTriggerReason.clear();
        
        qDebug() << "Emergency stop reset successfully";
        emit emergencyStopReset();
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "Emergency stop reset failed:" << e.what();
        return false;
    }
}

void EmergencyStop::setHardwareButtonEnabled(bool enabled)
{
    m_hardwareButtonEnabled = enabled;
    
    if (m_active) {
        if (enabled && !m_buttonCheckTimer->isActive()) {
            initializeHardwareButton();
            m_buttonCheckTimer->start();
        } else if (!enabled && m_buttonCheckTimer->isActive()) {
            m_buttonCheckTimer->stop();
        }
    }
    
    qDebug() << "Hardware emergency button" << (enabled ? "enabled" : "disabled");
}

void EmergencyStop::checkHardwareButton()
{
    if (!m_hardwareButtonEnabled || !m_active) return;
    
    try {
        if (!m_buttonRequest) return;

        // Read button state (assuming active low button)
        enum gpiod_line_value buttonValue = gpiod_line_request_get_value(m_buttonRequest, GPIO_EMERGENCY_BUTTON);
        bool currentButtonState = (buttonValue == GPIOD_LINE_VALUE_ACTIVE);  // Active low

        // Detect button press (transition from not pressed to pressed)
        if (currentButtonState && !m_lastButtonState) {
            qWarning() << "Hardware emergency button pressed";
            emit hardwareButtonPressed();
            trigger("Hardware emergency button pressed");
        }

        m_lastButtonState = currentButtonState;

    } catch (const std::exception& e) {
        qWarning() << "Error reading emergency button:" << e.what();
    }
}

void EmergencyStop::performEmergencyShutdown()
{
    qCritical() << "PERFORMING EMERGENCY SHUTDOWN";
    
    if (!m_hardware) {
        qCritical() << "No hardware manager available for emergency shutdown";
        return;
    }
    
    try {
        // By default, enter a seal-maintained safe state so the
        // outer AVL chamber remains attached unless a higher-level
        // safety manager escalates to full vent.
        m_hardware->enterSealMaintainedSafeState("Hardware emergency stop triggered");

        qDebug() << "Emergency shutdown (seal-maintained) completed";

    } catch (const std::exception& e) {
        qCritical() << "Emergency shutdown failed:" << e.what();
    }
}

bool EmergencyStop::validateResetConditions()
{
    if (!m_hardware) {
        qCritical() << "No hardware manager for reset validation";
        return false;
    }
    
    // Check if hardware is in safe state
    if (!m_hardware->isReady()) {
        qWarning() << "Hardware not ready for emergency stop reset";
        return false;
    }
    
    // Additional safety checks could be added here
    // For example:
    // - Check all pressures are within safe limits
    // - Verify all valves are in safe positions
    // - Confirm no active alarms
    
    return true;
}

void EmergencyStop::initializeHardwareButton()
{
    try {
        // Open GPIO chip (libgpiod v2.x)
        m_gpioChip = gpiod_chip_open("/dev/gpiochip0");
        if (!m_gpioChip) {
            throw std::runtime_error("Failed to open GPIO chip");
        }

        // Create line request for the emergency button (libgpiod v2.x)
        struct gpiod_request_config *req_cfg = gpiod_request_config_new();
        if (!req_cfg) {
            throw std::runtime_error("Failed to create request config");
        }
        gpiod_request_config_set_consumer(req_cfg, "VacuumController-EmergencyStop");

        struct gpiod_line_settings *settings = gpiod_line_settings_new();
        if (!settings) {
            gpiod_request_config_free(req_cfg);
            throw std::runtime_error("Failed to create line settings");
        }
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
        gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_FALLING); // Trigger on press (active low)
        gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

        struct gpiod_line_config *line_cfg = gpiod_line_config_new();
        if (!line_cfg) {
            gpiod_line_settings_free(settings);
            gpiod_request_config_free(req_cfg);
            throw std::runtime_error("Failed to create line config");
        }
        unsigned int offsets[] = {GPIO_EMERGENCY_BUTTON};
        gpiod_line_config_add_line_settings(line_cfg, offsets, 1, settings);

        m_buttonRequest = gpiod_chip_request_lines(m_gpioChip, req_cfg, line_cfg);

        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        gpiod_request_config_free(req_cfg);

        if (!m_buttonRequest) {
            throw std::runtime_error("Failed to request GPIO line for emergency button");
        }

        // Read initial state
        enum gpiod_line_value initial_value = gpiod_line_request_get_value(m_buttonRequest, GPIO_EMERGENCY_BUTTON);
        m_lastButtonState = (initial_value == GPIOD_LINE_VALUE_ACTIVE);

        qDebug() << "Emergency button initialized on GPIO" << GPIO_EMERGENCY_BUTTON;

    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize emergency button:" << e.what();
        m_hardwareButtonEnabled = false;
        // Clean up on failure
        if (m_buttonRequest) gpiod_line_request_release(m_buttonRequest);
        if (m_gpioChip) gpiod_chip_close(m_gpioChip);
        m_buttonRequest = nullptr;
        m_gpioChip = nullptr;
    }
}
