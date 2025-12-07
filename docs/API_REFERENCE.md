# Vacuum Controller API Reference

## Overview

This document provides comprehensive API documentation for the Vacuum Controller system, including all major classes, methods, signals, and integration points.

## Core Classes

### VacuumController

The main controller class that orchestrates all system components.

```cpp
class VacuumController : public QObject
{
    Q_OBJECT

public:
    enum SystemState {
        UNINITIALIZED,
        INITIALIZING,
        READY,
        RUNNING,
        PAUSED,
        ERROR,
        EMERGENCY_STOP
    };

    explicit VacuumController(QObject *parent = nullptr);
    ~VacuumController();

    // System control
    bool initialize();
    void shutdown();
    bool isReady() const;
    SystemState getSystemState() const;

    // Pattern control
    bool startPattern(const QString& patternName);
    void stopPattern();
    void pausePattern();
    void resumePattern();
    QString getCurrentPattern() const;

    // Real-time data
    double getAVLPressure() const;
    double getTankPressure() const;
    QJsonObject getSystemStatus() const;

    // Safety control
    void emergencyStop();
    bool resetEmergencyStop();
    bool isEmergencyStopActive() const;

signals:
    void systemStateChanged(SystemState newState);
    void pressureUpdated(double avlPressure, double tankPressure);
    void patternStarted(const QString& patternName);
    void patternStopped();
    void emergencyStopTriggered();
    void safetyAlert(const QString& message);
};
```

### HardwareManager

Manages all hardware interfaces including GPIO, SPI, and sensors.

```cpp
class HardwareManager : public QObject
{
    Q_OBJECT

public:
    explicit HardwareManager(QObject *parent = nullptr);
    ~HardwareManager();

    // Initialization
    bool initialize();
    void shutdown();
    bool isReady() const;

    // Sensor interface
    double readAVLPressure();
    double readTankPressure();
    bool areSensorsReady() const;

    // Actuator control
    void setSOL1(bool enabled);  // AVL valve
    void setSOL2(bool enabled);  // AVL vent
    void setSOL3(bool enabled);  // Tank vent
    void setPumpEnabled(bool enabled);
    void setPumpSpeed(double speedPercent);

    // Hardware status
    bool getSOL1State() const;
    bool getSOL2State() const;
    bool getSOL3State() const;
    bool getPumpState() const;
    double getPumpSpeed() const;

    // Emergency functions
    void emergencyStop();
    bool resetEmergencyStop();

    // Hardware testing
    bool performSelfTest();
    QJsonObject getHardwareStatus() const;

signals:
    void sensorDataReady(double avlPressure, double tankPressure);
    void hardwareError(const QString& component, const QString& error);
    void actuatorStateChanged(const QString& actuator, bool state);
};
```

### SafetyManager

Coordinates all safety systems and monitoring.

```cpp
class SafetyManager : public QObject
{
    Q_OBJECT

public:
    explicit SafetyManager(HardwareManager* hardware, QObject *parent = nullptr);
    ~SafetyManager();

    // Safety system control
    bool initialize();
    void shutdown();
    bool isSystemSafe() const;

    // Safety limits
    void setMaxPressure(double maxPressure);
    void setWarningThreshold(double threshold);
    void setAntiDetachmentThreshold(double threshold);

    // Safety status
    bool isAntiDetachmentActive() const;
    bool isOverpressureDetected() const;
    bool isEmergencyStopActive() const;

    // Safety testing
    bool performSafetyTest();
    QJsonObject getSafetyStatus() const;

signals:
    void safetyViolation(const QString& type, double value);
    void antiDetachmentActivated(double pressure);
    void overpressureDetected(double pressure);
    void safetySystemError(const QString& error);
};
```

### PatternEngine

Executes vacuum patterns with precise timing and safety integration.

```cpp
class PatternEngine : public QObject
{
    Q_OBJECT

public:
    enum PatternState {
        STOPPED,
        STARTING,
        RUNNING,
        PAUSED,
        STOPPING,
        ERROR
    };

    explicit PatternEngine(HardwareManager* hardware, QObject *parent = nullptr);
    ~PatternEngine();

    // Pattern execution
    bool startPattern(const QString& patternName);
    void stopPattern();
    void pausePattern();
    void resumePattern();
    void emergencyStop();

    // Pattern status
    QString getCurrentPattern() const;
    PatternState getState() const;
    int getCurrentStep() const;
    int getTotalSteps() const;
    double getProgress() const;
    qint64 getElapsedTime() const;

    // Real-time adjustments
    void setIntensity(double intensityPercent);
    void setSpeed(double speedMultiplier);
    void setPressureOffset(double offsetPercent);

    double getIntensity() const;
    double getSpeed() const;
    double getPressureOffset() const;

signals:
    void patternStarted(const QString& patternName);
    void patternStopped();
    void patternCompleted();
    void patternError(const QString& error);
    void stepChanged(int currentStep, int totalSteps);
    void progressUpdated(double progress);
};
```

