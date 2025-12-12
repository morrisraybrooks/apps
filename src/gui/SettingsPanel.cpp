#include "SettingsPanel.h"
#include "components/TouchButton.h"
#include "CalibrationInterface.h"
#include "../VacuumController.h"
#include "../control/OrgasmControlAlgorithm.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QApplication>

// Constants
const QString SettingsPanel::SETTINGS_FILE_PATH = "config/settings.json";
const double SettingsPanel::DEFAULT_MAX_PRESSURE = 100.0;
const double SettingsPanel::DEFAULT_WARNING_THRESHOLD = 80.0;
const double SettingsPanel::DEFAULT_ANTI_DETACHMENT_THRESHOLD = 50.0;
const int SettingsPanel::DEFAULT_SENSOR_TIMEOUT_MS = 1000;

// Anti-detachment constants (matching AntiDetachmentMonitor defaults)
const double SettingsPanel::DEFAULT_ANTI_DETACHMENT_WARNING_THRESHOLD = 60.0;  // 60 mmHg
const double SettingsPanel::DEFAULT_ANTI_DETACHMENT_HYSTERESIS = 5.0;          // 5 mmHg
const int SettingsPanel::DEFAULT_ANTI_DETACHMENT_RESPONSE_DELAY_MS = 100;      // 100 ms
const double SettingsPanel::DEFAULT_ANTI_DETACHMENT_MAX_VACUUM_INCREASE = 20.0; // 20%
const int SettingsPanel::DEFAULT_ANTI_DETACHMENT_MONITORING_RATE_HZ = 100;     // 100 Hz

// Arousal threshold constants (matching OrgasmControlAlgorithm defaults)
const double SettingsPanel::DEFAULT_EDGE_THRESHOLD = 0.70;
const double SettingsPanel::DEFAULT_ORGASM_THRESHOLD = 0.85;
const double SettingsPanel::DEFAULT_RECOVERY_THRESHOLD = 0.45;
const double SettingsPanel::DEFAULT_MILKING_ZONE_LOWER = 0.75;
const double SettingsPanel::DEFAULT_MILKING_ZONE_UPPER = 0.90;
const double SettingsPanel::DEFAULT_DANGER_THRESHOLD = 0.92;

