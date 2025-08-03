#include "CustomPatternDialog.h"
#include "../VacuumController.h"
#include "../patterns/PatternDefinitions.h"
#include "components/TouchButton.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QHeaderView>
#include <QApplication>

CustomPatternDialog::CustomPatternDialog(VacuumController* controller, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_basicInfoTab(nullptr)
    , m_stepEditorTab(nullptr)
    , m_visualDesignerTab(nullptr)
    , m_previewTab(nullptr)
    , m_advancedTab(nullptr)
    , m_patternNameEdit(nullptr)
    , m_patternTypeCombo(nullptr)
    , m_patternDescriptionEdit(nullptr)
    , m_basePressureSpin(nullptr)
    , m_speedSpin(nullptr)
    , m_intensitySpin(nullptr)
    , m_stepsList(nullptr)
    , m_stepPressureSpin(nullptr)
    , m_stepDurationSpin(nullptr)
    , m_stepActionCombo(nullptr)
    , m_stepDescriptionEdit(nullptr)
    , m_addStepButton(nullptr)
    , m_removeStepButton(nullptr)
    , m_moveUpButton(nullptr)
    , m_moveDownButton(nullptr)
    , m_duplicateStepButton(nullptr)
    , m_clearStepsButton(nullptr)
    , m_previewChart(nullptr)
    , m_previewButton(nullptr)
    , m_testButton(nullptr)
    , m_templateCombo(nullptr)
    , m_loadTemplateButton(nullptr)
    , m_exportButton(nullptr)
    , m_importButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
    , m_currentTab(0)
    , m_patternModified(false)
{
    setWindowTitle("Custom Pattern Editor");
    setModal(true);
    resize(1000, 700);
    
    setupUI();
    connectSignals();
    applyTouchOptimizedStyles();
    
    initializeDefaultPattern();
    
    qDebug() << "CustomPatternDialog created";
}

CustomPatternDialog::~CustomPatternDialog()
{
}

void CustomPatternDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(SPACING_NORMAL);
    m_mainLayout->setContentsMargins(SPACING_NORMAL, SPACING_NORMAL, SPACING_NORMAL, SPACING_NORMAL);
    
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    setupBasicInfoTab();
    setupStepEditorTab();
    setupVisualDesignerTab();
    setupPreviewTab();
    setupAdvancedTab();
    
    m_tabWidget->addTab(m_basicInfoTab, "Basic Info");
    m_tabWidget->addTab(m_stepEditorTab, "Step Editor");
    m_tabWidget->addTab(m_visualDesignerTab, "Visual Designer");
    m_tabWidget->addTab(m_previewTab, "Preview");
    m_tabWidget->addTab(m_advancedTab, "Advanced");
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_saveButton = new TouchButton("Save Pattern");
    m_saveButton->setButtonType(TouchButton::Primary);
    m_saveButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_cancelButton = new TouchButton("Cancel");
    m_cancelButton->setButtonType(TouchButton::Normal);
    m_cancelButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addLayout(buttonLayout);
}

void CustomPatternDialog::setupBasicInfoTab()
{
    m_basicInfoTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_basicInfoTab);
    layout->setSpacing(SPACING_NORMAL);
    
    QGroupBox* infoGroup = new QGroupBox("Pattern Information");
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_patternNameEdit = new QLineEdit();
    m_patternNameEdit->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_patternNameEdit->setPlaceholderText("Enter pattern name...");
    infoLayout->addRow("Name:", m_patternNameEdit);
    
    m_patternTypeCombo = new QComboBox();
    m_patternTypeCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_patternTypeCombo->addItems({"Continuous", "Pulsed", "Ramped", "Custom"});
    infoLayout->addRow("Type:", m_patternTypeCombo);
    
    m_patternDescriptionEdit = new QTextEdit();
    m_patternDescriptionEdit->setMaximumHeight(100);
    m_patternDescriptionEdit->setPlaceholderText("Enter pattern description...");
    infoLayout->addRow("Description:", m_patternDescriptionEdit);
    
    QGroupBox* paramGroup = new QGroupBox("Pattern Parameters");
    QFormLayout* paramLayout = new QFormLayout(paramGroup);
    
    m_basePressureSpin = new QDoubleSpinBox();
    m_basePressureSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_basePressureSpin->setRange(MIN_PRESSURE, MAX_PRESSURE);
    m_basePressureSpin->setValue(DEFAULT_PRESSURE);
    m_basePressureSpin->setSuffix(" mmHg");
    m_basePressureSpin->setDecimals(1);
    paramLayout->addRow("Base Pressure:", m_basePressureSpin);
    
    m_speedSpin = new QDoubleSpinBox();
    m_speedSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_speedSpin->setRange(0.1, 5.0);
    m_speedSpin->setValue(1.0);
    m_speedSpin->setSuffix("x");
    m_speedSpin->setDecimals(1);
    m_speedSpin->setSingleStep(0.1);
    paramLayout->addRow("Speed:", m_speedSpin);
    
    m_intensitySpin = new QDoubleSpinBox();
    m_intensitySpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_intensitySpin->setRange(0.0, 100.0);
    m_intensitySpin->setValue(50.0);
    m_intensitySpin->setSuffix("%");
    m_intensitySpin->setDecimals(1);
    paramLayout->addRow("Intensity:", m_intensitySpin);
    
    layout->addWidget(infoGroup);
    layout->addWidget(paramGroup);
    layout->addStretch();
}

