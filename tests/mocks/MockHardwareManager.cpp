#include "MockHardwareManager.h"
#include <QDebug>
#include <QThread>
#include <QMutexLocker>

MockHardwareManager::MockHardwareManager(QObject *parent)
    : QObject(parent)
    , m_pumpState(false)
    , m_pumpPWM(0)
    , m_emergencyStop(false)
    , m_pressure1(-20.0)
    , m_pressure2(-25.0)
    , m_sensorError1(false)
    , m_sensorError2(false)
    , m_gpioInitialized(false)
    , m_spiInitialized(false)
{
    // Initialize solenoid states
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
}

MockHardwareManager::~MockHardwareManager()
{
}

bool MockHardwareManager::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockHardwareManager: Initializing...";
    
    // Simulate initialization delay
    QThread::msleep(100);
    
    m_gpioInitialized = true;
    m_spiInitialized = true;
    
    // Reset to safe state
    m_pumpState = false;
    m_pumpPWM = 0;
    m_emergencyStop = false;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    qDebug() << "MockHardwareManager: Initialization complete";
    return true;
}

void MockHardwareManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockHardwareManager: Shutting down...";
    
    // Turn off all hardware
    m_pumpState = false;
    m_pumpPWM = 0;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    m_gpioInitialized = false;
    m_spiInitialized = false;
    
    qDebug() << "MockHardwareManager: Shutdown complete";
}

bool MockHardwareManager::setPump(bool enable)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_emergencyStop) {
        qDebug() << "MockHardwareManager: Cannot control pump - emergency stop active";
        return false;
    }
    
    m_pumpState = enable;
    qDebug() << "MockHardwareManager: Pump" << (enable ? "ON" : "OFF");
    
    emit pumpStateChanged(enable);
    return true;
}

bool MockHardwareManager::getPumpState() const
{
    QMutexLocker locker(&m_mutex);
    return m_pumpState;
}

bool MockHardwareManager::setPumpPWM(int percentage)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_emergencyStop) {
        return false;
    }
    
    if (percentage < 0 || percentage > 100) {
        return false;
    }
    
    m_pumpPWM = percentage;
    qDebug() << "MockHardwareManager: Pump PWM set to" << percentage << "%";
    
    emit pumpPWMChanged(percentage);
    return true;
}

int MockHardwareManager::getPumpPWM() const
{
    QMutexLocker locker(&m_mutex);
    return m_pumpPWM;
}

bool MockHardwareManager::setSolenoid(int solenoidNumber, bool open)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_emergencyStop) {
        qDebug() << "MockHardwareManager: Cannot control solenoids - emergency stop active";
        return false;
    }
    
    if (solenoidNumber < 1 || solenoidNumber > 3) {
        qDebug() << "MockHardwareManager: Invalid solenoid number:" << solenoidNumber;
        return false;
    }
    
    m_solenoidStates[solenoidNumber] = open;
    qDebug() << "MockHardwareManager: Solenoid" << solenoidNumber << (open ? "OPEN" : "CLOSED");
    
    emit solenoidStateChanged(solenoidNumber, open);
    return true;
}

bool MockHardwareManager::getSolenoidState(int solenoidNumber) const
{
    QMutexLocker locker(&m_mutex);
    
    if (solenoidNumber < 1 || solenoidNumber > 3) {
        return false;
    }
    
    return m_solenoidStates.value(solenoidNumber, false);
}

bool MockHardwareManager::setAllSolenoids(bool sol1, bool sol2, bool sol3)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_emergencyStop) {
        return false;
    }
    
    m_solenoidStates[1] = sol1;
    m_solenoidStates[2] = sol2;
    m_solenoidStates[3] = sol3;
    
    qDebug() << "MockHardwareManager: All solenoids set to" << sol1 << sol2 << sol3;
    
    emit solenoidStateChanged(1, sol1);
    emit solenoidStateChanged(2, sol2);
    emit solenoidStateChanged(3, sol3);
    
    return true;
}

double MockHardwareManager::readPressureSensor(int sensorNumber)
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        if (m_sensorError1) {
            return -999.0; // Error value
        }
        return m_pressure1;
    } else if (sensorNumber == 2) {
        if (m_sensorError2) {
            return -999.0; // Error value
        }
        return m_pressure2;
    }
    
    return 0.0;
}

void MockHardwareManager::setPressureSensorValue(int sensorNumber, double pressure)
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        m_pressure1 = pressure;
        emit pressureChanged(1, pressure);
    } else if (sensorNumber == 2) {
        m_pressure2 = pressure;
        emit pressureChanged(2, pressure);
    }
    
    qDebug() << "MockHardwareManager: Sensor" << sensorNumber << "pressure set to" << pressure << "mmHg";
}

bool MockHardwareManager::isEmergencyStop() const
{
    QMutexLocker locker(&m_mutex);
    return m_emergencyStop;
}

void MockHardwareManager::triggerEmergencyStop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockHardwareManager: EMERGENCY STOP TRIGGERED!";
    
    m_emergencyStop = true;
    
    // Immediately shut down all hardware
    m_pumpState = false;
    m_pumpPWM = 0;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    emit emergencyStopTriggered();
    emit pumpStateChanged(false);
    emit solenoidStateChanged(1, false);
    emit solenoidStateChanged(2, false);
    emit solenoidStateChanged(3, false);
}

void MockHardwareManager::resetEmergencyStop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockHardwareManager: Emergency stop reset";
    m_emergencyStop = false;
    
    emit emergencyStopReset();
}

void MockHardwareManager::simulateSensorError(int sensorNumber, bool hasError)
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        m_sensorError1 = hasError;
    } else if (sensorNumber == 2) {
        m_sensorError2 = hasError;
    }
    
    qDebug() << "MockHardwareManager: Sensor" << sensorNumber << "error simulation" << (hasError ? "ON" : "OFF");
    
    emit sensorErrorChanged(sensorNumber, hasError);
}

bool MockHardwareManager::isSensorError(int sensorNumber) const
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        return m_sensorError1;
    } else if (sensorNumber == 2) {
        return m_sensorError2;
    }
    
    return false;
}

bool MockHardwareManager::isGPIOInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_gpioInitialized;
}

bool MockHardwareManager::isSPIInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_spiInitialized;
}

bool MockHardwareManager::performSelfTest()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockHardwareManager: Performing self-test...";
    
    // Simulate self-test delay
    QThread::msleep(500);
    
    // Check if hardware is initialized
    if (!m_gpioInitialized || !m_spiInitialized) {
        qDebug() << "MockHardwareManager: Self-test FAILED - hardware not initialized";
        return false;
    }
    
    // Check for sensor errors
    if (m_sensorError1 || m_sensorError2) {
        qDebug() << "MockHardwareManager: Self-test FAILED - sensor errors detected";
        return false;
    }
    
    qDebug() << "MockHardwareManager: Self-test PASSED";
    return true;
}

void MockHardwareManager::resetToSafeState()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockHardwareManager: Resetting to safe state...";
    
    m_pumpState = false;
    m_pumpPWM = 0;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    emit pumpStateChanged(false);
    emit solenoidStateChanged(1, false);
    emit solenoidStateChanged(2, false);
    emit solenoidStateChanged(3, false);
    
    qDebug() << "MockHardwareManager: Safe state reset complete";
}
