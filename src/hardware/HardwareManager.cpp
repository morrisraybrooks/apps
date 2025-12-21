#include "HardwareManager.h"
#include "SensorInterface.h"
#include "ActuatorControl.h"
#include "MCP3008.h"
#include "TENSController.h"
#include "FluidSensor.h"
#include "MotionSensor.h"
#include "ClitoralOscillator.h"

#include <QDebug>
#include <QMutexLocker>
// Modern GPIO library using libgpiod
#include <gpiod.h>
#include <stdexcept>

HardwareManager::HardwareManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_emergencyStop(false)
    , m_pumpEnabled(false)
    , m_pumpSpeed(0.0)
    , m_sol1State(false)
    , m_sol2State(false)
    , m_sol3State(false)
    , m_sol4State(false)
    , m_sol5State(false)
    , m_simulationMode(false)
    , m_simulatedAVLPressure(0.0)
    , m_simulatedTankPressure(0.0)
    , m_simulatedClitoralPressure(0.0)
{
}

HardwareManager::~HardwareManager()
{
    shutdown();
}

bool HardwareManager::initialize()
{
    try {
        qDebug() << "Initializing Hardware Manager...";

        // In simulation mode we deliberately avoid touching any real hardware
        // interfaces (GPIO, SPI, ADC, etc.). The safety and test harnesses
        // only depend on the logical state (simulated pressures, pump speed,
        // emergency flags), so we can short‑circuit initialization here.
        if (m_simulationMode) {
            qDebug() << "Hardware Manager running in SIMULATION mode - skipping physical GPIO/SPI/ADC initialization";

            // Do not create MCP3008, SensorInterface, ActuatorControl, or
            // other physical peripherals. All reads will use the
            // m_simulated* members and logical state maintained by the
            // setter methods below.

            m_initialized = true;
            return true;
        }
        
        // Initialize GPIO using libgpiod
        qDebug() << "Initializing GPIO using libgpiod...";

        // Note: GPIO initialization will be handled by individual components
        // that need GPIO access (ActuatorControl, EmergencyStop, etc.)
        
        // Initialize GPIO pins
        initializeGPIO();
        
        // Initialize SPI communication
        initializeSPI();
        
        // Create and initialize MCP3008 ADC
        m_adc = std::make_unique<MCP3008>();
        if (!m_adc->initialize()) {
            throw std::runtime_error("Failed to initialize MCP3008 ADC");
        }
        
        // Create and initialize sensor interface
        m_sensorInterface = std::make_unique<SensorInterface>(m_adc.get());
        if (!m_sensorInterface->initialize()) {
            throw std::runtime_error("Failed to initialize sensor interface");
        }
        
        // Create and initialize actuator control
        m_actuatorControl = std::make_unique<ActuatorControl>();
        if (!m_actuatorControl->initialize()) {
            throw std::runtime_error("Failed to initialize actuator control");
        }
        
        // Connect signals
        connect(m_sensorInterface.get(), &SensorInterface::sensorError,
                this, &HardwareManager::handleSensorError);
        connect(m_actuatorControl.get(), &ActuatorControl::actuatorError,
                this, &HardwareManager::handleActuatorError);

        // Create and initialize TENS controller (integrated into clitoral cup)
        m_tensController = std::make_unique<TENSController>(this);
        if (!m_tensController->initialize()) {
            qWarning() << "TENS Controller initialization failed - continuing without TENS";
            // Note: TENS is optional, don't fail initialization if it's not available
        } else {
            connect(m_tensController.get(), &TENSController::faultDetected,
                    this, [this](const QString& reason) {
                emit hardwareError(QString("TENS fault: %1").arg(reason));
            });
            qDebug() << "TENS Controller initialized for clitoral cup electrodes";
        }

        // Create and initialize fluid sensor (for fluid collection measurement)
        m_fluidSensor = std::make_unique<FluidSensor>(FluidSensor::SensorType::LOAD_CELL_HX711, this);
        if (!m_fluidSensor->initialize()) {
            qWarning() << "Fluid Sensor initialization failed - continuing without fluid measurement";
            // Note: Fluid sensor is optional
        } else {
            connect(m_fluidSensor.get(), &FluidSensor::sensorError,
                    this, [this](const QString& error) {
                emit hardwareError(QString("Fluid sensor error: %1").arg(error));
            });
            connect(m_fluidSensor.get(), &FluidSensor::overflowCritical,
                    this, [this](double volumeMl) {
                emit hardwareError(QString("Fluid overflow critical: %1 mL").arg(volumeMl, 0, 'f', 1));
            });
            qDebug() << "Fluid Sensor initialized (HX711 load cell)";
        }

        // Create and initialize motion sensor (for stillness detection)
        m_motionSensor = std::make_unique<MotionSensor>(MotionSensor::SensorType::MPU6050_I2C, this);
        if (!m_motionSensor->initialize()) {
            qWarning() << "Motion Sensor initialization failed - continuing without motion detection";
            // Note: Motion sensor is optional
        } else {
            connect(m_motionSensor.get(), &MotionSensor::sensorError,
                    this, [this](const QString& error) {
                emit hardwareError(QString("Motion sensor error: %1").arg(error));
            });
            qDebug() << "Motion Sensor initialized (MPU6050 IMU)";
        }

        // Create and initialize clitoral oscillator (for air-pulse stimulation)
        m_clitoralOscillator = std::make_unique<ClitoralOscillator>(this, this);
        qDebug() << "Clitoral Oscillator initialized";

        // Perform hardware validation
        if (!validateHardware()) {
            throw std::runtime_error("Hardware validation failed");
        }
        
        m_initialized = true;
        qDebug() << "Hardware Manager initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Hardware initialization failed: %1").arg(e.what());
        qCritical() << m_lastError;
        safeShutdown();
        return false;
    }
}

