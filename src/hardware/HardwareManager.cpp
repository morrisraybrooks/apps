#include "HardwareManager.h"
#include "SensorInterface.h"
#include "ActuatorControl.h"
#include "MCP3008.h"
#include "TENSController.h"
#include "FluidSensor.h"
#include "MotionSensor.h"
#include "ClitoralOscillator.h"
#include "CameraManager.h"
#include "ConsentManager.h"
#include "IPneumaticHardware.h"
#include "PneumaticController.h"
#include "PneumaticSettings.h"

#include <QDebug>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <algorithm>
#include <limits>
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
    , m_tensEnableState(false)
    , m_tensPhaseState(false)
    , m_tensPWMValue(0)
    , m_urethralEnableState(false)
    , m_urethralPhaseState(false)
    , m_urethralPWMValue(0)
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

            createPneumatics();
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

        // Air side controller (tank, chambers, seal watch, pulser)
        createPneumatics();

        // Create and initialize clitoral oscillator (for air-pulse stimulation)
        m_clitoralOscillator = std::make_unique<ClitoralOscillator>(this, this);
        qDebug() << "Clitoral Oscillator initialized";

        // Connect ClitoralOscillator vacuum phases to TENSController for synchronization
        // This enables SYNC_SUCTION, SYNC_VENT, and ALTERNATING modes
        if (m_tensController && m_clitoralOscillator) {
            connect(m_clitoralOscillator.get(), &ClitoralOscillator::vacuumPhaseChanged,
                    m_tensController.get(), &TENSController::onVacuumPhaseChanged);
            qDebug() << "TENS/Vacuum synchronization connected";
        }

        // Create consent manager (must be before camera manager)
        m_consentManager = std::make_unique<ConsentManager>(this);
        qDebug() << "Consent Manager initialized";

        // Create and initialize camera manager (for visual motion detection)
        m_cameraManager = std::make_unique<CameraManager>(this);
        if (m_consentManager->hasConsent(ConsentManager::ConsentType::CAMERA_BODY) ||
            m_consentManager->hasConsent(ConsentManager::ConsentType::CAMERA_CUP)) {
            if (!m_cameraManager->initialize()) {
                qWarning() << "Camera Manager initialization failed - continuing without camera motion detection";
            } else {
                connect(m_cameraManager.get(), &CameraManager::cameraError,
                        this, [this](CameraMotionSensor::CameraRole role, const QString& error) {
                    QString roleStr = (role == CameraMotionSensor::CameraRole::PATIENT_MONITOR) ? "Body" : "Cup";
                    emit hardwareError(QString("%1 camera error: %2").arg(roleStr, error));
                });
                qDebug() << "Camera Manager initialized";
            }
        } else {
            qDebug() << "Camera Manager created but not initialized (no consent)";
        }

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

    // Air side first: pulser and pump off, chambers vented (tank keeps its vacuum)
    if (m_pneumatics) {
        m_pneumatics->stop();
        m_pneumatics.reset();
    }
    
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
    // Spec 5.5 (owner-approved 2026-11-16): any fault vents BOTH chambers.
    // Kept as a separate entry point so existing callers need no change.
    enterFullVentState(reason);
}

void HardwareManager::enterFullVentState(const QString& reason)
{
    // Pulser off, feeds closed, vents open, pump off, tank vent open (spec 5.5).
    // Must run before the lock below: the controller's writes take m_stateMutex.
    if (m_pneumatics) {
        m_pneumatics->emergencyStop();
    }

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
        if (m_pneumatics) m_pneumatics->rearm();  // vacuum requests may start the air side again
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

// Low-level TENS GPIO Control Methods

void HardwareManager::setTENSOutputEnable(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);

    if (m_emergencyStop && enabled) {
        qWarning() << "Cannot enable TENS output: Emergency stop active";
        return;
    }

    m_tensEnableState = enabled;

    if (!m_simulationMode && m_actuatorControl) {
        // Use ActuatorControl to set GPIO_TENS_ENABLE (GPIO 5)
        m_actuatorControl->setGPIO(GPIO_TENS_ENABLE, enabled);
    }

    qDebug() << "TENS output enable:" << (enabled ? "ON" : "OFF");
}

