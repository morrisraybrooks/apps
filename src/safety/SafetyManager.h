#ifndef SAFETYMANAGER_H
#define SAFETYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>

// Forward declarations
class HardwareManager;

/**
 * @brief Core safety management system for vacuum controller
 * 
 * This class implements critical safety features including:
 * - Overpressure protection (max 100 mmHg as per specification)
 * - Sensor error detection and response
 * - Emergency stop handling
 * - System health monitoring
 * - Automatic safety shutdowns
 */
class SafetyManager : public QObject
{
    Q_OBJECT

public:
    explicit SafetyManager(HardwareManager* hardware, QObject *parent = nullptr);
    ~SafetyManager();

    // Safety system states
    enum SafetyState {
        SAFE,           // All systems normal
        WARNING,        // Warning condition detected
        CRITICAL,       // Critical condition - immediate action required
        EMERGENCY_STOP  // Emergency stop activated
    };

    // Initialization
    bool initialize();
    void shutdown();
    bool isActive() const { return m_active; }

    // Safety configuration
    void setMaxPressure(double maxPressure);
    double getMaxPressure() const { return m_maxPressure; }
    
    void setWarningThreshold(double warningThreshold);
    double getWarningThreshold() const { return m_warningThreshold; }
    
    void setSensorTimeoutMs(int timeoutMs);
    int getSensorTimeoutMs() const { return m_sensorTimeoutMs; }

    // Safety status
    SafetyState getSafetyState() const { return m_safetyState; }
    bool isSystemSafe() const { return m_safetyState == SAFE; }
    bool isEmergencyStop() const { return m_safetyState == EMERGENCY_STOP; }
    
    // Manual safety controls
    void triggerEmergencyStop(const QString& reason);
    bool resetEmergencyStop();
    
    // System health
    bool performSafetyCheck();
    QString getLastSafetyError() const { return m_lastSafetyError; }
    
    // Safety statistics
    int getOverpressureEvents() const { return m_overpressureEvents; }
    int getSensorErrorEvents() const { return m_sensorErrorEvents; }
    int getEmergencyStopEvents() const { return m_emergencyStopEvents; }

signals:
    void safetyStateChanged(SafetyState newState);
    void overpressureDetected(double pressure);
    void sensorTimeout(const QString& sensor);
    void emergencyStopTriggered(const QString& reason);
    void systemError(const QString& error);
    void safetyWarning(const QString& warning);

private slots:
    void performSafetyMonitoring();
    void handleSensorError(const QString& sensor, const QString& error);

private:
    void setState(SafetyState newState);
    bool checkPressureLimits();
    bool checkSensorHealth();
    bool checkHardwareStatus();
    void handleOverpressure(double pressure);
    void handleCriticalError(const QString& error);
    void initializeSafetyParameters();

    // Hardware interface
    HardwareManager* m_hardware;
    
    // Safety state
    SafetyState m_safetyState;
    bool m_active;
    mutable QMutex m_stateMutex;
    
    // Safety parameters (as per specification)
    double m_maxPressure;        // Maximum allowed pressure (100 mmHg)
    double m_warningThreshold;   // Warning threshold (80% of max)
    int m_sensorTimeoutMs;       // Sensor timeout in milliseconds
    
    // Monitoring
    QTimer* m_monitoringTimer;
    
    // Safety tracking
    QString m_lastSafetyError;
    qint64 m_lastAVLReading;     // Timestamp of last AVL reading
    qint64 m_lastTankReading;    // Timestamp of last tank reading
    
    // Safety statistics
    int m_overpressureEvents;
    int m_sensorErrorEvents;
    int m_emergencyStopEvents;
    
    // Safety constants
    static const double DEFAULT_MAX_PRESSURE;      // 100.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD; // 80.0 mmHg
    static const int DEFAULT_SENSOR_TIMEOUT_MS;    // 1000 ms
    static const int MONITORING_INTERVAL_MS;       // 100 ms (10Hz)
    static const int MAX_CONSECUTIVE_ERRORS;       // 3 errors before emergency stop
    
    // Error tracking
    int m_consecutiveErrors;
};

#endif // SAFETYMANAGER_H