SettingsPanel::SettingsPanel(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_tabWidget(new QTabWidget(this))
    , m_mainLayout(new QVBoxLayout(this))
    , m_calibrationInProgress(false)
{
    setupUI();
    connectSignals();
    loadSettings();
}

SettingsPanel::~SettingsPanel()
{
}

void SettingsPanel::setupUI()
{
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Create tabs
    setupSafetyTab();
    setupCalibrationTab();
    setupArousalCalibrationTab();
    setupMilkingConfigurationTab();
    setupHardwareTab();
    setupDisplayTab();
    setupDiagnosticsTab();
    setupMaintenanceTab();
    
    // Add tab widget
    m_mainLayout->addWidget(m_tabWidget);
    
    // Create button layout
    m_buttonLayout = new QHBoxLayout();
    
    m_applyButton = new TouchButton("Apply Settings");
    m_applyButton->setButtonType(TouchButton::Primary);
    m_applyButton->setMinimumSize(150, 50);

    m_resetButton = new TouchButton("Reset to Defaults");
    m_resetButton->setButtonType(TouchButton::Warning);
    m_resetButton->setMinimumSize(150, 50);

    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_applyButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
}

void SettingsPanel::setupSafetyTab()
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

    // Anti-detachment Advanced Settings Group
    QGroupBox* antiDetachmentGroup = new QGroupBox("Anti-detachment Advanced Settings");
    antiDetachmentGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* antiDetachmentForm = new QFormLayout(antiDetachmentGroup);

    // Enable/disable anti-detachment system
    m_antiDetachmentEnabledCheck = new QCheckBox("Anti-detachment System Enabled");
    m_antiDetachmentEnabledCheck->setChecked(true);
    m_antiDetachmentEnabledCheck->setToolTip("Enable or disable the anti-detachment monitoring system");

    // Warning threshold (separate from main detachment threshold)
    m_antiDetachmentWarningThresholdSpin = new QDoubleSpinBox();
    m_antiDetachmentWarningThresholdSpin->setRange(30.0, 100.0);
    m_antiDetachmentWarningThresholdSpin->setSuffix(" mmHg");
    m_antiDetachmentWarningThresholdSpin->setDecimals(1);
    m_antiDetachmentWarningThresholdSpin->setValue(DEFAULT_ANTI_DETACHMENT_WARNING_THRESHOLD);
    m_antiDetachmentWarningThresholdSpin->setToolTip("Pressure threshold for anti-detachment warnings (should be higher than detachment threshold)");

    // Hysteresis to prevent oscillation
    m_antiDetachmentHysteresisSpin = new QDoubleSpinBox();
    m_antiDetachmentHysteresisSpin->setRange(1.0, 20.0);
    m_antiDetachmentHysteresisSpin->setSuffix(" mmHg");
    m_antiDetachmentHysteresisSpin->setDecimals(1);
    m_antiDetachmentHysteresisSpin->setValue(DEFAULT_ANTI_DETACHMENT_HYSTERESIS);
    m_antiDetachmentHysteresisSpin->setToolTip("Hysteresis value to prevent oscillation between states");

    // Response delay
    m_antiDetachmentResponseDelaySpin = new QSpinBox();
    m_antiDetachmentResponseDelaySpin->setRange(0, 1000);
    m_antiDetachmentResponseDelaySpin->setSuffix(" ms");
    m_antiDetachmentResponseDelaySpin->setValue(DEFAULT_ANTI_DETACHMENT_RESPONSE_DELAY_MS);
    m_antiDetachmentResponseDelaySpin->setToolTip("Delay before anti-detachment response activation (0-1000ms)");

    // Maximum vacuum increase
    m_antiDetachmentMaxVacuumIncreaseSpin = new QDoubleSpinBox();
    m_antiDetachmentMaxVacuumIncreaseSpin->setRange(5.0, 50.0);
    m_antiDetachmentMaxVacuumIncreaseSpin->setSuffix(" %");
    m_antiDetachmentMaxVacuumIncreaseSpin->setDecimals(1);
    m_antiDetachmentMaxVacuumIncreaseSpin->setValue(DEFAULT_ANTI_DETACHMENT_MAX_VACUUM_INCREASE);
    m_antiDetachmentMaxVacuumIncreaseSpin->setToolTip("Maximum vacuum increase allowed during anti-detachment response");

    // Monitoring rate
    m_antiDetachmentMonitoringRateSpin = new QSpinBox();
    m_antiDetachmentMonitoringRateSpin->setRange(10, 200);
    m_antiDetachmentMonitoringRateSpin->setSuffix(" Hz");
    m_antiDetachmentMonitoringRateSpin->setValue(DEFAULT_ANTI_DETACHMENT_MONITORING_RATE_HZ);
    m_antiDetachmentMonitoringRateSpin->setToolTip("Monitoring frequency for anti-detachment system (10-200 Hz)");

    // Add controls to form
    antiDetachmentForm->addRow(m_antiDetachmentEnabledCheck);
    antiDetachmentForm->addRow("Warning Threshold:", m_antiDetachmentWarningThresholdSpin);
    antiDetachmentForm->addRow("Hysteresis:", m_antiDetachmentHysteresisSpin);
    antiDetachmentForm->addRow("Response Delay:", m_antiDetachmentResponseDelaySpin);
    antiDetachmentForm->addRow("Max Vacuum Increase:", m_antiDetachmentMaxVacuumIncreaseSpin);
    antiDetachmentForm->addRow("Monitoring Rate:", m_antiDetachmentMonitoringRateSpin);

    safetyLayout->addWidget(pressureGroup);
    safetyLayout->addWidget(featuresGroup);
    safetyLayout->addWidget(sensorGroup);
    safetyLayout->addWidget(antiDetachmentGroup);
    safetyLayout->addStretch();
}

void SettingsPanel::setupCalibrationTab()
{
    m_calibrationInterface = new CalibrationInterface(m_controller, this);
    m_tabWidget->addTab(m_calibrationInterface, "Calibration");
}