void CustomPatternDialog::setupStepEditorTab()
{
    m_stepEditorTab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(m_stepEditorTab);
    layout->setSpacing(SPACING_NORMAL);
    
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    QLabel* stepsLabel = new QLabel("Pattern Steps:");
    stepsLabel->setStyleSheet("font-weight: bold; font-size: 14pt;");
    
    m_stepsList = new QListWidget();
    m_stepsList->setMinimumHeight(300);
    m_stepsList->setAlternatingRowColors(true);
    
    QHBoxLayout* stepButtonLayout = new QHBoxLayout();
    
    m_addStepButton = new TouchButton("Add Step");
    m_addStepButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_removeStepButton = new TouchButton("Remove");
    m_removeStepButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_moveUpButton = new TouchButton("Move Up");
    m_moveUpButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_moveDownButton = new TouchButton("Move Down");
    m_moveDownButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    stepButtonLayout->addWidget(m_addStepButton);
    stepButtonLayout->addWidget(m_removeStepButton);
    stepButtonLayout->addWidget(m_moveUpButton);
    stepButtonLayout->addWidget(m_moveDownButton);
    
    QHBoxLayout* stepButtonLayout2 = new QHBoxLayout();
    
    m_duplicateStepButton = new TouchButton("Duplicate");
    m_duplicateStepButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    m_clearStepsButton = new TouchButton("Clear All");
    m_clearStepsButton->setButtonType(TouchButton::Warning);
    m_clearStepsButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    
    stepButtonLayout2->addWidget(m_duplicateStepButton);
    stepButtonLayout2->addWidget(m_clearStepsButton);
    stepButtonLayout2->addStretch();
    
    leftLayout->addWidget(stepsLabel);
    leftLayout->addWidget(m_stepsList);
    leftLayout->addLayout(stepButtonLayout);
    leftLayout->addLayout(stepButtonLayout2);
    
    QVBoxLayout* rightLayout = new QVBoxLayout();
    
    QGroupBox* stepPropsGroup = new QGroupBox("Step Properties");
    QFormLayout* propsLayout = new QFormLayout(stepPropsGroup);
    
    m_stepPressureSpin = new QDoubleSpinBox();
    m_stepPressureSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_stepPressureSpin->setRange(MIN_PRESSURE, MAX_PRESSURE);
    m_stepPressureSpin->setValue(DEFAULT_PRESSURE);
    m_stepPressureSpin->setSuffix(" mmHg");
    m_stepPressureSpin->setDecimals(1);
    propsLayout->addRow("Pressure:", m_stepPressureSpin);
    
    m_stepDurationSpin = new QSpinBox();
    m_stepDurationSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_stepDurationSpin->setRange(MIN_STEP_DURATION, MAX_STEP_DURATION);
    m_stepDurationSpin->setValue(DEFAULT_STEP_DURATION);
    m_stepDurationSpin->setSuffix(" ms");
    propsLayout->addRow("Duration:", m_stepDurationSpin);
    
    m_stepActionCombo = new QComboBox();
    m_stepActionCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_stepActionCombo->addItems({"Hold", "Ramp", "Pulse", "Release"});
    propsLayout->addRow("Action:", m_stepActionCombo);
    
    m_stepDescriptionEdit = new QLineEdit();
    m_stepDescriptionEdit->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_stepDescriptionEdit->setPlaceholderText("Step description...");
    propsLayout->addRow("Description:", m_stepDescriptionEdit);
    
    rightLayout->addWidget(stepPropsGroup);
    rightLayout->addStretch();
    
    layout->addLayout(leftLayout, 2);
    layout->addLayout(rightLayout, 1);
}

void CustomPatternDialog::setupVisualDesignerTab()
{
    m_visualDesignerTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_visualDesignerTab);

    QLabel* designerLabel = new QLabel("Visual Pattern Designer");
    designerLabel->setAlignment(Qt::AlignCenter);
    designerLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #666; padding: 50px;");

    QLabel* comingSoonLabel = new QLabel("Graphical pattern design interface coming soon...\nUse the Step Editor tab to create patterns.");
    comingSoonLabel->setAlignment(Qt::AlignCenter);
    comingSoonLabel->setStyleSheet("font-size: 12pt; color: #888;");

    layout->addWidget(designerLabel);
    layout->addWidget(comingSoonLabel);
    layout->addStretch();
}

