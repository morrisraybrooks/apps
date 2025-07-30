#include "VacuumController.h"
#include "hardware/HardwareManager.h"
#include "safety/SafetyManager.h"
#include "safety/AntiDetachmentMonitor.h"
#include "patterns/PatternEngine.h"
#include "threading/ThreadManager.h"

#include <QDebug>
#include <QMutexLocker>
#include <stdexcept>

VacuumController::VacuumController(QObject *parent)
    : QObject(parent)
    , m_systemState(STOPPED)
    , m_avlPressure(0.0)
    , m_tankPressure(0.0)
    , m_maxPressure(100.0)  // 100 mmHg max as per specification
    , m_antiDetachmentThreshold(50.0)  // Default threshold
    , m_updateTimer(new QTimer(this))
    , m_initialized(false)
{
    // Set up update timer for real-time monitoring
    m_updateTimer->setInterval(50);  // 20Hz update rate for smooth UI
    connect(m_updateTimer, &QTimer::timeout, this, &VacuumController::onUpdateTimer);
}

VacuumController::~VacuumController()
{
    shutdown();
}

bool VacuumController::initialize()
{
    try {
        qDebug() << "Initializing Vacuum Controller...";
        
        // Initialize subsystems
        initializeSubsystems();
        
        // Connect internal signals
        connectSignals();
        
        // Start monitoring timer
        m_updateTimer->start();
        
        setState(STOPPED);
        m_initialized = true;
        
        qDebug() << "Vacuum Controller initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Initialization failed: %1").arg(e.what());
        qCritical() << m_lastError;
        setState(ERROR);
        return false;
    }
}

void VacuumController::shutdown()
{
    if (!m_initialized) return;
    
    qDebug() << "Shutting down Vacuum Controller...";
    
    // Stop all operations
    emergencyStop();
    
    // Stop timer
    m_updateTimer->stop();
    
    // Shutdown subsystems in reverse order
    if (m_threadManager) {
        m_threadManager->stopAllThreads();
    }
    m_threadManager.reset();
    m_antiDetachmentMonitor.reset();
    m_patternEngine.reset();
    m_safetyManager.reset();
    m_hardwareManager.reset();
    
    m_initialized = false;
    qDebug() << "Vacuum Controller shutdown complete";
}

bool VacuumController::isSystemReady() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_initialized && 
           m_systemState != ERROR && 
           m_systemState != EMERGENCY_STOP &&
           m_hardwareManager && 
           m_hardwareManager->isReady();
}

void VacuumController::startPattern(const QString& patternName)
{
    if (!isSystemReady()) {
        emit systemError("System not ready to start pattern");
        return;
    }
    
    try {
        m_patternEngine->startPattern(patternName);
        setState(RUNNING);
        emit patternStarted(patternName);
        qDebug() << "Started pattern:" << patternName;
        
    } catch (const std::exception& e) {
        QString error = QString("Failed to start pattern %1: %2").arg(patternName, e.what());
        emit systemError(error);
    }
}

void VacuumController::stopPattern()
{
    if (m_patternEngine) {
        m_patternEngine->stopPattern();
    }
    setState(STOPPED);
    emit patternStopped();
    qDebug() << "Pattern stopped";
}

void VacuumController::pausePattern()
{
    if (m_systemState == RUNNING && m_patternEngine) {
        m_patternEngine->pausePattern();
        setState(PAUSED);
        qDebug() << "Pattern paused";
    }
}

void VacuumController::resumePattern()
{
    if (m_systemState == PAUSED && m_patternEngine) {
        m_patternEngine->resumePattern();
        setState(RUNNING);
        qDebug() << "Pattern resumed";
    }
}

void VacuumController::emergencyStop()
{
    qWarning() << "EMERGENCY STOP ACTIVATED";
    
    // Immediately stop all operations
    if (m_patternEngine) {
        m_patternEngine->emergencyStop();
    }
    
    if (m_hardwareManager) {
        m_hardwareManager->emergencyStop();
    }
    
    setState(EMERGENCY_STOP);
    emit emergencyStopTriggered();
}

