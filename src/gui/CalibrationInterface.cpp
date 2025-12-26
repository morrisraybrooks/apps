#include "CalibrationInterface.h"
#include "components/TouchButton.h"
#include "styles/ModernMedicalStyle.h"
#include "../VacuumController.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QApplication>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>

// Constants
const int CalibrationInterface::BUTTON_MIN_HEIGHT;
const int CalibrationInterface::BUTTON_MIN_WIDTH;
const int CalibrationInterface::FONT_SIZE_NORMAL;
const int CalibrationInterface::FONT_SIZE_LARGE;
const int CalibrationInterface::SPACING_NORMAL;
const int CalibrationInterface::SPACING_LARGE;

CalibrationInterface::CalibrationInterface(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_calibrationManager(nullptr)
    , m_mainLayout(nullptr)
    , m_controlGroup(nullptr)
    , m_componentCombo(nullptr)
    , m_calibrationTypeCombo(nullptr)
    , m_startButton(nullptr)
    , m_cancelButton(nullptr)
    , m_validateButton(nullptr)
    , m_statusGroup(nullptr)
    , m_statusTable(nullptr)
    , m_refreshStatusButton(nullptr)
    , m_lastUpdateLabel(nullptr)
    , m_progressGroup(nullptr)
    , m_calibrationProgress(nullptr)
    , m_progressStatusLabel(nullptr)
    , m_currentStepLabel(nullptr)
    , m_elapsedTimeLabel(nullptr)
    , m_statusUpdateTimer(new QTimer(this))
    , m_resultsGroup(nullptr)
    , m_resultsTable(nullptr)
    , m_calibrationLog(nullptr)
    , m_exportResultsButton(nullptr)
    , m_clearLogButton(nullptr)
    , m_settingsGroup(nullptr)
    , m_minPointsSpin(nullptr)
    , m_maxErrorSpin(nullptr)
    , m_calibrationTimeoutSpin(nullptr)
    , m_autoSaveCheck(nullptr)
    , m_autoValidateCheck(nullptr)
    , m_saveSettingsButton(nullptr)
    , m_resetSettingsButton(nullptr)
    , m_importExportGroup(nullptr)
    , m_exportButton(nullptr)
    , m_importButton(nullptr)
    , m_backupButton(nullptr)
    , m_restoreButton(nullptr)
    , m_currentType(CalibrationManager::SENSOR_CALIBRATION)
    , m_calibrationInProgress(false)
    , m_currentProgress(0)
{
    // Get calibration manager from controller
    if (m_controller) {
        m_calibrationManager = m_controller->getCalibrationManager();
    }
    
    // Set up settings file path
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_settingsFilePath = appDataPath + "/calibration_settings.json";
    
    // Setup UI
    setupUI();
    connectSignals();
    applyTouchOptimizedStyles();
    
    // Load settings
    loadCalibrationSettings();
    
    // Setup status update timer
    m_statusUpdateTimer->setInterval(1000); // Update every second
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &CalibrationInterface::onStatusUpdateTimer);
    
    // Initial status update
    refreshCalibrationStatus();
    
    qDebug() << "CalibrationInterface initialized";
}

CalibrationInterface::~CalibrationInterface()
{
    if (m_calibrationInProgress) {
        cancelCurrentCalibration();
    }
}

void CalibrationInterface::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(SPACING_LARGE);
    m_mainLayout->setContentsMargins(SPACING_NORMAL, SPACING_NORMAL, SPACING_NORMAL, SPACING_NORMAL);
    
    // Create all panels
    setupControlPanel();
    setupStatusPanel();
    setupProgressPanel();
    setupResultsPanel();
    setupSettingsPanel();
    
    // Add panels to main layout
    m_mainLayout->addWidget(m_controlGroup);
    m_mainLayout->addWidget(m_statusGroup);
    m_mainLayout->addWidget(m_progressGroup);
    m_mainLayout->addWidget(m_resultsGroup);
    m_mainLayout->addWidget(m_settingsGroup);
    
    // Set stretch factors to optimize space usage
    m_mainLayout->setStretchFactor(m_controlGroup, 0);
    m_mainLayout->setStretchFactor(m_statusGroup, 1);
    m_mainLayout->setStretchFactor(m_progressGroup, 0);
    m_mainLayout->setStretchFactor(m_resultsGroup, 2);
    m_mainLayout->setStretchFactor(m_settingsGroup, 1);
}

