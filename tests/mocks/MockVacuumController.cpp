#include "MockVacuumController.h"
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>

MockVacuumController::MockVacuumController(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_systemReady(false)
    , m_emergencyStop(false)
    , m_currentPattern("")
    , m_patternRunning(false)
    , m_dataLogging(false)
    , m_calibrationMode(false)
    , m_safeMode(false)
{
}

MockVacuumController::~MockVacuumController()
{
}

bool MockVacuumController::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: Initializing system...";
    
    // Simulate initialization delay
    QThread::msleep(200);
    
    m_initialized = true;
    m_systemReady = true;
    m_emergencyStop = false;
    m_safeMode = false;
    
    qDebug() << "MockVacuumController: System initialization complete";
    
    emit systemInitialized();
    emit systemStatusChanged("READY");
    
    return true;
}

void MockVacuumController::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: Shutting down system...";
    
    // Stop all operations
    stopPattern();
    stopDataLogging();
    
    m_initialized = false;
    m_systemReady = false;
    
    qDebug() << "MockVacuumController: System shutdown complete";
    
    emit systemShutdown();
}

bool MockVacuumController::startPattern(const QString& patternName, const QJsonObject& parameters)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized || m_emergencyStop || m_safeMode) {
        qDebug() << "MockVacuumController: Cannot start pattern - system not ready";
        return false;
    }
    
    // Validate pattern parameters
    if (!validatePatternParameters(parameters)) {
        qDebug() << "MockVacuumController: Invalid pattern parameters";
        return false;
    }
    
    // Stop any existing pattern
    if (m_patternRunning) {
        stopPattern();
    }
    
    m_currentPattern = patternName;
    m_patternRunning = true;
    m_patternParameters = parameters;
    
    qDebug() << "MockVacuumController: Started pattern" << patternName;
    
    emit patternStarted(patternName);
    emit patternStatusChanged(patternName, "RUNNING");
    
    return true;
}

bool MockVacuumController::stopPattern()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_patternRunning) {
        return true;
    }
    
    QString stoppedPattern = m_currentPattern;
    m_currentPattern.clear();
    m_patternRunning = false;
    m_patternParameters = QJsonObject();
    
    qDebug() << "MockVacuumController: Stopped pattern" << stoppedPattern;
    
    emit patternStopped(stoppedPattern);
    emit patternStatusChanged(stoppedPattern, "STOPPED");
    
    return true;
}

bool MockVacuumController::isPatternRunning(const QString& patternName) const
{
    QMutexLocker locker(&m_mutex);
    
    if (patternName.isEmpty()) {
        return m_patternRunning;
    }
    
    return m_patternRunning && (m_currentPattern == patternName);
}

QString MockVacuumController::getCurrentPattern() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentPattern;
}

bool MockVacuumController::startDataLogging()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    m_dataLogging = true;
    m_loggedData.clear();
    
    qDebug() << "MockVacuumController: Data logging started";
    
    emit dataLoggingStarted();
    
    return true;
}

bool MockVacuumController::stopDataLogging()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_dataLogging) {
        return true;
    }
    
    m_dataLogging = false;
    
    qDebug() << "MockVacuumController: Data logging stopped";
    
    emit dataLoggingStopped();
    
    return true;
}

bool MockVacuumController::isDataLogging() const
{
    QMutexLocker locker(&m_mutex);
    return m_dataLogging;
}

QStringList MockVacuumController::getLoggedData() const
{
    QMutexLocker locker(&m_mutex);
    return m_loggedData;
}

void MockVacuumController::addLogEntry(const QString& entry)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_dataLogging) {
        m_loggedData.append(entry);
        emit dataLogged(entry);
    }
}

void MockVacuumController::triggerEmergencyStop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: EMERGENCY STOP TRIGGERED!";
    
    m_emergencyStop = true;
    
    // Stop all operations immediately
    stopPattern();
    
    emit emergencyStopTriggered();
    emit systemStatusChanged("EMERGENCY_STOP");
}

void MockVacuumController::resetEmergencyStop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: Emergency stop reset";
    
    m_emergencyStop = false;
    
    if (m_initialized && !m_safeMode) {
        m_systemReady = true;
        emit systemStatusChanged("READY");
    }
    
    emit emergencyStopReset();
}