void CustomPatternDialog::setupPreviewTab()
{
    m_previewTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_previewTab);
    layout->setSpacing(SPACING_NORMAL);

    QHBoxLayout* previewControlLayout = new QHBoxLayout();

    m_previewButton = new TouchButton("Update Preview");
    m_previewButton->setButtonType(TouchButton::Primary);
    m_previewButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    m_testButton = new TouchButton("Test Pattern");
    m_testButton->setButtonType(TouchButton::Warning);
    m_testButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    previewControlLayout->addWidget(m_previewButton);
    previewControlLayout->addWidget(m_testButton);
    previewControlLayout->addStretch();

    m_previewChart = new QLabel("Pattern Preview Chart");
    m_previewChart->setAlignment(Qt::AlignCenter);
    m_previewChart->setStyleSheet("border: 2px dashed #ccc; background-color: #f9f9f9; font-size: 14pt; color: #666;");
    m_previewChart->setMinimumHeight(300);

    QGroupBox* summaryGroup = new QGroupBox("Pattern Summary");
    QFormLayout* summaryLayout = new QFormLayout(summaryGroup);

    m_totalDurationLabel = new QLabel("0 ms");
    m_totalStepsLabel = new QLabel("0");
    m_avgPressureLabel = new QLabel("0 mmHg");
    m_maxPressureLabel = new QLabel("0 mmHg");

    summaryLayout->addRow("Total Duration:", m_totalDurationLabel);
    summaryLayout->addRow("Total Steps:", m_totalStepsLabel);
    summaryLayout->addRow("Average Pressure:", m_avgPressureLabel);
    summaryLayout->addRow("Maximum Pressure:", m_maxPressureLabel);

    layout->addLayout(previewControlLayout);
    layout->addWidget(m_previewChart);
    layout->addWidget(summaryGroup);
}

void CustomPatternDialog::setupAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_advancedTab);
    layout->setSpacing(SPACING_NORMAL);

    QGroupBox* templateGroup = new QGroupBox("Template Management");
    QHBoxLayout* templateLayout = new QHBoxLayout(templateGroup);

    QLabel* templateLabel = new QLabel("Template:");
    m_templateCombo = new QComboBox();
    m_templateCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_templateCombo->addItems({"Basic Continuous", "Simple Pulse", "Ramp Up", "Ramp Down", "Complex Pattern"});

    m_loadTemplateButton = new TouchButton("Load Template");
    m_loadTemplateButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    templateLayout->addWidget(templateLabel);
    templateLayout->addWidget(m_templateCombo);
    templateLayout->addWidget(m_loadTemplateButton);
    templateLayout->addStretch();

    QGroupBox* importExportGroup = new QGroupBox("Import/Export");
    QHBoxLayout* importExportLayout = new QHBoxLayout(importExportGroup);

    m_exportButton = new TouchButton("Export Pattern");
    m_exportButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    m_importButton = new TouchButton("Import Pattern");
    m_importButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);

    importExportLayout->addWidget(m_exportButton);
    importExportLayout->addWidget(m_importButton);
    importExportLayout->addStretch();

    QGroupBox* validationGroup = new QGroupBox("Pattern Validation");
    QVBoxLayout* validationLayout = new QVBoxLayout(validationGroup);

    m_validationResults = new QTextEdit();
    m_validationResults->setMaximumHeight(150);
    m_validationResults->setReadOnly(true);
    m_validationResults->setPlaceholderText("Pattern validation results will appear here...");

    TouchButton* validateButton = new TouchButton("Validate Pattern");
    validateButton->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
    connect(validateButton, &TouchButton::clicked, this, &CustomPatternDialog::validatePattern);

    validationLayout->addWidget(m_validationResults);
    validationLayout->addWidget(validateButton);

    QGroupBox* optionsGroup = new QGroupBox("Advanced Options");
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);

    m_loopPatternCheck = new QCheckBox();
    m_loopCountSpin = new QSpinBox();
    m_loopCountSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_loopCountSpin->setRange(1, 100);
    m_loopCountSpin->setValue(1);
    m_loopCountSpin->setEnabled(false);

    m_autoStartCheck = new QCheckBox();
    m_priorityCombo = new QComboBox();
    m_priorityCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_priorityCombo->addItems({"Low", "Normal", "High"});
    m_priorityCombo->setCurrentText("Normal");

    optionsLayout->addRow("Loop Pattern:", m_loopPatternCheck);
    optionsLayout->addRow("Loop Count:", m_loopCountSpin);
    optionsLayout->addRow("Auto Start:", m_autoStartCheck);
    optionsLayout->addRow("Priority:", m_priorityCombo);

    connect(m_loopPatternCheck, &QCheckBox::toggled, m_loopCountSpin, &QSpinBox::setEnabled);

    layout->addWidget(templateGroup);
    layout->addWidget(importExportGroup);
    layout->addWidget(validationGroup);
    layout->addWidget(optionsGroup);
    layout->addStretch();
}