void SettingsPanel::setupArousalCalibrationTab()
{
    m_arousalCalibrationTab = new QWidget();
    m_tabWidget->addTab(m_arousalCalibrationTab, "Arousal Thresholds");

    QVBoxLayout* arousalLayout = new QVBoxLayout(m_arousalCalibrationTab);

    // Current Arousal Display Group
    QGroupBox* currentGroup = new QGroupBox("Current Arousal Level");
    currentGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* currentLayout = new QVBoxLayout(currentGroup);

    m_currentArousalLabel = new QLabel("0.00");
    m_currentArousalLabel->setStyleSheet("font-size: 36pt; font-weight: bold; color: #2196F3;");
    m_currentArousalLabel->setAlignment(Qt::AlignCenter);

    m_arousalProgressBar = new QProgressBar();
    m_arousalProgressBar->setRange(0, 100);
    m_arousalProgressBar->setValue(0);
    m_arousalProgressBar->setTextVisible(true);
    m_arousalProgressBar->setFormat("%v%");
    m_arousalProgressBar->setMinimumHeight(30);

    currentLayout->addWidget(m_currentArousalLabel);
    currentLayout->addWidget(m_arousalProgressBar);

    // Threshold Settings Group
    QGroupBox* thresholdGroup = new QGroupBox("Arousal Thresholds");
    thresholdGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* thresholdForm = new QFormLayout(thresholdGroup);

    m_edgeThresholdSpin = new QDoubleSpinBox();
    m_edgeThresholdSpin->setRange(0.50, 0.95);
    m_edgeThresholdSpin->setSingleStep(0.01);
    m_edgeThresholdSpin->setDecimals(2);
    m_edgeThresholdSpin->setValue(DEFAULT_EDGE_THRESHOLD);
    m_edgeThresholdSpin->setToolTip("Arousal level at which edging begins (0.50-0.95)");

    m_orgasmThresholdSpin = new QDoubleSpinBox();
    m_orgasmThresholdSpin->setRange(0.85, 1.00);
    m_orgasmThresholdSpin->setSingleStep(0.01);
    m_orgasmThresholdSpin->setDecimals(2);
    m_orgasmThresholdSpin->setValue(DEFAULT_ORGASM_THRESHOLD);
    m_orgasmThresholdSpin->setToolTip("Arousal level at which orgasm is detected (0.85-1.00)");

    m_recoveryThresholdSpin = new QDoubleSpinBox();
    m_recoveryThresholdSpin->setRange(0.30, 0.80);
    m_recoveryThresholdSpin->setSingleStep(0.01);
    m_recoveryThresholdSpin->setDecimals(2);
    m_recoveryThresholdSpin->setValue(DEFAULT_RECOVERY_THRESHOLD);
    m_recoveryThresholdSpin->setToolTip("Arousal level for recovery from edge (0.30-0.80)");

    thresholdForm->addRow("Edge Threshold:", m_edgeThresholdSpin);
    thresholdForm->addRow("Orgasm Threshold:", m_orgasmThresholdSpin);
    thresholdForm->addRow("Recovery Threshold:", m_recoveryThresholdSpin);

    // Milking Zone Settings Group
    QGroupBox* milkingGroup = new QGroupBox("Milking Zone Configuration");
    milkingGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* milkingForm = new QFormLayout(milkingGroup);

    m_milkingZoneLowerSpin = new QDoubleSpinBox();
    m_milkingZoneLowerSpin->setRange(0.60, 0.85);
    m_milkingZoneLowerSpin->setSingleStep(0.01);
    m_milkingZoneLowerSpin->setDecimals(2);
    m_milkingZoneLowerSpin->setValue(DEFAULT_MILKING_ZONE_LOWER);
    m_milkingZoneLowerSpin->setToolTip("Lower bound of milking zone (0.60-0.85)");

    m_milkingZoneUpperSpin = new QDoubleSpinBox();
    m_milkingZoneUpperSpin->setRange(0.80, 0.95);
    m_milkingZoneUpperSpin->setSingleStep(0.01);
    m_milkingZoneUpperSpin->setDecimals(2);
    m_milkingZoneUpperSpin->setValue(DEFAULT_MILKING_ZONE_UPPER);
    m_milkingZoneUpperSpin->setToolTip("Upper bound of milking zone (0.80-0.95)");

    m_dangerThresholdSpin = new QDoubleSpinBox();
    m_dangerThresholdSpin->setRange(0.88, 0.98);
    m_dangerThresholdSpin->setSingleStep(0.01);
    m_dangerThresholdSpin->setDecimals(2);
    m_dangerThresholdSpin->setValue(DEFAULT_DANGER_THRESHOLD);
    m_dangerThresholdSpin->setToolTip("Danger zone threshold - approaching orgasm (0.88-0.98)");

    m_milkingFailureModeCombo = new QComboBox();
    m_milkingFailureModeCombo->addItem("Stop Session", 0);
    m_milkingFailureModeCombo->addItem("Ruin Orgasm", 1);
    m_milkingFailureModeCombo->addItem("Punish", 2);
    m_milkingFailureModeCombo->addItem("Continue", 3);
    m_milkingFailureModeCombo->setToolTip("Action when orgasm occurs during milking mode");

    milkingForm->addRow("Milking Zone Lower:", m_milkingZoneLowerSpin);
    milkingForm->addRow("Milking Zone Upper:", m_milkingZoneUpperSpin);
    milkingForm->addRow("Danger Threshold:", m_dangerThresholdSpin);
    milkingForm->addRow("Failure Mode:", m_milkingFailureModeCombo);

    // Advanced Options Group
    QGroupBox* advancedGroup = new QGroupBox("Advanced Options");
    advancedGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);

    m_tensEnabledCheck = new QCheckBox("Enable TENS Integration");
    m_tensEnabledCheck->setToolTip("Enable TENS unit for enhanced stimulation control");

    m_antiEscapeEnabledCheck = new QCheckBox("Enable Anti-Escape Mode");
    m_antiEscapeEnabledCheck->setToolTip("Prevent user from escaping stimulation");

    advancedLayout->addWidget(m_tensEnabledCheck);
    advancedLayout->addWidget(m_antiEscapeEnabledCheck);

    arousalLayout->addWidget(currentGroup);
    arousalLayout->addWidget(thresholdGroup);
    arousalLayout->addWidget(milkingGroup);
    arousalLayout->addWidget(advancedGroup);
    arousalLayout->addStretch();

    // Connect arousal threshold spinboxes to controller
    connect(m_edgeThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setEdgeThreshold(value);
        }
    });

    connect(m_orgasmThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setOrgasmThreshold(value);
        }
    });

    connect(m_recoveryThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setRecoveryThreshold(value);
        }
    });

    // Connect TENS and anti-escape checkboxes
    connect(m_tensEnabledCheck, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setTENSEnabled(checked);
        }
    });

    connect(m_antiEscapeEnabledCheck, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setAntiEscapeEnabled(checked);
        }
    });

    // Connect milking zone spinboxes to controller
    connect(m_milkingZoneLowerSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setMilkingZoneLower(value);
        }
    });

    connect(m_milkingZoneUpperSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setMilkingZoneUpper(value);
        }
    });

    connect(m_dangerThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setDangerThreshold(value);
        }
    });

    connect(m_milkingFailureModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
            m_controller->getOrgasmControlAlgorithm()->setMilkingFailureMode(index);
        }
    });
}

