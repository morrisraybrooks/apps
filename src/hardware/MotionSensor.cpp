#include "MotionSensor.h"
#include <QDebug>
#include <QtMath>
#include <QDateTime>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#endif

MotionSensor::MotionSensor(SensorType type, QObject* parent)
    : QObject(parent)
    , m_sensorType(type)
    , m_initialized(false)
    , m_calibrated(false)
    , m_sessionActive(false)
    , m_i2cBus(1)
    , m_i2cAddress(0x68)
    , m_i2cFileDescriptor(-1)
    , m_acceleration(0, 0, 0)
    , m_gyroscope(0, 0, 0)
    , m_accelOffset(0, 0, 0)
    , m_gyroOffset(0, 0, 0)
    , m_motionMagnitude(0.0)
    , m_motionLevel(MotionLevel::STILL)
    , m_stillnessScore(100.0)
    , m_isStill(true)
    , m_accelThresholdStill(0.05)
    , m_accelThresholdMinor(0.2)
    , m_accelThresholdModerate(0.5)
    , m_gyroThresholdStill(5.0)
    , m_gyroThresholdMinor(20.0)
    , m_gyroThresholdModerate(50.0)
    , m_violationCount(0)
    , m_warningCount(0)
    , m_lastViolationTime(0)
    , m_maxViolationIntensity(0.0)
    , m_violationDebounceMs(500)
    , m_stillnessSum(0.0)
    , m_stillnessSamples(0)
    , m_stillStartTime(0)
    , m_calibrationSamplesNeeded(CALIBRATION_SAMPLES)
    , m_sampleRateHz(DEFAULT_SAMPLE_RATE_HZ)
    , m_simulatedMotion(0.0)
{
    m_sampleTimer = new QTimer(this);
    connect(m_sampleTimer, &QTimer::timeout, this, &MotionSensor::onSampleTimer);
    
    m_calibrationTimer = new QTimer(this);
    connect(m_calibrationTimer, &QTimer::timeout, this, &MotionSensor::onCalibrationTimer);
    
    qDebug() << "MotionSensor created, type:" << static_cast<int>(type);
}

MotionSensor::~MotionSensor()
{
    shutdown();
}

bool MotionSensor::initialize()
{
    return initializeI2C(m_i2cBus, m_i2cAddress);
}

bool MotionSensor::initializeI2C(int bus, int address)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    m_i2cBus = bus;
    m_i2cAddress = address;
    
    if (m_sensorType == SensorType::SIMULATED) {
        qDebug() << "MotionSensor: Initializing in simulation mode";
        m_initialized = true;
        m_simulationTimer.start();
        m_sampleTimer->start(1000 / m_sampleRateHz);
        return true;
    }
    
#ifdef __linux__
    QString devicePath = QString("/dev/i2c-%1").arg(bus);
    m_i2cFileDescriptor = open(devicePath.toStdString().c_str(), O_RDWR);
    
    if (m_i2cFileDescriptor < 0) {
        QString error = QString("Failed to open I2C bus: %1").arg(devicePath);
        qWarning() << error;
        emit sensorError(error);
        return false;
    }
    
    if (ioctl(m_i2cFileDescriptor, I2C_SLAVE, address) < 0) {
        QString error = QString("Failed to set I2C address: 0x%1").arg(address, 2, 16, QChar('0'));
        qWarning() << error;
        emit sensorError(error);
        close(m_i2cFileDescriptor);
        m_i2cFileDescriptor = -1;
        return false;
    }
    
    // Wake up MPU6050 (clear sleep bit)
    uint8_t wakeCmd[2] = {MPU6050_REG_PWR_MGMT_1, 0x00};
    if (write(m_i2cFileDescriptor, wakeCmd, 2) != 2) {
        QString error = "Failed to wake MPU6050";
        qWarning() << error;
        emit sensorError(error);
        close(m_i2cFileDescriptor);
        m_i2cFileDescriptor = -1;
        return false;
    }
    
    // Configure accelerometer to ±2g range
    uint8_t accelConfig[2] = {MPU6050_REG_ACCEL_CONFIG, 0x00};
    write(m_i2cFileDescriptor, accelConfig, 2);
    
    // Configure gyroscope to ±250°/s range
    uint8_t gyroConfig[2] = {MPU6050_REG_GYRO_CONFIG, 0x00};
    write(m_i2cFileDescriptor, gyroConfig, 2);
    
    // Set DLPF for noise filtering (bandwidth ~20Hz)
    uint8_t dlpfConfig[2] = {MPU6050_REG_CONFIG, 0x04};
    write(m_i2cFileDescriptor, dlpfConfig, 2);
    
    qDebug() << "MPU6050 initialized on I2C bus" << bus << "address 0x" << Qt::hex << address;
