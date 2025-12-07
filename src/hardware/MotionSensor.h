#ifndef MOTIONSENSOR_H
#define MOTIONSENSOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <QVector3D>
#include <memory>

/**
 * @brief Motion Sensor Interface for stillness detection
 * 
 * Supports MPU6050 6-axis IMU (accelerometer + gyroscope) via I2C
 * for detecting body movement during NO_MOVING challenges.
 * 
 * Motion Detection Thresholds:
 * - Stillness: < 0.05g acceleration, < 5°/s rotation
 * - Minor movement: 0.05-0.2g, 5-20°/s (warning)
 * - Moderate movement: 0.2-0.5g, 20-50°/s (violation)
 * - Major movement: > 0.5g, > 50°/s (instant failure)
 * 
 * Hardware: MPU6050 on I2C (address 0x68 or 0x69)
 * Mounting: Sensor attached to V-Contour device or body harness
 */
class MotionSensor : public QObject
{
    Q_OBJECT

public:
    // Sensor types
    enum class SensorType {
        MPU6050_I2C,      // MPU6050 via I2C
        MPU9250_I2C,      // MPU9250 9-axis via I2C (with magnetometer)
        LSM6DS3_I2C,      // LSM6DS3 via I2C
        SIMULATED         // For testing without hardware
    };
    Q_ENUM(SensorType)

    // Motion intensity levels
    enum class MotionLevel {
        STILL,            // < 0.05g, < 5°/s - perfect stillness
        MINOR,            // 0.05-0.2g, 5-20°/s - breathing/pulse allowed
        MODERATE,         // 0.2-0.5g, 20-50°/s - violation warning
        MAJOR             // > 0.5g, > 50°/s - significant movement
    };
    Q_ENUM(MotionLevel)

    // Sensitivity presets
    enum class SensitivityPreset {
        LENIENT,          // Beginner: allows more movement
        NORMAL,           // Standard sensitivity
        STRICT,           // Advanced: minimal movement allowed
        EXTREME           // Expert: almost no movement tolerance
    };
    Q_ENUM(SensitivityPreset)

    explicit MotionSensor(SensorType type = SensorType::MPU6050_I2C,
                          QObject* parent = nullptr);
    ~MotionSensor();

    // Initialization
    bool initialize();
    bool initializeI2C(int bus = 1, int address = 0x68);
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Calibration
    bool calibrate(int durationMs = 3000);  // Calibrate baseline stillness
    bool isCalibrated() const { return m_calibrated; }
    void resetCalibration();

    // Motion readings
    QVector3D getAcceleration() const;      // Raw acceleration (g)
    QVector3D getGyroscope() const;         // Raw rotation rate (°/s)
    double getMotionMagnitude() const;      // Combined motion metric (0-1)
    MotionLevel getMotionLevel() const;     // Current motion category
    double getStillnessScore() const;       // 0-100% stillness quality

    // Violation tracking
    int getViolationCount() const { return m_violationCount; }
    int getWarningCount() const { return m_warningCount; }
    qint64 getLastViolationTime() const { return m_lastViolationTime; }
    double getViolationIntensity() const;   // Max violation intensity this session
    void resetViolations();

    // Stillness tracking
    qint64 getStillDurationMs() const;      // How long user has been still
    double getAverageStillness() const;     // Average stillness over session
    bool isCurrentlyStill() const;          // Below violation threshold

    // Configuration
    void setSensitivity(SensitivityPreset preset);
    void setCustomThresholds(double accelThreshold, double gyroThreshold);
    void setViolationDebounceMs(int ms) { m_violationDebounceMs = ms; }
    void setSampleRate(int hz);             // 10-1000 Hz

    // Session control
    void startSession();
    void endSession();
    void resetSession();

Q_SIGNALS:
    void motionDetected(MotionLevel level, double magnitude);
    void stillnessChanged(bool isStill, double stillnessScore);
    void violationDetected(MotionLevel level, double intensity);
    void warningIssued(const QString& message);
    void calibrationComplete(bool success);
    void calibrationProgress(int percent);
    void sensorError(const QString& error);

private Q_SLOTS:
    void onSampleTimer();
    void onCalibrationTimer();

private:
    bool readSensorData();
    void updateMotionLevel();
    void checkViolation();
    double calculateMagnitude(const QVector3D& accel, const QVector3D& gyro);
    void applyCalibrationOffset();
    void simulateMotion();

    // Sensor type and state
    SensorType m_sensorType;
    bool m_initialized;
    bool m_calibrated;
    bool m_sessionActive;

    // I2C parameters
    int m_i2cBus;
    int m_i2cAddress;
    int m_i2cFileDescriptor;

    // Raw sensor data
    QVector3D m_acceleration;     // Current acceleration (g)
    QVector3D m_gyroscope;        // Current rotation rate (°/s)
    QVector3D m_accelOffset;      // Calibration offset
    QVector3D m_gyroOffset;       // Calibration offset

    // Motion analysis
    double m_motionMagnitude;     // 0-1 combined metric
    MotionLevel m_motionLevel;
    double m_stillnessScore;      // 0-100%
    bool m_isStill;

    // Thresholds (based on sensitivity)
    double m_accelThresholdStill;    // g (stillness)
    double m_accelThresholdMinor;    // g (warning)
    double m_accelThresholdModerate; // g (violation)
    double m_gyroThresholdStill;     // °/s
    double m_gyroThresholdMinor;     // °/s
    double m_gyroThresholdModerate;  // °/s

    // Violation tracking
    int m_violationCount;
    int m_warningCount;
    qint64 m_lastViolationTime;
    double m_maxViolationIntensity;
    int m_violationDebounceMs;

    // Stillness tracking
    QElapsedTimer m_stillTimer;
    double m_stillnessSum;
    int m_stillnessSamples;
    qint64 m_stillStartTime;

    // Calibration
    QVector<QVector3D> m_calibrationAccelSamples;
    QVector<QVector3D> m_calibrationGyroSamples;
    QTimer* m_calibrationTimer;
    int m_calibrationSamplesNeeded;

    // Timers
    QTimer* m_sampleTimer;
    int m_sampleRateHz;

    // Thread safety
    mutable QMutex m_mutex;

    // Simulation
    double m_simulatedMotion;
    QElapsedTimer m_simulationTimer;

    // MPU6050 register addresses
    static const int MPU6050_REG_PWR_MGMT_1 = 0x6B;
    static const int MPU6050_REG_ACCEL_XOUT_H = 0x3B;
    static const int MPU6050_REG_GYRO_XOUT_H = 0x43;
    static const int MPU6050_REG_CONFIG = 0x1A;
    static const int MPU6050_REG_GYRO_CONFIG = 0x1B;
    static const int MPU6050_REG_ACCEL_CONFIG = 0x1C;

    // Constants
    static constexpr double ACCEL_SCALE_2G = 16384.0;  // LSB/g for ±2g range
    static constexpr double GYRO_SCALE_250 = 131.0;    // LSB/(°/s) for ±250°/s range
    static constexpr int DEFAULT_SAMPLE_RATE_HZ = 100;
    static constexpr int CALIBRATION_SAMPLES = 100;
};

#endif // MOTIONSENSOR_H