void CustomPatternDialog::connectSignals()
{
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &CustomPatternDialog::onTabChanged);

    connect(m_patternNameEdit, &QLineEdit::textChanged, this, &CustomPatternDialog::onPatternNameChanged);
    connect(m_patternTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomPatternDialog::onPatternTypeChanged);
    connect(m_patternDescriptionEdit, &QTextEdit::textChanged, this, &CustomPatternDialog::onParameterChanged);
    connect(m_basePressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternDialog::onParameterChanged);
    connect(m_speedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternDialog::onParameterChanged);
    connect(m_intensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternDialog::onParameterChanged);

    connect(m_stepsList, &QListWidget::currentRowChanged, this, &CustomPatternDialog::onStepSelectionChanged);
    connect(m_addStepButton, &TouchButton::clicked, this, &CustomPatternDialog::onStepAdded);
    connect(m_removeStepButton, &TouchButton::clicked, this, &CustomPatternDialog::onStepRemoved);
    connect(m_moveUpButton, &TouchButton::clicked, this, &CustomPatternDialog::moveStepUp);
    connect(m_moveDownButton, &TouchButton::clicked, this, &CustomPatternDialog::moveStepDown);
    connect(m_duplicateStepButton, &TouchButton::clicked, this, &CustomPatternDialog::duplicateStep);
    connect(m_clearStepsButton, &TouchButton::clicked, this, &CustomPatternDialog::clearAllSteps);

    connect(m_stepPressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternDialog::onStepModified);
    connect(m_stepDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CustomPatternDialog::onStepModified);
    connect(m_stepActionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomPatternDialog::onStepModified);
    connect(m_stepDescriptionEdit, &QLineEdit::textChanged, this, &CustomPatternDialog::onStepModified);

    connect(m_previewButton, &TouchButton::clicked, this, &CustomPatternDialog::onPreviewClicked);
    connect(m_testButton, &TouchButton::clicked, this, &CustomPatternDialog::onTestClicked);

    connect(m_loadTemplateButton, &TouchButton::clicked, this, &CustomPatternDialog::onLoadTemplateClicked);
    connect(m_exportButton, &TouchButton::clicked, this, &CustomPatternDialog::exportPattern);
    connect(m_importButton, &TouchButton::clicked, this, &CustomPatternDialog::importPattern);

    connect(m_saveButton, &TouchButton::clicked, this, &CustomPatternDialog::onSaveClicked);
    connect(m_cancelButton, &TouchButton::clicked, this, &QDialog::reject);
}

void CustomPatternDialog::applyTouchOptimizedStyles()
{
    setStyleSheet(
        "QGroupBox { font-size: 14pt; font-weight: bold; padding-top: 15px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; }"
        "QLineEdit, QTextEdit { font-size: 12pt; padding: 8px; border: 2px solid #ddd; border-radius: 5px; }"
        "QComboBox { font-size: 12pt; padding: 8px; border: 2px solid #ddd; border-radius: 5px; }"
        "QSpinBox, QDoubleSpinBox { font-size: 12pt; padding: 8px; border: 2px solid #ddd; border-radius: 5px; }"
        "QListWidget { font-size: 11pt; border: 2px solid #ddd; border-radius: 5px; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:selected { background-color: #2196F3; color: white; }"
        "QTabWidget::pane { border: 2px solid #ddd; border-radius: 5px; }"
        "QTabBar::tab { font-size: 12pt; padding: 10px 20px; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #2196F3; color: white; }"
    );
}

void CustomPatternDialog::initializeDefaultPattern()
{
    m_patternNameEdit->setText("New Custom Pattern");
    m_patternTypeCombo->setCurrentText("Custom");
    m_patternDescriptionEdit->setPlainText("Custom pattern created with the pattern editor");

    m_basePressureSpin->setValue(DEFAULT_PRESSURE);
    m_speedSpin->setValue(1.0);
    m_intensitySpin->setValue(50.0);

    addDefaultStep();

    updatePreview();
}

void CustomPatternDialog::addDefaultStep()
{
    PatternStep step;
    step.pressurePercent = DEFAULT_PRESSURE;
    step.durationMs = DEFAULT_STEP_DURATION;
    step.action = "Hold";
    step.description = "Default step";

    m_patternSteps.append(step);
    updateStepList();
}