void CalibrationInterface::setupControlPanel()
{
    m_controlGroup = new QGroupBox("Calibration Control");
    m_controlGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    QHBoxLayout* controlLayout = new QHBoxLayout(m_controlGroup);
    controlLayout->setSpacing(SPACING_NORMAL);
    
    // Component selection
    QLabel* componentLabel = new QLabel("Component:");
    componentLabel->setMinimumWidth(100);
    
    m_componentCombo = new QComboBox();
    m_componentCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_componentCombo->addItems({"AVL Sensor", "Tank Sensor", "Pump", "SOL1", "SOL2", "SOL3", "System"});
    
    // Calibration type selection
    QLabel* typeLabel = new QLabel("Type:");
    typeLabel->setMinimumWidth(80);
    
    m_calibrationTypeCombo = new QComboBox();
    m_calibrationTypeCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_calibrationTypeCombo->addItems({"Sensor Calibration", "Actuator Calibration", "System Calibration"});
    
    // Control buttons
    m_startButton = new TouchButton("Start Calibration");
    m_startButton->setButtonType(TouchButton::Primary);
    m_startButton->setMinimumSize(BUTTON_MIN_WIDTH + 30, BUTTON_MIN_HEIGHT);
    
    m_cancelButton = new TouchButton("Cancel");
    m_cancelButton->setButtonType(TouchButton::Warning);
    m_cancelButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    m_cancelButton->setEnabled(false);
    
    m_validateButton = new TouchButton("Validate");
    m_validateButton->setButtonType(TouchButton::Normal);
    m_validateButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    // Layout controls
    controlLayout->addWidget(componentLabel);
    controlLayout->addWidget(m_componentCombo);
    controlLayout->addWidget(typeLabel);
    controlLayout->addWidget(m_calibrationTypeCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_cancelButton);
    controlLayout->addWidget(m_validateButton);
}

void CalibrationInterface::setupStatusPanel()
{
    m_statusGroup = new QGroupBox("Calibration Status");
    m_statusGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    QVBoxLayout* statusLayout = new QVBoxLayout(m_statusGroup);
    
    // Status table
    m_statusTable = new QTableWidget(0, 6);
    m_statusTable->setHorizontalHeaderLabels({"Component", "Status", "Last Calibration", "Correlation", "Max Error", "Expired"});
    m_statusTable->horizontalHeader()->setStretchLastSection(true);
    m_statusTable->setAlternatingRowColors(true);
    m_statusTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_statusTable->setMinimumHeight(150);
    
    // Status controls
    QHBoxLayout* statusControlLayout = new QHBoxLayout();
    
    m_refreshStatusButton = new TouchButton("Refresh Status");
    m_refreshStatusButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_lastUpdateLabel = new QLabel("Last Update: Never");
    m_lastUpdateLabel->setStyleSheet("color: #666; font-style: italic;");
    
    statusControlLayout->addWidget(m_refreshStatusButton);
    statusControlLayout->addStretch();
    statusControlLayout->addWidget(m_lastUpdateLabel);
    
    statusLayout->addWidget(m_statusTable);
    statusLayout->addLayout(statusControlLayout);
}

void CalibrationInterface::setupProgressPanel()
{
    m_progressGroup = new QGroupBox("Calibration Progress");
    m_progressGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    m_progressGroup->setVisible(false); // Hidden by default
    
    QVBoxLayout* progressLayout = new QVBoxLayout(m_progressGroup);
    
    // Progress bar
    m_calibrationProgress = new QProgressBar();
    m_calibrationProgress->setMinimumHeight(30);
    m_calibrationProgress->setRange(0, 100);
    m_calibrationProgress->setValue(0);
    
    // Status labels
    m_progressStatusLabel = new QLabel("Ready");
    m_progressStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_currentStepLabel = new QLabel("Step: 0 of 0");
    m_elapsedTimeLabel = new QLabel("Elapsed: 00:00");
    
    QHBoxLayout* labelsLayout = new QHBoxLayout();
    labelsLayout->addWidget(m_currentStepLabel);
    labelsLayout->addStretch();
    labelsLayout->addWidget(m_elapsedTimeLabel);
    
    progressLayout->addWidget(m_calibrationProgress);
    progressLayout->addWidget(m_progressStatusLabel);
    progressLayout->addLayout(labelsLayout);
}