void VacuumController::resetEmergencyStop()
{
    if (m_systemState == EMERGENCY_STOP) {
        if (m_hardwareManager && m_hardwareManager->resetEmergencyStop()) {
            setState(STOPPED);
            qDebug() << "Emergency stop reset";
        } else {
            emit systemError("Failed to reset emergency stop");
        }
    }
}

void VacuumController::setMaxPressure(double maxPressure)
{
    if (maxPressure > 0 && maxPressure <= 150.0) {  // Reasonable safety limit
        m_maxPressure = maxPressure;
        if (m_safetyManager) {
            m_safetyManager->setMaxPressure(maxPressure);
        }
        qDebug() << "Max pressure set to:" << maxPressure << "mmHg";
    }
}

void VacuumController::setAntiDetachmentThreshold(double threshold)
{
    if (threshold > 0 && threshold < m_maxPressure) {
        m_antiDetachmentThreshold = threshold;
        if (m_antiDetachmentMonitor) {
            m_antiDetachmentMonitor->setThreshold(threshold);
        }
        qDebug() << "Anti-detachment threshold set to:" << threshold << "mmHg";
    }
}

void VacuumController::updateSensorReadings()
{
    if (!m_hardwareManager) return;
    
    try {
        double avl = m_hardwareManager->readAVLPressure();
        double tank = m_hardwareManager->readTankPressure();
        
        {
            QMutexLocker locker(&m_dataMutex);
            m_avlPressure = avl;
            m_tankPressure = tank;
        }
        
        emit pressureUpdated(avl, tank);
        
    } catch (const std::exception& e) {
        emit systemError(QString("Sensor reading error: %1").arg(e.what()));
    }
}

void VacuumController::handleEmergencyStop()
{
    emergencyStop();
}

void VacuumController::handleSystemError(const QString& error)
{
    qCritical() << "System error:" << error;
    m_lastError = error;
    setState(ERROR);
    emit systemError(error);
}

void VacuumController::onUpdateTimer()
{
    updateSensorReadings();
}

void VacuumController::setState(SystemState newState)
{
    QMutexLocker locker(&m_stateMutex);
    if (m_systemState != newState) {
        m_systemState = newState;
        emit systemStateChanged(newState);
    }
}

void VacuumController::initializeSubsystems()
{
    // Initialize hardware manager first
    m_hardwareManager = std::make_unique<HardwareManager>();
    if (!m_hardwareManager->initialize()) {
        throw std::runtime_error("Failed to initialize hardware manager");
    }
    
    // Initialize safety manager
    m_safetyManager = std::make_unique<SafetyManager>(m_hardwareManager.get());
    m_safetyManager->setMaxPressure(m_maxPressure);
    
    // Initialize pattern engine
    m_patternEngine = std::make_unique<PatternEngine>(m_hardwareManager.get());
    
    // Initialize anti-detachment monitor
    m_antiDetachmentMonitor = std::make_unique<AntiDetachmentMonitor>(m_hardwareManager.get());
    m_antiDetachmentMonitor->setThreshold(m_antiDetachmentThreshold);

    // Initialize thread manager
    m_threadManager = std::make_unique<ThreadManager>(m_hardwareManager.get());
    if (!m_threadManager->startAllThreads()) {
        throw std::runtime_error("Failed to start system threads");
    }
}

void VacuumController::connectSignals()
{
    // Connect safety manager signals
    if (m_safetyManager) {
        connect(m_safetyManager.get(), &SafetyManager::emergencyStopTriggered,
                this, &VacuumController::handleEmergencyStop);
        connect(m_safetyManager.get(), &SafetyManager::systemError,
                this, &VacuumController::handleSystemError);
    }
    
    // Connect anti-detachment monitor
    if (m_antiDetachmentMonitor) {
        connect(m_antiDetachmentMonitor.get(), &AntiDetachmentMonitor::detachmentDetected,
                this, &VacuumController::antiDetachmentActivated);
    }
}
