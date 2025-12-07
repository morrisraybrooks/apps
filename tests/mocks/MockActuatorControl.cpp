#include "MockActuatorControl.h"
#include <QDebug>
#include <QThread>
#include <QMutexLocker>

MockActuatorControl::MockActuatorControl(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_pumpState(false)
    , m_pumpPWM(0)
    , m_emergencyStop(false)
{
    // Initialize solenoid states
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
}

MockActuatorControl::~MockActuatorControl()
{
}

bool MockActuatorControl::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockActuatorControl: Initializing...";
    
    // Simulate initialization delay
    QThread::msleep(50);
    
    m_initialized = true;
    
    // Reset to safe state
    m_pumpState = false;
    m_pumpPWM = 0;
    m_emergencyStop = false;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    qDebug() << "MockActuatorControl: Initialization complete";
    return true;
}

void MockActuatorControl::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockActuatorControl: Shutting down...";
    
    // Turn off all actuators
    m_pumpState = false;
    m_pumpPWM = 0;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    m_initialized = false;
    
    qDebug() << "MockActuatorControl: Shutdown complete";
}

bool MockActuatorControl::setPump(bool enable)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        qDebug() << "MockActuatorControl: Not initialized";
        return false;
    }
    
    if (m_emergencyStop) {
        qDebug() << "MockActuatorControl: Cannot control pump - emergency stop active";
        return false;
    }
    
    m_pumpState = enable;
    qDebug() << "MockActuatorControl: Pump" << (enable ? "ON" : "OFF");
    
    emit pumpStateChanged(enable);
    return true;
}

bool MockActuatorControl::getPumpState() const
{
    QMutexLocker locker(&m_mutex);
    return m_pumpState;
}

bool MockActuatorControl::setPumpPWM(int percentage)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    if (m_emergencyStop) {
        return false;
    }
    
    if (percentage < 0 || percentage > 100) {
        qDebug() << "MockActuatorControl: Invalid PWM percentage:" << percentage;
        return false;
    }
    
    m_pumpPWM = percentage;
    qDebug() << "MockActuatorControl: Pump PWM set to" << percentage << "%";
    
    emit pumpPWMChanged(percentage);
    return true;
}

int MockActuatorControl::getPumpPWM() const
{
    QMutexLocker locker(&m_mutex);
    return m_pumpPWM;
}

bool MockActuatorControl::setSolenoid(int solenoidNumber, bool open)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        qDebug() << "MockActuatorControl: Not initialized";
        return false;
    }
    
    if (m_emergencyStop) {
        qDebug() << "MockActuatorControl: Cannot control solenoids - emergency stop active";
        return false;
    }
    
    if (solenoidNumber < 1 || solenoidNumber > 3) {
        qDebug() << "MockActuatorControl: Invalid solenoid number:" << solenoidNumber;
        return false;
    }
    
    m_solenoidStates[solenoidNumber] = open;
    qDebug() << "MockActuatorControl: Solenoid" << solenoidNumber << (open ? "OPEN" : "CLOSED");
    
    emit solenoidStateChanged(solenoidNumber, open);
    return true;
}

bool MockActuatorControl::getSolenoidState(int solenoidNumber) const
{
    QMutexLocker locker(&m_mutex);
    
    if (solenoidNumber < 1 || solenoidNumber > 3) {
        return false;
    }
    
    return m_solenoidStates.value(solenoidNumber, false);
}

bool MockActuatorControl::setAllSolenoids(bool sol1, bool sol2, bool sol3)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    if (m_emergencyStop) {
        return false;
    }
    
    m_solenoidStates[1] = sol1;
    m_solenoidStates[2] = sol2;
    m_solenoidStates[3] = sol3;
    
    qDebug() << "MockActuatorControl: All solenoids set to" << sol1 << sol2 << sol3;
    
    emit solenoidStateChanged(1, sol1);
    emit solenoidStateChanged(2, sol2);
    emit solenoidStateChanged(3, sol3);
    
    return true;
}

void MockActuatorControl::emergencyStop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockActuatorControl: EMERGENCY STOP TRIGGERED!";
    
    m_emergencyStop = true;
    
    // Immediately shut down all actuators
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

void MockActuatorControl::resetEmergencyStop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockActuatorControl: Emergency stop reset";
    m_emergencyStop = false;
    
    emit emergencyStopReset();
}

bool MockActuatorControl::isEmergencyStop() const
{
    QMutexLocker locker(&m_mutex);
    return m_emergencyStop;
}

bool MockActuatorControl::isInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

bool MockActuatorControl::performSelfTest()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockActuatorControl: Performing self-test...";
    
    if (!m_initialized) {
        qDebug() << "MockActuatorControl: Self-test FAILED - not initialized";
        return false;
    }
    
    // Simulate self-test delay
    QThread::msleep(300);
    
    // Test pump control
    bool originalPumpState = m_pumpState;
    int originalPumpPWM = m_pumpPWM;
    
    setPump(true);
    if (!m_pumpState) {
        qDebug() << "MockActuatorControl: Self-test FAILED - pump control";
        return false;
    }
    
    setPump(false);
    if (m_pumpState) {
        qDebug() << "MockActuatorControl: Self-test FAILED - pump control";
        return false;
    }
    
    // Test solenoid control
    QMap<int, bool> originalSolenoidStates = m_solenoidStates;
    
    for (int i = 1; i <= 3; ++i) {
        setSolenoid(i, true);
        if (!m_solenoidStates[i]) {
            qDebug() << "MockActuatorControl: Self-test FAILED - solenoid" << i << "control";
            return false;
        }
        
        setSolenoid(i, false);
        if (m_solenoidStates[i]) {
            qDebug() << "MockActuatorControl: Self-test FAILED - solenoid" << i << "control";
            return false;
        }
    }
    
    // Restore original states
    m_pumpState = originalPumpState;
    m_pumpPWM = originalPumpPWM;
    m_solenoidStates = originalSolenoidStates;
    
    qDebug() << "MockActuatorControl: Self-test PASSED";
    return true;
}

void MockActuatorControl::resetToSafeState()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockActuatorControl: Resetting to safe state...";
    
    m_pumpState = false;
    m_pumpPWM = 0;
    m_solenoidStates[1] = false;
    m_solenoidStates[2] = false;
    m_solenoidStates[3] = false;
    
    emit pumpStateChanged(false);
    emit solenoidStateChanged(1, false);
    emit solenoidStateChanged(2, false);
    emit solenoidStateChanged(3, false);
    
    qDebug() << "MockActuatorControl: Safe state reset complete";
}

QMap<int, bool> MockActuatorControl::getAllSolenoidStates() const
{
    QMutexLocker locker(&m_mutex);
    return m_solenoidStates;
}

bool MockActuatorControl::testActuator(int actuatorId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized || m_emergencyStop) {
        return false;
    }
    
    qDebug() << "MockActuatorControl: Testing actuator" << actuatorId;
    
    // Simulate actuator test
    QThread::msleep(100);
    
    if (actuatorId == 0) { // Pump
        bool originalState = m_pumpState;
        setPump(true);
        QThread::msleep(50);
        setPump(originalState);
        return true;
    }
    else if (actuatorId >= 1 && actuatorId <= 3) { // Solenoids
        bool originalState = m_solenoidStates[actuatorId];
        setSolenoid(actuatorId, true);
        QThread::msleep(50);
        setSolenoid(actuatorId, originalState);
        return true;
    }
    
    return false;
}