void SettingsPanel::setupMilkingConfigurationTab()
{
    m_milkingConfigTab = new QWidget();
    m_tabWidget->addTab(m_milkingConfigTab, "Milking Mode");

    QVBoxLayout* milkingLayout = new QVBoxLayout(m_milkingConfigTab);
    milkingLayout->setSpacing(15);

    // Session Configuration Group
    QGroupBox* sessionGroup = new QGroupBox("Milking Session Configuration");
    sessionGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* sessionForm = new QFormLayout(sessionGroup);

    m_milkingDurationSpin = new QSpinBox();
    m_milkingDurationSpin->setRange(5, 120);
    m_milkingDurationSpin->setValue(30);
    m_milkingDurationSpin->setSuffix(" min");
    m_milkingDurationSpin->setToolTip("Duration of milking session (5-120 minutes)");

    m_milkingTargetOrgasmsSpin = new QSpinBox();
    m_milkingTargetOrgasmsSpin->setRange(0, 10);
    m_milkingTargetOrgasmsSpin->setValue(0);
    m_milkingTargetOrgasmsSpin->setToolTip("Target orgasms (0 = pure milking, no orgasms allowed)");

    sessionForm->addRow("Session Duration:", m_milkingDurationSpin);
    sessionForm->addRow("Target Orgasms:", m_milkingTargetOrgasmsSpin);

    // Intensity Control Group
    QGroupBox* intensityGroup = new QGroupBox("Intensity Control");
    intensityGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* intensityForm = new QFormLayout(intensityGroup);

    m_milkingIntensityMinSpin = new QDoubleSpinBox();
    m_milkingIntensityMinSpin->setRange(0.1, 0.5);
    m_milkingIntensityMinSpin->setSingleStep(0.05);
    m_milkingIntensityMinSpin->setDecimals(2);
    m_milkingIntensityMinSpin->setValue(0.20);
    m_milkingIntensityMinSpin->setToolTip("Minimum stimulation intensity during milking");

    m_milkingIntensityMaxSpin = new QDoubleSpinBox();
    m_milkingIntensityMaxSpin->setRange(0.5, 1.0);
    m_milkingIntensityMaxSpin->setSingleStep(0.05);
    m_milkingIntensityMaxSpin->setDecimals(2);
    m_milkingIntensityMaxSpin->setValue(0.70);
    m_milkingIntensityMaxSpin->setToolTip("Maximum stimulation intensity during milking");

    m_milkingAutoAdjustCheck = new QCheckBox("Auto-adjust intensity based on arousal");
    m_milkingAutoAdjustCheck->setChecked(true);
    m_milkingAutoAdjustCheck->setToolTip("Automatically adjust intensity to maintain arousal in milking zone");

    intensityForm->addRow("Minimum Intensity:", m_milkingIntensityMinSpin);
    intensityForm->addRow("Maximum Intensity:", m_milkingIntensityMaxSpin);
    intensityForm->addRow("", m_milkingAutoAdjustCheck);

    // PID Control Group (Advanced)
    QGroupBox* pidGroup = new QGroupBox("PID Control (Advanced)");
    pidGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QFormLayout* pidForm = new QFormLayout(pidGroup);

    m_milkingPidKpSpin = new QDoubleSpinBox();
    m_milkingPidKpSpin->setRange(0.0, 2.0);
    m_milkingPidKpSpin->setSingleStep(0.1);
    m_milkingPidKpSpin->setDecimals(2);
    m_milkingPidKpSpin->setValue(0.5);
    m_milkingPidKpSpin->setToolTip("Proportional gain for arousal control");

    m_milkingPidKiSpin = new QDoubleSpinBox();
    m_milkingPidKiSpin->setRange(0.0, 1.0);
    m_milkingPidKiSpin->setSingleStep(0.05);
    m_milkingPidKiSpin->setDecimals(2);
    m_milkingPidKiSpin->setValue(0.1);
    m_milkingPidKiSpin->setToolTip("Integral gain for arousal control");

    m_milkingPidKdSpin = new QDoubleSpinBox();
    m_milkingPidKdSpin->setRange(0.0, 1.0);
    m_milkingPidKdSpin->setSingleStep(0.05);
    m_milkingPidKdSpin->setDecimals(2);
    m_milkingPidKdSpin->setValue(0.2);
    m_milkingPidKdSpin->setToolTip("Derivative gain for arousal control");

    pidForm->addRow("Kp (Proportional):", m_milkingPidKpSpin);
    pidForm->addRow("Ki (Integral):", m_milkingPidKiSpin);
    pidForm->addRow("Kd (Derivative):", m_milkingPidKdSpin);

    // Status Display Group
    QGroupBox* statusGroup = new QGroupBox("Milking Status");
    statusGroup->setStyleSheet("QGroupBox { font-size: 14pt; font-weight: bold; }");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);

    m_milkingStatusLabel = new QLabel("Status: Not Active");
    m_milkingStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #666;");

    m_milkingZoneProgressBar = new QProgressBar();
    m_milkingZoneProgressBar->setRange(0, 100);
    m_milkingZoneProgressBar->setValue(0);
    m_milkingZoneProgressBar->setFormat("Zone Time: %v%");
    m_milkingZoneProgressBar->setMinimumHeight(30);
    m_milkingZoneProgressBar->setStyleSheet(
        "QProgressBar { border: 2px solid #ccc; border-radius: 5px; background: #f0f0f0; }"
        "QProgressBar::chunk { background: #795548; border-radius: 3px; }"
    );

    statusLayout->addWidget(m_milkingStatusLabel);
    statusLayout->addWidget(m_milkingZoneProgressBar);

    milkingLayout->addWidget(sessionGroup);
    milkingLayout->addWidget(intensityGroup);
    milkingLayout->addWidget(pidGroup);
    milkingLayout->addWidget(statusGroup);
    milkingLayout->addStretch();

    // Connect milking status updates from algorithm
    if (m_controller && m_controller->getOrgasmControlAlgorithm()) {
        OrgasmControlAlgorithm* algo = m_controller->getOrgasmControlAlgorithm();

        connect(algo, &OrgasmControlAlgorithm::milkingZoneEntered,
                this, [this](double arousal) {
            m_milkingStatusLabel->setText(QString("Status: In Milking Zone (Arousal: %1)").arg(arousal, 0, 'f', 2));
            m_milkingStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #795548;");
        });

        connect(algo, &OrgasmControlAlgorithm::dangerZoneEntered,
                this, [this](double arousal) {
            m_milkingStatusLabel->setText(QString("Status: DANGER ZONE (Arousal: %1)").arg(arousal, 0, 'f', 2));
            m_milkingStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #F44336;");
        });

        connect(algo, &OrgasmControlAlgorithm::dangerZoneExited,
                this, [this](double arousal) {
            m_milkingStatusLabel->setText(QString("Status: Recovered (Arousal: %1)").arg(arousal, 0, 'f', 2));
            m_milkingStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #4CAF50;");
        });

        connect(algo, &OrgasmControlAlgorithm::unwantedOrgasm,
                this, [this](int count, qint64 duration) {
            m_milkingStatusLabel->setText(QString("Status: ORGASM FAILURE #%1 at %2s").arg(count).arg(duration / 1000));
            m_milkingStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #E91E63;");
        });

        connect(algo, &OrgasmControlAlgorithm::milkingSessionComplete,
                this, [this](qint64 duration, bool success, int dangerEntries) {
            QString status = success ? "SUCCESS" : "FAILED";
            m_milkingStatusLabel->setText(QString("Session Complete: %1 (%2 min, %3 danger entries)")
                .arg(status).arg(duration / 60000).arg(dangerEntries));
            m_milkingStatusLabel->setStyleSheet(QString("font-size: 16pt; font-weight: bold; color: %1;")
                .arg(success ? "#4CAF50" : "#F44336"));
        });

        connect(algo, &OrgasmControlAlgorithm::milkingZoneMaintained,
                this, [this](qint64 durationMs, double avgArousal) {
            int percent = qMin(100, static_cast<int>(durationMs / 600));  // 60 seconds = 100%
            m_milkingZoneProgressBar->setValue(percent);
            m_milkingZoneProgressBar->setFormat(QString("Zone Time: %1s (Avg: %2)")
                .arg(durationMs / 1000).arg(avgArousal, 0, 'f', 2));
        });
    }
}