#else
    qWarning() << "MotionSensor: I2C not supported on this platform, using simulation";
    m_sensorType = SensorType::SIMULATED;
    m_simulationTimer.start();
#endif
    
    m_initialized = true;
    m_sampleTimer->start(1000 / m_sampleRateHz);
    
    return true;
}

void MotionSensor::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    m_sampleTimer->stop();
    m_calibrationTimer->stop();
    
#ifdef __linux__
    if (m_i2cFileDescriptor >= 0) {
        close(m_i2cFileDescriptor);
        m_i2cFileDescriptor = -1;
    }
#endif
    
    m_initialized = false;
    qDebug() << "MotionSensor shutdown";
}

bool MotionSensor::calibrate(int durationMs)
{
    if (!m_initialized) {
        qWarning() << "MotionSensor: Cannot calibrate - not initialized";
        return false;
    }

    QMutexLocker locker(&m_mutex);

    m_calibrationAccelSamples.clear();
    m_calibrationGyroSamples.clear();
    m_calibrationSamplesNeeded = durationMs / (1000 / m_sampleRateHz);

    locker.unlock();

    qDebug() << "Starting motion sensor calibration, samples needed:" << m_calibrationSamplesNeeded;
    m_calibrationTimer->start(1000 / m_sampleRateHz);

    return true;
}

void MotionSensor::onCalibrationTimer()
{
    QMutexLocker locker(&m_mutex);

    if (!readSensorData()) {
        return;
    }

    m_calibrationAccelSamples.append(m_acceleration);
    m_calibrationGyroSamples.append(m_gyroscope);

    int progress = (m_calibrationAccelSamples.size() * 100) / m_calibrationSamplesNeeded;
    emit calibrationProgress(progress);

    if (m_calibrationAccelSamples.size() >= m_calibrationSamplesNeeded) {
        m_calibrationTimer->stop();

        // Calculate average offsets
        QVector3D accelSum(0, 0, 0);
        QVector3D gyroSum(0, 0, 0);

        for (const auto& sample : m_calibrationAccelSamples) {
            accelSum += sample;
        }
        for (const auto& sample : m_calibrationGyroSamples) {
            gyroSum += sample;
        }

        m_accelOffset = accelSum / m_calibrationAccelSamples.size();
        m_gyroOffset = gyroSum / m_calibrationGyroSamples.size();

        // Subtract gravity from Z-axis (assuming device is horizontal)
        m_accelOffset.setZ(m_accelOffset.z() - 1.0);

        m_calibrated = true;

        qDebug() << "Calibration complete. Accel offset:" << m_accelOffset
                 << "Gyro offset:" << m_gyroOffset;

        locker.unlock();
        emit calibrationComplete(true);
    }
}

void MotionSensor::resetCalibration()
{
    QMutexLocker locker(&m_mutex);
    m_calibrated = false;
    m_accelOffset = QVector3D(0, 0, 0);
    m_gyroOffset = QVector3D(0, 0, 0);
    qDebug() << "Calibration reset";
}