void HardwareManager::shutdown()
{
    if (!m_initialized) return;
    
    qDebug() << "Shutting down Hardware Manager...";
    
    // Safe shutdown sequence
    safeShutdown();
    
    // Shutdown subsystems
    if (m_tensController) {
        m_tensController->shutdown();
    }
    if (m_actuatorControl) {
        m_actuatorControl->shutdown();
    }
    if (m_sensorInterface) {
        m_sensorInterface->shutdown();
    }
    if (m_adc) {
        m_adc->shutdown();
    }

    // Reset subsystems
    m_tensController.reset();
    m_actuatorControl.reset();
    m_sensorInterface.reset();
    m_adc.reset();
    
    m_initialized = false;
    qDebug() << "Hardware Manager shutdown complete";
}

double HardwareManager::readAVLPressure()
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        return m_simulatedAVLPressure;
    }

    if (!m_sensorInterface) {
        throw std::runtime_error("Sensor interface not initialized");
    }
    return m_sensorInterface->getFilteredAVLPressure();
}

double HardwareManager::readTankPressure()
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        return m_simulatedTankPressure;
    }

    if (!m_sensorInterface) {
        throw std::runtime_error("Sensor interface not initialized");
    }
    return m_sensorInterface->getFilteredTankPressure();
}

double HardwareManager::readClitoralPressure()
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        return m_simulatedClitoralPressure;
    }

    if (!m_sensorInterface) {
        throw std::runtime_error("Sensor interface not initialized");
    }
    return m_sensorInterface->getFilteredClitoralPressure();
}

double HardwareManager::readFluidVolumeMl()
{
    QMutexLocker locker(&m_stateMutex);

    if (!m_fluidSensor || !m_fluidSensor->isReady()) {
        return 0.0;
    }
    return m_fluidSensor->getCurrentVolumeMl();
}

double HardwareManager::readFluidFlowRate()
{
    QMutexLocker locker(&m_stateMutex);

    if (!m_fluidSensor || !m_fluidSensor->isReady()) {
        return 0.0;
    }
    return m_fluidSensor->getFlowRateMlPerMin();
}

double HardwareManager::readCumulativeFluidMl()
{
    QMutexLocker locker(&m_stateMutex);

    if (!m_fluidSensor || !m_fluidSensor->isReady()) {
        return 0.0;
    }
    return m_fluidSensor->getCumulativeVolumeMl();
}

void HardwareManager::setPumpSpeed(double speedPercent)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_emergencyStop) {
        qWarning() << "Cannot set pump speed: Emergency stop active";
        return;
    }

    // Always track requested pump speed locally so that safety logic and
    // tests (including simulation mode) can observe the commanded value
    // even when no physical ActuatorControl is present.
    m_pumpSpeed = speedPercent;

    // Only talk to real hardware when not in simulation mode.
    if (m_actuatorControl && !m_simulationMode) {
        m_actuatorControl->setPumpSpeed(speedPercent);
    }
}

void HardwareManager::setPumpEnabled(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_emergencyStop && enabled) {
        qWarning() << "Cannot enable pump: Emergency stop active";
        return;
    }

    // Track logical pump enable state even when no physical hardware is
    // present (e.g., in tests or full simulation mode).
    m_pumpEnabled = enabled;

    if (m_actuatorControl && !m_simulationMode) {
        m_actuatorControl->setPumpEnabled(enabled);
    }
}

