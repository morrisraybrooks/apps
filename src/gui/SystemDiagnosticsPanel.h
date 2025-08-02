#ifndef SYSTEMDIAGNOSTICSPANEL_H
#define SYSTEMDIAGNOSTICSPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

// Forward declarations
class VacuumController;
class HardwareManager;
class SafetyManager;
class PerformanceMonitor;
class TouchButton;
class StatusIndicator;
class MultiStatusIndicator;

/**
 * @brief Comprehensive system diagnostics and monitoring interface
 * 
 * This panel provides detailed system diagnostics including:
 * - Real-time hardware status monitoring
 * - Sensor performance and calibration status
 * - Actuator states and performance metrics
 * - System health indicators and alerts
 * - Performance metrics and resource usage
 * - Error logs and diagnostic information
 * - Hardware testing and validation tools
 */
class SystemDiagnosticsPanel : public QWidget
{
    Q_OBJECT

public:
    struct DiagnosticData {
        QDateTime timestamp;
        QString component;
        QString status;
        QString details;
        QJsonObject metrics;
        
        DiagnosticData() : timestamp(QDateTime::currentDateTime()) {}
    };

    explicit SystemDiagnosticsPanel(VacuumController* controller, QWidget *parent = nullptr);
    ~SystemDiagnosticsPanel();

    // Diagnostic control
    void startDiagnostics();
    void stopDiagnostics();
    void refreshDiagnostics();
    bool isDiagnosticsRunning() const { return m_diagnosticsRunning; }
    
    // Data access
    QList<DiagnosticData> getDiagnosticHistory() const;
    QJsonObject getCurrentSystemStatus() const;
    QJsonObject getPerformanceMetrics() const;

public slots:
    void updateDiagnostics();
    void runSystemTest();
    void runHardwareTest();
    void runSensorTest();
    void runSafetyTest();
    void exportDiagnostics();

signals:
    void diagnosticAlert(const QString& component, const QString& message);
    void systemTestCompleted(bool success);
    void hardwareTestCompleted(const QString& component, bool success);

private slots:
    void onDiagnosticTimer();
    void onTestButtonClicked();
    void onExportButtonClicked();
    void onRefreshButtonClicked();
    void onComponentSelected();

private:
    void setupUI();
    void setupOverviewTab();
    void setupHardwareTab();
    void setupSensorsTab();
    void setupActuatorsTab();
    void setupPerformanceTab();
    void setupLogsTab();
    void setupTestingTab();
    void connectSignals();
    
    void updateOverviewStatus();
    void updateHardwareStatus();
    void updateSensorStatus();
    void updateActuatorStatus();
    void updatePerformanceMetrics();
    void updateLogDisplay();
    
    void addDiagnosticEntry(const DiagnosticData& data);
    void clearDiagnosticHistory();
    
    // Test procedures
    bool testGPIOPins();
    bool testSPICommunication();
    bool testSensorReadings();
    bool testActuatorControl();
    bool testSafetySystem();
    bool testSystemPerformance();
    
    // Status formatting
    QString formatUptime(qint64 uptimeMs);
    QString formatMemoryUsage(qint64 bytes);
    QString formatCPUUsage(double percentage);
    QString formatTemperature(double celsius);
    
    // Controller interfaces
    VacuumController* m_controller;
    HardwareManager* m_hardwareManager;
    SafetyManager* m_safetyManager;
    PerformanceMonitor* m_performanceMonitor;
    
    // Main UI
    QTabWidget* m_tabWidget;
    QVBoxLayout* m_mainLayout;
    
    // Overview Tab
    QWidget* m_overviewTab;
    MultiStatusIndicator* m_systemStatusIndicator;
    QLabel* m_systemUptimeLabel;
    QLabel* m_systemVersionLabel;
    QLabel* m_lastUpdateLabel;
    QProgressBar* m_systemHealthBar;
    
    // Hardware Tab
    QWidget* m_hardwareTab;
    QGroupBox* m_gpioStatusGroup;
    QGroupBox* m_spiStatusGroup;
    QGroupBox* m_powerStatusGroup;
    QTableWidget* m_hardwareTable;
    
    // Sensors Tab
    QWidget* m_sensorsTab;
    QGroupBox* m_sensorReadingsGroup;
    QGroupBox* m_sensorCalibrationGroup;
    QLabel* m_avlSensorStatusLabel;
    QLabel* m_tankSensorStatusLabel;
    QLabel* m_avlReadingLabel;
    QLabel* m_tankReadingLabel;
    QLabel* m_lastCalibrationLabel;
    QProgressBar* m_sensorAccuracyBar;
    
    // Actuators Tab
    QWidget* m_actuatorsTab;
    QGroupBox* m_valveStatusGroup;
    QGroupBox* m_pumpStatusGroup;
    StatusIndicator* m_sol1StatusIndicator;
    StatusIndicator* m_sol2StatusIndicator;
    StatusIndicator* m_sol3StatusIndicator;
    StatusIndicator* m_pumpStatusIndicator;
    QLabel* m_pumpSpeedLabel;
    QLabel* m_pumpCurrentLabel;
    
    // Performance Tab
    QWidget* m_performanceTab;
    QGroupBox* m_cpuMemoryGroup;
    QGroupBox* m_threadingGroup;
    QGroupBox* m_timingGroup;
    QLabel* m_cpuUsageLabel;
    QLabel* m_memoryUsageLabel;
    QLabel* m_threadCountLabel;
    QLabel* m_dataRateLabel;
    QLabel* m_guiFrameRateLabel;
    QLabel* m_safetyCheckRateLabel;
    QProgressBar* m_cpuUsageBar;
    QProgressBar* m_memoryUsageBar;
    
    // Logs Tab
    QWidget* m_logsTab;
    QTextEdit* m_logDisplay;
    TouchButton* m_clearLogsButton;
    TouchButton* m_exportLogsButton;
    TouchButton* m_refreshLogsButton;
    
    // Testing Tab
    QWidget* m_testingTab;
    QGroupBox* m_testControlsGroup;
    QGroupBox* m_testResultsGroup;
    TouchButton* m_runSystemTestButton;
    TouchButton* m_runHardwareTestButton;
    TouchButton* m_runSensorTestButton;
    TouchButton* m_runSafetyTestButton;
    QTextEdit* m_testResultsDisplay;
    QProgressBar* m_testProgressBar;
    
    // Diagnostic state
    bool m_diagnosticsRunning;
    QTimer* m_diagnosticTimer;
    QList<DiagnosticData> m_diagnosticHistory;
    int m_maxHistoryEntries;
    
    // Test state
    bool m_testInProgress;
    QString m_currentTest;
    
    // Constants
    static const int DIAGNOSTIC_UPDATE_INTERVAL = 2000;  // 2 seconds
    static const int MAX_DIAGNOSTIC_HISTORY = 1000;
    static const int MAX_LOG_LINES = 10000;
};

#endif // SYSTEMDIAGNOSTICSPANEL_H