void CalibrationInterface::setupResultsPanel()
{
    m_resultsGroup = new QGroupBox("Calibration Results");
    m_resultsGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    QHBoxLayout* resultsLayout = new QHBoxLayout(m_resultsGroup);
    
    // Results table
    m_resultsTable = new QTableWidget(0, 4);
    m_resultsTable->setHorizontalHeaderLabels({"Point", "Reference", "Measured", "Error %"});
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->setMinimumWidth(300);
    
    // Calibration log
    QVBoxLayout* logLayout = new QVBoxLayout();
    
    QLabel* logLabel = new QLabel("Calibration Log:");
    logLabel->setStyleSheet("font-weight: bold;");
    
    m_calibrationLog = new QTextEdit();
    m_calibrationLog->setMaximumHeight(200);
    m_calibrationLog->setReadOnly(true);
    m_calibrationLog->setStyleSheet("font-family: monospace; font-size: 10pt;");
    
    // Log controls
    QHBoxLayout* logControlLayout = new QHBoxLayout();
    
    m_exportResultsButton = new TouchButton("Export Results");
    m_exportResultsButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_clearLogButton = new TouchButton("Clear Log");
    m_clearLogButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    logControlLayout->addWidget(m_exportResultsButton);
    logControlLayout->addWidget(m_clearLogButton);
    logControlLayout->addStretch();
    
    logLayout->addWidget(logLabel);
    logLayout->addWidget(m_calibrationLog);
    logLayout->addLayout(logControlLayout);
    
    resultsLayout->addWidget(m_resultsTable);
    resultsLayout->addLayout(logLayout);
}

void CalibrationInterface::setupSettingsPanel()
{
    m_settingsGroup = new QGroupBox("Calibration Settings");
    m_settingsGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());

    QHBoxLayout* settingsLayout = new QHBoxLayout(m_settingsGroup);

    // Settings controls
    QGridLayout* settingsGrid = new QGridLayout();

    // Minimum calibration points
    settingsGrid->addWidget(new QLabel("Min Points:"), 0, 0);
    m_minPointsSpin = new QSpinBox();
    m_minPointsSpin->setRange(3, 20);
    m_minPointsSpin->setValue(5);
    m_minPointsSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    settingsGrid->addWidget(m_minPointsSpin, 0, 1);

    // Maximum calibration error
    settingsGrid->addWidget(new QLabel("Max Error (%):"), 0, 2);
    m_maxErrorSpin = new QDoubleSpinBox();
    m_maxErrorSpin->setRange(0.1, 10.0);
    m_maxErrorSpin->setValue(2.0);
    m_maxErrorSpin->setDecimals(1);
    m_maxErrorSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    settingsGrid->addWidget(m_maxErrorSpin, 0, 3);

    // Calibration timeout
    settingsGrid->addWidget(new QLabel("Timeout (min):"), 1, 0);
    m_calibrationTimeoutSpin = new QSpinBox();
    m_calibrationTimeoutSpin->setRange(1, 60);
    m_calibrationTimeoutSpin->setValue(5);
    m_calibrationTimeoutSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    settingsGrid->addWidget(m_calibrationTimeoutSpin, 1, 1);

    // Auto-save checkbox
    m_autoSaveCheck = new QCheckBox("Auto-save results");
    m_autoSaveCheck->setChecked(true);
    settingsGrid->addWidget(m_autoSaveCheck, 1, 2);

    // Auto-validate checkbox
    m_autoValidateCheck = new QCheckBox("Auto-validate");
    m_autoValidateCheck->setChecked(true);
    settingsGrid->addWidget(m_autoValidateCheck, 1, 3);

    // Settings buttons
    QVBoxLayout* settingsButtonLayout = new QVBoxLayout();

    m_saveSettingsButton = new TouchButton("Save Settings");
    m_saveSettingsButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    m_resetSettingsButton = new TouchButton("Reset Settings");
    m_resetSettingsButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    settingsButtonLayout->addWidget(m_saveSettingsButton);
    settingsButtonLayout->addWidget(m_resetSettingsButton);
    settingsButtonLayout->addStretch();

    // Import/Export section
    QVBoxLayout* importExportLayout = new QVBoxLayout();

    QLabel* importExportLabel = new QLabel("Data Management:");
    importExportLabel->setStyleSheet("font-weight: bold;");

    m_exportButton = new TouchButton("Export Data");
    m_exportButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    m_importButton = new TouchButton("Import Data");
    m_importButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    m_backupButton = new TouchButton("Backup All");
    m_backupButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    m_restoreButton = new TouchButton("Restore All");
    m_restoreButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    importExportLayout->addWidget(importExportLabel);
    importExportLayout->addWidget(m_exportButton);
    importExportLayout->addWidget(m_importButton);
    importExportLayout->addWidget(m_backupButton);
    importExportLayout->addWidget(m_restoreButton);
    importExportLayout->addStretch();

    settingsLayout->addLayout(settingsGrid);
    settingsLayout->addLayout(settingsButtonLayout);
    settingsLayout->addLayout(importExportLayout);
}

