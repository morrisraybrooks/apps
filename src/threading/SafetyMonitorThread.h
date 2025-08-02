#ifndef SAFETYMONITORTHREAD_H
#define SAFETYMONITORTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QMutex>
#include <QtCore/QObject>

#ifndef QT_NO_KEYWORDS
#define slots Q_SLOTS
#define signals Q_SIGNALS
#endif

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

Q_SIGNALS:
    void safetyViolation(const QString& message);
    void emergencyStopTriggered();
    void emergencyStopRequired(const QString& reason);
    void pressureAlarm(double pressure, const QString& sensor);
    void monitoringStarted();
    void monitoringStopped();
    void monitoringError(const QString& error);
    void threadStarted();  // Emitted when thread is fully running

protected:
    void run() override;

private Q_SLOTS:
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
