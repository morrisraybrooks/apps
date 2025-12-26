#ifndef ANTIDETACHMENTMONITOR_H
#define ANTIDETACHMENTMONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include "../core/StatefulComponent.h"

// Forward declarations
class HardwareManager;
class EmergencyStopCoordinator;
class ISafetyLogger;

/**
 * @brief Safety-critical anti-detachment monitoring system
 *
 * This is the most critical safety feature of the vacuum controller.
 * It continuously monitors the Applied Vacuum Line (AVL) pressure and
 * automatically increases vacuum if cup detachment is detected.
 *
 * Key Features:
 * - High-frequency monitoring (100Hz) for rapid response
 * - Adjustable threshold settings
 * - Automatic SOL1 valve control
 * - Fail-safe operation
 * - Unified logging via ISafetyLogger (no custom CSV logging)
 * - Centralized emergency stop via EmergencyStopCoordinator
 * - Redundant safety checks
 *
 * Refactored to eliminate code duplication:
 * - Uses StatefulComponent<DetachmentState> for state management
 * - Uses ISafetyLogger for all safety logging (delegates to DataLogger)
 * - Uses EmergencyStopCoordinator for emergency stop integration
 * - Uses SafeOperationHelper for consistent error handling
 */
class AntiDetachmentMonitor : public QObject, public StatefulComponent<int>
{
    Q_OBJECT

public:
    /**
     * @brief Detachment states for the monitoring system
     * Note: Using int for StatefulComponent template to allow enum flexibility
     */
    enum DetachmentState {
        ATTACHED = 0,       // Cup properly attached
        WARNING = 1,        // Pressure approaching threshold
        DETACHMENT_RISK = 2,// High risk of detachment
        DETACHED = 3,       // Cup detached - emergency action
        SYSTEM_ERROR = 4    // Monitor system error
    };
    Q_ENUM(DetachmentState)

    explicit AntiDetachmentMonitor(HardwareManager* hardware, QObject *parent = nullptr);
    ~AntiDetachmentMonitor();

    // System control
    bool initialize();
    void shutdown();
    bool isActive() const { return m_active; }
    
    // Monitoring control
    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();
    
    // Threshold configuration
    void setThreshold(double thresholdMmHg);
    double getThreshold() const { return m_detachmentThreshold; }
    
    void setWarningThreshold(double thresholdMmHg);
    double getWarningThreshold() const { return m_warningThreshold; }
    
    void setHysteresis(double hysteresisMmHg);
    double getHysteresis() const { return m_hysteresis; }
    
    // Response configuration
    void setResponseDelay(int delayMs);
    int getResponseDelay() const { return m_responseDelayMs; }
    
    void setMaxVacuumIncrease(double maxIncrease);
    double getMaxVacuumIncrease() const { return m_maxVacuumIncrease; }
    
    // Status
    DetachmentState getCurrentState() const { return static_cast<DetachmentState>(getState()); }
    double getCurrentAVLPressure() const { return m_currentAVLPressure; }
    bool isSOL1Active() const { return m_sol1Active; }

    // Statistics
    int getDetachmentEvents() const { return m_detachmentEvents; }
    int getWarningEvents() const { return m_warningEvents; }
    qint64 getLastDetachmentTime() const { return m_lastDetachmentTime; }
    double getAverageResponseTime() const { return m_averageResponseTime; }

    // Safety validation
    bool performSelfTest();
    QString getLastError() const { return m_lastError; }

    // NEW: Centralized emergency stop integration (replaces SafetyManager direct calls)
    void setEmergencyStopCoordinator(EmergencyStopCoordinator* coordinator);

    // NEW: Unified safety logging (replaces custom CSV logging)
    void setSafetyLogger(ISafetyLogger* logger);

    // Error recovery
    bool resetSystemError();

    // DEPRECATED: Use setSafetyLogger instead (maintained for backward compatibility)
    void setSafetyLogPath(const QString& logPath);
    QString getSafetyLogPath() const { return m_safetyLogPath; }

Q_SIGNALS:
    void detachmentDetected(double avlPressure);
    void detachmentWarning(double avlPressure);
    void detachmentResolved();
    void stateChanged(DetachmentState newState);
    void sol1Activated(double targetPressure);
    void sol1Deactivated();
    void systemError(const QString& error);
    void selfTestCompleted(bool passed);

private Q_SLOTS:
    void performMonitoringCycle();
    void onResponseTimer();

private:
    // State management - uses StatefulComponent base class
    void setState(DetachmentState newState);
    QString stateToString(int state) const override;
    void onStateTransition(int oldState, int newState);

    void processAVLReading(double avlPressure);
    void activateAntiDetachment();
    void deactivateAntiDetachment();
    void handleDetachmentEvent();
    void handleWarningEvent();
    void updateStatistics();

    // Logging via ISafetyLogger (unified logging, no custom CSV)
    void logEvent(const QString& event, double pressure);

    // NOTE: Use SafetyConstants::isValidPressure() for pressure validation
    double calculateTargetVacuum(double currentPressure);
    void applyVacuumCorrection(double targetPressure);

    // Emergency stop callback for coordinator
    void onEmergencyStopTriggered(const QString& reason);

    // Hardware interface
    HardwareManager* m_hardware;

    // System state (m_stateMutex now in StatefulComponent base)
    bool m_active;
    bool m_monitoring;
    bool m_paused;

    // Monitoring configuration
    double m_detachmentThreshold;    // Main threshold (mmHg)
    double m_warningThreshold;       // Warning threshold (mmHg)
    double m_hysteresis;             // Hysteresis to prevent oscillation
    int m_monitoringRateHz;          // Monitoring frequency
    int m_responseDelayMs;           // Delay before response
    double m_maxVacuumIncrease;      // Maximum vacuum increase (%)

    // Current readings
    double m_currentAVLPressure;
    QQueue<double> m_pressureHistory;  // For trend analysis
    qint64 m_lastReadingTime;

    // Response system
    bool m_sol1Active;
    double m_targetVacuumLevel;
    QTimer* m_responseTimer;
    qint64 m_detectionTime;

    // Statistics
    int m_detachmentEvents;
    int m_warningEvents;
    qint64 m_lastDetachmentTime;
    double m_totalResponseTime;
    int m_responseCount;
    double m_averageResponseTime;

    // Monitoring timer
    QTimer* m_monitoringTimer;

    // Error handling
    QString m_lastError;
    int m_consecutiveErrors;

    // NEW: Centralized emergency stop (replaces m_safetyManager)
    EmergencyStopCoordinator* m_emergencyStopCoordinator;

    // NEW: Unified safety logging (replaces file-based logging)
    ISafetyLogger* m_safetyLogger;

    // DEPRECATED: Legacy logging path (for backward compatibility)
    QString m_safetyLogPath;

    // Constants - behavior-specific values kept here, common ones use SafetyConstants
    static const double DEFAULT_DETACHMENT_THRESHOLD;  // 50.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD;     // 60.0 mmHg
    static const double DEFAULT_HYSTERESIS;            // 5.0 mmHg
    static const int DEFAULT_MONITORING_RATE_HZ;       // 100 Hz
    static const int DEFAULT_RESPONSE_DELAY_MS;        // 100 ms
    static const double DEFAULT_MAX_VACUUM_INCREASE;   // 20.0%
    static const int PRESSURE_HISTORY_SIZE;            // 10 samples
    // NOTE: Use SafetyConstants::MAX_CONSECUTIVE_ERRORS, MIN_VALID_PRESSURE, MAX_VALID_PRESSURE directly
};

#endif // ANTIDETACHMENTMONITOR_H
