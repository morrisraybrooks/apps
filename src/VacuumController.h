#ifndef VACUUMCONTROLLER_H
#define VACUUMCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <memory>

// Forward declarations
class HardwareManager;
class SafetyManager;
class PatternEngine;
class AntiDetachmentMonitor;
class ThreadManager;

/**
 * @brief Main controller class for the vacuum therapy system
 * 
 * This class coordinates all subsystems including hardware control,
 * safety monitoring, pattern execution, and user interface updates.
 * It serves as the central hub for the entire vacuum controller system.
 */
class VacuumController : public QObject
{
    Q_OBJECT

public:
    explicit VacuumController(QObject *parent = nullptr);
    ~VacuumController();

    // System initialization and shutdown
    bool initialize();
    void shutdown();
    
    // System state
    enum SystemState {
        STOPPED,
        RUNNING,
        PAUSED,
        ERROR,
        EMERGENCY_STOP
    };
    
    SystemState getSystemState() const { return m_systemState; }
    bool isSystemReady() const;
    
    // Hardware access
    HardwareManager* getHardwareManager() const { return m_hardwareManager.get(); }
    SafetyManager* getSafetyManager() const { return m_safetyManager.get(); }
    PatternEngine* getPatternEngine() const { return m_patternEngine.get(); }
    ThreadManager* getThreadManager() const { return m_threadManager.get(); }
    
    // Pattern control
    void startPattern(const QString& patternName);
    void stopPattern();
    void pausePattern();
    void resumePattern();
    
    // Safety controls
    void emergencyStop();
    void resetEmergencyStop();
    
    // Pressure readings (in mmHg)
    double getAVLPressure() const { return m_avlPressure; }
    double getTankPressure() const { return m_tankPressure; }
    
    // System settings
    void setMaxPressure(double maxPressure);
    double getMaxPressure() const { return m_maxPressure; }
    
    void setAntiDetachmentThreshold(double threshold);
    double getAntiDetachmentThreshold() const { return m_antiDetachmentThreshold; }

public slots:
    void updateSensorReadings();
    void handleEmergencyStop();
    void handleSystemError(const QString& error);

signals:
    void systemStateChanged(SystemState newState);
    void pressureUpdated(double avlPressure, double tankPressure);
    void emergencyStopTriggered();
    void systemError(const QString& error);
    void antiDetachmentActivated();
    void patternStarted(const QString& patternName);
    void patternStopped();

private slots:
    void onUpdateTimer();

private:
    void setState(SystemState newState);
    void initializeSubsystems();
    void connectSignals();

    // Subsystem managers
    std::unique_ptr<HardwareManager> m_hardwareManager;
    std::unique_ptr<SafetyManager> m_safetyManager;
    std::unique_ptr<PatternEngine> m_patternEngine;
    std::unique_ptr<AntiDetachmentMonitor> m_antiDetachmentMonitor;
    std::unique_ptr<ThreadManager> m_threadManager;
    
    // System state
    SystemState m_systemState;
    mutable QMutex m_stateMutex;
    
    // Sensor data
    double m_avlPressure;      // Applied Vacuum Line pressure
    double m_tankPressure;     // Tank vacuum pressure
    mutable QMutex m_dataMutex;
    
    // System parameters
    double m_maxPressure;              // Maximum allowed pressure (mmHg)
    double m_antiDetachmentThreshold;  // Anti-detachment activation threshold
    
    // Update timer for real-time monitoring
    QTimer* m_updateTimer;
    
    // System status
    bool m_initialized;
    QString m_lastError;
};

#endif // VACUUMCONTROLLER_H