void CustomPatternDialog::loadPattern(const QString& patternName)
{
    qDebug() << "Loading pattern:" << patternName;

    // Try to load from custom patterns first
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/custom_patterns.json";
    QFile configFile(configPath);

    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        if (doc.isObject()) {
            QJsonObject patternsObj = doc.object();
            if (patternsObj.contains(patternName)) {
                QJsonObject patternData = patternsObj[patternName].toObject();
                setPatternData(patternData);
                m_patternModified = false;
                qDebug() << "Pattern loaded from custom patterns:" << patternName;
                return;
            }
        }
        configFile.close();
    }

    // If not found in custom patterns, try to load from built-in patterns via controller
    if (m_controller) {
        // Get pattern data from the pattern definitions
        auto patternDefinitions = m_controller->getPatternDefinitions();
        if (patternDefinitions && patternDefinitions->hasPattern(patternName)) {
            auto patternInfo = patternDefinitions->getPattern(patternName);

            // Convert PatternInfo to JSON format for the dialog
            QJsonObject patternData;
            patternData["name"] = patternInfo.name;
            patternData["type"] = patternInfo.type;
            patternData["description"] = patternInfo.description;
            patternData["base_pressure"] = patternInfo.basePressure;
            patternData["speed"] = patternInfo.speed;
            patternData["intensity"] = patternInfo.intensity;

            QJsonArray stepsArray;
            for (const auto& step : patternInfo.steps) {
                QJsonObject stepObj;
                stepObj["pressure_percent"] = step.pressurePercent;
                stepObj["duration_ms"] = step.durationMs;
                stepObj["action"] = step.action;
                stepObj["description"] = step.description;
                stepObj["parameters"] = step.parameters;
                stepsArray.append(stepObj);
            }
            patternData["steps"] = stepsArray;

            setPatternData(patternData);
            m_patternModified = false;
            qDebug() << "Pattern loaded from built-in patterns:" << patternName;
            return;
        }
    }

    // Pattern not found
    QMessageBox::warning(this, "Pattern Not Found",
                        QString("Pattern '%1' could not be loaded.\n\n"
                               "The pattern may have been deleted or is not available.").arg(patternName));
}

void CustomPatternDialog::createNewPattern()
{
    qDebug() << "Creating new pattern";

    initializeDefaultPattern();

    m_patternSteps.clear();
    addDefaultStep();

    m_patternModified = true;
}

QJsonObject CustomPatternDialog::getPatternData() const
{
    QJsonObject data;
    data["name"] = m_patternNameEdit->text();
    data["type"] = m_patternTypeCombo->currentText();
    data["description"] = m_patternDescriptionEdit->toPlainText();
    data["base_pressure"] = m_basePressureSpin->value();
    data["speed"] = m_speedSpin->value();
    data["intensity"] = m_intensitySpin->value();

    QJsonArray stepsArray;
    for (const PatternStep& step : m_patternSteps) {
        stepsArray.append(stepToJson(step));
    }
    data["steps"] = stepsArray;

    data["loop_pattern"] = m_loopPatternCheck->isChecked();
    data["loop_count"] = m_loopCountSpin->value();
    data["auto_start"] = m_autoStartCheck->isChecked();
    data["priority"] = m_priorityCombo->currentText();

    return data;
}

void CustomPatternDialog::setPatternData(const QJsonObject& data)
{
    // Set basic pattern information
    m_patternNameEdit->setText(data["name"].toString());

    // Set pattern type if it exists in combo box
    QString patternType = data["type"].toString();
    int typeIndex = m_patternTypeCombo->findText(patternType);
    if (typeIndex >= 0) {
        m_patternTypeCombo->setCurrentIndex(typeIndex);
    }

    m_patternDescriptionEdit->setPlainText(data["description"].toString());
    m_basePressureSpin->setValue(data["base_pressure"].toDouble(50.0));
    m_speedSpin->setValue(data["speed"].toDouble(1.0));
    m_intensitySpin->setValue(data["intensity"].toDouble(50.0));

    // Set advanced options
    m_loopPatternCheck->setChecked(data["loop_pattern"].toBool(false));
    m_loopCountSpin->setValue(data["loop_count"].toInt(1));
    m_autoStartCheck->setChecked(data["auto_start"].toBool(false));

    QString priority = data["priority"].toString();
    int priorityIndex = m_priorityCombo->findText(priority);
    if (priorityIndex >= 0) {
        m_priorityCombo->setCurrentIndex(priorityIndex);
    }

    // Load pattern steps
    m_patternSteps.clear();
    QJsonArray stepsArray = data["steps"].toArray();
    for (const QJsonValue& stepValue : stepsArray) {
        QJsonObject stepObj = stepValue.toObject();
        PatternStep step;
        step.pressurePercent = stepObj["pressure_percent"].toDouble();
        step.durationMs = stepObj["duration_ms"].toInt();
        step.action = stepObj["action"].toString();
        step.description = stepObj["description"].toString();
        step.parameters = stepObj["parameters"].toObject();
        m_patternSteps.append(step);
    }

    // Update UI
    updateStepList();
    updatePreview();

    qDebug() << "Pattern data loaded into dialog:" << data["name"].toString();
}

void CustomPatternDialog::onTabChanged(int index)
{
    m_currentTab = index;

    if (index == 3) { 
        updatePreview();
    }
}

void CustomPatternDialog::onPatternNameChanged()
{
    m_patternModified = true;
}

void CustomPatternDialog::onPatternTypeChanged()
{
    m_patternModified = true;

    QString type = m_patternTypeCombo->currentText();

    if (type == "Continuous") {
        m_stepDurationSpin->setValue(5000);
    } else if (type == "Pulsed") {
        m_stepDurationSpin->setValue(1000);
    }
}

void CustomPatternDialog::onParameterChanged()
{
    m_patternModified = true;
    updatePreview();
}