bool MockVacuumController::isEmergencyStop() const
{
    QMutexLocker locker(&m_mutex);
    return m_emergencyStop;
}

bool MockVacuumController::startCalibration()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized || m_emergencyStop) {
        return false;
    }
    
    // Stop any running operations
    stopPattern();
    
    m_calibrationMode = true;
    
    qDebug() << "MockVacuumController: Calibration mode started";
    
    emit calibrationStarted();
    emit systemStatusChanged("CALIBRATING");
    
    return true;
}

bool MockVacuumController::stopCalibration()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_calibrationMode) {
        return true;
    }
    
    m_calibrationMode = false;
    
    if (m_initialized && !m_emergencyStop && !m_safeMode) {
        m_systemReady = true;
        emit systemStatusChanged("READY");
    }
    
    qDebug() << "MockVacuumController: Calibration mode stopped";
    
    emit calibrationStopped();
    
    return true;
}

bool MockVacuumController::isCalibrationMode() const
{
    QMutexLocker locker(&m_mutex);
    return m_calibrationMode;
}

bool MockVacuumController::isSystemReady() const
{
    QMutexLocker locker(&m_mutex);
    return m_systemReady && !m_emergencyStop && !m_safeMode;
}

bool MockVacuumController::isSystemInSafeMode() const
{
    QMutexLocker locker(&m_mutex);
    return m_safeMode;
}

void MockVacuumController::enterSafeMode(const QString& reason)
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: Entering safe mode -" << reason;
    
    m_safeMode = true;
    m_systemReady = false;
    
    // Stop all operations
    stopPattern();
    
    emit safeModeEntered(reason);
    emit systemStatusChanged("SAFE_MODE");
}

void MockVacuumController::exitSafeMode()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: Exiting safe mode";
    
    m_safeMode = false;
    
    if (m_initialized && !m_emergencyStop) {
        m_systemReady = true;
        emit systemStatusChanged("READY");
    }
    
    emit safeModeExited();
}

bool MockVacuumController::performSystemSelfCheck()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockVacuumController: Performing system self-check...";
    
    if (!m_initialized) {
        qDebug() << "MockVacuumController: Self-check FAILED - not initialized";
        return false;
    }
    
    // Simulate self-check delay
    QThread::msleep(1000);
    
    // Check system state
    if (m_emergencyStop) {
        qDebug() << "MockVacuumController: Self-check FAILED - emergency stop active";
        return false;
    }
    
    qDebug() << "MockVacuumController: System self-check PASSED";
    
    emit selfCheckCompleted(true);
    
    return true;
}

QJsonObject MockVacuumController::getSystemStatus() const
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject status;
    status["initialized"] = m_initialized;
    status["ready"] = m_systemReady;
    status["emergency_stop"] = m_emergencyStop;
    status["safe_mode"] = m_safeMode;
    status["calibration_mode"] = m_calibrationMode;
    status["pattern_running"] = m_patternRunning;
    status["current_pattern"] = m_currentPattern;
    status["data_logging"] = m_dataLogging;
    
    return status;
}

bool MockVacuumController::validatePatternParameters(const QJsonObject& parameters) const
{
    // Check required fields
    if (!parameters.contains("type") || !parameters.contains("duration_ms")) {
        return false;
    }
    
    QString type = parameters["type"].toString();
    int duration = parameters["duration_ms"].toInt();
    
    // Validate duration
    if (duration <= 0 || duration > 300000) { // Max 5 minutes
        return false;
    }
    
    // Validate pressure if specified
    if (parameters.contains("pressure_mmhg")) {
        double pressure = parameters["pressure_mmhg"].toDouble();
        if (pressure < -150 || pressure > 50) { // Safety limits
            return false;
        }
    }
    
    // Type-specific validation
    if (type == "pulse") {
        if (!parameters.contains("pulse_width_ms") || !parameters.contains("pulse_interval_ms")) {
            return false;
        }
    }
    else if (type == "wave") {
        if (!parameters.contains("min_pressure_mmhg") || !parameters.contains("max_pressure_mmhg")) {
            return false;
        }
    }
    else if (type == "constant") {
        if (!parameters.contains("pressure_mmhg")) {
            return false;
        }
    }
    else {
        return false; // Unknown type
    }
    
    return true;
}
