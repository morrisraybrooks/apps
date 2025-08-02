#include "MockSensorInterface.h"
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QRandomGenerator>

MockSensorInterface::MockSensorInterface(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_pressure1(-20.0)
    , m_pressure2(-25.0)
    , m_sensorError1(false)
    , m_sensorError2(false)
    , m_noiseLevel(1.0)
    , m_calibrationOffset1(0.0)
    , m_calibrationOffset2(0.0)
    , m_calibrationScale1(1.0)
    , m_calibrationScale2(1.0)
{
}

MockSensorInterface::~MockSensorInterface()
{
}

bool MockSensorInterface::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockSensorInterface: Initializing...";
    
    // Simulate initialization delay
    QThread::msleep(50);
    
    m_initialized = true;
    
    // Reset sensor states
    m_sensorError1 = false;
    m_sensorError2 = false;
    m_pressure1 = -20.0;
    m_pressure2 = -25.0;
    
    qDebug() << "MockSensorInterface: Initialization complete";
    return true;
}

void MockSensorInterface::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockSensorInterface: Shutting down...";
    m_initialized = false;
    qDebug() << "MockSensorInterface: Shutdown complete";
}

double MockSensorInterface::readPressure(int sensorNumber)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        qDebug() << "MockSensorInterface: Not initialized";
        return -999.0;
    }
    
    if (sensorNumber == 1) {
        if (m_sensorError1) {
            return -999.0; // Error value
        }
        
        // Apply calibration and add noise
        double rawValue = m_pressure1;
        double calibratedValue = (rawValue + m_calibrationOffset1) * m_calibrationScale1;
        double noise = (QRandomGenerator::global()->generateDouble() - 0.5) * m_noiseLevel;
        
        return calibratedValue + noise;
    }
    else if (sensorNumber == 2) {
        if (m_sensorError2) {
            return -999.0; // Error value
        }
        
        // Apply calibration and add noise
        double rawValue = m_pressure2;
        double calibratedValue = (rawValue + m_calibrationOffset2) * m_calibrationScale2;
        double noise = (QRandomGenerator::global()->generateDouble() - 0.5) * m_noiseLevel;
        
        return calibratedValue + noise;
    }
    
    qDebug() << "MockSensorInterface: Invalid sensor number:" << sensorNumber;
    return 0.0;
}

void MockSensorInterface::setPressureValue(int sensorNumber, double pressure)
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        m_pressure1 = pressure;
        emit pressureChanged(1, pressure);
    } else if (sensorNumber == 2) {
        m_pressure2 = pressure;
        emit pressureChanged(2, pressure);
    }
    
    qDebug() << "MockSensorInterface: Sensor" << sensorNumber << "pressure set to" << pressure << "mmHg";
}

bool MockSensorInterface::calibrateSensor(int sensorNumber, double knownPressure, double measuredValue)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    if (sensorNumber == 1) {
        // Simple offset calibration
        m_calibrationOffset1 = knownPressure - measuredValue;
        qDebug() << "MockSensorInterface: Sensor 1 calibrated with offset" << m_calibrationOffset1;
        return true;
    }
    else if (sensorNumber == 2) {
        // Simple offset calibration
        m_calibrationOffset2 = knownPressure - measuredValue;
        qDebug() << "MockSensorInterface: Sensor 2 calibrated with offset" << m_calibrationOffset2;
        return true;
    }
    
    return false;
}

bool MockSensorInterface::performZeroCalibration(int sensorNumber)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    qDebug() << "MockSensorInterface: Performing zero calibration for sensor" << sensorNumber;
    
    // Simulate calibration process
    QThread::msleep(100);
    
    if (sensorNumber == 1) {
        m_calibrationOffset1 = -m_pressure1; // Zero out current reading
        return true;
    }
    else if (sensorNumber == 2) {
        m_calibrationOffset2 = -m_pressure2; // Zero out current reading
        return true;
    }
    
    return false;
}

void MockSensorInterface::simulateError(int sensorNumber, bool hasError)
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        m_sensorError1 = hasError;
    } else if (sensorNumber == 2) {
        m_sensorError2 = hasError;
    }
    
    qDebug() << "MockSensorInterface: Sensor" << sensorNumber << "error simulation" << (hasError ? "ON" : "OFF");
    
    emit sensorErrorChanged(sensorNumber, hasError);
}