## GUI Classes

### MainWindow

The main application window containing all GUI components.

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Window management
    void showFullScreen();
    void showNormal();
    void setTouchMode(bool enabled);

    // Component access
    PressureMonitor* getPressureMonitor() const;
    PatternSelector* getPatternSelector() const;
    SafetyPanel* getSafetyPanel() const;

public slots:
    void updateSystemStatus();
    void showSettingsDialog();
    void showAboutDialog();

signals:
    void emergencyStopRequested();
    void settingsChanged();
};
```

### PressureMonitor

Real-time pressure monitoring and visualization.

```cpp
class PressureMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit PressureMonitor(VacuumController* controller, QWidget *parent = nullptr);
    ~PressureMonitor();

    // Display configuration
    void setTimeRange(int seconds);
    void setAutoScale(bool enabled);
    void setPressureRange(double min, double max);

    // Data management
    void clearHistory();
    void exportData(const QString& filePath);

public slots:
    void updatePressure(double avlPressure, double tankPressure);
    void onThresholdChanged(double threshold);

signals:
    void pressureAlarm(const QString& type, double pressure);
    void dataExported(const QString& filePath);
};
```

### PatternSelector

Pattern selection and configuration interface.

```cpp
class PatternSelector : public QWidget
{
    Q_OBJECT

public:
    explicit PatternSelector(VacuumController* controller, QWidget *parent = nullptr);
    ~PatternSelector();

    // Pattern management
    void loadPatterns();
    void refreshPatternList();
    QString getSelectedPattern() const;

    // Pattern configuration
    QJsonObject getCurrentParameters() const;
    void setPatternParameters(const QJsonObject& parameters);

public slots:
    void selectPattern(const QString& patternName);
    void onParameterChanged();

signals:
    void patternSelected(const QString& patternName);
    void parametersChanged(const QString& patternName, const QJsonObject& parameters);
    void previewRequested(const QString& patternName);
};
```

## Safety Classes

### AntiDetachmentMonitor

Critical safety system for preventing cup detachment.

```cpp
class AntiDetachmentMonitor : public QObject
{
    Q_OBJECT

public:
    enum DetachmentState {
        ATTACHED,
        WARNING,
        DETACHMENT_RISK,
        DETACHED,
        SYSTEM_ERROR
    };

    explicit AntiDetachmentMonitor(HardwareManager* hardware, QObject *parent = nullptr);
    ~AntiDetachmentMonitor();

    // System control
    bool initialize();
    void shutdown();
    bool isActive() const;

    // Monitoring control
    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();

    // Configuration
    void setThreshold(double thresholdMmHg);
    void setWarningThreshold(double thresholdMmHg);
    void setHysteresis(double hysteresisMmHg);
    void setResponseDelay(int delayMs);

    // Status
    DetachmentState getCurrentState() const;
    double getCurrentAVLPressure() const;
    bool isSOL1Active() const;

signals:
    void detachmentDetected(double avlPressure);
    void detachmentWarning(double avlPressure);
    void detachmentResolved();
    void stateChanged(DetachmentState newState);
    void systemError(const QString& error);
};
```

### EmergencyStop

Emergency stop system with hardware and software triggers.

```cpp
class EmergencyStop : public QObject
{
    Q_OBJECT

public:
    explicit EmergencyStop(HardwareManager* hardware, QObject *parent = nullptr);
    ~EmergencyStop();

    // System control
    bool initialize();
    void shutdown();
    bool isActive() const;

    // Emergency stop control
    void trigger(const QString& reason);
    bool reset();
    bool isTriggered() const;

    // Hardware button
    void setHardwareButtonEnabled(bool enabled);
    bool isHardwareButtonEnabled() const;

    // Status
    QString getLastTriggerReason() const;
    qint64 getLastTriggerTime() const;
    int getTriggerCount() const;

signals:
    void emergencyStopTriggered(const QString& reason);
    void emergencyStopReset();
    void hardwareButtonPressed();
};
```

## Utility Classes

### DataLogger

Comprehensive data logging system.

```cpp
class DataLogger : public QObject
{
    Q_OBJECT

public:
    enum LogType {
        PRESSURE_DATA,
        PATTERN_EXECUTION,
        SAFETY_EVENTS,
        USER_ACTIONS,
        SYSTEM_PERFORMANCE,
        ERROR_EVENTS
    };