bool MotionSensor::readSensorData()
{
    if (m_sensorType == SensorType::SIMULATED) {
        simulateMotion();
        return true;
    }

#ifdef __linux__
    if (m_i2cFileDescriptor < 0) {
        return false;
    }

    // Read accelerometer data (6 bytes: X, Y, Z each 16-bit)
    uint8_t regAddr = MPU6050_REG_ACCEL_XOUT_H;
    if (write(m_i2cFileDescriptor, &regAddr, 1) != 1) {
        return false;
    }

    uint8_t accelData[6];
    if (read(m_i2cFileDescriptor, accelData, 6) != 6) {
        return false;
    }

    int16_t ax = (accelData[0] << 8) | accelData[1];
    int16_t ay = (accelData[2] << 8) | accelData[3];
    int16_t az = (accelData[4] << 8) | accelData[5];

    m_acceleration.setX(ax / ACCEL_SCALE_2G);
    m_acceleration.setY(ay / ACCEL_SCALE_2G);
    m_acceleration.setZ(az / ACCEL_SCALE_2G);

    // Read gyroscope data (6 bytes: X, Y, Z each 16-bit)
    regAddr = MPU6050_REG_GYRO_XOUT_H;
    if (write(m_i2cFileDescriptor, &regAddr, 1) != 1) {
        return false;
    }

    uint8_t gyroData[6];
    if (read(m_i2cFileDescriptor, gyroData, 6) != 6) {
        return false;
    }

    int16_t gx = (gyroData[0] << 8) | gyroData[1];
    int16_t gy = (gyroData[2] << 8) | gyroData[3];
    int16_t gz = (gyroData[4] << 8) | gyroData[5];

    m_gyroscope.setX(gx / GYRO_SCALE_250);
    m_gyroscope.setY(gy / GYRO_SCALE_250);
    m_gyroscope.setZ(gz / GYRO_SCALE_250);

    // Apply calibration offsets
    if (m_calibrated) {
        applyCalibrationOffset();
    }

    return true;
#else
    simulateMotion();
    return true;
#endif
}

void MotionSensor::applyCalibrationOffset()
{
    m_acceleration -= m_accelOffset;
    m_gyroscope -= m_gyroOffset;
}

void MotionSensor::simulateMotion()
{
    // Generate simulated motion based on time
    double elapsed = m_simulationTimer.elapsed() / 1000.0;

    // Base stillness with occasional random movements
    double breathingMotion = 0.02 * sin(elapsed * 0.5);  // Breathing simulation
    double randomMotion = m_simulatedMotion;

    m_acceleration.setX(breathingMotion + (qrand() % 100 - 50) / 5000.0);
    m_acceleration.setY((qrand() % 100 - 50) / 5000.0);
    m_acceleration.setZ(1.0 + breathingMotion);  // Gravity + breathing

    m_gyroscope.setX(randomMotion * 10 + (qrand() % 100 - 50) / 50.0);
    m_gyroscope.setY(randomMotion * 10 + (qrand() % 100 - 50) / 50.0);
    m_gyroscope.setZ((qrand() % 100 - 50) / 100.0);

    if (m_calibrated) {
        applyCalibrationOffset();
    }
}

void MotionSensor::onSampleTimer()
{
    QMutexLocker locker(&m_mutex);

    if (!readSensorData()) {
        return;
    }

    updateMotionLevel();

    if (m_sessionActive) {
        checkViolation();

        // Update stillness statistics
        m_stillnessSum += m_stillnessScore;
        m_stillnessSamples++;
    }

    locker.unlock();

    emit motionDetected(m_motionLevel, m_motionMagnitude);
    emit stillnessChanged(m_isStill, m_stillnessScore);
}