void CustomPatternDialog::onStepSelectionChanged(int row)
{
    if (row >= 0 && row < m_patternSteps.size()) {
        const PatternStep& step = m_patternSteps[row];

        m_stepPressureSpin->setValue(step.pressurePercent);
        m_stepDurationSpin->setValue(step.durationMs);
        m_stepActionCombo->setCurrentText(step.action);
        m_stepDescriptionEdit->setText(step.description);

        bool hasSelection = (row >= 0);
        m_removeStepButton->setEnabled(hasSelection);
        m_moveUpButton->setEnabled(hasSelection && row > 0);
        m_moveDownButton->setEnabled(hasSelection && row < m_patternSteps.size() - 1);
        m_duplicateStepButton->setEnabled(hasSelection);
    }
}

void CustomPatternDialog::onStepAdded()
{
    addPatternStep();
}

void CustomPatternDialog::onStepRemoved()
{
    removePatternStep();
}

void CustomPatternDialog::onStepModified()
{
    int currentRow = m_stepsList->currentRow();
    if (currentRow >= 0 && currentRow < m_patternSteps.size()) {
        PatternStep& step = m_patternSteps[currentRow];

        step.pressurePercent = m_stepPressureSpin->value();
        step.durationMs = m_stepDurationSpin->value();
        step.action = m_stepActionCombo->currentText();
        step.description = m_stepDescriptionEdit->text();

        updateStepList();

        m_stepsList->setCurrentRow(currentRow);

        m_patternModified = true;
        updatePreview();
    }
}

void CustomPatternDialog::onPreviewClicked()
{
    updatePreview();
}

void CustomPatternDialog::onTestClicked()
{
    if (!validatePatternData()) {
        QMessageBox::warning(this, "Invalid Pattern",
                           "Please fix the pattern validation errors before testing.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Test Pattern",
        "This will run the pattern on the vacuum controller.\n\n"
        "Make sure the system is in a safe state before proceeding.\n\n"
        "Continue with pattern test?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, "Pattern Test",
                                "Pattern testing functionality will be implemented when "
                                "the pattern engine integration is complete.");
    }
}

void CustomPatternDialog::onSaveClicked()
{
    if (!validatePatternData()) {
        QMessageBox::warning(this, "Invalid Pattern",
                           "Please fix the pattern validation errors before saving.");
        return;
    }

    savePattern();
}

void CustomPatternDialog::onLoadTemplateClicked()
{
    QString templateName = m_templateCombo->currentText();
    loadTemplate(templateName);
}

void CustomPatternDialog::addPatternStep()
{
    PatternStep newStep;
    newStep.pressurePercent = m_stepPressureSpin->value();
    newStep.durationMs = m_stepDurationSpin->value();
    newStep.action = m_stepActionCombo->currentText();
    newStep.description = m_stepDescriptionEdit->text();

    int insertIndex = m_stepsList->currentRow() + 1;
    if (insertIndex < 0 || insertIndex > m_patternSteps.size()) {
        insertIndex = m_patternSteps.size();
    }

    m_patternSteps.insert(insertIndex, newStep);
    updateStepList();

    m_stepsList->setCurrentRow(insertIndex);

    m_patternModified = true;
    updatePreview();
}

void CustomPatternDialog::removePatternStep()
{
    int currentRow = m_stepsList->currentRow();
    if (currentRow >= 0 && currentRow < m_patternSteps.size()) {
        m_patternSteps.removeAt(currentRow);
        updateStepList();

        if (currentRow < m_patternSteps.size()) {
            m_stepsList->setCurrentRow(currentRow);
        } else if (currentRow > 0) {
            m_stepsList->setCurrentRow(currentRow - 1);
        }

        m_patternModified = true;
        updatePreview();
    }
}

void CustomPatternDialog::moveStepUp()
{
    int currentRow = m_stepsList->currentRow();
    if (currentRow > 0 && currentRow < m_patternSteps.size()) {
        m_patternSteps.swapItemsAt(currentRow, currentRow - 1);
        updateStepList();
        m_stepsList->setCurrentRow(currentRow - 1);

        m_patternModified = true;
        updatePreview();
    }
}

void CustomPatternDialog::moveStepDown()
{
    int currentRow = m_stepsList->currentRow();
    if (currentRow >= 0 && currentRow < m_patternSteps.size() - 1) {
        m_patternSteps.swapItemsAt(currentRow, currentRow + 1);
        updateStepList();
        m_stepsList->setCurrentRow(currentRow + 1);

        m_patternModified = true;
        updatePreview();
    }
}

void CustomPatternDialog::duplicateStep()
{
    int currentRow = m_stepsList->currentRow();
    if (currentRow >= 0 && currentRow < m_patternSteps.size()) {
        PatternStep duplicatedStep = m_patternSteps[currentRow];
        duplicatedStep.description += " (Copy)";

        m_patternSteps.insert(currentRow + 1, duplicatedStep);
        updateStepList();
        m_stepsList->setCurrentRow(currentRow + 1);

        m_patternModified = true;
        updatePreview();
    }
}

void CustomPatternDialog::clearAllSteps()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Clear All Steps",
        "Are you sure you want to remove all pattern steps?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_patternSteps.clear();
        updateStepList();

        m_patternModified = true;
        updatePreview();
    }
}

