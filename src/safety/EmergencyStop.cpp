#include "EmergencyStop.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <gpiod.h>
#include <stdexcept>

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
    , m_buttonLine(nullptr)
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

    // Release GPIO resources
    if (m_buttonLine) {
        gpiod_line_release(m_buttonLine);
        m_buttonLine = nullptr;
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
        if (!m_buttonLine) return;

        // Read button state (assuming active low button)
        int buttonValue = gpiod_line_get_value(m_buttonLine);
        bool currentButtonState = (buttonValue == 0);  // Active low

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
        // Immediately stop all hardware operations
        m_hardware->emergencyStop();
        
        qDebug() << "Emergency shutdown completed";
        
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
        // Open GPIO chip
        m_gpioChip = gpiod_chip_open_by_name("gpiochip0");
        if (!m_gpioChip) {
            throw std::runtime_error("Failed to open GPIO chip");
        }

        // Get GPIO line for emergency button
        m_buttonLine = gpiod_chip_get_line(m_gpioChip, GPIO_EMERGENCY_BUTTON);
        if (!m_buttonLine) {
            throw std::runtime_error("Failed to get GPIO line for emergency button");
        }

        // Request line as input with pull-up
        if (gpiod_line_request_input(m_buttonLine, "VacuumController-EmergencyStop") < 0) {
            throw std::runtime_error("Failed to request GPIO line as input");
        }

        // Read initial state (button is active low)
        m_lastButtonState = (gpiod_line_get_value(m_buttonLine) == 0);

        qDebug() << "Emergency button initialized on GPIO" << GPIO_EMERGENCY_BUTTON;

    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize emergency button:" << e.what();
        m_hardwareButtonEnabled = false;
    }
}
