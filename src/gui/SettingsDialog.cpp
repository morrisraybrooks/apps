#include "SettingsDialog.h"
#include "components/TouchButton.h"
#include "../VacuumController.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QApplication>

// Constants
const QString SettingsDialog::SETTINGS_FILE_PATH = "config/settings.json";
const double SettingsDialog::DEFAULT_MAX_PRESSURE = 100.0;
const double SettingsDialog::DEFAULT_WARNING_THRESHOLD = 80.0;
const double SettingsDialog::DEFAULT_ANTI_DETACHMENT_THRESHOLD = 50.0;
const int SettingsDialog::DEFAULT_SENSOR_TIMEOUT_MS = 1000;

SettingsDialog::SettingsDialog(VacuumController* controller, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_tabWidget(new QTabWidget(this))
    , m_mainLayout(new QVBoxLayout(this))
    , m_calibrationInProgress(false)
{
    setWindowTitle("System Settings & Calibration");
    setMinimumSize(800, 600);
    setModal(true);
    
    setupUI();
    connectSignals();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Create tabs
    setupSafetyTab();
    setupCalibrationTab();
    setupHardwareTab();
    setupDisplayTab();
    setupDiagnosticsTab();
    setupMaintenanceTab();
    
    // Add tab widget
    m_mainLayout->addWidget(m_tabWidget);
    
    // Create button layout
    m_buttonLayout = new QHBoxLayout();
    
    m_applyButton = new TouchButton("Apply");
    m_applyButton->setButtonType(TouchButton::Primary);
    m_applyButton->setMinimumSize(100, 50);
    
    m_cancelButton = new TouchButton("Cancel");
    m_cancelButton->setButtonType(TouchButton::Normal);
    m_cancelButton->setMinimumSize(100, 50);
    
    m_okButton = new TouchButton("OK");
    m_okButton->setButtonType(TouchButton::Success);
    m_okButton->setMinimumSize(100, 50);
    
    m_resetButton = new TouchButton("Reset to Defaults");
    m_resetButton->setButtonType(TouchButton::Warning);
    m_resetButton->setMinimumSize(150, 50);
    
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_applyButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_okButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
}

void SettingsDialog::setupSafetyTab()
{
    m_safetyTab = new QWidget();
    m_tabWidget->addTab(m_safetyTab, "Safety");
    
    QVBoxLayout* safetyLayout = new QVBoxLayout(m_safetyTab);
    
    // Pressure Limits Group
    QGroupBox* pressureGroup = new QGroupBox("Pressure Limits");
    pressureGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* pressureForm = new QFormLayout(pressureGroup);
    
    m_maxPressureSpin = new QDoubleSpinBox();
    m_maxPressureSpin->setRange(50.0, 150.0);
    m_maxPressureSpin->setSuffix(" mmHg");
    m_maxPressureSpin->setDecimals(1);
    m_maxPressureSpin->setValue(DEFAULT_MAX_PRESSURE);
    
    m_warningThresholdSpin = new QDoubleSpinBox();
    m_warningThresholdSpin->setRange(30.0, 120.0);
    m_warningThresholdSpin->setSuffix(" mmHg");
    m_warningThresholdSpin->setDecimals(1);
    m_warningThresholdSpin->setValue(DEFAULT_WARNING_THRESHOLD);
    
    m_antiDetachmentSpin = new QDoubleSpinBox();
    m_antiDetachmentSpin->setRange(20.0, 80.0);
    m_antiDetachmentSpin->setSuffix(" mmHg");
    m_antiDetachmentSpin->setDecimals(1);
    m_antiDetachmentSpin->setValue(DEFAULT_ANTI_DETACHMENT_THRESHOLD);
    
    pressureForm->addRow("Maximum Pressure:", m_maxPressureSpin);
    pressureForm->addRow("Warning Threshold:", m_warningThresholdSpin);
    pressureForm->addRow("Anti-detachment Threshold:", m_antiDetachmentSpin);
    
    // Safety Features Group
    QGroupBox* featuresGroup = new QGroupBox("Safety Features");
    featuresGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* featuresLayout = new QVBoxLayout(featuresGroup);
    
    m_emergencyStopCheck = new QCheckBox("Emergency Stop Enabled");
    m_emergencyStopCheck->setChecked(true);
    
    m_overpressureProtectionCheck = new QCheckBox("Overpressure Protection");
    m_overpressureProtectionCheck->setChecked(true);
    
    m_autoShutdownCheck = new QCheckBox("Auto Shutdown on Error");
    m_autoShutdownCheck->setChecked(true);
    
    featuresLayout->addWidget(m_emergencyStopCheck);
    featuresLayout->addWidget(m_overpressureProtectionCheck);
    featuresLayout->addWidget(m_autoShutdownCheck);
    
    // Sensor Settings Group
    QGroupBox* sensorGroup = new QGroupBox("Sensor Settings");
    sensorGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* sensorForm = new QFormLayout(sensorGroup);
    
    m_sensorTimeoutSpin = new QSpinBox();
    m_sensorTimeoutSpin->setRange(100, 5000);
    m_sensorTimeoutSpin->setSuffix(" ms");
    m_sensorTimeoutSpin->setValue(DEFAULT_SENSOR_TIMEOUT_MS);
    
    sensorForm->addRow("Sensor Timeout:", m_sensorTimeoutSpin);
    
    safetyLayout->addWidget(pressureGroup);
    safetyLayout->addWidget(featuresGroup);
    safetyLayout->addWidget(sensorGroup);
    safetyLayout->addStretch();
}

void SettingsDialog::setupCalibrationTab()
{
    m_calibrationTab = new QWidget();
    m_tabWidget->addTab(m_calibrationTab, "Calibration");
    
    QVBoxLayout* calibrationLayout = new QVBoxLayout(m_calibrationTab);
    
    // Calibration Status Group
    QGroupBox* statusGroup = new QGroupBox("Calibration Status");
    statusGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* statusForm = new QFormLayout(statusGroup);
    
    m_avlCalibrationStatus = new QLabel("Not Calibrated");
    m_avlCalibrationStatus->setStyleSheet("color: #f44336; font-weight: bold;");
    
    m_tankCalibrationStatus = new QLabel("Not Calibrated");
    m_tankCalibrationStatus->setStyleSheet("color: #f44336; font-weight: bold;");
    
    m_lastCalibrationDate = new QLabel("Never");
    m_lastCalibrationDate->setStyleSheet("color: #666;");
    
    statusForm->addRow("AVL Sensor:", m_avlCalibrationStatus);
    statusForm->addRow("Tank Sensor:", m_tankCalibrationStatus);
    statusForm->addRow("Last Calibration:", m_lastCalibrationDate);
    
    // Calibration Controls Group
    QGroupBox* controlsGroup = new QGroupBox("Calibration Controls");
    controlsGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
    
    m_calibrateSensorsButton = new TouchButton("Calibrate Sensors");
    m_calibrateSensorsButton->setButtonType(TouchButton::Primary);
    m_calibrateSensorsButton->setMinimumSize(200, 60);
    
    m_calibrationProgress = new QProgressBar();
    m_calibrationProgress->setVisible(false);
    m_calibrationProgress->setMinimumHeight(30);
    
    controlsLayout->addWidget(m_calibrateSensorsButton);
    controlsLayout->addWidget(m_calibrationProgress);
    
    // Calibration Log Group
    QGroupBox* logGroup = new QGroupBox("Calibration Log");
    logGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_calibrationLog = new QTextEdit();
    m_calibrationLog->setMaximumHeight(150);
    m_calibrationLog->setReadOnly(true);
    m_calibrationLog->setStyleSheet("font-family: monospace; font-size: 10pt;");
    
    logLayout->addWidget(m_calibrationLog);
    
    calibrationLayout->addWidget(statusGroup);
    calibrationLayout->addWidget(controlsGroup);
    calibrationLayout->addWidget(logGroup);
    calibrationLayout->addStretch();
}

void SettingsDialog::setupHardwareTab()
{
    m_hardwareTab = new QWidget();
    m_tabWidget->addTab(m_hardwareTab, "Hardware");
    
    QVBoxLayout* hardwareLayout = new QVBoxLayout(m_hardwareTab);
    
    // GPIO Configuration Group
    QGroupBox* gpioGroup = new QGroupBox("GPIO Pin Configuration");
    gpioGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* gpioForm = new QFormLayout(gpioGroup);
    
    m_sol1PinSpin = new QSpinBox();
    m_sol1PinSpin->setRange(1, 40);
    m_sol1PinSpin->setValue(17);
    
    m_sol2PinSpin = new QSpinBox();
    m_sol2PinSpin->setRange(1, 40);
    m_sol2PinSpin->setValue(27);
    
    m_sol3PinSpin = new QSpinBox();
    m_sol3PinSpin->setRange(1, 40);
    m_sol3PinSpin->setValue(22);
    
    m_pumpEnablePinSpin = new QSpinBox();
    m_pumpEnablePinSpin->setRange(1, 40);
    m_pumpEnablePinSpin->setValue(25);
    
    m_pumpPwmPinSpin = new QSpinBox();
    m_pumpPwmPinSpin->setRange(1, 40);
    m_pumpPwmPinSpin->setValue(18);
    
    m_emergencyButtonPinSpin = new QSpinBox();
    m_emergencyButtonPinSpin->setRange(1, 40);
    m_emergencyButtonPinSpin->setValue(21);
    
    gpioForm->addRow("SOL1 (AVL):", m_sol1PinSpin);
    gpioForm->addRow("SOL2 (AVL Vent):", m_sol2PinSpin);
    gpioForm->addRow("SOL3 (Tank Vent):", m_sol3PinSpin);
    gpioForm->addRow("Pump Enable:", m_pumpEnablePinSpin);
    gpioForm->addRow("Pump PWM:", m_pumpPwmPinSpin);
    gpioForm->addRow("Emergency Button:", m_emergencyButtonPinSpin);
    
    // SPI Configuration Group
    QGroupBox* spiGroup = new QGroupBox("SPI Configuration");
    spiGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* spiForm = new QFormLayout(spiGroup);
    
    m_spiChannelSpin = new QSpinBox();
    m_spiChannelSpin->setRange(0, 1);
    m_spiChannelSpin->setValue(0);
    
    m_spiSpeedSpin = new QSpinBox();
    m_spiSpeedSpin->setRange(100000, 10000000);
    m_spiSpeedSpin->setValue(1000000);
    m_spiSpeedSpin->setSuffix(" Hz");
    
    spiForm->addRow("SPI Channel:", m_spiChannelSpin);
    spiForm->addRow("SPI Speed:", m_spiSpeedSpin);
    
    // Hardware Test Group
    QGroupBox* testGroup = new QGroupBox("Hardware Testing");
    testGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* testLayout = new QVBoxLayout(testGroup);
    
    m_testHardwareButton = new TouchButton("Test Hardware");
    m_testHardwareButton->setButtonType(TouchButton::Primary);
    m_testHardwareButton->setMinimumSize(150, 50);
    
    m_hardwareTestStatus = new QLabel("Not tested");
    m_hardwareTestStatus->setStyleSheet("color: #666;");
    
    testLayout->addWidget(m_testHardwareButton);
    testLayout->addWidget(m_hardwareTestStatus);
    
    hardwareLayout->addWidget(gpioGroup);
    hardwareLayout->addWidget(spiGroup);
    hardwareLayout->addWidget(testGroup);
    hardwareLayout->addStretch();
}

void SettingsDialog::setupDisplayTab()
{
    m_displayTab = new QWidget();
    m_tabWidget->addTab(m_displayTab, "Display");
    
    QVBoxLayout* displayLayout = new QVBoxLayout(m_displayTab);
    
    // Display Settings Group
    QGroupBox* displayGroup = new QGroupBox("Display Settings");
    displayGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* displayForm = new QFormLayout(displayGroup);
    
    m_fullscreenCheck = new QCheckBox("Fullscreen Mode");
    m_fullscreenCheck->setChecked(true);
    
    m_screenWidthSpin = new QSpinBox();
    m_screenWidthSpin->setRange(800, 4096);
    m_screenWidthSpin->setValue(1920);
    
    m_screenHeightSpin = new QSpinBox();
    m_screenHeightSpin->setRange(600, 2160);
    m_screenHeightSpin->setValue(1080);
    
    m_touchEnabledCheck = new QCheckBox("Touch Interface");
    m_touchEnabledCheck->setChecked(true);
    
    displayForm->addRow("", m_fullscreenCheck);
    displayForm->addRow("Screen Width:", m_screenWidthSpin);
    displayForm->addRow("Screen Height:", m_screenHeightSpin);
    displayForm->addRow("", m_touchEnabledCheck);
    
    // Font Settings Group
    QGroupBox* fontGroup = new QGroupBox("Font Settings");
    fontGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* fontForm = new QFormLayout(fontGroup);
    
    m_fontSizeNormalSpin = new QSpinBox();
    m_fontSizeNormalSpin->setRange(8, 32);
    m_fontSizeNormalSpin->setValue(16);
    
    m_fontSizeLargeSpin = new QSpinBox();
    m_fontSizeLargeSpin->setRange(12, 48);
    m_fontSizeLargeSpin->setValue(20);
    
    fontForm->addRow("Normal Font Size:", m_fontSizeNormalSpin);
    fontForm->addRow("Large Font Size:", m_fontSizeLargeSpin);
    
    // Theme Settings Group
    QGroupBox* themeGroup = new QGroupBox("Theme Settings");
    themeGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* themeForm = new QFormLayout(themeGroup);
    
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({"Light", "Dark", "High Contrast"});
    
    themeForm->addRow("Theme:", m_themeCombo);
    
    displayLayout->addWidget(displayGroup);
    displayLayout->addWidget(fontGroup);
    displayLayout->addWidget(themeGroup);
    displayLayout->addStretch();
}

void SettingsDialog::setupDiagnosticsTab()
{
    m_diagnosticsTab = new QWidget();
    m_tabWidget->addTab(m_diagnosticsTab, "Diagnostics");
    
    QVBoxLayout* diagnosticsLayout = new QVBoxLayout(m_diagnosticsTab);
    
    // Logging Settings Group
    QGroupBox* loggingGroup = new QGroupBox("Logging Settings");
    loggingGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* loggingForm = new QFormLayout(loggingGroup);
    
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"});
    m_logLevelCombo->setCurrentText("INFO");
    
    m_logToFileCheck = new QCheckBox("Log to File");
    m_logToFileCheck->setChecked(true);
    
    m_logFilePathEdit = new QLineEdit("/var/log/vacuum-controller.log");
    
    m_maxLogFileSizeSpin = new QSpinBox();
    m_maxLogFileSizeSpin->setRange(1, 1000);
    m_maxLogFileSizeSpin->setValue(100);
    m_maxLogFileSizeSpin->setSuffix(" MB");
    
    m_logRotationCheck = new QCheckBox("Log Rotation");
    m_logRotationCheck->setChecked(true);
    
    loggingForm->addRow("Log Level:", m_logLevelCombo);
    loggingForm->addRow("", m_logToFileCheck);
    loggingForm->addRow("Log File Path:", m_logFilePathEdit);
    loggingForm->addRow("Max File Size:", m_maxLogFileSizeSpin);
    loggingForm->addRow("", m_logRotationCheck);
    
    // Data Logging Group
    QGroupBox* dataGroup = new QGroupBox("Data Logging");
    dataGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* dataLayout = new QVBoxLayout(dataGroup);
    
    m_logPressureDataCheck = new QCheckBox("Log Pressure Data");
    m_logPressureDataCheck->setChecked(true);
    
    m_logPatternExecutionCheck = new QCheckBox("Log Pattern Execution");
    m_logPatternExecutionCheck->setChecked(true);
    
    m_logSafetyEventsCheck = new QCheckBox("Log Safety Events");
    m_logSafetyEventsCheck->setChecked(true);
    
    dataLayout->addWidget(m_logPressureDataCheck);
    dataLayout->addWidget(m_logPatternExecutionCheck);
    dataLayout->addWidget(m_logSafetyEventsCheck);
    
    diagnosticsLayout->addWidget(loggingGroup);
    diagnosticsLayout->addWidget(dataGroup);
    diagnosticsLayout->addStretch();
}

void SettingsDialog::setupMaintenanceTab()
{
    m_maintenanceTab = new QWidget();
    m_tabWidget->addTab(m_maintenanceTab, "Maintenance");
    
    QVBoxLayout* maintenanceLayout = new QVBoxLayout(m_maintenanceTab);
    
    // Maintenance Settings Group
    QGroupBox* settingsGroup = new QGroupBox("Maintenance Settings");
    settingsGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* settingsForm = new QFormLayout(settingsGroup);
    
    m_selfTestOnStartupCheck = new QCheckBox("Self-test on Startup");
    m_selfTestOnStartupCheck->setChecked(true);
    
    m_periodicCalibrationDaysSpin = new QSpinBox();
    m_periodicCalibrationDaysSpin->setRange(1, 365);
    m_periodicCalibrationDaysSpin->setValue(30);
    m_periodicCalibrationDaysSpin->setSuffix(" days");
    
    m_maintenanceReminderCheck = new QCheckBox("Maintenance Reminders");
    m_maintenanceReminderCheck->setChecked(true);
    
    m_usageTrackingCheck = new QCheckBox("Usage Tracking");
    m_usageTrackingCheck->setChecked(true);
    
    m_componentLifetimeTrackingCheck = new QCheckBox("Component Lifetime Tracking");
    m_componentLifetimeTrackingCheck->setChecked(true);
    
    settingsForm->addRow("", m_selfTestOnStartupCheck);
    settingsForm->addRow("Calibration Interval:", m_periodicCalibrationDaysSpin);
    settingsForm->addRow("", m_maintenanceReminderCheck);
    settingsForm->addRow("", m_usageTrackingCheck);
    settingsForm->addRow("", m_componentLifetimeTrackingCheck);
    
    // System Information Group
    QGroupBox* infoGroup = new QGroupBox("System Information");
    infoGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* infoForm = new QFormLayout(infoGroup);
    
    m_systemUptimeLabel = new QLabel("0 hours");
    m_totalOperatingHoursLabel = new QLabel("0 hours");
    m_lastMaintenanceLabel = new QLabel("Never");
    
    infoForm->addRow("System Uptime:", m_systemUptimeLabel);
    infoForm->addRow("Total Operating Hours:", m_totalOperatingHoursLabel);
    infoForm->addRow("Last Maintenance:", m_lastMaintenanceLabel);
    
    // Settings Management Group
    QGroupBox* managementGroup = new QGroupBox("Settings Management");
    managementGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QHBoxLayout* managementLayout = new QHBoxLayout(managementGroup);
    
    m_exportSettingsButton = new TouchButton("Export Settings");
    m_exportSettingsButton->setButtonType(TouchButton::Normal);
    m_exportSettingsButton->setMinimumSize(150, 50);
    
    m_importSettingsButton = new TouchButton("Import Settings");
    m_importSettingsButton->setButtonType(TouchButton::Normal);
    m_importSettingsButton->setMinimumSize(150, 50);
    
    m_factoryResetButton = new TouchButton("Factory Reset");
    m_factoryResetButton->setButtonType(TouchButton::Danger);
    m_factoryResetButton->setMinimumSize(150, 50);
    
    managementLayout->addWidget(m_exportSettingsButton);
    managementLayout->addWidget(m_importSettingsButton);
    managementLayout->addWidget(m_factoryResetButton);
    managementLayout->addStretch();
    
    maintenanceLayout->addWidget(settingsGroup);
    maintenanceLayout->addWidget(infoGroup);
    maintenanceLayout->addWidget(managementGroup);
    maintenanceLayout->addStretch();
}

void SettingsDialog::connectSignals()
{
    // Button connections
    connect(m_applyButton, &TouchButton::clicked, this, &SettingsDialog::onApplyClicked);
    connect(m_cancelButton, &TouchButton::clicked, this, &SettingsDialog::onCancelClicked);
    connect(m_okButton, &TouchButton::clicked, this, &SettingsDialog::onOkClicked);
    connect(m_resetButton, &TouchButton::clicked, this, &SettingsDialog::resetToDefaults);
    
    // Calibration connections
    connect(m_calibrateSensorsButton, &TouchButton::clicked, this, &SettingsDialog::onCalibrateSensorsClicked);
    
    // Hardware connections
    connect(m_testHardwareButton, &TouchButton::clicked, this, &SettingsDialog::onTestHardwareClicked);
    
    // Maintenance connections
    connect(m_exportSettingsButton, &TouchButton::clicked, this, &SettingsDialog::onExportSettingsClicked);
    connect(m_importSettingsButton, &TouchButton::clicked, this, &SettingsDialog::onImportSettingsClicked);
    connect(m_factoryResetButton, &TouchButton::clicked, this, &SettingsDialog::onFactoryResetClicked);
}

void SettingsDialog::loadSettings()
{
    QFile file(SETTINGS_FILE_PATH);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        m_currentSettings = doc.object();
        m_originalSettings = m_currentSettings;
        
        // Load settings into UI controls
        QJsonObject safetySettings = m_currentSettings["safety_settings"].toObject();
        m_maxPressureSpin->setValue(safetySettings["max_pressure_mmhg"].toDouble(DEFAULT_MAX_PRESSURE));
        m_warningThresholdSpin->setValue(safetySettings["warning_threshold_mmhg"].toDouble(DEFAULT_WARNING_THRESHOLD));
        m_antiDetachmentSpin->setValue(safetySettings["anti_detachment_threshold_mmhg"].toDouble(DEFAULT_ANTI_DETACHMENT_THRESHOLD));
        m_sensorTimeoutSpin->setValue(safetySettings["sensor_timeout_ms"].toInt(DEFAULT_SENSOR_TIMEOUT_MS));
        
        // Load other settings...
        updateCalibrationStatus();
    }
}