    explicit DataLogger(VacuumController* controller, QObject *parent = nullptr);
    ~DataLogger();

    // Logging control
    void startLogging();
    void stopLogging();
    bool isLogging() const;

    // Configuration
    void setLogType(LogType type, bool enabled);
    void setLogDirectory(const QString& directory);
    void setMaxFileSize(int sizeMB);

    // Manual logging
    void logPressureData(double avlPressure, double tankPressure);
    void logPatternEvent(const QString& patternName, const QString& event);
    void logSafetyEvent(const QString& event, const QString& details);
    void logUserAction(const QString& action, const QString& details);

    // Data export
    bool exportLogs(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime);

signals:
    void loggingStarted();
    void loggingStopped();
    void logFileRotated(const QString& newFileName);
    void logError(const QString& error);
};
```

### ErrorManager

Centralized error handling and recovery system.

```cpp
class ErrorManager : public QObject
{
    Q_OBJECT

public:
    enum ErrorSeverity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL,
        FATAL
    };

    enum ErrorCategory {
        HARDWARE,
        SENSOR,
        SAFETY,
        PATTERN,
        GUI,
        SYSTEM
    };

    explicit ErrorManager(QObject *parent = nullptr);
    ~ErrorManager();

    // Error reporting
    void reportError(ErrorSeverity severity, ErrorCategory category,
                    const QString& component, const QString& message,
                    const QString& details = QString());

    // Convenience methods
    void reportInfo(const QString& component, const QString& message);
    void reportWarning(const QString& component, const QString& message);
    void reportError(const QString& component, const QString& message);
    void reportCritical(const QString& component, const QString& message);
    void reportFatal(const QString& component, const QString& message);

    // Error management
    void resolveError(int errorId);
    void clearResolvedErrors();
    void clearAllErrors();

    // System health
    bool isSystemHealthy() const;
    QString getSystemHealthReport() const;

signals:
    void errorReported(const ErrorRecord& error);
    void criticalErrorOccurred(const ErrorRecord& error);
    void systemHealthChanged(bool healthy);
};
```

## Integration Examples

### Basic System Initialization

```cpp
#include "VacuumController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create and initialize vacuum controller
    VacuumController controller;
    if (!controller.initialize()) {
        qCritical() << "Failed to initialize vacuum controller";
        return -1;
    }
    
    // Create main window
    MainWindow window;
    window.showFullScreen();
    
    return app.exec();
}
```

### Pattern Execution

```cpp
// Start a pattern
VacuumController controller;
if (controller.isReady()) {
    controller.startPattern("Medium Pulse");
}

// Monitor pattern progress
connect(&controller, &VacuumController::patternStarted, [](const QString& name) {
    qDebug() << "Pattern started:" << name;
});

connect(&controller, &VacuumController::patternStopped, []() {
    qDebug() << "Pattern stopped";
});
```

### Safety Monitoring

```cpp
// Monitor safety events
SafetyManager safetyManager(&hardwareManager);
connect(&safetyManager, &SafetyManager::safetyViolation, 
        [](const QString& type, double value) {
    qWarning() << "Safety violation:" << type << "Value:" << value;
});

// Configure safety thresholds
safetyManager.setMaxPressure(100.0);  // 100 mmHg
safetyManager.setAntiDetachmentThreshold(50.0);  // 50 mmHg
```

### Data Logging

```cpp
// Start comprehensive logging
DataLogger logger(&controller);
logger.setLogType(DataLogger::PRESSURE_DATA, true);
logger.setLogType(DataLogger::SAFETY_EVENTS, true);
logger.startLogging();

// Export data
QDateTime startTime = QDateTime::currentDateTime().addDays(-1);
QDateTime endTime = QDateTime::currentDateTime();
logger.exportLogs("pressure_data.csv", startTime, endTime);
```

## Error Handling

### Exception Safety

All API methods are designed to be exception-safe and provide clear error reporting through return values and signals.

### Error Codes

Common error conditions and their handling:

- **Hardware Initialization Failure**: Check connections and permissions
- **Sensor Communication Error**: Verify SPI configuration and sensor connections
- **Safety System Failure**: Immediate emergency stop and system shutdown
- **Pattern Validation Error**: Invalid pattern parameters or safety violations
- **Memory Allocation Error**: System resource exhaustion

### Best Practices

1. **Always check return values** from initialization methods
2. **Connect to error signals** for real-time error handling
3. **Implement proper cleanup** in destructors and error paths
4. **Use RAII principles** for resource management
5. **Log all significant events** for debugging and compliance
