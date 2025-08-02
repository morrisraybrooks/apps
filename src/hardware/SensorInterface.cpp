#include "SensorInterface.h"
#include "MCP3008.h"
#include <QDebug>
#include <QMutexLocker>
#include <cmath>
#include <algorithm>

// Constants
const double SensorInterface::DEFAULT_MIN_VOLTAGE = 0.1;  // Below MPX5010DP range
const double SensorInterface::DEFAULT_MAX_VOLTAGE = 5.0;  // Above MPX5010DP range

SensorInterface::SensorInterface(MCP3008* adc, QObject *parent)
    : QObject(parent)
    , m_adc(adc)
    , m_initialized(false)
    , m_currentAVL(0.0)
    , m_currentTank(0.0)
    , m_filteredAVL(0.0)
    , m_filteredTank(0.0)
    , m_avlSensorHealthy(true)
    , m_tankSensorHealthy(true)
    , m_avlErrorCount(0)
    , m_tankErrorCount(0)
    , m_filteringEnabled(true)
    , m_filterAlpha(0.1)  // Light filtering
    , m_minVoltage(DEFAULT_MIN_VOLTAGE)
    , m_maxVoltage(DEFAULT_MAX_VOLTAGE)
    , m_updateTimer(new QTimer(this))
{
    // Set up update timer
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &SensorInterface::updateReadings);
}

SensorInterface::~SensorInterface()
{
    shutdown();
}