void CustomPatternDialog::loadTemplate(const QString& templateName)
{
    qDebug() << "Loading template:" << templateName;

    m_patternSteps.clear();

    if (templateName == "Basic Continuous") {
        PatternStep step;
        step.pressurePercent = 50.0;
        step.durationMs = 10000;
        step.action = "Hold";
        step.description = "Continuous vacuum";
        m_patternSteps.append(step);

    } else if (templateName == "Simple Pulse") {
        PatternStep step1;
        step1.pressurePercent = 70.0;
        step1.durationMs = 2000;
        step1.action = "Hold";
        step1.description = "Vacuum on";
        m_patternSteps.append(step1);

        PatternStep step2;
        step2.pressurePercent = 0.0;
        step2.durationMs = 1000;
        step2.action = "Release";
        step2.description = "Vacuum off";
        m_patternSteps.append(step2);

    } else if (templateName == "Ramp Up") {
        for (int i = 1; i <= 5; ++i) {
            PatternStep step;
            step.pressurePercent = i * 20.0;
            step.durationMs = 2000;
            step.action = "Ramp";
            step.description = QString("Ramp to %1%").arg(i * 20);
            m_patternSteps.append(step);
        }

    } else if (templateName == "Ramp Down") {
        for (int i = 5; i >= 1; --i) {
            PatternStep step;
            step.pressurePercent = i * 20.0;
            step.durationMs = 2000;
            step.action = "Ramp";
            step.description = QString("Ramp to %1%").arg(i * 20);
            m_patternSteps.append(step);
        }

    } else if (templateName == "Complex Pattern") {
        QList<QPair<double, QString>> complexSteps = {
            {30.0, "Initial vacuum"},
            {60.0, "Increase pressure"},
            {40.0, "Reduce pressure"},
            {80.0, "High pressure"},
            {20.0, "Low pressure"},
            {50.0, "Final hold"}
        };

        for (const auto& stepData : complexSteps) {
            PatternStep step;
            step.pressurePercent = stepData.first;
            step.durationMs = 3000;
            step.action = "Ramp";
            step.description = stepData.second;
            m_patternSteps.append(step);
        }
    }

    m_patternNameEdit->setText(templateName + " Pattern");

    updateStepList();
    updatePreview();

    m_patternModified = true;

    QMessageBox::information(this, "Template Loaded",
                           QString("Template '%1' has been loaded successfully.").arg(templateName));
}

void CustomPatternDialog::exportPattern()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Pattern",
        QString("%1.json").arg(m_patternNameEdit->text()),
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        QJsonObject patternData = getPatternData();
        QJsonDocument doc(patternData);

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();

            QMessageBox::information(this, "Export Complete",
                                   QString("Pattern exported to:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Export Failed",
                                QString("Failed to export pattern to:\n%1").arg(fileName));
        }
    }
}

void CustomPatternDialog::importPattern()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Pattern",
        "",
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                QJsonObject patternData = doc.object();

                m_patternNameEdit->setText(patternData["name"].toString());
                m_patternTypeCombo->setCurrentText(patternData["type"].toString());
                m_patternDescriptionEdit->setPlainText(patternData["description"].toString());
                m_basePressureSpin->setValue(patternData["base_pressure"].toDouble());
                m_speedSpin->setValue(patternData["speed"].toDouble());
                m_intensitySpin->setValue(patternData["intensity"].toDouble());

                m_patternSteps.clear();
                QJsonArray stepsArray = patternData["steps"].toArray();
                for (const QJsonValue& stepValue : stepsArray) {
                    m_patternSteps.append(jsonToStep(stepValue.toObject()));
                }

                m_loopPatternCheck->setChecked(patternData["loop_pattern"].toBool());
                m_loopCountSpin->setValue(patternData["loop_count"].toInt());
                m_autoStartCheck->setChecked(patternData["auto_start"].toBool());
                m_priorityCombo->setCurrentText(patternData["priority"].toString());

                updateStepList();
                updatePreview();

                m_patternModified = true;

                QMessageBox::information(this, "Import Complete",
                                       QString("Pattern imported from:\n%1").arg(fileName));
            } else {
                QMessageBox::warning(this, "Import Failed",
                                    "Invalid pattern file format.");
            }
        } else {
            QMessageBox::warning(this, "Import Failed",
                                QString("Failed to read pattern file:\n%1").arg(fileName));
        }
    }
}

