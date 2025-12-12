#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>
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
class OrgasmControlAlgorithm;

/**
 * @brief Comprehensive settings and calibration panel
 *
 * This panel provides access to all system configuration options:
 * - Safety parameters and limits
 * - Sensor calibration and validation
 * - Hardware configuration
 * - Display and UI preferences
 * - System diagnostics and maintenance
 * - Data logging and export settings
 */
class SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPanel(VacuumController* controller, QWidget *parent = nullptr);
    ~SettingsPanel();

public Q_SLOTS:
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

private Q_SLOTS:
    void onTestHardwareClicked();
    void onExportSettingsClicked();
    void onImportSettingsClicked();
    void onFactoryResetClicked();
    void onApplyClicked();
    

private:
    void setupUI();
    void setupSafetyTab();
    void setupCalibrationTab();
    void setupArousalCalibrationTab();
    void setupMilkingConfigurationTab();
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

    // Anti-detachment Advanced Settings
    QDoubleSpinBox* m_antiDetachmentWarningThresholdSpin;
    QDoubleSpinBox* m_antiDetachmentHysteresisSpin;
    QSpinBox* m_antiDetachmentResponseDelaySpin;
    QDoubleSpinBox* m_antiDetachmentMaxVacuumIncreaseSpin;
    QSpinBox* m_antiDetachmentMonitoringRateSpin;
    QCheckBox* m_antiDetachmentEnabledCheck;
    
    // Calibration Tab
    CalibrationInterface* m_calibrationInterface;

    // Arousal Calibration Tab
    QWidget* m_arousalCalibrationTab;
    QDoubleSpinBox* m_edgeThresholdSpin;
    QDoubleSpinBox* m_orgasmThresholdSpin;
    QDoubleSpinBox* m_recoveryThresholdSpin;
    QDoubleSpinBox* m_milkingZoneLowerSpin;
    QDoubleSpinBox* m_milkingZoneUpperSpin;
    QDoubleSpinBox* m_dangerThresholdSpin;
    QComboBox* m_milkingFailureModeCombo;
    QCheckBox* m_tensEnabledCheck;
    QCheckBox* m_antiEscapeEnabledCheck;
    QLabel* m_currentArousalLabel;
    QProgressBar* m_arousalProgressBar;

    // Milking Configuration Tab
    QWidget* m_milkingConfigTab;
    QSpinBox* m_milkingDurationSpin;
    QSpinBox* m_milkingTargetOrgasmsSpin;
    QDoubleSpinBox* m_milkingIntensityMinSpin;
    QDoubleSpinBox* m_milkingIntensityMaxSpin;
    QDoubleSpinBox* m_milkingPidKpSpin;
    QDoubleSpinBox* m_milkingPidKiSpin;
    QDoubleSpinBox* m_milkingPidKdSpin;
    QCheckBox* m_milkingAutoAdjustCheck;
    QLabel* m_milkingStatusLabel;
    QProgressBar* m_milkingZoneProgressBar;
    
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

    // Anti-detachment constants
    static const double DEFAULT_ANTI_DETACHMENT_WARNING_THRESHOLD;
    static const double DEFAULT_ANTI_DETACHMENT_HYSTERESIS;
    static const int DEFAULT_ANTI_DETACHMENT_RESPONSE_DELAY_MS;
    static const double DEFAULT_ANTI_DETACHMENT_MAX_VACUUM_INCREASE;
    static const int DEFAULT_ANTI_DETACHMENT_MONITORING_RATE_HZ;

    // Arousal threshold constants
    static const double DEFAULT_EDGE_THRESHOLD;
    static const double DEFAULT_ORGASM_THRESHOLD;
    static const double DEFAULT_RECOVERY_THRESHOLD;
    static const double DEFAULT_MILKING_ZONE_LOWER;
    static const double DEFAULT_MILKING_ZONE_UPPER;
    static const double DEFAULT_DANGER_THRESHOLD;
};

#endif // SETTINGSPANEL_H
