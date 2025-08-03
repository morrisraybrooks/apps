#ifndef SENSORINTERFACE_H
#define SENSORINTERFACE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <memory>

class MCP3008;

/**
 * @brief Interface for pressure sensor management
 * 
 * This class manages the two MPX5010DP pressure sensors:
 * - Sensor 1: AVL (Applied Vacuum Line) on MCP3008 channel 0
 * - Sensor 2: Vacuum tank on MCP3008 channel 1
 * 
 * Provides filtered readings, error detection, and calibration.
 */
class SensorInterface : public QObject
{
    Q_OBJECT

public:
    explicit SensorInterface(MCP3008* adc, QObject *parent = nullptr);
    ~SensorInterface();

    // Initialization
    bool initialize();
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Pressure readings (in mmHg)
    double readAVLPressure();      // Applied Vacuum Line pressure
    double readTankPressure();     // Tank vacuum pressure
    
    // Filtered readings (with noise reduction)
    double getFilteredAVLPressure() const { return m_filteredAVL; }
    double getFilteredTankPressure() const { return m_filteredTank; }
    
    // Sensor status
    bool isAVLSensorHealthy() const { return m_avlSensorHealthy; }
    bool isTankSensorHealthy() const { return m_tankSensorHealthy; }
    
    // Calibration
    void calibrateAVLSensor(double zeroVoltage, double fullScaleVoltage,
                           double zeroPressure, double fullScalePressure);
    void calibrateTankSensor(double zeroVoltage, double fullScaleVoltage,
                            double zeroPressure, double fullScalePressure);
    
    // Configuration
    void setFilteringEnabled(bool enabled) { m_filteringEnabled = enabled; }
    void setFilterAlpha(double alpha);  // 0.0 = no filtering, 1.0 = maximum filtering
    
    // Error thresholds
    void setErrorThresholds(double minVoltage, double maxVoltage);
    
    // Diagnostics
    QString getLastError() const { return m_lastError; }
    int getAVLErrorCount() const { return m_avlErrorCount; }
    int getTankErrorCount() const { return m_tankErrorCount; }

Q_SIGNALS:
    void sensorError(const QString& sensor, const QString& error);
    void sensorRecovered(const QString& sensor);
    void pressureUpdated(double avlPressure, double tankPressure);

private Q_SLOTS:
    void updateReadings();

private:
    void initializeFiltering();
    double applyFilter(double newValue, double& filteredValue);
    bool validateReading(double voltage, const QString& sensorName);
    void checkSensorHealth();

    // Hardware interface
    MCP3008* m_adc;
    
    // System state
    bool m_initialized;
    mutable QMutex m_dataMutex;
    
    // Current readings
    double m_currentAVL;
    double m_currentTank;
    double m_filteredAVL;
    double m_filteredTank;
    
    // Sensor health monitoring
    bool m_avlSensorHealthy;
    bool m_tankSensorHealthy;
    int m_avlErrorCount;
    int m_tankErrorCount;
    
    // Filtering configuration
    bool m_filteringEnabled;
    double m_filterAlpha;  // Exponential moving average factor
    
    // Error detection
    double m_minVoltage;   // Minimum valid voltage
    double m_maxVoltage;   // Maximum valid voltage
    QString m_lastError;
    
    // Update timer
    QTimer* m_updateTimer;
    
    // Channel assignments (as per specification)
    static const int AVL_CHANNEL = 0;   // MCP3008 channel 0
    static const int TANK_CHANNEL = 1;  // MCP3008 channel 1
    
    // Default error thresholds for MPX5010DP
    static const double DEFAULT_MIN_VOLTAGE;  // 0.1V (below sensor range)
    static const double DEFAULT_MAX_VOLTAGE;  // 5.0V (above sensor range)
    
    // Health check parameters
    static const int MAX_CONSECUTIVE_ERRORS = 5;
    static const int UPDATE_INTERVAL_MS = 50;  // 20Hz update rate
};

#endif // SENSORINTERFACE_H
