#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QJsonObject>

// Forward declarations
class VacuumController;
class TouchButton;
class CalibrationInterface;

/**
 * @brief Comprehensive settings and calibration dialog
 * 
 * This dialog provides access to all system configuration options:
 * - Safety parameters and limits
 * - Sensor calibration and validation
 * - Hardware configuration
 * - Display and UI preferences
 * - System diagnostics and maintenance
 * - Data logging and export settings
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(VacuumController* controller, QWidget *parent = nullptr);
    ~SettingsDialog();

public slots:
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

private slots:
    void onTestHardwareClicked();
    void onExportSettingsClicked();
    void onImportSettingsClicked();
    void onFactoryResetClicked();
    void onApplyClicked();
    void onCancelClicked();
    void onOkClicked();
    

private:
    void setupUI();
    void setupSafetyTab();
    void setupCalibrationTab();
    void setupHardwareTab();
    void setupDisplayTab();
    void setupDiagnosticsTab();
    void setupMaintenanceTab();
    void connectSignals();
    
    bool validateSettings();
    
    // Controller interface
    VacuumController* m_controller;
    
    // Main UI
    QTabWidget* m_tabWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_buttonLayout;
    
    // Buttons
    TouchButton* m_applyButton;
    TouchButton* m_cancelButton;
    TouchButton* m_okButton;
    TouchButton* m_resetButton;
    
    // Safety Settings Tab
    QWidget* m_safetyTab;
    QDoubleSpinBox* m_maxPressureSpin;
    QDoubleSpinBox* m_warningThresholdSpin;
    QDoubleSpinBox* m_antiDetachmentSpin;
    QSpinBox* m_sensorTimeoutSpin;
    QCheckBox* m_emergencyStopCheck;
    QCheckBox* m_overpressureProtectionCheck;
    QCheckBox* m_autoShutdownCheck;
    
    // Calibration Tab
    CalibrationInterface* m_calibrationInterface;
    
    // Hardware Tab
    QWidget* m_hardwareTab;
    QSpinBox* m_sol1PinSpin;
    QSpinBox* m_sol2PinSpin;
    QSpinBox* m_sol3PinSpin;
    QSpinBox* m_pumpEnablePinSpin;
    QSpinBox* m_pumpPwmPinSpin;
    QSpinBox* m_emergencyButtonPinSpin;
    QSpinBox* m_spiChannelSpin;
    QSpinBox* m_spiSpeedSpin;
    TouchButton* m_testHardwareButton;
    QLabel* m_hardwareTestStatus;
    
    // Display Tab
    QWidget* m_displayTab;
    QCheckBox* m_fullscreenCheck;
    QSpinBox* m_screenWidthSpin;
    QSpinBox* m_screenHeightSpin;
    QSpinBox* m_fontSizeNormalSpin;
    QSpinBox* m_fontSizeLargeSpin;
    QCheckBox* m_touchEnabledCheck;
    QComboBox* m_themeCombo;
    QSpinBox* m_chartTimeRangeSpin;
    QCheckBox* m_showGridCheck;
    QCheckBox* m_showAlarmsCheck;
    
    // Diagnostics Tab
    QWidget* m_diagnosticsTab;
    QComboBox* m_logLevelCombo;
    QCheckBox* m_logToFileCheck;
    QLineEdit* m_logFilePathEdit;
    QSpinBox* m_maxLogFileSizeSpin;
    QCheckBox* m_logRotationCheck;
    QCheckBox* m_logPressureDataCheck;
    QCheckBox* m_logPatternExecutionCheck;
    QCheckBox* m_logSafetyEventsCheck;
    TouchButton* m_exportLogsButton;
    TouchButton* m_clearLogsButton;
    
    // Maintenance Tab
    QWidget* m_maintenanceTab;
    QCheckBox* m_selfTestOnStartupCheck;
    QSpinBox* m_periodicCalibrationDaysSpin;
    QCheckBox* m_maintenanceReminderCheck;
    QCheckBox* m_usageTrackingCheck;
    QCheckBox* m_componentLifetimeTrackingCheck;
    QLabel* m_systemUptimeLabel;
    QLabel* m_totalOperatingHoursLabel;
    QLabel* m_lastMaintenanceLabel;
    TouchButton* m_exportSettingsButton;
    TouchButton* m_importSettingsButton;
    TouchButton* m_factoryResetButton;
    
    // Settings storage
    QJsonObject m_currentSettings;
    QJsonObject m_originalSettings;
    
    // Calibration state
    bool m_calibrationInProgress;
    
    // Constants
    static const QString SETTINGS_FILE_PATH;
    static const double DEFAULT_MAX_PRESSURE;
    static const double DEFAULT_WARNING_THRESHOLD;
    static const double DEFAULT_ANTI_DETACHMENT_THRESHOLD;
    static const int DEFAULT_SENSOR_TIMEOUT_MS;
};

#endif // SETTINGSDIALOG_H