void HardwareManager::setSOL1(bool open)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_emergencyStop && open) {
        qWarning() << "Cannot open SOL1: Emergency stop active";
        return;
    }
    
    if (m_actuatorControl) {
        m_actuatorControl->setSOL1(open);
        m_sol1State = open;
    }
}

void HardwareManager::setSOL2(bool open)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_actuatorControl) {
        m_actuatorControl->setSOL2(open);
        m_sol2State = open;
    }
}

void HardwareManager::setSOL3(bool open)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_actuatorControl) {
        m_actuatorControl->setSOL3(open);
        m_sol3State = open;
    }
}

void HardwareManager::setSOL4(bool open)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_emergencyStop && open) {
        qWarning() << "Cannot open SOL4: Emergency stop active";
        return;
    }

    if (m_actuatorControl) {
        m_actuatorControl->setSOL4(open);
        m_sol4State = open;
    }
}

void HardwareManager::setSOL5(bool open)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_actuatorControl) {
        m_actuatorControl->setSOL5(open);
        m_sol5State = open;
    }
}

void HardwareManager::emergencyStop()
{
    // Preserve legacy API but route through the seal-maintained safe state.
    enterSealMaintainedSafeState("HardwareManager::emergencyStop() invoked");
}

void HardwareManager::enterSealMaintainedSafeState(const QString& reason)
{
    QMutexLocker locker(&m_stateMutex);

    qWarning() << "HARDWARE SEAL-MAINTAINED SAFE STATE:" << reason;

    m_emergencyStop = true;

    // Immediately stop TENS (electrical safety first!)
    if (m_tensController) {
        m_tensController->emergencyStop();
    }

    // Stop pump and place valves into a state that vents inner circuits
    // while preserving the outer AVL seal.
    if (m_actuatorControl) {
        // Pump off
        m_actuatorControl->setPumpEnabled(false);
        m_actuatorControl->setPumpSpeed(0.0);

        // Outer V-seal chamber: close vacuum, keep vent closed (seal maintained)
        m_actuatorControl->setSOL1(false);  // No new vacuum to AVL
        m_actuatorControl->setSOL2(false);  // Do NOT vent AVL

        // Tank and clitoral circuit: fully vent
        m_actuatorControl->setSOL3(true);   // Tank vent open
        m_actuatorControl->setSOL4(false);  // Clitoral vacuum closed
        m_actuatorControl->setSOL5(true);   // Clitoral vent open
    }

    // Mirror state locally
    m_pumpEnabled = false;
    m_pumpSpeed = 0.0;
    m_sol1State = false;
    m_sol2State = false;
    m_sol3State = true;
    m_sol4State = false;
    m_sol5State = true;

    emit hardwareError("Seal-maintained safe state activated");
}

void HardwareManager::enterFullVentState(const QString& reason)
{
    QMutexLocker locker(&m_stateMutex);

    qCritical() << "HARDWARE FULL-VENT EMERGENCY STATE:" << reason;

    m_emergencyStop = true;

    // Immediately stop TENS (electrical safety first!)
    if (m_tensController) {
        m_tensController->emergencyStop();
    }

    // Use low-level actuator emergency stop to vent all chambers and stop pump
    if (m_actuatorControl) {
        m_actuatorControl->emergencyStop();
    }

    // Reflect expected valve and pump state after full-vent emergency
    m_pumpEnabled = false;
    m_pumpSpeed = 0.0;
    m_sol1State = false;  // Outer vacuum closed
    m_sol2State = true;   // Outer vent open
    m_sol3State = true;   // Tank vent open
    m_sol4State = false;  // Clitoral vacuum closed
    m_sol5State = true;   // Clitoral vent open

    emit hardwareError("Full-vent emergency state activated");
}

bool HardwareManager::resetEmergencyStop()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (!m_emergencyStop) {
        return true;  // Already reset
    }
    
    if (m_actuatorControl && m_actuatorControl->resetEmergencyStop()) {
        m_emergencyStop = false;
        qDebug() << "Hardware emergency stop reset";
        return true;
    }
    
    return false;
}