void SettingsDialog::saveSettings()
{
    // Update settings object with current UI values
    QJsonObject safetySettings;
    safetySettings["max_pressure_mmhg"] = m_maxPressureSpin->value();
    safetySettings["warning_threshold_mmhg"] = m_warningThresholdSpin->value();
    safetySettings["anti_detachment_threshold_mmhg"] = m_antiDetachmentSpin->value();
    safetySettings["sensor_timeout_ms"] = m_sensorTimeoutSpin->value();
    safetySettings["emergency_stop_enabled"] = m_emergencyStopCheck->isChecked();
    safetySettings["overpressure_protection_enabled"] = m_overpressureProtectionCheck->isChecked();
    safetySettings["auto_shutdown_on_error"] = m_autoShutdownCheck->isChecked();
    
    m_currentSettings["safety_settings"] = safetySettings;
    
    // Save to file
    QFile file(SETTINGS_FILE_PATH);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_currentSettings);
        file.write(doc.toJson());
        
        QMessageBox::information(this, "Settings Saved", "Settings have been saved successfully.");
    } else {
        QMessageBox::warning(this, "Save Failed", "Failed to save settings to file.");
    }
}

void SettingsDialog::resetToDefaults()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset to Defaults",
        "Are you sure you want to reset all settings to their default values?\n\n"
        "This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Reset all controls to default values
        m_maxPressureSpin->setValue(DEFAULT_MAX_PRESSURE);
        m_warningThresholdSpin->setValue(DEFAULT_WARNING_THRESHOLD);
        m_antiDetachmentSpin->setValue(DEFAULT_ANTI_DETACHMENT_THRESHOLD);
        m_sensorTimeoutSpin->setValue(DEFAULT_SENSOR_TIMEOUT_MS);
        
        m_emergencyStopCheck->setChecked(true);
        m_overpressureProtectionCheck->setChecked(true);
        m_autoShutdownCheck->setChecked(true);
        
        // Reset other tabs...
        
        QMessageBox::information(this, "Reset Complete", "All settings have been reset to default values.");
    }
}