void CalibrationInterface::connectSignals()
{
    // Connect CalibrationManager signals
    if (m_calibrationManager) {
        connect(m_calibrationManager, &CalibrationManager::calibrationStarted,
                this, &CalibrationInterface::onCalibrationStarted);
        connect(m_calibrationManager, &CalibrationManager::calibrationProgress,
                this, &CalibrationInterface::onCalibrationProgress);
        connect(m_calibrationManager, &CalibrationManager::calibrationPointAdded,
                this, &CalibrationInterface::onCalibrationPointAdded);
        connect(m_calibrationManager, &CalibrationManager::calibrationCompleted,
                this, &CalibrationInterface::onCalibrationCompleted);
        connect(m_calibrationManager, &CalibrationManager::calibrationFailed,
                this, &CalibrationInterface::onCalibrationFailed);
        connect(m_calibrationManager, &CalibrationManager::calibrationDataSaved,
                this, &CalibrationInterface::onCalibrationDataSaved);
        connect(m_calibrationManager, &CalibrationManager::calibrationValidated,
                this, &CalibrationInterface::onCalibrationValidated);
    }

    // Connect UI signals
    connect(m_componentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CalibrationInterface::onComponentSelectionChanged);
    connect(m_calibrationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CalibrationInterface::onCalibrationTypeChanged);

    connect(m_startButton, &TouchButton::clicked, this, &CalibrationInterface::onStartCalibrationClicked);
    connect(m_cancelButton, &TouchButton::clicked, this, &CalibrationInterface::onCancelCalibrationClicked);
    connect(m_validateButton, &TouchButton::clicked, this, &CalibrationInterface::onValidateCalibrationClicked);

    connect(m_refreshStatusButton, &TouchButton::clicked, this, &CalibrationInterface::refreshCalibrationStatus);
    connect(m_exportResultsButton, &TouchButton::clicked, this, &CalibrationInterface::onExportCalibrationClicked);
    connect(m_clearLogButton, &TouchButton::clicked, [this]() { m_calibrationLog->clear(); });

    connect(m_saveSettingsButton, &TouchButton::clicked, this, &CalibrationInterface::saveCalibrationSettings);
    connect(m_resetSettingsButton, &TouchButton::clicked, this, &CalibrationInterface::onResetCalibrationClicked);

    connect(m_exportButton, &TouchButton::clicked, this, &CalibrationInterface::onExportCalibrationClicked);
    connect(m_importButton, &TouchButton::clicked, this, &CalibrationInterface::onImportCalibrationClicked);

    // Settings change signals
    connect(m_minPointsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CalibrationInterface::onSettingsChanged);
    connect(m_maxErrorSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CalibrationInterface::onSettingsChanged);
    connect(m_calibrationTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CalibrationInterface::onSettingsChanged);
    connect(m_autoSaveCheck, &QCheckBox::toggled, this, &CalibrationInterface::onSettingsChanged);
    connect(m_autoValidateCheck, &QCheckBox::toggled, this, &CalibrationInterface::onSettingsChanged);
}

void CalibrationInterface::applyTouchOptimizedStyles()
{
    // Apply touch-friendly styles for 800x480 display
    setStyleSheet(
        "QGroupBox {"
        "  font-size: 14pt;"
        "  font-weight: bold;"
        "  padding-top: 15px;"
        "  margin-top: 10px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px 0 5px;"
        "}"
        "QComboBox {"
        "  font-size: 12pt;"
        "  padding: 5px;"
        "  border: 2px solid #ddd;"
        "  border-radius: 5px;"
        "}"
        "QSpinBox, QDoubleSpinBox {"
        "  font-size: 12pt;"
        "  padding: 5px;"
        "  border: 2px solid #ddd;"
        "  border-radius: 5px;"
        "}"
        "QTableWidget {"
        "  font-size: 11pt;"
        "  gridline-color: #ddd;"
        "  selection-background-color: #2196F3;"
        "}"
        "QTableWidget::item {"
        "  padding: 8px;"
        "}"
        "QProgressBar {"
        "  border: 2px solid #ddd;"
        "  border-radius: 5px;"
        "  text-align: center;"
        "  font-size: 12pt;"
        "  font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #4CAF50;"
        "  border-radius: 3px;"
        "}"
        "QCheckBox {"
        "  font-size: 12pt;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 20px;"
        "  height: 20px;"
        "}"
    );
}

