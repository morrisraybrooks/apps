#ifndef LIGHTWEIGHTSAFETYMONITOR_H
#define LIGHTWEIGHTSAFETYMONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QElapsedTimer>

// Forward declarations
class HardwareManager;

/**
 * @brief Lightweight safety monitor designed for EGLFS compatibility
 * 
 * This monitor runs in the main thread using a timer-based approach
 * to avoid thread conflicts with EGLFS rendering.
 */
class LightweightSafetyMonitor : public QObject
{
    Q_OBJECT

public:
    explicit LightweightSafetyMonitor(HardwareManager* hardware, QObject *parent = nullptr);
    ~LightweightSafetyMonitor();

    // Control methods
    void startMonitoring();
    void stopMonitoring();
    void setMonitoringRate(int hz);
    
    // Status methods
    bool isMonitoring() const { return m_active; }
    int getMonitoringRate() const { return m_monitoringRateHz; }
    
    // Safety thresholds
    void setMaxPressure(double maxPressure) { m_maxPressure = maxPressure; }
    void setWarningThreshold(double warningThreshold) { m_warningThreshold = warningThreshold; }
    
    double getMaxPressure() const { return m_maxPressure; }
    double getWarningThreshold() const { return m_warningThreshold; }

Q_SIGNALS:
    void safetyViolation(const QString& message);
    void pressureWarning(double pressure, const QString& sensor);
    void pressureAlarm(double pressure, const QString& sensor);
    void emergencyStopRequired(const QString& reason);
    void monitoringStarted();
    void monitoringStopped();
    void systemHealthWarning(const QString& message);

private Q_SLOTS:
    void performSafetyCheck();

private:
    // Safety check methods
    void checkPressureLimits();
    void checkSystemHealth();
    void checkHardwareStatus();
    
    // Utility methods
    // NOTE: Use SafetyConstants::isValidPressure() for pressure validation
    void logSafetyEvent(const QString& event);

    // Hardware interface
    HardwareManager* m_hardware;
    
    // Monitoring control
    QTimer* m_monitorTimer;
    bool m_active;
    int m_monitoringRateHz;
    
    // Safety thresholds
    double m_maxPressure;
    double m_warningThreshold;
    
    // Performance tracking
    QElapsedTimer m_performanceTimer;
    int m_checkCount;
    double m_averageCheckTime;
    
    // Error tracking
    int m_consecutiveErrors;
    QString m_lastError;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Constants - most are now accessed via SafetyConstants namespace
    static const double DEFAULT_MAX_PRESSURE;      // 75.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD; // 60.0 mmHg
    static const int DEFAULT_MONITORING_RATE_HZ;   // 20 Hz (reduced for GUI compatibility)
    // NOTE: Use SafetyConstants::MAX_CONSECUTIVE_ERRORS, MIN_VALID_PRESSURE, MAX_VALID_PRESSURE directly
};

#endif // LIGHTWEIGHTSAFETYMONITOR_H
