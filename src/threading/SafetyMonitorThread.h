#ifndef SAFETYMONITORTHREAD_H
#define SAFETYMONITORTHREAD_H

#include <QThread>
#include <QTimer>
#include <QMutex>

// Forward declarations
class HardwareManager;

/**
 * @brief Thread for monitoring safety conditions
 */
class SafetyMonitorThread : public QThread
{
    Q_OBJECT

public:
    explicit SafetyMonitorThread(HardwareManager* hardware, QObject *parent = nullptr);
    ~SafetyMonitorThread();

    // Thread control
    void startMonitoring();
    void stopMonitoring();
    void setMonitoringRate(int rateHz);

    // Status
    bool isMonitoring() const { return m_monitoring; }
    int getMonitoringRate() const { return m_monitoringRateHz; }

signals:
    void safetyViolation(const QString& message);
    void emergencyStopTriggered();
    void emergencyStopRequired(const QString& reason);
    void pressureAlarm(double pressure, const QString& sensor);
    void monitoringStarted();
    void monitoringStopped();
    void monitoringError(const QString& error);

protected:
    void run() override;

private slots:
    void performSafetyCheck();

private:
    void checkPressureLimits();
    void checkEmergencyStop();
    void checkSystemHealth();

    HardwareManager* m_hardware;
    QTimer* m_monitorTimer;
    bool m_monitoring;
    bool m_stopRequested;
    int m_monitoringRateHz;
    mutable QMutex m_mutex;

    // Safety thresholds
    static const double MAX_SAFE_PRESSURE;
    static const double MIN_SAFE_PRESSURE;
    static const int DEFAULT_MONITORING_RATE_HZ = 20;
};

#endif // SAFETYMONITORTHREAD_H