// Public interface methods
void CalibrationInterface::refreshCalibrationStatus()
{
    if (!m_calibrationManager) return;

    updateCalibrationStatus();
    m_lastUpdateLabel->setText(QString("Last Update: %1")
                              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void CalibrationInterface::showCalibrationResults(const QString& component)
{
    // This method would show detailed results for a specific component
    // Implementation would depend on specific requirements
    Q_UNUSED(component)
}

void CalibrationInterface::resetInterface()
{
    m_calibrationInProgress = false;
    m_currentProgress = 0;
    m_currentStatus.clear();

    m_progressGroup->setVisible(false);
    m_calibrationProgress->setValue(0);
    m_progressStatusLabel->setText("Ready");

    enableCalibrationControls(true);
    clearResultsTable();
}

// Calibration control slots
void CalibrationInterface::startSensorCalibration()
{
    if (!m_calibrationManager || m_calibrationInProgress) return;

    QString component = m_componentCombo->currentText();
    if (component == "System") {
        m_calibrationManager->startSystemCalibration();
    } else {
        m_calibrationManager->startSensorCalibration(component);
    }
}

void CalibrationInterface::startActuatorCalibration()
{
    if (!m_calibrationManager || m_calibrationInProgress) return;

    QString component = m_componentCombo->currentText();
    m_calibrationManager->startActuatorCalibration(component);
}

void CalibrationInterface::startSystemCalibration()
{
    if (!m_calibrationManager || m_calibrationInProgress) return;

    m_calibrationManager->startSystemCalibration();
}

void CalibrationInterface::cancelCurrentCalibration()
{
    if (!m_calibrationManager || !m_calibrationInProgress) return;

    m_calibrationManager->cancelCalibration();
    resetInterface();

    m_calibrationLog->append(QString("[%1] Calibration cancelled by user")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void CalibrationInterface::saveCalibrationSettings()
{
    // Update settings object
    m_calibrationSettings["min_calibration_points"] = m_minPointsSpin->value();
    m_calibrationSettings["max_calibration_error"] = m_maxErrorSpin->value();
    m_calibrationSettings["calibration_timeout_minutes"] = m_calibrationTimeoutSpin->value();
    m_calibrationSettings["auto_save_enabled"] = m_autoSaveCheck->isChecked();
    m_calibrationSettings["auto_validate_enabled"] = m_autoValidateCheck->isChecked();

    // Save to file
    QJsonDocument doc(m_calibrationSettings);
    QFile file(m_settingsFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();

        // Apply settings to calibration manager
        if (m_calibrationManager) {
            m_calibrationManager->setMinCalibrationPoints(m_minPointsSpin->value());
            m_calibrationManager->setMaxCalibrationError(m_maxErrorSpin->value() / 100.0);
            m_calibrationManager->setAutoSaveEnabled(m_autoSaveCheck->isChecked());
        }

        QMessageBox::information(this, "Settings Saved", "Calibration settings have been saved successfully.");
        emit settingsChanged();
    } else {
        QMessageBox::warning(this, "Save Failed", "Failed to save calibration settings.");
    }
}

void CalibrationInterface::loadCalibrationSettings()
{
    QFile file(m_settingsFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (doc.isObject()) {
            m_calibrationSettings = doc.object();

            // Apply settings to UI
            m_minPointsSpin->setValue(m_calibrationSettings.value("min_calibration_points").toInt(5));
            m_maxErrorSpin->setValue(m_calibrationSettings.value("max_calibration_error").toDouble(2.0));
            m_calibrationTimeoutSpin->setValue(m_calibrationSettings.value("calibration_timeout_minutes").toInt(5));
            m_autoSaveCheck->setChecked(m_calibrationSettings.value("auto_save_enabled").toBool(true));
            m_autoValidateCheck->setChecked(m_calibrationSettings.value("auto_validate_enabled").toBool(true));

            // Apply settings to calibration manager
            if (m_calibrationManager) {
                m_calibrationManager->setMinCalibrationPoints(m_minPointsSpin->value());
                m_calibrationManager->setMaxCalibrationError(m_maxErrorSpin->value() / 100.0);
                m_calibrationManager->setAutoSaveEnabled(m_autoSaveCheck->isChecked());
            }
        }
    } else {
        // Set default settings
        m_calibrationSettings["min_calibration_points"] = 5;
        m_calibrationSettings["max_calibration_error"] = 2.0;
        m_calibrationSettings["calibration_timeout_minutes"] = 5;
        m_calibrationSettings["auto_save_enabled"] = true;
        m_calibrationSettings["auto_validate_enabled"] = true;
    }
}

// CalibrationManager signal handlers
void CalibrationInterface::onCalibrationStarted(const QString& component, CalibrationManager::CalibrationType type)
{
    m_calibrationInProgress = true;
    m_currentComponent = component;
    m_currentType = type;
    m_calibrationStartTime = QDateTime::currentDateTime();

    m_progressGroup->setVisible(true);
    m_statusUpdateTimer->start();

    enableCalibrationControls(false);
    clearResultsTable();

    m_calibrationLog->append(QString("[%1] Started %2 calibration for %3")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(type == CalibrationManager::SENSOR_CALIBRATION ? "sensor" :
                                 type == CalibrationManager::ACTUATOR_CALIBRATION ? "actuator" : "system")
                            .arg(component));

    emit calibrationStarted(component);
}

void CalibrationInterface::onCalibrationProgress(int percentage, const QString& status)
{
    m_currentProgress = percentage;
    m_currentStatus = status;

    m_calibrationProgress->setValue(percentage);
    m_progressStatusLabel->setText(status);

    m_calibrationLog->append(QString("[%1] %2 (%3%)")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(status).arg(percentage));
}

void CalibrationInterface::onCalibrationPointAdded(const CalibrationManager::CalibrationPoint& point)
{
    addCalibrationPointToTable(point);

    m_calibrationLog->append(QString("[%1] Point added: ref=%2, measured=%3")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(point.referenceValue, 0, 'f', 2)
                            .arg(point.measuredValue, 0, 'f', 2));
}

void CalibrationInterface::onCalibrationCompleted(const CalibrationManager::CalibrationResult& result)
{
    m_calibrationInProgress = false;
    m_statusUpdateTimer->stop();

    m_calibrationProgress->setValue(100);
    m_progressStatusLabel->setText("Calibration Completed Successfully!");

    enableCalibrationControls(true);

    // Show results
    QString resultText = QString("Calibration completed for %1:\n"
                                "Slope: %2\n"
                                "Offset: %3\n"
                                "Correlation (R²): %4\n"
                                "Max Error: %5%")
                        .arg(result.component)
                        .arg(result.slope, 0, 'f', 4)
                        .arg(result.offset, 0, 'f', 4)
                        .arg(result.correlation, 0, 'f', 3)
                        .arg(result.maxError, 0, 'f', 2);

    m_calibrationLog->append(QString("[%1] %2")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(resultText.replace('\n', ' ')));

    QMessageBox::information(this, "Calibration Complete", resultText);

    // Refresh status
    refreshCalibrationStatus();

    emit calibrationCompleted(result.component, true);
}

void CalibrationInterface::onCalibrationFailed(const QString& component, const QString& error)
{
    m_calibrationInProgress = false;
    m_statusUpdateTimer->stop();

    m_progressStatusLabel->setText("Calibration Failed");

    enableCalibrationControls(true);

    m_calibrationLog->append(QString("[%1] Calibration failed for %2: %3")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(component).arg(error));

    showCalibrationError(error);

    emit calibrationCompleted(component, false);
}

void CalibrationInterface::onCalibrationDataSaved(const QString& component)
{
    m_calibrationLog->append(QString("[%1] Calibration data saved for %2")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(component));
}

void CalibrationInterface::onCalibrationValidated(const QString& component, bool valid)
{
    QString status = valid ? "valid" : "invalid";
    m_calibrationLog->append(QString("[%1] Calibration validation for %2: %3")
                            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                            .arg(component).arg(status));

    if (!valid) {
        QMessageBox::warning(this, "Validation Failed",
                           QString("Calibration validation failed for %1").arg(component));
    }
}

// UI interaction slots
void CalibrationInterface::onComponentSelectionChanged()
{
    QString component = m_componentCombo->currentText();

    // Update calibration type combo based on component
    if (component == "AVL Sensor" || component == "Tank Sensor") {
        m_calibrationTypeCombo->setCurrentText("Sensor Calibration");
    } else if (component == "Pump" || component.contains("SOL")) {
        m_calibrationTypeCombo->setCurrentText("Actuator Calibration");
    } else if (component == "System") {
        m_calibrationTypeCombo->setCurrentText("System Calibration");
    }
}

void CalibrationInterface::onCalibrationTypeChanged()
{
    QString type = m_calibrationTypeCombo->currentText();

    if (type == "Sensor Calibration") {
        m_currentType = CalibrationManager::SENSOR_CALIBRATION;
    } else if (type == "Actuator Calibration") {
        m_currentType = CalibrationManager::ACTUATOR_CALIBRATION;
    } else if (type == "System Calibration") {
        m_currentType = CalibrationManager::SYSTEM_CALIBRATION;
    }
}

void CalibrationInterface::onStartCalibrationClicked()
{
    if (m_calibrationInProgress) return;

    QString component = m_componentCombo->currentText();

    // Confirm calibration start
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Start Calibration",
        QString("Start calibration for %1?\n\n"
                "This process may take several minutes.\n"
                "Ensure the system is ready for calibration.").arg(component),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_currentType == CalibrationManager::SENSOR_CALIBRATION) {
            startSensorCalibration();
        } else if (m_currentType == CalibrationManager::ACTUATOR_CALIBRATION) {
            startActuatorCalibration();
        } else if (m_currentType == CalibrationManager::SYSTEM_CALIBRATION) {
            startSystemCalibration();
        }
    }
}

void CalibrationInterface::onCancelCalibrationClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Cancel Calibration",
        "Are you sure you want to cancel the current calibration?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        cancelCurrentCalibration();
    }
}