void MotionSensor::updateMotionLevel()
{
    // Calculate motion magnitude from calibrated acceleration (excluding gravity)
    double accelMag = qSqrt(m_acceleration.x() * m_acceleration.x() +
                           m_acceleration.y() * m_acceleration.y() +
                           (m_acceleration.z() - 1.0) * (m_acceleration.z() - 1.0));

    // Calculate rotation magnitude
    double gyroMag = m_gyroscope.length();

    // Combined motion metric
    m_motionMagnitude = calculateMagnitude(m_acceleration, m_gyroscope);

    // Determine motion level based on thresholds
    bool wasStill = m_isStill;

    if (accelMag < m_accelThresholdStill && gyroMag < m_gyroThresholdStill) {
        m_motionLevel = MotionLevel::STILL;
        m_stillnessScore = 100.0 - (accelMag / m_accelThresholdStill * 50.0);
        m_isStill = true;
    } else if (accelMag < m_accelThresholdMinor && gyroMag < m_gyroThresholdMinor) {
        m_motionLevel = MotionLevel::MINOR;
        m_stillnessScore = 50.0 - (accelMag / m_accelThresholdMinor * 25.0);
        m_isStill = true;  // Minor movement allowed
    } else if (accelMag < m_accelThresholdModerate && gyroMag < m_gyroThresholdModerate) {
        m_motionLevel = MotionLevel::MODERATE;
        m_stillnessScore = 25.0 - (accelMag / m_accelThresholdModerate * 25.0);
        m_isStill = false;
    } else {
        m_motionLevel = MotionLevel::MAJOR;
        m_stillnessScore = 0.0;
        m_isStill = false;
    }

    m_stillnessScore = qBound(0.0, m_stillnessScore, 100.0);

    // Track stillness duration
    if (m_isStill && !wasStill) {
        m_stillStartTime = QDateTime::currentMSecsSinceEpoch();
    }
}

double MotionSensor::calculateMagnitude(const QVector3D& accel, const QVector3D& gyro)
{
    // Normalize acceleration (excluding gravity) to 0-1 range
    double accelMag = qSqrt(accel.x() * accel.x() +
                           accel.y() * accel.y() +
                           (accel.z() - 1.0) * (accel.z() - 1.0));
    double accelNorm = qMin(accelMag / 1.0, 1.0);  // Cap at 1g

    // Normalize gyro to 0-1 range
    double gyroMag = gyro.length();
    double gyroNorm = qMin(gyroMag / 100.0, 1.0);  // Cap at 100°/s

    // Weighted combination (accelerometer more sensitive for sudden movements)
    return 0.6 * accelNorm + 0.4 * gyroNorm;
}

void MotionSensor::checkViolation()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_motionLevel == MotionLevel::MODERATE) {
        // Warning for moderate movement
        if (now - m_lastViolationTime > m_violationDebounceMs) {
            m_warningCount++;
            emit warningIssued(QString("Movement warning #%1").arg(m_warningCount));
        }
    } else if (m_motionLevel == MotionLevel::MAJOR) {
        // Violation for major movement
        if (now - m_lastViolationTime > m_violationDebounceMs) {
            m_violationCount++;
            m_lastViolationTime = now;

            double intensity = m_motionMagnitude;
            if (intensity > m_maxViolationIntensity) {
                m_maxViolationIntensity = intensity;
            }

            emit violationDetected(m_motionLevel, intensity);
        }
    }
}

// Getter implementations
QVector3D MotionSensor::getAcceleration() const
{
    QMutexLocker locker(&m_mutex);
    return m_acceleration;
}

QVector3D MotionSensor::getGyroscope() const
{
    QMutexLocker locker(&m_mutex);
    return m_gyroscope;
}

double MotionSensor::getMotionMagnitude() const
{
    QMutexLocker locker(&m_mutex);
    return m_motionMagnitude;
}

MotionSensor::MotionLevel MotionSensor::getMotionLevel() const
{
    QMutexLocker locker(&m_mutex);
    return m_motionLevel;
}

double MotionSensor::getStillnessScore() const
{
    QMutexLocker locker(&m_mutex);
    return m_stillnessScore;
}

double MotionSensor::getViolationIntensity() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxViolationIntensity;
}