void HardwareManager::setTENSPhasePolarity(bool positive)
{
    QMutexLocker locker(&m_stateMutex);

    m_tensPhaseState = positive;

    if (!m_simulationMode && m_actuatorControl) {
        // Use ActuatorControl to set GPIO_TENS_PHASE (GPIO 6)
        // HIGH = positive phase, LOW = negative phase
        m_actuatorControl->setGPIO(GPIO_TENS_PHASE, positive);
    }
}

void HardwareManager::setTENSPWMDutyCycle(int pwmValue)
{
    QMutexLocker locker(&m_stateMutex);

    // Clamp to valid range (0-1024 for 10-bit PWM)
    m_tensPWMValue = std::clamp(pwmValue, 0, 1024);

    if (!m_simulationMode && m_actuatorControl) {
        // Use ActuatorControl to set hardware PWM on GPIO_TENS_PWM (GPIO 12)
        // GPIO 12 is hardware PWM channel 0 on Raspberry Pi
        m_actuatorControl->setPWM(GPIO_TENS_PWM, m_tensPWMValue);
    }
}

bool HardwareManager::readTENSFaultPin() const
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        // In simulation mode, return fault if simulated
        return m_simulatedFailures.contains("TENS");
    }

    if (m_actuatorControl) {
        // Read GPIO_TENS_FAULT (GPIO 16) - active low fault signal
        return m_actuatorControl->readGPIO(GPIO_TENS_FAULT);
    }

    return false;  // No fault if no actuator control
}

// ============================================================================
// URETHRAL ELECTRODE GPIO CONTROL
// ============================================================================

void HardwareManager::setUrethralOutputEnable(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    m_urethralEnableState = enabled;

    if (m_actuatorControl && !m_simulationMode) {
        m_actuatorControl->setGPIO(GPIO_URETHRAL_ENABLE, enabled);
    }
}

void HardwareManager::setUrethralPhasePolarity(bool positive)
{
    QMutexLocker locker(&m_stateMutex);
    m_urethralPhaseState = positive;

    if (m_actuatorControl && !m_simulationMode) {
        m_actuatorControl->setGPIO(GPIO_URETHRAL_PHASE, positive);
    }
}

void HardwareManager::setUrethralPWMDutyCycle(int pwmValue)
{
    QMutexLocker locker(&m_stateMutex);

    // Clamp to valid range (0-1024 for 10-bit PWM)
    m_urethralPWMValue = std::clamp(pwmValue, 0, 1024);

    if (m_actuatorControl && !m_simulationMode) {
        // GPIO 13 is hardware PWM channel 1 on Raspberry Pi
        m_actuatorControl->setPWM(GPIO_URETHRAL_PWM, m_urethralPWMValue);
    }
}

bool HardwareManager::readUrethralFaultPin() const
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        return m_simulatedFailures.contains("URETHRAL");
    }

    if (m_actuatorControl) {
        return m_actuatorControl->readGPIO(GPIO_URETHRAL_FAULT);
    }

    return false;
}

double HardwareManager::readUrethralImpedance() const
{
    QMutexLocker locker(&m_stateMutex);

    if (m_simulationMode) {
        // Simulate good electrode contact (low impedance)
        return m_simulatedFailures.contains("URETHRAL_CONTACT") ? 10000.0 : 500.0;
    }

    if (m_adc) {
        // Read impedance measurement from ADC channel 3
        // The impedance measurement circuit outputs a voltage proportional to impedance
        // Typical range: 0-5000 ohms maps to 0-3.3V
        uint16_t rawValue = m_adc->readRawValue(ADC_CHANNEL_URETHRAL_IMPEDANCE);
        // Convert 10-bit ADC value (0-1023) to impedance (0-10000 ohms)
        double impedance = (rawValue / 1023.0) * 10000.0;
        return impedance;
    }

    return 10000.0;  // Return high impedance if no ADC (indicates no contact)
}