void CalibrationInterface::onValidateCalibrationClicked()
{
    if (!m_calibrationManager) return;

    QString component = m_componentCombo->currentText();

    if (component == "System") {
        // Validate all components
        QStringList components = {"AVL Sensor", "Tank Sensor", "Pump", "SOL1", "SOL2", "SOL3"};
        for (const QString& comp : components) {
            m_calibrationManager->validateCalibration(comp);
        }
    } else {
        m_calibrationManager->validateCalibration(component);
    }
}

void CalibrationInterface::onExportCalibrationClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Calibration Data",
        QString("calibration_data_%1.json").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        if (exportCalibrationData(fileName)) {
            QMessageBox::information(this, "Export Complete",
                                   QString("Calibration data exported to:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Export Failed", "Failed to export calibration data.");
        }
    }
}

void CalibrationInterface::onImportCalibrationClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Calibration Data",
        "",
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        if (importCalibrationData(fileName)) {
            QMessageBox::information(this, "Import Complete", "Calibration data imported successfully.");
            refreshCalibrationStatus();
        } else {
            QMessageBox::warning(this, "Import Failed", "Failed to import calibration data.");
        }
    }
}

void CalibrationInterface::onResetCalibrationClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset Settings",
        "Reset all calibration settings to default values?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Reset to defaults
        m_minPointsSpin->setValue(5);
        m_maxErrorSpin->setValue(2.0);
        m_calibrationTimeoutSpin->setValue(5);
        m_autoSaveCheck->setChecked(true);
        m_autoValidateCheck->setChecked(true);

        saveCalibrationSettings();
    }
}

