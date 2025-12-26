#ifndef SAFETYMANAGER_H
#define SAFETYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include "../core/StatefulComponent.h"
#include "SafetyConstants.h"

// Forward declarations
class HardwareManager;
class CrashHandler;
class EmergencyStopCoordinator;
class ISafetyLogger;

/**
 * @brief Core safety management system for vacuum controller
 *
 * This class implements critical safety features including:
 * - Overpressure protection (max 75 mmHg with MPX5010DP sensor)
 * - Sensor error detection and response
 * - Emergency stop handling
 * - System health monitoring
 * - Automatic safety shutdowns
 *
 * Refactored to eliminate code duplication:
 * - Uses StatefulComponent<int> for state management (SafetyState enum)
 * - Uses EmergencyStopCoordinator for centralized emergency stop handling
 * - Uses ISafetyLogger for unified safety logging
 * - Uses SafeOperationHelper for consistent error handling
 */
class SafetyManager : public QObject, public StatefulComponent<int>
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

    // Safety status - uses StatefulComponent base class
    SafetyState getSafetyState() const { return static_cast<SafetyState>(getState()); }
    bool isSystemSafe() const { return getState() == SAFE; }
    bool isEmergencyStop() const { return getState() == EMERGENCY_STOP; }

    // Manual safety controls
    void triggerEmergencyStop(const QString& reason);
    bool resetEmergencyStop();

    // System health
    bool performSafetyCheck();
    QString getLastSafetyError() const { return m_lastSafetyError; }

    // Auto-recovery mechanisms
    void enableAutoRecovery(bool enabled);
    bool isAutoRecoveryEnabled() const { return m_autoRecoveryEnabled; }
    void setCrashHandler(CrashHandler* crashHandler);
    void performSystemRecovery();
    void handleSystemCrash(const QString& crashInfo);

    // Safety statistics
    int getOverpressureEvents() const { return m_overpressureEvents; }
    int getSensorErrorEvents() const { return m_sensorErrorEvents; }
    int getEmergencyStopEvents() const { return m_emergencyStopEvents; }
    int getRecoveryAttempts() const { return m_recoveryAttempts; }

    // Key safety thresholds (exposed for monitoring and tests)
    double tissueDamageRiskPressure() const { return SafetyConstants::TISSUE_DAMAGE_RISK_MMHG; }
    int monitoringIntervalMs() const { return MONITORING_INTERVAL_MS; }

    // NEW: Centralized emergency stop integration
    void setEmergencyStopCoordinator(EmergencyStopCoordinator* coordinator);

    // NEW: Unified safety logging
    void setSafetyLogger(ISafetyLogger* logger);

Q_SIGNALS:
    void safetyStateChanged(SafetyState newState);
    void overpressureDetected(double pressure);
    void sensorTimeout(const QString& sensor);
    void emergencyStopTriggered(const QString& reason);
    void systemError(const QString& error);
    void safetyWarning(const QString& warning);
    void systemRecoveryStarted();
    void systemRecoveryCompleted(bool success);
    void crashDetected(const QString& crashInfo);

private Q_SLOTS:
    void performSafetyMonitoring();
    void handleSensorError(const QString& sensor, const QString& error);
    void onCrashDetected(const QString& crashInfo);
    void onSystemStateRestored();

private:
    // State management - uses StatefulComponent base class
    void setState(SafetyState newState);
    QString stateToString(int state) const override;
    void onStateTransition(int oldState, int newState);

    // Emergency stop callback for coordinator
    void onEmergencyStopTriggered(const QString& reason);

    // Safety logging helper
    void logSafetyEvent(const QString& event);

    bool checkPressureLimits();
    bool checkSensorHealth();
    bool checkHardwareStatus();
    void handleOverpressure(double pressure);
    void handleCriticalError(const QString& error);
    void initializeSafetyParameters();

    // Helpers for advanced safety conditions
    bool isSensorDataValid(double avlPressure, double tankPressure) const;
    void evaluateRunawayAndInvalidSensors(double avlPressure, double tankPressure);

    // Hardware interface
    HardwareManager* m_hardware;
    CrashHandler* m_crashHandler;

    // NEW: Centralized emergency stop and logging
    EmergencyStopCoordinator* m_emergencyStopCoordinator;
    ISafetyLogger* m_safetyLogger;

    // Safety state - REMOVED m_safetyState (now in StatefulComponent base)
    bool m_active;
    // NOTE: m_stateMutex is inherited from StatefulComponent as protected

    // Safety parameters (as per specification)
    double m_maxPressure;        // Maximum allowed pressure (75 mmHg default; sensor FS)
    double m_warningThreshold;   // Warning threshold (80% of max)
    int m_sensorTimeoutMs;       // Sensor timeout in milliseconds

    // Monitoring
    QTimer* m_monitoringTimer;

    // Safety tracking
    QString m_lastSafetyError;
    qint64 m_lastAVLReading;     // Timestamp of last AVL reading
    qint64 m_lastTankReading;    // Timestamp of last tank reading

    // Runaway pump + invalid sensor detection
    int m_consecutiveInvalidSensorReadings;
    int m_consecutiveRunawaySamples;

    // Safety statistics
    int m_overpressureEvents;
    int m_sensorErrorEvents;
    int m_emergencyStopEvents;
    int m_recoveryAttempts;

    // Auto-recovery
    bool m_autoRecoveryEnabled;
    bool m_recoveryInProgress;

    // Safety constants - behavior-specific defaults kept here
    static const double DEFAULT_MAX_PRESSURE;      // 75.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD; // 60.0 mmHg
    static const int DEFAULT_SENSOR_TIMEOUT_MS;    // 1000 ms
    static const int MONITORING_INTERVAL_MS;       // 100 ms (10Hz)
    // NOTE: Use SafetyConstants::MAX_CONSECUTIVE_ERRORS, TISSUE_DAMAGE_RISK_MMHG directly

    // Error tracking
    int m_consecutiveErrors;
};

#endif // SAFETYMANAGER_H
