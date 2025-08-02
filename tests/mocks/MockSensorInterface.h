#ifndef MOCKSENSORINTERFACE_H
#define MOCKSENSORINTERFACE_H

#include <QObject>
#include <QMutex>
#include <QList>

/**
 * @brief Mock sensor interface for testing
 * 
 * Provides a complete mock implementation of the sensor interface
 * for testing purposes. Simulates pressure sensors with configurable
 * noise, calibration, and error conditions.
 */
class MockSensorInterface : public QObject
{
    Q_OBJECT

public:
    explicit MockSensorInterface(QObject *parent = nullptr);
    ~MockSensorInterface();

    // Initialization and shutdown
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // Sensor reading
    double readPressure(int sensorNumber);
    double getAverageReading(int sensorNumber, int sampleCount);
    
    // Test control methods
    void setPressureValue(int sensorNumber, double pressure);
    void simulateError(int sensorNumber, bool hasError);
    bool hasSensorError(int sensorNumber) const;
    void setNoiseLevel(double noiseLevel);
    double getNoiseLevel() const;
    
    // Calibration
    bool calibrateSensor(int sensorNumber, double knownPressure, double measuredValue);
    bool performZeroCalibration(int sensorNumber);
    QList<double> getCalibrationData(int sensorNumber) const;
    void setCalibrationData(int sensorNumber, const QList<double>& calibrationData);
    void resetCalibration(int sensorNumber);
    
    // System operations
    bool performSelfTest();

signals:
    // Sensor signals
    void pressureChanged(int sensorNumber, double pressure);
    void sensorErrorChanged(int sensorNumber, bool hasError);
    void calibrationChanged(int sensorNumber);

private:
    // Sensor state
    bool m_initialized;
    double m_pressure1;
    double m_pressure2;
    bool m_sensorError1;
    bool m_sensorError2;
    
    // Simulation parameters
    double m_noiseLevel;
    
    // Calibration data
    double m_calibrationOffset1;
    double m_calibrationOffset2;
    double m_calibrationScale1;
    double m_calibrationScale2;
    
    // Thread safety
    mutable QMutex m_mutex;
};

#endif // MOCKSENSORINTERFACE_H