// ============================================================================
// AIR SIDE: PNEUMATIC CONTROLLER
// ============================================================================

namespace {

/// Real-machine adapter for PneumaticController. A failed read returns NaN,
/// which makes the controller stop (never treat a dead sensor as 0 mmHg).
class HardwarePneumatics : public IPneumaticHardware
{
public:
    explicit HardwarePneumatics(HardwareManager* hw) : m_hw(hw) {}

    double legMmHg() override { return read([this] { return m_hw->readAVLPressure(); }); }
    double tankMmHg() override { return read([this] { return m_hw->readTankPressure(); }); }
    double clitMmHg() override { return read([this] { return m_hw->readClitoralPressure(); }); }

    void setPump(bool on) override
    {
        m_hw->setPumpEnabled(on);
        m_hw->setPumpSpeed(on ? 100.0 : 0.0);
    }

    void setValve(int sol, bool open) override
    {
        switch (sol) {
        case 1: m_hw->setSOL1(open); break;
        case 2: m_hw->setSOL2(open); break;
        case 3: m_hw->setSOL3(open); break;
        case 4: m_hw->setSOL4(open); break;
        case 5: m_hw->setSOL5(open); break;
        default: break;
        }
    }

    void setPulserDuty(double percent) override { m_hw->setPulserDuty(percent); }

private:
    template <typename F>
    double read(F f)
    {
        try {
            return f();
        } catch (const std::exception& e) {
            qWarning() << "Pressure read failed:" << e.what();
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    HardwareManager* m_hw;
};

QString findConfigFile(const QString& name)
{
    const QStringList candidates = {
        "config/" + name,
        "../config/" + name,
        "/usr/local/share/vacuum-controller/" + name,
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + name,
    };
    for (const QString& path : candidates) {
        if (QFile::exists(path)) return path;
    }
    return "config/" + name;
}

}  // namespace

void HardwareManager::createPneumatics()
{
    PneumaticSettings settings;
    QFile settingsFile(findConfigFile("settings.json"));
    if (settingsFile.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(settingsFile.readAll()).object();
        settings = PneumaticSettings::fromJson(root.value("pneumatics").toObject());
    } else {
        qWarning() << "settings.json not found - using default pneumatic settings";
    }
    settings.benchMode = m_pneumaticsBenchMode;

    m_pneumaticsHw = std::make_unique<HardwarePneumatics>(this);
    m_pneumatics = std::make_unique<PneumaticController>(m_pneumaticsHw.get(), settings);

    const QString calibration = findConfigFile("pulser_calibration.json");
    if (m_pneumatics->loadCalibration(calibration)) {
        qDebug() << "Pulser calibration loaded from" << calibration;
    } else {
        qWarning() << "Clitoral pulser unavailable until it is calibrated on the bench"
                   << "(docs/pulser-bench-calibration.md)";
    }
}

void HardwareManager::setPulserDuty(double percent)
{
    QMutexLocker locker(&m_stateMutex);
    if (m_emergencyStop && percent > 0.0) {
        qWarning() << "Cannot run the pulser: Emergency stop active";
        return;
    }
    if (m_simulationMode) {
        m_simulatedPulserDuty = std::clamp(percent, 0.0, 100.0);
        return;
    }
    if (m_actuatorControl) {
        m_actuatorControl->setPulserDuty(percent);
    }
}

double HardwareManager::getPulserDuty() const
{
    QMutexLocker locker(&m_stateMutex);
    if (m_simulationMode) return m_simulatedPulserDuty;
    return m_actuatorControl ? m_actuatorControl->getPulserDuty() : 0.0;
}

double HardwareManager::sampleClitoralSwing(int durationMs)
{
    if (m_simulationMode || !m_sensorInterface) return 0.0;

    double low = std::numeric_limits<double>::max();
    double high = std::numeric_limits<double>::lowest();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < durationMs) {
        const double p = m_sensorInterface->readClitoralPressure();  // raw ADC read, unfiltered
        if (p < 0.0) continue;  // read error value
        low = std::min(low, p);
        high = std::max(high, p);
    }
    return high > low ? high - low : 0.0;
}
