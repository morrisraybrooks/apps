#ifndef MOCKVACUUMCONTROLLER_H
#define MOCKVACUUMCONTROLLER_H

#include <QObject>
#include <QMutex>
#include <QJsonObject>
#include <QStringList>

/**
 * @brief Mock vacuum controller for testing
 * 
 * Provides a complete mock implementation of the main vacuum controller
 * for testing purposes. Simulates all high-level system operations
 * including pattern execution, data logging, calibration, and safety systems.
 */
class MockVacuumController : public QObject
{
    Q_OBJECT

public:
    explicit MockVacuumController(QObject *parent = nullptr);
    ~MockVacuumController();

    // System control
    bool initialize();
    void shutdown();
    bool isSystemReady() const;
    bool performSystemSelfCheck();
    QJsonObject getSystemStatus() const;
    
    // Pattern control
    bool startPattern(const QString& patternName, const QJsonObject& parameters);
    bool stopPattern();
    bool isPatternRunning(const QString& patternName = QString()) const;
    QString getCurrentPattern() const;
    
    // Data logging
    bool startDataLogging();
    bool stopDataLogging();
    bool isDataLogging() const;
    QStringList getLoggedData() const;
    void addLogEntry(const QString& entry);
    
    // Emergency and safety
    void triggerEmergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStop() const;
    bool isSystemInSafeMode() const;
    void enterSafeMode(const QString& reason);
    void exitSafeMode();
    
    // Calibration
    bool startCalibration();
    bool stopCalibration();
    bool isCalibrationMode() const;

signals:
    // System signals
    void systemInitialized();
    void systemShutdown();
    void systemStatusChanged(const QString& status);
    void selfCheckCompleted(bool passed);
    
    // Pattern signals
    void patternStarted(const QString& patternName);
    void patternStopped(const QString& patternName);
    void patternStatusChanged(const QString& patternName, const QString& status);
    
    // Data logging signals
    void dataLoggingStarted();
    void dataLoggingStopped();
    void dataLogged(const QString& entry);
    
    // Safety signals
    void emergencyStopTriggered();
    void emergencyStopReset();
    void safeModeEntered(const QString& reason);
    void safeModeExited();
    
    // Calibration signals
    void calibrationStarted();
    void calibrationStopped();

private:
    // Helper methods
    bool validatePatternParameters(const QJsonObject& parameters) const;
    
    // System state
    bool m_initialized;
    bool m_systemReady;
    bool m_emergencyStop;
    bool m_safeMode;
    bool m_calibrationMode;
    
    // Pattern state
    QString m_currentPattern;
    bool m_patternRunning;
    QJsonObject m_patternParameters;
    
    // Data logging state
    bool m_dataLogging;
    QStringList m_loggedData;
    
    // Thread safety
    mutable QMutex m_mutex;
};

#endif // MOCKVACUUMCONTROLLER_H