bool CustomPatternDialog::validatePatternData()
{
    QStringList errors;

    if (m_patternNameEdit->text().trimmed().isEmpty()) {
        errors.append("Pattern name is required");
    }

    if (m_patternSteps.isEmpty()) {
        errors.append("Pattern must have at least one step");
    }

    for (int i = 0; i < m_patternSteps.size(); ++i) {
        const PatternStep& step = m_patternSteps[i];

        if (step.pressurePercent < MIN_PRESSURE || step.pressurePercent > MAX_PRESSURE) {
            errors.append(QString("Step %1: Pressure out of range (%2-%3 mmHg)")
                         .arg(i + 1).arg(MIN_PRESSURE).arg(MAX_PRESSURE));
        }

        if (step.durationMs < MIN_STEP_DURATION || step.durationMs > MAX_STEP_DURATION) {
            errors.append(QString("Step %1: Duration out of range (%2-%3 ms)")
                         .arg(i + 1).arg(MIN_STEP_DURATION).arg(MAX_STEP_DURATION));
        }
    }

    if (errors.isEmpty()) {
        m_validationResults->setPlainText("✓ Pattern validation passed successfully.");
        m_validationResults->setStyleSheet("color: green;");
        return true;
    } else {
        QString errorText = "✗ Pattern validation failed:\n\n";
        for (const QString& error : errors) {
            errorText += "• " + error + "\n";
        }
        m_validationResults->setPlainText(errorText);
        m_validationResults->setStyleSheet("color: red;");
        return false;
    }
}

void CustomPatternDialog::validatePattern()
{
    validatePatternData();
}

bool CustomPatternDialog::savePattern()
{
    QJsonObject patternData = getPatternData();
    QString patternName = patternData["name"].toString();

    if (m_patternModified) {
        emit patternCreated(patternName, patternData);
    }

    accept();
    return true;
}

void CustomPatternDialog::updatePreview()
{
    int totalDuration = 0;
    double totalPressure = 0.0;
    double maxPressure = 0.0;

    for (const PatternStep& step : m_patternSteps) {
        totalDuration += step.durationMs;
        totalPressure += step.pressurePercent;
        maxPressure = qMax(maxPressure, step.pressurePercent);
    }

    double avgPressure = m_patternSteps.isEmpty() ? 0.0 : totalPressure / m_patternSteps.size();

    m_totalDurationLabel->setText(QString("%1 ms (%2 s)").arg(totalDuration).arg(totalDuration / 1000.0, 0, 'f', 1));
    m_totalStepsLabel->setText(QString::number(m_patternSteps.size()));
    m_avgPressureLabel->setText(QString("%1 mmHg").arg(avgPressure, 0, 'f', 1));
    m_maxPressureLabel->setText(QString("%1 mmHg").arg(maxPressure, 0, 'f', 1));

    QString chartText = QString("Pattern Preview\n\n"
                               "Steps: %1\n"
                               "Duration: %2 s\n"
                               "Avg Pressure: %3 mmHg\n"
                               "Max Pressure: %4 mmHg")
                       .arg(m_patternSteps.size())
                       .arg(totalDuration / 1000.0, 0, 'f', 1)
                       .arg(avgPressure, 0, 'f', 1)
                       .arg(maxPressure, 0, 'f', 1);

    m_previewChart->setText(chartText);
}

void CustomPatternDialog::updateStepList()
{
    m_stepsList->clear();

    for (int i = 0; i < m_patternSteps.size(); ++i) {
        const PatternStep& step = m_patternSteps[i];
        QString stepText = QString("Step %1: %2 mmHg for %3 ms (%4)")
                          .arg(i + 1)
                          .arg(step.pressurePercent, 0, 'f', 1)
                          .arg(step.durationMs)
                          .arg(step.action);

        if (!step.description.isEmpty()) {
            stepText += QString(" - %1").arg(step.description);
        }

        m_stepsList->addItem(stepText);
    }
}

QJsonObject CustomPatternDialog::stepToJson(const PatternStep& step) const
{
    QJsonObject stepObj;
    stepObj["pressure_percent"] = step.pressurePercent;
    stepObj["duration_ms"] = step.durationMs;
    stepObj["action"] = step.action;
    stepObj["description"] = step.description;
    stepObj["parameters"] = step.parameters;

    return stepObj;
}

CustomPatternDialog::PatternStep CustomPatternDialog::jsonToStep(const QJsonObject& json) const
{
    PatternStep step;
    step.pressurePercent = json["pressure_percent"].toDouble();
    step.durationMs = json["duration_ms"].toInt();
    step.action = json["action"].toString();
    step.description = json["description"].toString();
    step.parameters = json["parameters"].toObject();

    return step;
}

void CustomPatternDialog::onOkClicked() {
    savePattern();
}

void CustomPatternDialog::onCancelClicked() {
    reject();
}

void CustomPatternDialog::onApplyClicked() {
    savePattern();
}

void CustomPatternDialog::onResetPattern() {
    initializeDefaultPattern();
}

void CustomPatternDialog::onTemplateSelected() {
    loadTemplate(m_templateCombo->currentText());
}

void CustomPatternDialog::onImportPattern() {
    importPattern();
}

void CustomPatternDialog::onExportPattern() {
    exportPattern();
}

void CustomPatternDialog::previewPattern() {
    updatePreview();
}

void CustomPatternDialog::stopPreview() {
    // stop preview
}

void CustomPatternDialog::testPattern() {
    onTestClicked();
}

void CustomPatternDialog::onStepSelected() {
    onStepSelectionChanged(m_stepsList->currentRow());
}

void CustomPatternDialog::onPreviewTimer() {
    // preview timer
}