qint64 MotionSensor::getStillDurationMs() const
{
    QMutexLocker locker(&m_mutex);
    if (!m_isStill || m_stillStartTime == 0) {
        return 0;
    }
    return QDateTime::currentMSecsSinceEpoch() - m_stillStartTime;
}

double MotionSensor::getAverageStillness() const
{
    QMutexLocker locker(&m_mutex);
    if (m_stillnessSamples == 0) {
        return 100.0;
    }
    return m_stillnessSum / m_stillnessSamples;
}

bool MotionSensor::isCurrentlyStill() const
{
    QMutexLocker locker(&m_mutex);
    return m_isStill;
}

// Configuration
void MotionSensor::setSensitivity(SensitivityPreset preset)
{
    QMutexLocker locker(&m_mutex);

    switch (preset) {
        case SensitivityPreset::LENIENT:
            m_accelThresholdStill = 0.1;
            m_accelThresholdMinor = 0.4;
            m_accelThresholdModerate = 0.8;
            m_gyroThresholdStill = 10.0;
            m_gyroThresholdMinor = 40.0;
            m_gyroThresholdModerate = 80.0;
            break;
        case SensitivityPreset::NORMAL:
            m_accelThresholdStill = 0.05;
            m_accelThresholdMinor = 0.2;
            m_accelThresholdModerate = 0.5;
            m_gyroThresholdStill = 5.0;
            m_gyroThresholdMinor = 20.0;
            m_gyroThresholdModerate = 50.0;
            break;
        case SensitivityPreset::STRICT:
            m_accelThresholdStill = 0.03;
            m_accelThresholdMinor = 0.1;
            m_accelThresholdModerate = 0.3;
            m_gyroThresholdStill = 3.0;
            m_gyroThresholdMinor = 10.0;
            m_gyroThresholdModerate = 30.0;
            break;
        case SensitivityPreset::EXTREME:
            m_accelThresholdStill = 0.02;
            m_accelThresholdMinor = 0.05;
            m_accelThresholdModerate = 0.15;
            m_gyroThresholdStill = 2.0;
            m_gyroThresholdMinor = 5.0;
            m_gyroThresholdModerate = 15.0;
            break;
    }

    qDebug() << "Motion sensitivity set to preset:" << static_cast<int>(preset);
}

void MotionSensor::setCustomThresholds(double accelThreshold, double gyroThreshold)
{
    QMutexLocker locker(&m_mutex);
    m_accelThresholdModerate = accelThreshold;
    m_gyroThresholdModerate = gyroThreshold;
    m_accelThresholdMinor = accelThreshold * 0.4;
    m_gyroThresholdMinor = gyroThreshold * 0.4;
    m_accelThresholdStill = accelThreshold * 0.1;
    m_gyroThresholdStill = gyroThreshold * 0.1;
}

void MotionSensor::setSampleRate(int hz)
{
    QMutexLocker locker(&m_mutex);
    m_sampleRateHz = qBound(10, hz, 1000);
    if (m_sampleTimer->isActive()) {
        m_sampleTimer->setInterval(1000 / m_sampleRateHz);
    }
}

// Session management
void MotionSensor::startSession()
{
    QMutexLocker locker(&m_mutex);
    m_sessionActive = true;
    resetViolations();
    m_stillnessSum = 0.0;
    m_stillnessSamples = 0;
    m_stillTimer.start();
    qDebug() << "Motion sensor session started";
}

void MotionSensor::endSession()
{
    QMutexLocker locker(&m_mutex);
    m_sessionActive = false;
    qDebug() << "Motion sensor session ended. Violations:" << m_violationCount
             << "Avg stillness:" << getAverageStillness() << "%";
}

void MotionSensor::resetSession()
{
    QMutexLocker locker(&m_mutex);
    resetViolations();
    m_stillnessSum = 0.0;
    m_stillnessSamples = 0;
    m_stillTimer.restart();
}

void MotionSensor::resetViolations()
{
    m_violationCount = 0;
    m_warningCount = 0;
    m_lastViolationTime = 0;
    m_maxViolationIntensity = 0.0;
}

