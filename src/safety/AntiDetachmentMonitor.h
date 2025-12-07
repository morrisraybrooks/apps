#ifndef ANTIDETACHMENTMONITOR_H
#define ANTIDETACHMENTMONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QDateTime>

// Forward declarations
class HardwareManager;

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
 * - Comprehensive logging for safety compliance
 * - Redundant safety checks
 */
class AntiDetachmentMonitor : public QObject
{
    Q_OBJECT

public:
    enum DetachmentState {
        ATTACHED,           // Cup properly attached
        WARNING,            // Pressure approaching threshold
        DETACHMENT_RISK,    // High risk of detachment
        DETACHED,           // Cup detached - emergency action
        SYSTEM_ERROR        // Monitor system error
    };

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
    DetachmentState getCurrentState() const { return m_currentState; }
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
    void setState(DetachmentState newState);
    void processAVLReading(double avlPressure);
    void activateAntiDetachment();
    void deactivateAntiDetachment();
    void handleDetachmentEvent();
    void handleWarningEvent();
    void updateStatistics();
    void logEvent(const QString& event, double pressure);
    
    bool validatePressureReading(double pressure);
    double calculateTargetVacuum(double currentPressure);
    void applyVacuumCorrection(double targetPressure);
    
    // Hardware interface
    HardwareManager* m_hardware;
    
    // System state
    bool m_active;
    bool m_monitoring;
    bool m_paused;
    DetachmentState m_currentState;
    DetachmentState m_previousState;
    mutable QMutex m_stateMutex;
    
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
    
    // Statistics and logging
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
    
    // Constants
    static const double DEFAULT_DETACHMENT_THRESHOLD;  // 50.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD;     // 60.0 mmHg
    static const double DEFAULT_HYSTERESIS;            // 5.0 mmHg
    static const int DEFAULT_MONITORING_RATE_HZ;       // 100 Hz
    static const int DEFAULT_RESPONSE_DELAY_MS;        // 100 ms
    static const double DEFAULT_MAX_VACUUM_INCREASE;   // 20.0%
    static const int PRESSURE_HISTORY_SIZE;            // 10 samples
    static const int MAX_CONSECUTIVE_ERRORS;           // 3 errors
    static const double MIN_VALID_PRESSURE;            // 0.0 mmHg
    static const double MAX_VALID_PRESSURE;            // 200.0 mmHg
};

#endif // ANTIDETACHMENTMONITOR_H