bool SensorInterface::initialize()
{
    if (!m_adc) {
        m_lastError = "ADC interface not provided";
        qCritical() << m_lastError;
        return false;
    }
    
    if (!m_adc->isReady()) {
        m_lastError = "ADC not ready";
        qCritical() << m_lastError;
        return false;
    }
    
    try {
        // Initialize filtering
        initializeFiltering();
        
        // Perform initial readings to establish baseline
        double avlVoltage = m_adc->readVoltage(AVL_CHANNEL);
        double tankVoltage = m_adc->readVoltage(TANK_CHANNEL);
        
        if (avlVoltage < 0 || tankVoltage < 0) {
            throw std::runtime_error("Failed to read initial sensor values");
        }
        
        // Initialize filtered values
        m_currentAVL = m_adc->readPressure(AVL_CHANNEL);
        m_currentTank = m_adc->readPressure(TANK_CHANNEL);
        m_filteredAVL = m_currentAVL;
        m_filteredTank = m_currentTank;
        
        // Start continuous monitoring
        m_updateTimer->start();
        
        m_initialized = true;
        qDebug() << "Sensor interface initialized successfully";
        qDebug() << QString("Initial readings - AVL: %1 mmHg, Tank: %2 mmHg")
                    .arg(m_currentAVL, 0, 'f', 1).arg(m_currentTank, 0, 'f', 1);
        
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Sensor interface initialization failed: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void SensorInterface::shutdown()
{
    if (m_initialized) {
        m_updateTimer->stop();
        m_initialized = false;
        qDebug() << "Sensor interface shutdown complete";
    }
}

double SensorInterface::readAVLPressure()
{
    if (!m_initialized || !m_adc) {
        return -1.0;  // Error value
    }
    
    try {
        double pressure = m_adc->readPressure(AVL_CHANNEL);
        if (pressure >= 0) {
            QMutexLocker locker(&m_dataMutex);
            m_currentAVL = pressure;
            
            if (m_filteringEnabled) {
                m_filteredAVL = applyFilter(pressure, m_filteredAVL);
            } else {
                m_filteredAVL = pressure;
            }
        }
        return pressure;
        
    } catch (const std::exception& e) {
        emit sensorError("AVL", QString("Read error: %1").arg(e.what()));
        return -1.0;
    }
}

double SensorInterface::readTankPressure()
{
    if (!m_initialized || !m_adc) {
        return -1.0;  // Error value
    }
    
    try {
        double pressure = m_adc->readPressure(TANK_CHANNEL);
        if (pressure >= 0) {
            QMutexLocker locker(&m_dataMutex);
            m_currentTank = pressure;
            
            if (m_filteringEnabled) {
                m_filteredTank = applyFilter(pressure, m_filteredTank);
            } else {
                m_filteredTank = pressure;
            }
        }
        return pressure;
        
    } catch (const std::exception& e) {
        emit sensorError("Tank", QString("Read error: %1").arg(e.what()));
        return -1.0;
    }
}

void SensorInterface::calibrateAVLSensor(double zeroVoltage, double fullScaleVoltage,
                                        double zeroPressure, double fullScalePressure)
{
    if (m_adc) {
        m_adc->calibrateChannel(AVL_CHANNEL, zeroVoltage, fullScaleVoltage,
                               zeroPressure, fullScalePressure);
        qDebug() << "AVL sensor calibrated";
    }
}

void SensorInterface::calibrateTankSensor(double zeroVoltage, double fullScaleVoltage,
                                         double zeroPressure, double fullScalePressure)
{
    if (m_adc) {
        m_adc->calibrateChannel(TANK_CHANNEL, zeroVoltage, fullScaleVoltage,
                               zeroPressure, fullScalePressure);
        qDebug() << "Tank sensor calibrated";
    }
}

void SensorInterface::setFilterAlpha(double alpha)
{
    if (alpha >= 0.0 && alpha <= 1.0) {
        m_filterAlpha = alpha;
        qDebug() << "Filter alpha set to:" << alpha;
    }
}

void SensorInterface::setErrorThresholds(double minVoltage, double maxVoltage)
{
    if (minVoltage < maxVoltage && minVoltage >= 0.0 && maxVoltage <= 5.0) {
        m_minVoltage = minVoltage;
        m_maxVoltage = maxVoltage;
        qDebug() << QString("Error thresholds set: %1V - %2V").arg(minVoltage).arg(maxVoltage);
    }
}

void SensorInterface::updateReadings()
{
    if (!m_initialized) return;
    
    // Read both sensors
    double avlPressure = readAVLPressure();
    double tankPressure = readTankPressure();
    
    // Check sensor health
    checkSensorHealth();
    
    // Emit updated readings if valid
    if (avlPressure >= 0 && tankPressure >= 0) {
        emit pressureUpdated(m_filteredAVL, m_filteredTank);
    }
}

void SensorInterface::initializeFiltering()
{
    // Exponential moving average filter parameters
    // Alpha = 0.1 provides good noise reduction with reasonable response time
    m_filterAlpha = 0.1;
    m_filteringEnabled = true;
    
    qDebug() << "Sensor filtering initialized with alpha =" << m_filterAlpha;
}

double SensorInterface::applyFilter(double newValue, double& filteredValue)
{
    // Exponential moving average: filtered = alpha * new + (1-alpha) * filtered
    filteredValue = (m_filterAlpha * newValue) + ((1.0 - m_filterAlpha) * filteredValue);
    return filteredValue;
}

bool SensorInterface::validateReading(double voltage, const QString& sensorName)
{
    if (voltage < m_minVoltage || voltage > m_maxVoltage) {
        emit sensorError(sensorName, 
                        QString("Voltage out of range: %1V (valid: %2V - %3V)")
                        .arg(voltage, 0, 'f', 2)
                        .arg(m_minVoltage, 0, 'f', 1)
                        .arg(m_maxVoltage, 0, 'f', 1));
        return false;
    }
    
    return true;
}

void SensorInterface::checkSensorHealth()
{
    // Check AVL sensor
    double avlVoltage = m_adc ? m_adc->readVoltage(AVL_CHANNEL) : -1.0;
    if (avlVoltage < 0 || !validateReading(avlVoltage, "AVL")) {
        m_avlErrorCount++;
        if (m_avlErrorCount >= MAX_CONSECUTIVE_ERRORS && m_avlSensorHealthy) {
            m_avlSensorHealthy = false;
            emit sensorError("AVL", "Sensor unhealthy - too many consecutive errors");
        }
    } else {
        if (m_avlErrorCount > 0) {
            m_avlErrorCount = std::max(0, m_avlErrorCount - 1);  // Gradual recovery
        }
        if (!m_avlSensorHealthy && m_avlErrorCount == 0) {
            m_avlSensorHealthy = true;
            emit sensorRecovered("AVL");
        }
    }
    
    // Check Tank sensor
    double tankVoltage = m_adc ? m_adc->readVoltage(TANK_CHANNEL) : -1.0;
    if (tankVoltage < 0 || !validateReading(tankVoltage, "Tank")) {
        m_tankErrorCount++;
        if (m_tankErrorCount >= MAX_CONSECUTIVE_ERRORS && m_tankSensorHealthy) {
            m_tankSensorHealthy = false;
            emit sensorError("Tank", "Sensor unhealthy - too many consecutive errors");
        }
    } else {
        if (m_tankErrorCount > 0) {
            m_tankErrorCount = std::max(0, m_tankErrorCount - 1);  // Gradual recovery
        }
        if (!m_tankSensorHealthy && m_tankErrorCount == 0) {
            m_tankSensorHealthy = true;
            emit sensorRecovered("Tank");
        }
    }
}