void SettingsPanel::setupHardwareTab()
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

void SettingsPanel::setupDisplayTab()
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

void SettingsPanel::setupDiagnosticsTab()
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

void SettingsPanel::setupMaintenanceTab()
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

void SettingsPanel::connectSignals()
{
    // Button connections
    connect(m_applyButton, &TouchButton::clicked, this, &SettingsPanel::onApplyClicked);
    connect(m_resetButton, &TouchButton::clicked, this, &SettingsPanel::resetToDefaults);
    
    // Hardware connections
    connect(m_testHardwareButton, &TouchButton::clicked, this, &SettingsPanel::onTestHardwareClicked);
    
    // Maintenance connections
    connect(m_exportSettingsButton, &TouchButton::clicked, this, &SettingsPanel::onExportSettingsClicked);
    connect(m_importSettingsButton, &TouchButton::clicked, this, &SettingsPanel::onImportSettingsClicked);
    connect(m_factoryResetButton, &TouchButton::clicked, this, &SettingsPanel::onFactoryResetClicked);
}

void SettingsPanel::loadSettings()
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

        // Load anti-detachment advanced settings
        m_antiDetachmentEnabledCheck->setChecked(safetySettings["anti_detachment_enabled"].toBool(true));
        m_antiDetachmentWarningThresholdSpin->setValue(safetySettings["anti_detachment_warning_threshold_mmhg"].toDouble(DEFAULT_ANTI_DETACHMENT_WARNING_THRESHOLD));
        m_antiDetachmentHysteresisSpin->setValue(safetySettings["anti_detachment_hysteresis_mmhg"].toDouble(DEFAULT_ANTI_DETACHMENT_HYSTERESIS));
        m_antiDetachmentResponseDelaySpin->setValue(safetySettings["anti_detachment_response_delay_ms"].toInt(DEFAULT_ANTI_DETACHMENT_RESPONSE_DELAY_MS));
        m_antiDetachmentMaxVacuumIncreaseSpin->setValue(safetySettings["anti_detachment_max_vacuum_increase_percent"].toDouble(DEFAULT_ANTI_DETACHMENT_MAX_VACUUM_INCREASE));
        m_antiDetachmentMonitoringRateSpin->setValue(safetySettings["anti_detachment_monitoring_rate_hz"].toInt(DEFAULT_ANTI_DETACHMENT_MONITORING_RATE_HZ));

        // Load safety feature checkboxes
        m_emergencyStopCheck->setChecked(safetySettings["emergency_stop_enabled"].toBool(true));
        m_overpressureProtectionCheck->setChecked(safetySettings["overpressure_protection_enabled"].toBool(true));
        m_autoShutdownCheck->setChecked(safetySettings["auto_shutdown_on_error"].toBool(true));

        // Load other settings...
    }
}

