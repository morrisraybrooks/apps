#ifndef EMERGENCYSTOP_H
#define EMERGENCYSTOP_H

#include <QObject>
#include <QMutex>
#include <QTimer>

// Forward declarations
class HardwareManager;
struct gpiod_chip;
struct gpiod_line_request;

/**
 * @brief Emergency stop system for vacuum controller
 * 
 * This class provides hardware and software emergency stop functionality
 * with immediate system shutdown and safe state recovery.
 * Implements both physical button and software-triggered emergency stops.
 */
class EmergencyStop : public QObject
{
    Q_OBJECT

public:
    explicit EmergencyStop(HardwareManager* hardware, QObject *parent = nullptr);
    ~EmergencyStop();

    // Initialization
    bool initialize();
    void shutdown();
    bool isActive() const { return m_active; }

    // Emergency stop control
    void trigger(const QString& reason);
    bool reset();
    bool isTriggered() const { return m_triggered; }
    
    // Status information
    QString getLastTriggerReason() const { return m_lastTriggerReason; }
    qint64 getLastTriggerTime() const { return m_lastTriggerTime; }
    int getTriggerCount() const { return m_triggerCount; }
    
    // Configuration
    void setHardwareButtonEnabled(bool enabled);
    bool isHardwareButtonEnabled() const { return m_hardwareButtonEnabled; }

Q_SIGNALS:
    void emergencyStopTriggered(const QString& reason);
    void emergencyStopReset();
    void hardwareButtonPressed();

private Q_SLOTS:
    void checkHardwareButton();

private:
    void performEmergencyShutdown();
    bool validateResetConditions();
    void initializeHardwareButton();

    // Hardware interface
    HardwareManager* m_hardware;
    
    // Emergency stop state
    bool m_active;
    bool m_triggered;
    mutable QMutex m_stateMutex;
    
    // Trigger information
    QString m_lastTriggerReason;
    qint64 m_lastTriggerTime;
    int m_triggerCount;
    
    // Hardware button support
    bool m_hardwareButtonEnabled;
    bool m_lastButtonState;
    QTimer* m_buttonCheckTimer;

    // GPIO control using libgpiod
    struct gpiod_chip* m_gpioChip;
    struct gpiod_line_request* m_buttonRequest; // For libgpiod v2.x
    
    // GPIO pin for emergency stop button (if implemented)
    static const int GPIO_EMERGENCY_BUTTON = 21;  // GPIO 21 for emergency button
    static const int BUTTON_CHECK_INTERVAL_MS = 50;  // 20Hz button checking
};

#endif // EMERGENCYSTOP_H
