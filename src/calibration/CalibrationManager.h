#ifndef CALIBRATIONMANAGER_H
#define CALIBRATIONMANAGER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMutex>

// Forward declarations
class HardwareManager;
class SensorInterface;
class ActuatorControl;
class DataLogger;

/**
 * @brief Manages system calibration for sensors and actuators
 * 
 * This class provides comprehensive calibration functionality for the vacuum controller system.
 * It handles sensor calibration, actuator calibration, calibration data persistence,
 * and calibration validation.
 */
class CalibrationManager : public QObject
{
    Q_OBJECT

public:
    enum CalibrationType {
        SENSOR_CALIBRATION,
        ACTUATOR_CALIBRATION,
        SYSTEM_CALIBRATION
    };

    enum CalibrationState {
        IDLE,
        PREPARING,
        COLLECTING_DATA,
        CALCULATING,
        VALIDATING,
        COMPLETE,
        FAILED
    };

    struct CalibrationPoint {
        double referenceValue;    // Known reference value
        double measuredValue;     // Measured value from sensor/actuator
        QDateTime timestamp;      // When this point was collected
        bool valid;              // Whether this point is valid
        
        CalibrationPoint() : referenceValue(0.0), measuredValue(0.0), valid(false) {}
    };

    struct CalibrationResult {
        QString component;        // Component name (e.g., "AVL Sensor", "Tank Sensor")
        CalibrationType type;     // Type of calibration
        double slope;            // Calibration slope
        double offset;           // Calibration offset
        double correlation;      // Correlation coefficient (R²)
        double maxError;         // Maximum error in calibration range
        QDateTime timestamp;     // When calibration was performed
        bool successful;         // Whether calibration was successful
        QString errorMessage;    // Error message if failed
        QList<CalibrationPoint> points; // Calibration points used
        
        CalibrationResult() : type(SENSOR_CALIBRATION), slope(1.0), offset(0.0), 
                             correlation(0.0), maxError(0.0), successful(false) {}
    };

    explicit CalibrationManager(HardwareManager* hardware, QObject *parent = nullptr);
    ~CalibrationManager();

    // Calibration control
    void startSensorCalibration(const QString& sensorName);
    void startActuatorCalibration(const QString& actuatorName);
    void startSystemCalibration();
    void cancelCalibration();
    
    // Calibration state
    CalibrationState getCurrentState() const { return m_currentState; }
    QString getCurrentComponent() const { return m_currentComponent; }
    int getProgress() const { return m_progress; }
    
    // Calibration data management
    bool saveCalibrationData(const CalibrationResult& result);
    bool loadCalibrationData(const QString& component, CalibrationResult& result);
    QStringList getAvailableCalibrations() const;
    bool isComponentCalibrated(const QString& component) const;
    QDateTime getLastCalibrationDate(const QString& component) const;
    
    // Calibration validation
    bool validateCalibration(const QString& component);
    bool isCalibrationExpired(const QString& component, int maxDays = 30) const;
    QJsonObject getCalibrationStatus() const;
    
    // Configuration
    void setCalibrationDataPath(const QString& path);
    void setMinCalibrationPoints(int points) { m_minCalibrationPoints = points; }
    void setMaxCalibrationError(double maxError) { m_maxCalibrationError = maxError; }
    void setAutoSaveEnabled(bool enabled) { m_autoSaveEnabled = enabled; }

public slots:
    void addCalibrationPoint(double referenceValue, double measuredValue);
    void completeCurrentCalibration();
    void onCalibrationTimer();

signals:
    void calibrationStarted(const QString& component, CalibrationType type);
    void calibrationProgress(int percentage, const QString& status);
    void calibrationPointAdded(const CalibrationPoint& point);
    void calibrationCompleted(const CalibrationResult& result);
    void calibrationFailed(const QString& component, const QString& error);
    void calibrationDataSaved(const QString& component);
    void calibrationValidated(const QString& component, bool valid);

private slots:
    void performCalibrationStep();
    void collectCalibrationData();
    void calculateCalibrationParameters();
    void validateCalibrationResult();

private:
    void initializeCalibrationManager();
    void setupCalibrationTimer();
    void resetCalibrationState();
    
    // Calibration calculations
    bool calculateLinearCalibration(const QList<CalibrationPoint>& points, 
                                   double& slope, double& offset, double& correlation);
    double calculateCorrelationCoefficient(const QList<CalibrationPoint>& points, 
                                          double slope, double offset);
    double calculateMaxError(const QList<CalibrationPoint>& points, 
                            double slope, double offset);
    
    // Data persistence
    QString getCalibrationFilePath(const QString& component) const;
    bool saveCalibrationToFile(const CalibrationResult& result);
    bool loadCalibrationFromFile(const QString& component, CalibrationResult& result);
    
    // Sensor-specific calibration
    void calibrateAVLSensor();
    void calibrateTankSensor();
    void calibratePressureSensors();
    
    // Actuator-specific calibration
    void calibratePumpSpeed();
    void calibrateValveResponse();
    void calibrateActuators();
    
    // System-wide calibration
    void performSystemCalibration();
    void validateSystemCalibration();
    
    // Hardware interfaces
    HardwareManager* m_hardware;
    SensorInterface* m_sensorInterface;
    ActuatorControl* m_actuatorControl;
    DataLogger* m_dataLogger;
    
    // Calibration state
    CalibrationState m_currentState;
    QString m_currentComponent;
    CalibrationType m_currentType;
    int m_progress;
    int m_currentStep;
    int m_totalSteps;
    
    // Calibration data
    QList<CalibrationPoint> m_currentPoints;
    CalibrationResult m_currentResult;
    QMap<QString, CalibrationResult> m_calibrationCache;
    
    // Configuration
    QString m_calibrationDataPath;
    int m_minCalibrationPoints;
    double m_maxCalibrationError;
    bool m_autoSaveEnabled;
    
    // Timing
    QTimer* m_calibrationTimer;
    int m_calibrationInterval;
    QDateTime m_calibrationStartTime;
    
    // Thread safety
    mutable QMutex m_dataMutex;
    
    // Constants
    static const int DEFAULT_MIN_CALIBRATION_POINTS;
    static const double DEFAULT_MAX_CALIBRATION_ERROR;
    static const int DEFAULT_CALIBRATION_INTERVAL;
    static const int DEFAULT_CALIBRATION_TIMEOUT;
};

#endif // CALIBRATIONMANAGER_H