void CalibrationInterface::onSettingsChanged()
{
    // Settings have changed - could auto-save or mark as modified
    emit settingsChanged();
}

void CalibrationInterface::onStatusUpdateTimer()
{
    if (m_calibrationInProgress && m_calibrationStartTime.isValid()) {
        qint64 elapsed = m_calibrationStartTime.msecsTo(QDateTime::currentDateTime());
        int seconds = elapsed / 1000;
        int minutes = seconds / 60;
        seconds = seconds % 60;

        m_elapsedTimeLabel->setText(QString("Elapsed: %1:%2")
                                   .arg(minutes, 2, 10, QChar('0'))
                                   .arg(seconds, 2, 10, QChar('0')));
    }
}

// Utility methods
void CalibrationInterface::updateCalibrationStatus()
{
    if (!m_calibrationManager) return;

    // Clear existing rows
    m_statusTable->setRowCount(0);

    // Get calibration status
    QJsonObject status = m_calibrationManager->getCalibrationStatus();
    QJsonArray components = status["components"].toArray();

    // Populate status table
    for (const QJsonValue& componentValue : components) {
        QJsonObject component = componentValue.toObject();

        int row = m_statusTable->rowCount();
        m_statusTable->insertRow(row);

        m_statusTable->setItem(row, 0, new QTableWidgetItem(component["name"].toString()));

        QString statusText = component["calibrated"].toBool() ? "Calibrated" : "Not Calibrated";
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
        statusItem->setForeground(component["calibrated"].toBool() ? QColor("#4CAF50") : QColor("#f44336"));
        m_statusTable->setItem(row, 1, statusItem);

        QString lastCalibration = component["last_calibration"].toString();
        if (!lastCalibration.isEmpty()) {
            QDateTime dateTime = QDateTime::fromString(lastCalibration, Qt::ISODate);
            m_statusTable->setItem(row, 2, new QTableWidgetItem(dateTime.toString("yyyy-MM-dd hh:mm")));
        } else {
            m_statusTable->setItem(row, 2, new QTableWidgetItem("Never"));
        }

        m_statusTable->setItem(row, 3, new QTableWidgetItem(
            QString::number(component["correlation"].toDouble(), 'f', 3)));

        m_statusTable->setItem(row, 4, new QTableWidgetItem(
            QString("%1%").arg(component["max_error"].toDouble(), 0, 'f', 1)));

        QString expiredText = component["expired"].toBool() ? "Yes" : "No";
        QTableWidgetItem* expiredItem = new QTableWidgetItem(expiredText);
        expiredItem->setForeground(component["expired"].toBool() ? QColor("#f44336") : QColor("#4CAF50"));
        m_statusTable->setItem(row, 5, expiredItem);
    }

    // Resize columns to content
    m_statusTable->resizeColumnsToContents();
}

