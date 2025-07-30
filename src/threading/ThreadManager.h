#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QObject>
#include <QMutex>
#include <memory>

// Forward declarations
class HardwareManager;
class DataAcquisitionThread;
class GuiUpdateThread;
class SafetyMonitorThread;

/**
 * @brief Central manager for all system threads
 * 
 * This class coordinates the multi-threaded architecture of the vacuum controller:
 * - Data Acquisition Thread (50Hz sensor sampling)
 * - GUI Update Thread (30 FPS display updates)
 * - Safety Monitor Thread (100Hz safety checks)
 * - Pattern Execution Thread (precise timing control)
 * 
 * Provides thread lifecycle management, synchronization, and error handling.
 */
class ThreadManager : public QObject
{
    Q_OBJECT

public:
    enum ThreadState {
        STOPPED,
        STARTING,
        RUNNING,
        PAUSING,
        PAUSED,
        STOPPING,
        ERROR
    };

    explicit ThreadManager(HardwareManager* hardware, QObject *parent = nullptr);
    ~ThreadManager();

    // Thread lifecycle
    bool startAllThreads();
    void stopAllThreads();
    void pauseAllThreads();
    void resumeAllThreads();
    
    // Individual thread control
    bool startDataAcquisition();
    bool startGuiUpdates();
    bool startSafetyMonitoring();
    
    void stopDataAcquisition();
    void stopGuiUpdates();
    void stopSafetyMonitoring();
    
    // Thread access
    DataAcquisitionThread* getDataAcquisitionThread() const { return m_dataThread.get(); }
    GuiUpdateThread* getGuiUpdateThread() const { return m_guiThread.get(); }
    SafetyMonitorThread* getSafetyMonitorThread() const { return m_safetyThread.get(); }
    
    // Status
    ThreadState getOverallState() const { return m_overallState; }
    bool areAllThreadsRunning() const;
    bool areAllThreadsStopped() const;
    
    // Configuration
    void setDataAcquisitionRate(int hz);
    void setGuiUpdateRate(int fps);
    void setSafetyMonitorRate(int hz);
    
    // Statistics
    QString getThreadStatistics() const;
    int getActiveThreadCount() const;
    
    // Emergency controls
    void emergencyStopAllThreads();
    bool resetAfterEmergencyStop();

signals:
    void allThreadsStarted();
    void allThreadsStopped();
    void threadError(const QString& threadName, const QString& error);
    void threadStateChanged(const QString& threadName, ThreadState state);
    void emergencyStopTriggered();

private slots:
    void onDataThreadStarted();
    void onDataThreadStopped();
    void onDataThreadError(const QString& error);
    
    void onGuiThreadStarted();
    void onGuiThreadStopped();
    void onGuiThreadError(const QString& error);
    
    void onSafetyThreadStarted();
    void onSafetyThreadStopped();
    void onSafetyThreadError(const QString& error);

private:
    void updateOverallState();
    void connectThreadSignals();
    void initializeThreads();
    void cleanupThreads();
    bool waitForThreadsToStop(int timeoutMs = 5000);

    // Hardware interface
    HardwareManager* m_hardware;
    
    // Thread instances
    std::unique_ptr<DataAcquisitionThread> m_dataThread;
    std::unique_ptr<GuiUpdateThread> m_guiThread;
    std::unique_ptr<SafetyMonitorThread> m_safetyThread;
    
    // State management
    ThreadState m_overallState;
    mutable QMutex m_stateMutex;
    
    // Thread states
    bool m_dataThreadRunning;
    bool m_guiThreadRunning;
    bool m_safetyThreadRunning;
    
    // Error tracking
    QString m_lastError;
    int m_errorCount;
    
    // Emergency stop state
    bool m_emergencyStop;
    
    // Configuration
    int m_dataAcquisitionRateHz;
    int m_guiUpdateRateFps;
    int m_safetyMonitorRateHz;
    
    // Constants
    static const int DEFAULT_DATA_RATE_HZ = 50;
    static const int DEFAULT_GUI_RATE_FPS = 30;
    static const int DEFAULT_SAFETY_RATE_HZ = 100;
    static const int THREAD_STOP_TIMEOUT_MS = 5000;
    static const int MAX_THREAD_ERRORS = 5;
};

/**
 * @brief Safety monitoring thread for critical system checks
 * 
 * This high-priority thread performs safety checks at 100Hz to ensure
 * rapid response to dangerous conditions.
 */
class SafetyMonitorThread : public QThread
{
    Q_OBJECT

public:
    explicit SafetyMonitorThread(HardwareManager* hardware, QObject *parent = nullptr);
    ~SafetyMonitorThread();

    void startMonitoring();
    void stopMonitoring();
    void setMonitoringRate(int hz);
    
    bool isMonitoring() const { return m_monitoring; }
    int getMonitoringRate() const { return m_monitoringRateHz; }

signals:
    void safetyViolation(const QString& violation);
    void emergencyStopRequired(const QString& reason);
    void monitoringStarted();
    void monitoringStopped();
    void monitoringError(const QString& error);

protected:
    void run() override;

private slots:
    void performSafetyCheck();

private:
    void initializeMonitoring();
    void cleanupMonitoring();
    bool checkPressureLimits();
    bool checkSensorHealth();
    bool checkHardwareStatus();

    HardwareManager* m_hardware;
    bool m_monitoring;
    bool m_stopRequested;
    int m_monitoringRateHz;
    int m_monitoringIntervalMs;
    QTimer* m_monitoringTimer;
    
    // Safety thresholds
    double m_maxPressure;
    double m_criticalPressure;
    int m_consecutiveErrors;
    
    static const int DEFAULT_MONITORING_RATE_HZ = 100;
    static const int MAX_CONSECUTIVE_ERRORS = 3;
};

#endif // THREADMANAGER_H