bool MockSensorInterface::hasSensorError(int sensorNumber) const
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        return m_sensorError1;
    } else if (sensorNumber == 2) {
        return m_sensorError2;
    }
    
    return false;
}

void MockSensorInterface::setNoiseLevel(double noiseLevel)
{
    QMutexLocker locker(&m_mutex);
    m_noiseLevel = qMax(0.0, noiseLevel);
    qDebug() << "MockSensorInterface: Noise level set to" << m_noiseLevel;
}

double MockSensorInterface::getNoiseLevel() const
{
    QMutexLocker locker(&m_mutex);
    return m_noiseLevel;
}

bool MockSensorInterface::isInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

bool MockSensorInterface::performSelfTest()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MockSensorInterface: Performing self-test...";
    
    if (!m_initialized) {
        qDebug() << "MockSensorInterface: Self-test FAILED - not initialized";
        return false;
    }
    
    // Simulate self-test delay
    QThread::msleep(200);
    
    // Check for sensor errors
    if (m_sensorError1 || m_sensorError2) {
        qDebug() << "MockSensorInterface: Self-test FAILED - sensor errors detected";
        return false;
    }
    
    // Test sensor readings
    double reading1 = readPressure(1);
    double reading2 = readPressure(2);
    
    if (reading1 == -999.0 || reading2 == -999.0) {
        qDebug() << "MockSensorInterface: Self-test FAILED - invalid readings";
        return false;
    }
    
    qDebug() << "MockSensorInterface: Self-test PASSED";
    return true;
}

QList<double> MockSensorInterface::getCalibrationData(int sensorNumber) const
{
    QMutexLocker locker(&m_mutex);
    
    QList<double> calibrationData;
    
    if (sensorNumber == 1) {
        calibrationData << m_calibrationOffset1 << m_calibrationScale1;
    } else if (sensorNumber == 2) {
        calibrationData << m_calibrationOffset2 << m_calibrationScale2;
    }
    
    return calibrationData;
}

void MockSensorInterface::setCalibrationData(int sensorNumber, const QList<double>& calibrationData)
{
    QMutexLocker locker(&m_mutex);
    
    if (calibrationData.size() < 2) {
        return;
    }
    
    if (sensorNumber == 1) {
        m_calibrationOffset1 = calibrationData[0];
        m_calibrationScale1 = calibrationData[1];
        qDebug() << "MockSensorInterface: Sensor 1 calibration set - offset:" << m_calibrationOffset1 << "scale:" << m_calibrationScale1;
    } else if (sensorNumber == 2) {
        m_calibrationOffset2 = calibrationData[0];
        m_calibrationScale2 = calibrationData[1];
        qDebug() << "MockSensorInterface: Sensor 2 calibration set - offset:" << m_calibrationOffset2 << "scale:" << m_calibrationScale2;
    }
}

void MockSensorInterface::resetCalibration(int sensorNumber)
{
    QMutexLocker locker(&m_mutex);
    
    if (sensorNumber == 1) {
        m_calibrationOffset1 = 0.0;
        m_calibrationScale1 = 1.0;
        qDebug() << "MockSensorInterface: Sensor 1 calibration reset";
    } else if (sensorNumber == 2) {
        m_calibrationOffset2 = 0.0;
        m_calibrationScale2 = 1.0;
        qDebug() << "MockSensorInterface: Sensor 2 calibration reset";
    }
}

double MockSensorInterface::getAverageReading(int sensorNumber, int sampleCount)
{
    QMutexLocker locker(&m_mutex);
    
    if (sampleCount <= 0) {
        return readPressure(sensorNumber);
    }
    
    double sum = 0.0;
    int validSamples = 0;
    
    for (int i = 0; i < sampleCount; ++i) {
        double reading = readPressure(sensorNumber);
        if (reading != -999.0) {
            sum += reading;
            validSamples++;
        }
        QThread::msleep(1); // Small delay between samples
    }
    
    if (validSamples == 0) {
        return -999.0;
    }
    
    return sum / validSamples;
}