void SettingsPanel::saveSettings()
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

    // Save anti-detachment advanced settings
    safetySettings["anti_detachment_enabled"] = m_antiDetachmentEnabledCheck->isChecked();
    safetySettings["anti_detachment_warning_threshold_mmhg"] = m_antiDetachmentWarningThresholdSpin->value();
    safetySettings["anti_detachment_hysteresis_mmhg"] = m_antiDetachmentHysteresisSpin->value();
    safetySettings["anti_detachment_response_delay_ms"] = m_antiDetachmentResponseDelaySpin->value();
    safetySettings["anti_detachment_max_vacuum_increase_percent"] = m_antiDetachmentMaxVacuumIncreaseSpin->value();
    safetySettings["anti_detachment_monitoring_rate_hz"] = m_antiDetachmentMonitoringRateSpin->value();
    
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

void SettingsPanel::resetToDefaults()
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

        // Reset anti-detachment advanced settings
        m_antiDetachmentEnabledCheck->setChecked(true);
        m_antiDetachmentWarningThresholdSpin->setValue(DEFAULT_ANTI_DETACHMENT_WARNING_THRESHOLD);
        m_antiDetachmentHysteresisSpin->setValue(DEFAULT_ANTI_DETACHMENT_HYSTERESIS);
        m_antiDetachmentResponseDelaySpin->setValue(DEFAULT_ANTI_DETACHMENT_RESPONSE_DELAY_MS);
        m_antiDetachmentMaxVacuumIncreaseSpin->setValue(DEFAULT_ANTI_DETACHMENT_MAX_VACUUM_INCREASE);
        m_antiDetachmentMonitoringRateSpin->setValue(DEFAULT_ANTI_DETACHMENT_MONITORING_RATE_HZ);

        // Reset other tabs...
        
        QMessageBox::information(this, "Reset Complete", "All settings have been reset to default values.");
    }
}