void CalibrationInterface::addCalibrationPointToTable(const CalibrationManager::CalibrationPoint& point)
{
    int row = m_resultsTable->rowCount();
    m_resultsTable->insertRow(row);

    m_resultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(point.referenceValue, 'f', 2)));
    m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(point.measuredValue, 'f', 2)));

    // Calculate error percentage
    double error = 0.0;
    if (std::abs(point.referenceValue) > 1e-10) {
        error = std::abs(point.measuredValue - point.referenceValue) / std::abs(point.referenceValue) * 100.0;
    }

    QTableWidgetItem* errorItem = new QTableWidgetItem(QString("%1%").arg(error, 0, 'f', 1));
    if (error > 5.0) {
        errorItem->setForeground(QColor("#f44336")); // Red for high error
    } else if (error > 2.0) {
        errorItem->setForeground(QColor("#FF9800")); // Orange for medium error
    } else {
        errorItem->setForeground(QColor("#4CAF50")); // Green for low error
    }

    m_resultsTable->setItem(row, 3, errorItem);

    // Scroll to bottom
    m_resultsTable->scrollToBottom();
}

void CalibrationInterface::clearResultsTable()
{
    m_resultsTable->setRowCount(0);
}

void CalibrationInterface::showCalibrationError(const QString& error)
{
    QMessageBox::critical(this, "Calibration Error",
                         QString("Calibration failed:\n\n%1").arg(error));
}

void CalibrationInterface::showCalibrationSuccess(const QString& message)
{
    QMessageBox::information(this, "Calibration Success", message);
}

void CalibrationInterface::enableCalibrationControls(bool enabled)
{
    m_startButton->setEnabled(enabled);
    m_cancelButton->setEnabled(!enabled);
    m_componentCombo->setEnabled(enabled);
    m_calibrationTypeCombo->setEnabled(enabled);
}

bool CalibrationInterface::exportCalibrationData(const QString& filePath)
{
    if (!m_calibrationManager) return false;

    // Get all calibration data
    QJsonObject exportData;
    exportData["export_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    exportData["export_version"] = "1.0";

    QJsonArray calibrations;
    QStringList components = m_calibrationManager->getAvailableCalibrations();

    for (const QString& component : components) {
        CalibrationManager::CalibrationResult result;
        if (m_calibrationManager->loadCalibrationData(component, result)) {
            calibrations.append(result.toJson(false));  // Export without points for summary
        }
    }

    exportData["calibrations"] = calibrations;
    exportData["settings"] = m_calibrationSettings;

    // Save to file
    QJsonDocument doc(exportData);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }

    return false;
}

bool CalibrationInterface::importCalibrationData(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject importData = doc.object();

    // Import calibrations
    QJsonArray calibrations = importData["calibrations"].toArray();
    for (const QJsonValue& calibrationValue : calibrations) {
        CalibrationManager::CalibrationResult result =
            CalibrationManager::CalibrationResult::fromJson(calibrationValue.toObject(), false);

        if (m_calibrationManager) {
            m_calibrationManager->saveCalibrationData(result);
        }
    }

    // Import settings if available
    if (importData.contains("settings")) {
        m_calibrationSettings = importData["settings"].toObject();

        // Apply imported settings to UI
        m_minPointsSpin->setValue(m_calibrationSettings.value("min_calibration_points").toInt(5));
        m_maxErrorSpin->setValue(m_calibrationSettings.value("max_calibration_error").toDouble(2.0));
        m_calibrationTimeoutSpin->setValue(m_calibrationSettings.value("calibration_timeout_minutes").toInt(5));
        m_autoSaveCheck->setChecked(m_calibrationSettings.value("auto_save_enabled").toBool(true));
        m_autoValidateCheck->setChecked(m_calibrationSettings.value("auto_validate_enabled").toBool(true));
    }

    return true;
}
