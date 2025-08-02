#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QTimer>
#include <memory>

#ifndef QT_NO_KEYWORDS
#define slots Q_SLOTS
#define signals Q_SIGNALS
#endif

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

Q_SIGNALS:
    void allThreadsStarted();
    void allThreadsStopped();
    void threadError(const QString& threadName, const QString& error);
    void threadStateChanged(const QString& threadName, ThreadState state);
    void emergencyStopTriggered();

private Q_SLOTS:
    void onDataThreadStarted();
    void onDataThreadStopped();
    void onDataThreadError(const QString& error);
    
    void onGuiThreadStarted();
    void onGuiThreadStopped();
    void onGuiThreadError(const QString& error);
    
    void onSafetyThreadStarted();
    void onSafetyThreadStopped();
    void onSafetyThreadError(const QString& error);

    // Integrated safety monitoring slots
    void onSafetyViolation(const QString& message);
    void onSafetyWarning(const QString& message);

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



#endif // THREADMANAGER_H