bool HardwareManager::performSelfTest()
{
    if (!m_initialized) {
        m_lastError = "Hardware not initialized";
        return false;
    }
    
    try {
        // Test ADC communication
        if (!m_adc || !m_adc->isReady()) {
            throw std::runtime_error("ADC not ready");
        }
        
        // Test sensor readings
        double avl = readAVLPressure();
        double tank = readTankPressure();
        
        if (avl < 0 || tank < 0) {
            throw std::runtime_error("Invalid sensor readings");
        }
        
        // Test actuator control
        if (!m_actuatorControl || !m_actuatorControl->performSelfTest()) {
            throw std::runtime_error("Actuator self-test failed");
        }
        
        qDebug() << "Hardware self-test passed";
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Self-test failed: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void HardwareManager::handleSensorError(const QString& error)
{
    emit sensorError("Sensor", error);
}

void HardwareManager::handleActuatorError(const QString& error)
{
    emit actuatorError("Actuator", error);
}

void HardwareManager::initializeGPIO()
{
    // GPIO initialization is handled by individual components
    qDebug() << "GPIO pins initialized";
}

void HardwareManager::initializeSPI()
{
    // SPI initialization is handled by MCP3008 class
    qDebug() << "SPI interface ready";
}

bool HardwareManager::validateHardware()
{
    // Perform basic hardware validation
    if (!m_adc || !m_adc->isReady()) {
        qCritical() << "ADC validation failed";
        return false;
    }
    
    if (!m_sensorInterface || !m_sensorInterface->isReady()) {
        qCritical() << "Sensor interface validation failed";
        return false;
    }
    
    if (!m_actuatorControl || !m_actuatorControl->isReady()) {
        qCritical() << "Actuator control validation failed";
        return false;
    }
    
    return true;
}

void HardwareManager::safeShutdown()
{
    // Ensure all actuators are in safe state
    m_pumpEnabled = false;
    m_pumpSpeed = 0.0;

    // Outer V-seal chamber: close vacuum, open vent
    m_sol1State = false; // Outer chamber vacuum closed
    m_sol2State = true;  // Outer chamber vent valve open
    m_sol3State = true;  // Tank vent valve open

    // Clitoral cylinder: close vacuum, open vent
    m_sol4State = false; // Clitoral cylinder vacuum closed
    m_sol5State = true;  // Clitoral cylinder vent valve open
}

void HardwareManager::setSimulationMode(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);

    m_simulationMode = enabled;

    if (enabled) {
        qDebug() << "Hardware simulation mode enabled";
        // Initialize simulated values for all three sensors
        m_simulatedAVLPressure = 0.0;
        m_simulatedTankPressure = 0.0;
        m_simulatedClitoralPressure = 0.0;
        m_simulatedFailures.clear();
    } else {
        qDebug() << "Hardware simulation mode disabled";
    }
}

void HardwareManager::setSimulatedPressure(double pressure)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        m_simulatedAVLPressure = pressure;
        m_simulatedTankPressure = pressure * 0.8;     // Tank typically lower
        m_simulatedClitoralPressure = pressure * 0.5; // Clitoral varies during oscillation
    }
}

void HardwareManager::setSimulatedSensorValues(double avlPressure, double tankPressure)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        m_simulatedAVLPressure = avlPressure;
        m_simulatedTankPressure = tankPressure;
    }
}

void HardwareManager::simulateHardwareFailure(const QString& component)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        if (!m_simulatedFailures.contains(component)) {
            m_simulatedFailures.append(component);
            qDebug() << "Simulating hardware failure for:" << component;
            emit hardwareError(QString("Simulated failure: %1").arg(component));
        }
    }
}

void HardwareManager::simulateSensorError(const QString& sensor)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        qDebug() << "Simulating sensor error for:" << sensor;
        emit sensorError(sensor, "Simulated sensor error");
    }
}

void HardwareManager::resetHardwareSimulation()
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        m_simulatedAVLPressure = 0.0;
        m_simulatedTankPressure = 0.0;
        m_simulatedClitoralPressure = 0.0;
        m_simulatedFailures.clear();
        qDebug() << "Hardware simulation reset";
    }
}

// TENS Control Methods (integrated with clitoral cup electrodes)

void HardwareManager::setTENSEnabled(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);

    if (!m_tensController) {
        qWarning() << "TENS Controller not available";
        return;
    }

    if (m_emergencyStop && enabled) {
        qWarning() << "Cannot enable TENS: Emergency stop active";
        return;
    }

    if (enabled) {
        m_tensController->start();
    } else {
        m_tensController->stop();
    }
}

void HardwareManager::setTENSFrequency(double hz)
{
    if (m_tensController) {
        m_tensController->setFrequency(hz);
    }
}

void HardwareManager::setTENSPulseWidth(int microseconds)
{
    if (m_tensController) {
        m_tensController->setPulseWidth(microseconds);
    }
}

void HardwareManager::setTENSAmplitude(double percent)
{
    if (m_tensController) {
        m_tensController->setAmplitude(percent);
    }
}

bool HardwareManager::isTENSRunning() const
{
    return m_tensController && m_tensController->isRunning();
}

bool HardwareManager::isTENSFault() const
{
    return m_tensController && m_tensController->isFaultDetected();
}