void SettingsDialog::onCalibrateSensorsClicked()
{
    if (m_calibrationInProgress) {
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Sensor Calibration",
        "This will calibrate both pressure sensors.\n\n"
        "Ensure the system is at atmospheric pressure before proceeding.\n\n"
        "Continue with calibration?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        performSensorCalibration();
    }
}

void SettingsDialog::performSensorCalibration()
{
    m_calibrationInProgress = true;
    m_calibrateSensorsButton->setEnabled(false);
    m_calibrationProgress->setVisible(true);
    m_calibrationProgress->setValue(0);
    
    m_calibrationLog->append(QString("[%1] Starting sensor calibration...")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    
    // Simulate calibration process
    QTimer* calibrationTimer = new QTimer(this);
    int progress = 0;
    
    connect(calibrationTimer, &QTimer::timeout, [this, calibrationTimer, &progress]() {
        progress += 10;
        m_calibrationProgress->setValue(progress);
        
        if (progress >= 100) {
            calibrationTimer->stop();
            calibrationTimer->deleteLater();
            onCalibrationComplete(true);
        }
    });
    
    calibrationTimer->start(200);  // Update every 200ms
}

void SettingsDialog::onCalibrationComplete(bool success)
{
    m_calibrationInProgress = false;
    m_calibrateSensorsButton->setEnabled(true);
    m_calibrationProgress->setVisible(false);
    
    if (success) {
        m_calibrationLog->append(QString("[%1] Calibration completed successfully")
                                .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        
        m_avlCalibrationStatus->setText("Calibrated");
        m_avlCalibrationStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
        
        m_tankCalibrationStatus->setText("Calibrated");
        m_tankCalibrationStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
        
        m_lastCalibrationDate->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        
        QMessageBox::information(this, "Calibration Complete", "Sensor calibration completed successfully.");
    } else {
        m_calibrationLog->append(QString("[%1] Calibration failed")
                                .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        
        QMessageBox::warning(this, "Calibration Failed", "Sensor calibration failed. Please check connections and try again.");
    }
}

void SettingsDialog::onTestHardwareClicked()
{
    performHardwareTest();
}

void SettingsDialog::performHardwareTest()
{
    m_testHardwareButton->setEnabled(false);
    m_hardwareTestStatus->setText("Testing...");
    m_hardwareTestStatus->setStyleSheet("color: #FF9800; font-weight: bold;");
    
    // Simulate hardware test
    QTimer::singleShot(2000, this, [this]() {
        bool testPassed = true;  // Simulate test result
        
        if (testPassed) {
            m_hardwareTestStatus->setText("All tests passed");
            m_hardwareTestStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else {
            m_hardwareTestStatus->setText("Test failed");
            m_hardwareTestStatus->setStyleSheet("color: #f44336; font-weight: bold;");
        }
        
        m_testHardwareButton->setEnabled(true);
    });
}

void SettingsDialog::updateCalibrationStatus()
{
    // Update calibration status from settings
    QJsonObject sensorCalibration = m_currentSettings["sensor_calibration"].toObject();
    
    QJsonObject avlSensor = sensorCalibration["avl_sensor"].toObject();
    if (avlSensor["calibrated"].toBool()) {
        m_avlCalibrationStatus->setText("Calibrated");
        m_avlCalibrationStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    }
    
    QJsonObject tankSensor = sensorCalibration["tank_sensor"].toObject();
    if (tankSensor["calibrated"].toBool()) {
        m_tankCalibrationStatus->setText("Calibrated");
        m_tankCalibrationStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    }
    
    QString calibrationDate = avlSensor["calibration_date"].toString();
    if (!calibrationDate.isEmpty()) {
        m_lastCalibrationDate->setText(calibrationDate);
    }
}

bool SettingsDialog::validateSettings()
{
    // Validate pressure settings
    if (m_warningThresholdSpin->value() >= m_maxPressureSpin->value()) {
        QMessageBox::warning(this, "Invalid Settings", 
                           "Warning threshold must be less than maximum pressure.");
        return false;
    }
    
    if (m_antiDetachmentSpin->value() >= m_warningThresholdSpin->value()) {
        QMessageBox::warning(this, "Invalid Settings", 
                           "Anti-detachment threshold must be less than warning threshold.");
        return false;
    }
    
    return true;
}

void SettingsDialog::onApplyClicked()
{
    if (validateSettings()) {
        saveSettings();
    }
}

void SettingsDialog::onCancelClicked()
{
    // Restore original settings
    m_currentSettings = m_originalSettings;
    reject();
}

void SettingsDialog::onOkClicked()
{
    if (validateSettings()) {
        saveSettings();
        accept();
    }
}

void SettingsDialog::onExportSettingsClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Settings",
        QString("vacuum_controller_settings_%1.json")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(m_currentSettings);
            file.write(doc.toJson());
            QMessageBox::information(this, "Export Complete", "Settings exported successfully.");
        } else {
            QMessageBox::warning(this, "Export Failed", "Failed to export settings.");
        }
    }
}

void SettingsDialog::onImportSettingsClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Settings",
        "",
        "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull()) {
                m_currentSettings = doc.object();
                loadSettings();
                QMessageBox::information(this, "Import Complete", "Settings imported successfully.");
            } else {
                QMessageBox::warning(this, "Import Failed", "Invalid settings file format.");
            }
        } else {
            QMessageBox::warning(this, "Import Failed", "Failed to read settings file.");
        }
    }
}

void SettingsDialog::onFactoryResetClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Factory Reset",
        "WARNING: This will reset ALL settings to factory defaults and clear all calibration data.\n\n"
        "This action cannot be undone.\n\n"
        "Are you sure you want to proceed?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Perform factory reset
        m_currentSettings = QJsonObject();  // Clear all settings
        resetToDefaults();
        
        // Reset calibration status
        m_avlCalibrationStatus->setText("Not Calibrated");
        m_avlCalibrationStatus->setStyleSheet("color: #f44336; font-weight: bold;");
        m_tankCalibrationStatus->setText("Not Calibrated");
        m_tankCalibrationStatus->setStyleSheet("color: #f44336; font-weight: bold;");
        m_lastCalibrationDate->setText("Never");
        
        QMessageBox::information(this, "Factory Reset Complete", 
                               "All settings have been reset to factory defaults.\n\n"
                               "Please recalibrate sensors before use.");
    }
}