void SettingsPanel::onTestHardwareClicked()
{
    // performHardwareTest();
}

bool SettingsPanel::validateSettings()
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

    // Validate anti-detachment advanced settings
    if (m_antiDetachmentWarningThresholdSpin->value() <= m_antiDetachmentSpin->value()) {
        QMessageBox::warning(this, "Invalid Settings",
                           "Anti-detachment warning threshold must be higher than detachment threshold.");
        return false;
    }

    if (m_antiDetachmentHysteresisSpin->value() >= m_antiDetachmentSpin->value()) {
        QMessageBox::warning(this, "Invalid Settings",
                           "Hysteresis value must be less than detachment threshold.");
        return false;
    }

    if (m_antiDetachmentMaxVacuumIncreaseSpin->value() > 50.0) {
        QMessageBox::warning(this, "Invalid Settings",
                           "Maximum vacuum increase should not exceed 50% for safety.");
        return false;
    }

    if (m_antiDetachmentMonitoringRateSpin->value() < 10) {
        QMessageBox::warning(this, "Invalid Settings",
                           "Monitoring rate should be at least 10 Hz for effective detection.");
        return false;
    }

    return true;
}

void SettingsPanel::onApplyClicked()
{
    if (validateSettings()) {
        saveSettings();
    }
}



void SettingsPanel::onExportSettingsClicked()
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

void SettingsPanel::onImportSettingsClicked()
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

void SettingsPanel::onFactoryResetClicked()
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
        
        QMessageBox::information(this, "Factory Reset Complete",
                               "All settings have been reset to factory defaults.\n\n"
                               "Please recalibrate sensors before use.");
    }
}

