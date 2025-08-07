#include "CustomPatternEditor.h"
#include "../VacuumController.h"
#include "../patterns/PatternDefinitions.h"
#include "components/TouchButton.h"
#include "styles/ModernMedicalStyle.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QHeaderView>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <QtGui/QPainterPath>
#include <cmath>
#include <QtCore/qmath.h>

CustomPatternEditor::CustomPatternEditor(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
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
    // Set up as a full-screen widget for embedded use
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(ModernMedicalStyle::scaleValue(800), ModernMedicalStyle::scaleValue(600));
    
    setupUI();
    connectSignals();
    applyTouchOptimizedStyles();
    
    initializeDefaultPattern();
    
    qDebug() << "CustomPatternEditor created";
}

CustomPatternEditor::~CustomPatternEditor()
{
}

void CustomPatternEditor::setupUI()
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
    setupEdgingTab();

    m_tabWidget->addTab(m_basicInfoTab, "Basic Info");
    m_tabWidget->addTab(m_stepEditorTab, "Step Editor");
    m_tabWidget->addTab(m_visualDesignerTab, "Visual Designer");
    m_tabWidget->addTab(m_previewTab, "Preview");
    m_tabWidget->addTab(m_advancedTab, "Advanced");
    m_tabWidget->addTab(m_edgingTab, "Edging Controls");
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    TouchButton* backButton = new TouchButton("← Back to Patterns");
    backButton->setButtonType(TouchButton::Normal);
    backButton->setMinimumSize(ModernMedicalStyle::scaleValue(BUTTON_MIN_WIDTH), ModernMedicalStyle::scaleValue(BUTTON_MIN_HEIGHT));

    m_saveButton = new TouchButton("Save Pattern");
    m_saveButton->setButtonType(TouchButton::Primary);
    m_saveButton->setMinimumSize(ModernMedicalStyle::scaleValue(BUTTON_MIN_WIDTH), ModernMedicalStyle::scaleValue(BUTTON_MIN_HEIGHT));

    TouchButton* resetButton = new TouchButton("Reset");
    resetButton->setButtonType(TouchButton::Warning);
    resetButton->setMinimumSize(ModernMedicalStyle::scaleValue(BUTTON_MIN_WIDTH), ModernMedicalStyle::scaleValue(BUTTON_MIN_HEIGHT));

    buttonLayout->addWidget(backButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(m_saveButton);

    // Connect the new buttons
    connect(backButton, &TouchButton::clicked, this, &CustomPatternEditor::onBackClicked);
    connect(resetButton, &TouchButton::clicked, this, &CustomPatternEditor::onResetClicked);
    
    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addLayout(buttonLayout);
}

void CustomPatternEditor::setupBasicInfoTab()
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

void CustomPatternEditor::setupStepEditorTab()
{
    m_stepEditorTab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(m_stepEditorTab);
    layout->setSpacing(SPACING_NORMAL);
    
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    QLabel* stepsLabel = new QLabel("Pattern Steps:");
    stepsLabel->setStyleSheet(ModernMedicalStyle::getLabelStyle("subtitle"));
    
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

void CustomPatternEditor::setupVisualDesignerTab()
{
    m_visualDesignerTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_visualDesignerTab);

    QLabel* designerLabel = new QLabel("Visual Pattern Designer");
    designerLabel->setAlignment(Qt::AlignCenter);
    designerLabel->setStyleSheet(ModernMedicalStyle::getLabelStyle("display-title"));

    QLabel* comingSoonLabel = new QLabel("Graphical pattern design interface coming soon...\nUse the Step Editor tab to create patterns.");
    comingSoonLabel->setAlignment(Qt::AlignCenter);
    comingSoonLabel->setStyleSheet(ModernMedicalStyle::getLabelStyle("secondary"));

    layout->addWidget(designerLabel);
    layout->addWidget(comingSoonLabel);
    layout->addStretch();
}

void CustomPatternEditor::setupPreviewTab()
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
    m_previewChart->setStyleSheet(
        ModernMedicalStyle::getLabelStyle("secondary") +
        QString("border: 2px dashed %1; background-color: %2; padding: %3px;")
            .arg(ModernMedicalStyle::Colors::BORDER_MEDIUM.name())
            .arg(ModernMedicalStyle::Colors::BACKGROUND_LIGHT.name())
            .arg(ModernMedicalStyle::scaleValue(20))
    );
    m_previewChart->setMinimumHeight(ModernMedicalStyle::scaleValue(300));

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

void CustomPatternEditor::setupAdvancedTab()
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
    connect(validateButton, &TouchButton::clicked, this, &CustomPatternEditor::validatePattern);

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

void CustomPatternEditor::setupEdgingTab()
{
    m_edgingTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_edgingTab);
    layout->setSpacing(SPACING_NORMAL);

    // Create scroll area for edging controls
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(SPACING_NORMAL);

    // Build-up Controls Group
    m_buildupGroup = new QGroupBox("Build-up Phase");
    QGridLayout* buildupLayout = new QGridLayout(m_buildupGroup);

    // Build-up intensity
    buildupLayout->addWidget(new QLabel("Build-up Intensity:"), 0, 0);
    m_buildupIntensitySlider = new QSlider(Qt::Horizontal);
    m_buildupIntensitySlider->setRange(10, 95);
    m_buildupIntensitySlider->setValue(70);
    m_buildupIntensitySlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_buildupIntensitySpin = new QDoubleSpinBox();
    m_buildupIntensitySpin->setRange(10.0, 95.0);
    m_buildupIntensitySpin->setValue(70.0);
    m_buildupIntensitySpin->setSuffix("%");
    m_buildupIntensitySpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    buildupLayout->addWidget(m_buildupIntensitySlider, 0, 1);
    buildupLayout->addWidget(m_buildupIntensitySpin, 0, 2);

    // Build-up duration
    buildupLayout->addWidget(new QLabel("Build-up Duration:"), 1, 0);
    m_buildupDurationSlider = new QSlider(Qt::Horizontal);
    m_buildupDurationSlider->setRange(5000, 60000);
    m_buildupDurationSlider->setValue(15000);
    m_buildupDurationSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_buildupDurationSpin = new QSpinBox();
    m_buildupDurationSpin->setRange(5000, 60000);
    m_buildupDurationSpin->setValue(15000);
    m_buildupDurationSpin->setSuffix(" ms");
    m_buildupDurationSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    buildupLayout->addWidget(m_buildupDurationSlider, 1, 1);
    buildupLayout->addWidget(m_buildupDurationSpin, 1, 2);

    // Build-up curve type
    buildupLayout->addWidget(new QLabel("Build-up Curve:"), 2, 0);
    m_buildupCurveCombo = new QComboBox();
    m_buildupCurveCombo->addItems({"Linear", "Exponential", "Logarithmic", "S-Curve", "Custom"});
    m_buildupCurveCombo->setCurrentText("Exponential");
    m_buildupCurveCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    buildupLayout->addWidget(m_buildupCurveCombo, 2, 1, 1, 2);

    // Gradual buildup option
    m_gradualBuildupCheck = new QCheckBox("Gradual Step-wise Build-up");
    m_gradualBuildupCheck->setChecked(true);
    buildupLayout->addWidget(m_gradualBuildupCheck, 3, 0, 1, 3);

    // Build-up steps
    buildupLayout->addWidget(new QLabel("Build-up Steps:"), 4, 0);
    m_buildupStepsSlider = new QSlider(Qt::Horizontal);
    m_buildupStepsSlider->setRange(3, 20);
    m_buildupStepsSlider->setValue(8);
    m_buildupStepsSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_buildupStepsSpin = new QSpinBox();
    m_buildupStepsSpin->setRange(3, 20);
    m_buildupStepsSpin->setValue(8);
    m_buildupStepsSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    buildupLayout->addWidget(m_buildupStepsSlider, 4, 1);
    buildupLayout->addWidget(m_buildupStepsSpin, 4, 2);

    scrollLayout->addWidget(m_buildupGroup);

    // Peak/Hold Controls Group
    m_peakGroup = new QGroupBox("Peak/Hold Phase");
    QGridLayout* peakLayout = new QGridLayout(m_peakGroup);

    // Peak intensity
    peakLayout->addWidget(new QLabel("Peak Intensity:"), 0, 0);
    m_peakIntensitySlider = new QSlider(Qt::Horizontal);
    m_peakIntensitySlider->setRange(70, 100);
    m_peakIntensitySlider->setValue(85);
    m_peakIntensitySlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_peakIntensitySpin = new QDoubleSpinBox();
    m_peakIntensitySpin->setRange(70.0, 100.0);
    m_peakIntensitySpin->setValue(85.0);
    m_peakIntensitySpin->setSuffix("%");
    m_peakIntensitySpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    peakLayout->addWidget(m_peakIntensitySlider, 0, 1);
    peakLayout->addWidget(m_peakIntensitySpin, 0, 2);

    // Hold duration
    peakLayout->addWidget(new QLabel("Hold Duration:"), 1, 0);
    m_holdDurationSlider = new QSlider(Qt::Horizontal);
    m_holdDurationSlider->setRange(1000, 10000);
    m_holdDurationSlider->setValue(3000);
    m_holdDurationSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_holdDurationSpin = new QSpinBox();
    m_holdDurationSpin->setRange(1000, 10000);
    m_holdDurationSpin->setValue(3000);
    m_holdDurationSpin->setSuffix(" ms");
    m_holdDurationSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    peakLayout->addWidget(m_holdDurationSlider, 1, 1);
    peakLayout->addWidget(m_holdDurationSpin, 1, 2);

    // Variable peak option
    m_variablePeakCheck = new QCheckBox("Variable Peak Intensity");
    m_variablePeakCheck->setChecked(false);
    peakLayout->addWidget(m_variablePeakCheck, 2, 0, 1, 3);

    // Peak variation
    peakLayout->addWidget(new QLabel("Peak Variation:"), 3, 0);
    m_peakVariationSlider = new QSlider(Qt::Horizontal);
    m_peakVariationSlider->setRange(0, 20);
    m_peakVariationSlider->setValue(5);
    m_peakVariationSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_peakVariationSlider->setEnabled(false);
    m_peakVariationSpin = new QDoubleSpinBox();
    m_peakVariationSpin->setRange(0.0, 20.0);
    m_peakVariationSpin->setValue(5.0);
    m_peakVariationSpin->setSuffix("%");
    m_peakVariationSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_peakVariationSpin->setEnabled(false);
    peakLayout->addWidget(m_peakVariationSlider, 3, 1);
    peakLayout->addWidget(m_peakVariationSpin, 3, 2);

    scrollLayout->addWidget(m_peakGroup);

    // Cooldown/Release Controls Group
    m_cooldownGroup = new QGroupBox("Cooldown/Release Phase");
    QGridLayout* cooldownLayout = new QGridLayout(m_cooldownGroup);

    // Cooldown duration
    cooldownLayout->addWidget(new QLabel("Cooldown Duration:"), 0, 0);
    m_cooldownDurationSlider = new QSlider(Qt::Horizontal);
    m_cooldownDurationSlider->setRange(2000, 15000);
    m_cooldownDurationSlider->setValue(5000);
    m_cooldownDurationSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_cooldownDurationSpin = new QSpinBox();
    m_cooldownDurationSpin->setRange(2000, 15000);
    m_cooldownDurationSpin->setValue(5000);
    m_cooldownDurationSpin->setSuffix(" ms");
    m_cooldownDurationSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    cooldownLayout->addWidget(m_cooldownDurationSlider, 0, 1);
    cooldownLayout->addWidget(m_cooldownDurationSpin, 0, 2);

    // Cooldown curve type
    cooldownLayout->addWidget(new QLabel("Cooldown Curve:"), 1, 0);
    m_cooldownCurveCombo = new QComboBox();
    m_cooldownCurveCombo->addItems({"Linear", "Exponential", "Logarithmic", "Immediate"});
    m_cooldownCurveCombo->setCurrentText("Exponential");
    m_cooldownCurveCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    cooldownLayout->addWidget(m_cooldownCurveCombo, 1, 1, 1, 2);

    // Complete cooldown option
    m_completeCooldownCheck = new QCheckBox("Complete Release to Zero");
    m_completeCooldownCheck->setChecked(true);
    cooldownLayout->addWidget(m_completeCooldownCheck, 2, 0, 1, 3);

    // Cooldown minimum intensity
    cooldownLayout->addWidget(new QLabel("Cooldown Min Intensity:"), 3, 0);
    m_cooldownIntensitySlider = new QSlider(Qt::Horizontal);
    m_cooldownIntensitySlider->setRange(0, 30);
    m_cooldownIntensitySlider->setValue(10);
    m_cooldownIntensitySlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_cooldownIntensitySlider->setEnabled(false);
    m_cooldownIntensitySpin = new QDoubleSpinBox();
    m_cooldownIntensitySpin->setRange(0.0, 30.0);
    m_cooldownIntensitySpin->setValue(10.0);
    m_cooldownIntensitySpin->setSuffix("%");
    m_cooldownIntensitySpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_cooldownIntensitySpin->setEnabled(false);
    cooldownLayout->addWidget(m_cooldownIntensitySlider, 3, 1);
    cooldownLayout->addWidget(m_cooldownIntensitySpin, 3, 2);

    scrollLayout->addWidget(m_cooldownGroup);

    // Cycle Controls Group
    m_cycleGroup = new QGroupBox("Cycle Configuration");
    QGridLayout* cycleLayout = new QGridLayout(m_cycleGroup);

    // Number of edge cycles
    cycleLayout->addWidget(new QLabel("Edge Cycles:"), 0, 0);
    m_edgeCyclesSpin = new QSpinBox();
    m_edgeCyclesSpin->setRange(1, 20);
    m_edgeCyclesSpin->setValue(3);
    m_edgeCyclesSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    cycleLayout->addWidget(m_edgeCyclesSpin, 0, 1);

    // Infinite cycles option
    m_infiniteCyclesCheck = new QCheckBox("Infinite Cycles (Manual Stop)");
    m_infiniteCyclesCheck->setChecked(false);
    cycleLayout->addWidget(m_infiniteCyclesCheck, 0, 2);

    // Cycle delay
    cycleLayout->addWidget(new QLabel("Delay Between Cycles:"), 1, 0);
    m_cycleDelaySlider = new QSlider(Qt::Horizontal);
    m_cycleDelaySlider->setRange(1000, 30000);
    m_cycleDelaySlider->setValue(5000);
    m_cycleDelaySlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_cycleDelaySpin = new QSpinBox();
    m_cycleDelaySpin->setRange(1000, 30000);
    m_cycleDelaySpin->setValue(5000);
    m_cycleDelaySpin->setSuffix(" ms");
    m_cycleDelaySpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    cycleLayout->addWidget(m_cycleDelaySlider, 1, 1);
    cycleLayout->addWidget(m_cycleDelaySpin, 1, 2);

    // Increasing intensity option
    m_increasingIntensityCheck = new QCheckBox("Increasing Intensity Each Cycle");
    m_increasingIntensityCheck->setChecked(false);
    cycleLayout->addWidget(m_increasingIntensityCheck, 2, 0, 1, 3);

    // Intensity increment
    cycleLayout->addWidget(new QLabel("Intensity Increment:"), 3, 0);
    m_intensityIncrementSlider = new QSlider(Qt::Horizontal);
    m_intensityIncrementSlider->setRange(1, 10);
    m_intensityIncrementSlider->setValue(3);
    m_intensityIncrementSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_intensityIncrementSlider->setEnabled(false);
    m_intensityIncrementSpin = new QDoubleSpinBox();
    m_intensityIncrementSpin->setRange(1.0, 10.0);
    m_intensityIncrementSpin->setValue(3.0);
    m_intensityIncrementSpin->setSuffix("%");
    m_intensityIncrementSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_intensityIncrementSpin->setEnabled(false);
    cycleLayout->addWidget(m_intensityIncrementSlider, 3, 1);
    cycleLayout->addWidget(m_intensityIncrementSpin, 3, 2);

    scrollLayout->addWidget(m_cycleGroup);

    // Sensitivity/Auto-detection Controls Group
    m_sensitivityGroup = new QGroupBox("Sensitivity & Auto-Detection");
    QGridLayout* sensitivityLayout = new QGridLayout(m_sensitivityGroup);

    // Auto edge detection
    m_autoEdgeDetectionCheck = new QCheckBox("Enable Automatic Edge Detection");
    m_autoEdgeDetectionCheck->setChecked(false);
    sensitivityLayout->addWidget(m_autoEdgeDetectionCheck, 0, 0, 1, 3);

    // Sensitivity threshold
    sensitivityLayout->addWidget(new QLabel("Sensitivity Threshold:"), 1, 0);
    m_sensitivityThresholdSlider = new QSlider(Qt::Horizontal);
    m_sensitivityThresholdSlider->setRange(60, 95);
    m_sensitivityThresholdSlider->setValue(80);
    m_sensitivityThresholdSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_sensitivityThresholdSlider->setEnabled(false);
    m_sensitivityThresholdSpin = new QDoubleSpinBox();
    m_sensitivityThresholdSpin->setRange(60.0, 95.0);
    m_sensitivityThresholdSpin->setValue(80.0);
    m_sensitivityThresholdSpin->setSuffix("%");
    m_sensitivityThresholdSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_sensitivityThresholdSpin->setEnabled(false);
    sensitivityLayout->addWidget(m_sensitivityThresholdSlider, 1, 1);
    sensitivityLayout->addWidget(m_sensitivityThresholdSpin, 1, 2);

    // Detection window
    sensitivityLayout->addWidget(new QLabel("Detection Window:"), 2, 0);
    m_detectionWindowSlider = new QSlider(Qt::Horizontal);
    m_detectionWindowSlider->setRange(500, 5000);
    m_detectionWindowSlider->setValue(2000);
    m_detectionWindowSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_detectionWindowSlider->setEnabled(false);
    m_detectionWindowSpin = new QSpinBox();
    m_detectionWindowSpin->setRange(500, 5000);
    m_detectionWindowSpin->setValue(2000);
    m_detectionWindowSpin->setSuffix(" ms");
    m_detectionWindowSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_detectionWindowSpin->setEnabled(false);
    sensitivityLayout->addWidget(m_detectionWindowSlider, 2, 1);
    sensitivityLayout->addWidget(m_detectionWindowSpin, 2, 2);

    // Adaptive sensitivity
    m_adaptiveSensitivityCheck = new QCheckBox("Adaptive Sensitivity Learning");
    m_adaptiveSensitivityCheck->setChecked(false);
    m_adaptiveSensitivityCheck->setEnabled(false);
    sensitivityLayout->addWidget(m_adaptiveSensitivityCheck, 3, 0, 1, 3);

    // Response time
    sensitivityLayout->addWidget(new QLabel("Response Time:"), 4, 0);
    m_responseTimeSlider = new QSlider(Qt::Horizontal);
    m_responseTimeSlider->setRange(100, 2000);
    m_responseTimeSlider->setValue(500);
    m_responseTimeSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_responseTimeSlider->setEnabled(false);
    m_responseTimeSpin = new QSpinBox();
    m_responseTimeSpin->setRange(100, 2000);
    m_responseTimeSpin->setValue(500);
    m_responseTimeSpin->setSuffix(" ms");
    m_responseTimeSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_responseTimeSpin->setEnabled(false);
    sensitivityLayout->addWidget(m_responseTimeSlider, 4, 1);
    sensitivityLayout->addWidget(m_responseTimeSpin, 4, 2);

    scrollLayout->addWidget(m_sensitivityGroup);

    // Intensity Curve Controls Group
    m_intensityCurveGroup = new QGroupBox("Intensity Curve Configuration");
    QGridLayout* curveLayout = new QGridLayout(m_intensityCurveGroup);

    // Curve type
    curveLayout->addWidget(new QLabel("Curve Type:"), 0, 0);
    m_intensityCurveTypeCombo = new QComboBox();
    m_intensityCurveTypeCombo->addItems({"Linear", "Exponential", "Logarithmic", "S-Curve", "Sine Wave", "Custom"});
    m_intensityCurveTypeCombo->setCurrentText("Exponential");
    m_intensityCurveTypeCombo->setMinimumHeight(BUTTON_MIN_HEIGHT);
    curveLayout->addWidget(m_intensityCurveTypeCombo, 0, 1, 1, 2);

    // Curve exponent
    curveLayout->addWidget(new QLabel("Curve Exponent:"), 1, 0);
    m_curveExponentSlider = new QSlider(Qt::Horizontal);
    m_curveExponentSlider->setRange(50, 300);
    m_curveExponentSlider->setValue(150);
    m_curveExponentSlider->setMinimumHeight(BUTTON_MIN_HEIGHT);
    m_curveExponentSpin = new QDoubleSpinBox();
    m_curveExponentSpin->setRange(0.5, 3.0);
    m_curveExponentSpin->setValue(1.5);
    m_curveExponentSpin->setDecimals(2);
    m_curveExponentSpin->setMinimumHeight(BUTTON_MIN_HEIGHT);
    curveLayout->addWidget(m_curveExponentSlider, 1, 1);
    curveLayout->addWidget(m_curveExponentSpin, 1, 2);

    // Custom curve option
    m_customCurveCheck = new QCheckBox("Custom Curve Editor");
    m_customCurveCheck->setChecked(false);
    curveLayout->addWidget(m_customCurveCheck, 2, 0, 1, 3);

    // Curve preview
    m_curvePreviewView = new QGraphicsView();
    m_curvePreviewView->setMinimumHeight(150);
    m_curvePreviewView->setMaximumHeight(200);
    m_curvePreviewScene = new QGraphicsScene();
    m_curvePreviewView->setScene(m_curvePreviewScene);
    curveLayout->addWidget(m_curvePreviewView, 3, 0, 1, 3);

    // Curve control buttons
    QHBoxLayout* curveButtonLayout = new QHBoxLayout();
    m_resetCurveButton = new TouchButton("Reset Curve");
    m_previewCurveButton = new TouchButton("Preview Curve");
    curveButtonLayout->addWidget(m_resetCurveButton);
    curveButtonLayout->addWidget(m_previewCurveButton);
    curveButtonLayout->addStretch();
    curveLayout->addLayout(curveButtonLayout, 4, 0, 1, 3);

    scrollLayout->addWidget(m_intensityCurveGroup);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    layout->addWidget(scrollArea);
}

void CustomPatternEditor::connectSignals()
{
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &CustomPatternEditor::onTabChanged);

    connect(m_patternNameEdit, &QLineEdit::textChanged, this, &CustomPatternEditor::onPatternNameChanged);
    connect(m_patternTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomPatternEditor::onPatternTypeChanged);
    connect(m_patternDescriptionEdit, &QTextEdit::textChanged, this, &CustomPatternEditor::onParameterChanged);
    connect(m_basePressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternEditor::onParameterChanged);
    connect(m_speedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternEditor::onParameterChanged);
    connect(m_intensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternEditor::onParameterChanged);

    connect(m_stepsList, &QListWidget::currentRowChanged, this, &CustomPatternEditor::onStepSelectionChanged);
    connect(m_addStepButton, &TouchButton::clicked, this, &CustomPatternEditor::onStepAdded);
    connect(m_removeStepButton, &TouchButton::clicked, this, &CustomPatternEditor::onStepRemoved);
    connect(m_moveUpButton, &TouchButton::clicked, this, &CustomPatternEditor::moveStepUp);
    connect(m_moveDownButton, &TouchButton::clicked, this, &CustomPatternEditor::moveStepDown);
    connect(m_duplicateStepButton, &TouchButton::clicked, this, &CustomPatternEditor::duplicateStep);
    connect(m_clearStepsButton, &TouchButton::clicked, this, &CustomPatternEditor::clearAllSteps);

    connect(m_stepPressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CustomPatternEditor::onStepModified);
    connect(m_stepDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CustomPatternEditor::onStepModified);
    connect(m_stepActionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomPatternEditor::onStepModified);
    connect(m_stepDescriptionEdit, &QLineEdit::textChanged, this, &CustomPatternEditor::onStepModified);

    connect(m_previewButton, &TouchButton::clicked, this, &CustomPatternEditor::onPreviewClicked);
    connect(m_testButton, &TouchButton::clicked, this, &CustomPatternEditor::onTestClicked);

    connect(m_loadTemplateButton, &TouchButton::clicked, this, &CustomPatternEditor::onLoadTemplateClicked);
    connect(m_exportButton, &TouchButton::clicked, this, &CustomPatternEditor::exportPattern);
    connect(m_importButton, &TouchButton::clicked, this, &CustomPatternEditor::importPattern);

    connect(m_saveButton, &TouchButton::clicked, this, &CustomPatternEditor::onSaveClicked);

    // Connect edging control signals
    if (m_buildupIntensitySlider) {
        connect(m_buildupIntensitySlider, &QSlider::valueChanged, this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_buildupIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_buildupDurationSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_buildupDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_buildupCurveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_gradualBuildupCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_buildupStepsSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onBuildupParameterChanged);
        connect(m_buildupStepsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onBuildupParameterChanged);

        connect(m_peakIntensitySlider, &QSlider::valueChanged, this, &CustomPatternEditor::onPeakParameterChanged);
        connect(m_peakIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onPeakParameterChanged);
        connect(m_holdDurationSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onPeakParameterChanged);
        connect(m_holdDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onPeakParameterChanged);
        connect(m_variablePeakCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onPeakParameterChanged);
        connect(m_peakVariationSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onPeakParameterChanged);
        connect(m_peakVariationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onPeakParameterChanged);

        connect(m_cooldownDurationSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onCooldownParameterChanged);
        connect(m_cooldownDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onCooldownParameterChanged);
        connect(m_cooldownCurveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CustomPatternEditor::onCooldownParameterChanged);
        connect(m_completeCooldownCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onCooldownParameterChanged);
        connect(m_cooldownIntensitySlider, &QSlider::valueChanged, this, &CustomPatternEditor::onCooldownParameterChanged);
        connect(m_cooldownIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onCooldownParameterChanged);

        connect(m_edgeCyclesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onCycleParameterChanged);
        connect(m_infiniteCyclesCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onCycleParameterChanged);
        connect(m_cycleDelaySlider, &QSlider::valueChanged, this, &CustomPatternEditor::onCycleParameterChanged);
        connect(m_cycleDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onCycleParameterChanged);
        connect(m_increasingIntensityCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onCycleParameterChanged);
        connect(m_intensityIncrementSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onCycleParameterChanged);
        connect(m_intensityIncrementSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onCycleParameterChanged);

        connect(m_autoEdgeDetectionCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onAutoDetectionToggled);
        connect(m_sensitivityThresholdSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onSensitivityParameterChanged);
        connect(m_sensitivityThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onSensitivityParameterChanged);
        connect(m_detectionWindowSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onSensitivityParameterChanged);
        connect(m_detectionWindowSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onSensitivityParameterChanged);
        connect(m_adaptiveSensitivityCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onSensitivityParameterChanged);
        connect(m_responseTimeSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onSensitivityParameterChanged);
        connect(m_responseTimeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomPatternEditor::onSensitivityParameterChanged);

        connect(m_intensityCurveTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CustomPatternEditor::onIntensityCurveChanged);
        connect(m_curveExponentSlider, &QSlider::valueChanged, this, &CustomPatternEditor::onIntensityCurveChanged);
        connect(m_curveExponentSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CustomPatternEditor::onIntensityCurveChanged);
        connect(m_customCurveCheck, &QCheckBox::toggled, this, &CustomPatternEditor::onIntensityCurveChanged);
        connect(m_resetCurveButton, &TouchButton::clicked, this, &CustomPatternEditor::onResetCurveClicked);
        connect(m_previewCurveButton, &TouchButton::clicked, this, &CustomPatternEditor::onCurvePreviewClicked);

        // Connect slider-spinbox synchronization
        connect(m_buildupIntensitySlider, &QSlider::valueChanged, [this](int value) {
            m_buildupIntensitySpin->setValue(value);
        });
        connect(m_buildupIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
            m_buildupIntensitySlider->setValue(static_cast<int>(value));
        });

        // Add similar connections for other slider-spinbox pairs
        connect(m_buildupDurationSlider, &QSlider::valueChanged, [this](int value) {
            m_buildupDurationSpin->setValue(value);
        });
        connect(m_buildupDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
            m_buildupDurationSlider->setValue(value);
        });

        connect(m_buildupStepsSlider, &QSlider::valueChanged, [this](int value) {
            m_buildupStepsSpin->setValue(value);
        });
        connect(m_buildupStepsSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
            m_buildupStepsSlider->setValue(value);
        });

        // Enable/disable controls based on checkboxes
        connect(m_variablePeakCheck, &QCheckBox::toggled, [this](bool enabled) {
            m_peakVariationSlider->setEnabled(enabled);
            m_peakVariationSpin->setEnabled(enabled);
        });

        connect(m_completeCooldownCheck, &QCheckBox::toggled, [this](bool enabled) {
            m_cooldownIntensitySlider->setEnabled(!enabled);
            m_cooldownIntensitySpin->setEnabled(!enabled);
        });

        connect(m_infiniteCyclesCheck, &QCheckBox::toggled, [this](bool enabled) {
            m_edgeCyclesSpin->setEnabled(!enabled);
        });

        connect(m_increasingIntensityCheck, &QCheckBox::toggled, [this](bool enabled) {
            m_intensityIncrementSlider->setEnabled(enabled);
            m_intensityIncrementSpin->setEnabled(enabled);
        });

        connect(m_gradualBuildupCheck, &QCheckBox::toggled, [this](bool enabled) {
            m_buildupStepsSlider->setEnabled(enabled);
            m_buildupStepsSpin->setEnabled(enabled);
        });
    }
}

void CustomPatternEditor::applyTouchOptimizedStyles()
{
    // Use ModernMedicalStyle system instead of hardcoded CSS
    setStyleSheet(
        ModernMedicalStyle::getGroupBoxStyle() +
        ModernMedicalStyle::getInputFieldStyle() +
        ModernMedicalStyle::getListWidgetStyle() +
        ModernMedicalStyle::getTabWidgetStyle()
    );
}

void CustomPatternEditor::initializeDefaultPattern()
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

void CustomPatternEditor::addDefaultStep()
{
    PatternStep step;
    step.pressurePercent = DEFAULT_PRESSURE;
    step.durationMs = DEFAULT_STEP_DURATION;
    step.action = "Hold";
    step.description = "Default step";

    m_patternSteps.append(step);
    updateStepList();
}

void CustomPatternEditor::loadPattern(const QString& patternName)
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

void CustomPatternEditor::createNewPattern()
{
    qDebug() << "Creating new pattern";

    initializeDefaultPattern();

    m_patternSteps.clear();
    addDefaultStep();

    m_patternModified = true;
}

QJsonObject CustomPatternEditor::getPatternData() const
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

    // Add edging parameters if this is an edging pattern
    QString patternType = m_patternTypeCombo->currentText();
    if (patternType == "Edging" || patternType == "Custom") {
        data["edging_parameters"] = getEdgingParameters();
    }

    return data;
}

void CustomPatternEditor::setPatternData(const QJsonObject& data)
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

    // Load edging parameters if available
    if (data.contains("edging_parameters")) {
        setEdgingParameters(data["edging_parameters"].toObject());
    }

    // Update UI
    updateStepList();
    updatePreview();
    updateEdgingControls();

    qDebug() << "Pattern data loaded into dialog:" << data["name"].toString();
}

void CustomPatternEditor::onTabChanged(int index)
{
    m_currentTab = index;

    if (index == 3) { 
        updatePreview();
    }
}

void CustomPatternEditor::onPatternNameChanged()
{
    m_patternModified = true;
}

void CustomPatternEditor::onPatternTypeChanged()
{
    m_patternModified = true;

    QString type = m_patternTypeCombo->currentText();

    if (type == "Continuous") {
        m_stepDurationSpin->setValue(5000);
    } else if (type == "Pulsed") {
        m_stepDurationSpin->setValue(1000);
    } else if (type == "Edging" || type == "Custom") {
        // Enable edging controls and generate default edging pattern
        updateEdgingControls();
        if (type == "Edging") {
            generateEdgingSteps();
        }
    }

    // Update edging tab availability
    updateEdgingControls();
}

void CustomPatternEditor::onParameterChanged()
{
    m_patternModified = true;
    updatePreview();
}

void CustomPatternEditor::onStepSelectionChanged(int row)
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

void CustomPatternEditor::onStepAdded()
{
    addPatternStep();
}

void CustomPatternEditor::onStepRemoved()
{
    removePatternStep();
}

void CustomPatternEditor::onStepModified()
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

void CustomPatternEditor::onPreviewClicked()
{
    updatePreview();
}

void CustomPatternEditor::onTestClicked()
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

void CustomPatternEditor::onSaveClicked()
{
    if (!validatePatternData()) {
        QMessageBox::warning(this, "Invalid Pattern",
                           "Please fix the pattern validation errors before saving.");
        return;
    }

    if (savePattern()) {
        emit backToPatternSelector();
    }
}

void CustomPatternEditor::onLoadTemplateClicked()
{
    QString templateName = m_templateCombo->currentText();
    loadTemplate(templateName);
}

void CustomPatternEditor::addPatternStep()
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

void CustomPatternEditor::removePatternStep()
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

void CustomPatternEditor::moveStepUp()
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

void CustomPatternEditor::moveStepDown()
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

void CustomPatternEditor::duplicateStep()
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

void CustomPatternEditor::clearAllSteps()
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

void CustomPatternEditor::loadTemplate(const QString& templateName)
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

void CustomPatternEditor::exportPattern()
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

void CustomPatternEditor::importPattern()
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

bool CustomPatternEditor::validatePatternData()
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
        m_validationResults->setStyleSheet(QString("color: %1;").arg(ModernMedicalStyle::Colors::MEDICAL_GREEN.name()));
        return true;
    } else {
        QString errorText = "✗ Pattern validation failed:\n\n";
        for (const QString& error : errors) {
            errorText += "• " + error + "\n";
        }
        m_validationResults->setPlainText(errorText);
        m_validationResults->setStyleSheet(QString("color: %1;").arg(ModernMedicalStyle::Colors::MEDICAL_RED.name()));
        return false;
    }
}

void CustomPatternEditor::validatePattern()
{
    validatePatternData();
}

bool CustomPatternEditor::savePattern()
{
    QJsonObject patternData = getPatternData();
    QString patternName = patternData["name"].toString();

    if (m_patternModified) {
        emit patternCreated(patternName, patternData);
    }

    return true;
}

void CustomPatternEditor::updatePreview()
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

void CustomPatternEditor::updateStepList()
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

QJsonObject CustomPatternEditor::stepToJson(const PatternStep& step) const
{
    QJsonObject stepObj;
    stepObj["pressure_percent"] = step.pressurePercent;
    stepObj["duration_ms"] = step.durationMs;
    stepObj["action"] = step.action;
    stepObj["description"] = step.description;
    stepObj["parameters"] = step.parameters;

    return stepObj;
}

CustomPatternEditor::PatternStep CustomPatternEditor::jsonToStep(const QJsonObject& json) const
{
    PatternStep step;
    step.pressurePercent = json["pressure_percent"].toDouble();
    step.durationMs = json["duration_ms"].toInt();
    step.action = json["action"].toString();
    step.description = json["description"].toString();
    step.parameters = json["parameters"].toObject();

    return step;
}



void CustomPatternEditor::onBackClicked() {
    emit backToPatternSelector();
}

void CustomPatternEditor::onResetClicked() {
    resetEditor();
}

void CustomPatternEditor::onResetPattern() {
    initializeDefaultPattern();
}

void CustomPatternEditor::onTemplateSelected() {
    loadTemplate(m_templateCombo->currentText());
}

void CustomPatternEditor::onImportPattern() {
    importPattern();
}

void CustomPatternEditor::onExportPattern() {
    exportPattern();
}

void CustomPatternEditor::previewPattern() {
    updatePreview();
}

void CustomPatternEditor::stopPreview() {
    // stop preview
}

void CustomPatternEditor::testPattern() {
    onTestClicked();
}

void CustomPatternEditor::showEditor() {
    show();
    resetEditor();
}

void CustomPatternEditor::hideEditor() {
    hide();
    emit editorClosed();
}

void CustomPatternEditor::resetEditor() {
    initializeDefaultPattern();
}

void CustomPatternEditor::onStepSelected() {
    onStepSelectionChanged(m_stepsList->currentRow());
}

void CustomPatternEditor::onPreviewTimer() {
    // preview timer
}

// Edging pattern control slot implementations
void CustomPatternEditor::onEdgingParameterChanged()
{
    m_patternModified = true;
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onBuildupParameterChanged()
{
    m_patternModified = true;
    syncEdgingSliders();
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onPeakParameterChanged()
{
    m_patternModified = true;
    syncEdgingSliders();
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onCooldownParameterChanged()
{
    m_patternModified = true;
    syncEdgingSliders();
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onCycleParameterChanged()
{
    m_patternModified = true;
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onSensitivityParameterChanged()
{
    m_patternModified = true;
    validateEdgingParameters();
}

void CustomPatternEditor::onIntensityCurveChanged()
{
    m_patternModified = true;
    previewIntensityCurve();
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onCurvePreviewClicked()
{
    previewIntensityCurve();
}

void CustomPatternEditor::onResetCurveClicked()
{
    resetEdgingToDefaults();
    previewIntensityCurve();
    generateEdgingSteps();
    updatePreview();
}

void CustomPatternEditor::onAutoDetectionToggled(bool enabled)
{
    enableEdgingControls(enabled);
    m_patternModified = true;
}

// Edging pattern helper method implementations
void CustomPatternEditor::updateEdgingControls()
{
    if (!m_buildupIntensitySlider) return;

    // Update controls based on current pattern type
    bool isEdgingPattern = (m_patternTypeCombo->currentText() == "Edging" ||
                           m_patternTypeCombo->currentText() == "Custom");

    m_edgingTab->setEnabled(isEdgingPattern);

    if (isEdgingPattern) {
        syncEdgingSliders();
        previewIntensityCurve();
    }
}

void CustomPatternEditor::syncEdgingSliders()
{
    if (!m_buildupIntensitySlider) return;

    // Sync all slider-spinbox pairs without triggering signals
    m_buildupIntensitySlider->blockSignals(true);
    m_buildupIntensitySpin->blockSignals(true);
    m_buildupIntensitySlider->setValue(static_cast<int>(m_buildupIntensitySpin->value()));
    m_buildupIntensitySpin->setValue(m_buildupIntensitySlider->value());
    m_buildupIntensitySlider->blockSignals(false);
    m_buildupIntensitySpin->blockSignals(false);

    // Sync other pairs similarly
    m_buildupDurationSlider->blockSignals(true);
    m_buildupDurationSpin->blockSignals(true);
    m_buildupDurationSlider->setValue(m_buildupDurationSpin->value());
    m_buildupDurationSpin->setValue(m_buildupDurationSlider->value());
    m_buildupDurationSlider->blockSignals(false);
    m_buildupDurationSpin->blockSignals(false);

    // Continue for other slider-spinbox pairs...
    m_peakIntensitySlider->blockSignals(true);
    m_peakIntensitySpin->blockSignals(true);
    m_peakIntensitySlider->setValue(static_cast<int>(m_peakIntensitySpin->value()));
    m_peakIntensitySpin->setValue(m_peakIntensitySlider->value());
    m_peakIntensitySlider->blockSignals(false);
    m_peakIntensitySpin->blockSignals(false);

    m_holdDurationSlider->blockSignals(true);
    m_holdDurationSpin->blockSignals(true);
    m_holdDurationSlider->setValue(m_holdDurationSpin->value());
    m_holdDurationSpin->setValue(m_holdDurationSlider->value());
    m_holdDurationSlider->blockSignals(false);
    m_holdDurationSpin->blockSignals(false);

    m_cooldownDurationSlider->blockSignals(true);
    m_cooldownDurationSpin->blockSignals(true);
    m_cooldownDurationSlider->setValue(m_cooldownDurationSpin->value());
    m_cooldownDurationSpin->setValue(m_cooldownDurationSlider->value());
    m_cooldownDurationSlider->blockSignals(false);
    m_cooldownDurationSpin->blockSignals(false);

    // Update curve exponent
    m_curveExponentSlider->blockSignals(true);
    m_curveExponentSpin->blockSignals(true);
    m_curveExponentSlider->setValue(static_cast<int>(m_curveExponentSpin->value() * 100));
    m_curveExponentSpin->setValue(m_curveExponentSlider->value() / 100.0);
    m_curveExponentSlider->blockSignals(false);
    m_curveExponentSpin->blockSignals(false);
}

void CustomPatternEditor::generateEdgingSteps()
{
    if (!m_buildupIntensitySlider) return;

    // Clear existing steps
    m_patternSteps.clear();

    // Get parameters from controls
    double buildupIntensity = m_buildupIntensitySpin->value();
    int buildupDuration = m_buildupDurationSpin->value();
    QString buildupCurve = m_buildupCurveCombo->currentText();
    bool gradualBuildup = m_gradualBuildupCheck->isChecked();
    int buildupSteps = m_buildupStepsSpin->value();

    double peakIntensity = m_peakIntensitySpin->value();
    int holdDuration = m_holdDurationSpin->value();
    bool variablePeak = m_variablePeakCheck->isChecked();
    double peakVariation = m_peakVariationSpin->value();

    int cooldownDuration = m_cooldownDurationSpin->value();
    QString cooldownCurve = m_cooldownCurveCombo->currentText();
    bool completeCooldown = m_completeCooldownCheck->isChecked();
    double cooldownIntensity = m_cooldownIntensitySpin->value();

    int edgeCycles = m_infiniteCyclesCheck->isChecked() ? 1 : m_edgeCyclesSpin->value();
    int cycleDelay = m_cycleDelaySpin->value();
    bool increasingIntensity = m_increasingIntensityCheck->isChecked();
    double intensityIncrement = m_intensityIncrementSpin->value();

    QString curveType = m_intensityCurveTypeCombo->currentText();
    double curveExponent = m_curveExponentSpin->value();

    // Generate steps for each cycle
    for (int cycle = 0; cycle < edgeCycles; ++cycle) {
        double cycleIntensityMultiplier = 1.0;
        if (increasingIntensity && cycle > 0) {
            cycleIntensityMultiplier = 1.0 + (cycle * intensityIncrement / 100.0);
        }

        // Build-up phase
        if (gradualBuildup) {
            // Create gradual build-up steps
            for (int step = 0; step < buildupSteps; ++step) {
                double progress = static_cast<double>(step) / (buildupSteps - 1);
                double intensity = calculateIntensityCurve(progress, buildupCurve, curveExponent) * buildupIntensity * cycleIntensityMultiplier;
                int stepDuration = buildupDuration / buildupSteps;

                PatternStep buildupStep;
                buildupStep.pressurePercent = qMin(intensity, 100.0);
                buildupStep.durationMs = stepDuration;
                buildupStep.action = "vacuum";
                buildupStep.description = QString("Build-up Step %1/%2 (Cycle %3)").arg(step + 1).arg(buildupSteps).arg(cycle + 1);

                // Add edging-specific parameters
                QJsonObject params;
                params["phase"] = "buildup";
                params["cycle"] = cycle + 1;
                params["step"] = step + 1;
                params["total_steps"] = buildupSteps;
                params["curve_type"] = buildupCurve;
                params["target_intensity"] = buildupIntensity;
                buildupStep.parameters = params;

                m_patternSteps.append(buildupStep);
            }
        } else {
            // Single build-up step
            PatternStep buildupStep;
            buildupStep.pressurePercent = qMin(buildupIntensity * cycleIntensityMultiplier, 100.0);
            buildupStep.durationMs = buildupDuration;
            buildupStep.action = "vacuum";
            buildupStep.description = QString("Build-up Phase (Cycle %1)").arg(cycle + 1);

            QJsonObject params;
            params["phase"] = "buildup";
            params["cycle"] = cycle + 1;
            params["curve_type"] = buildupCurve;
            params["target_intensity"] = buildupIntensity;
            buildupStep.parameters = params;

            m_patternSteps.append(buildupStep);
        }

        // Peak/Hold phase
        double actualPeakIntensity = peakIntensity * cycleIntensityMultiplier;
        if (variablePeak) {
            // Add some variation to peak intensity
            double variation = (QRandomGenerator::global()->bounded(200) - 100) / 100.0 * peakVariation;
            actualPeakIntensity += actualPeakIntensity * variation / 100.0;
        }

        PatternStep peakStep;
        peakStep.pressurePercent = qMin(actualPeakIntensity, 100.0);
        peakStep.durationMs = holdDuration;
        peakStep.action = "hold";
        peakStep.description = QString("Peak Hold (Cycle %1)").arg(cycle + 1);

        QJsonObject peakParams;
        peakParams["phase"] = "peak";
        peakParams["cycle"] = cycle + 1;
        peakParams["variable_peak"] = variablePeak;
        peakParams["peak_variation"] = peakVariation;
        peakParams["target_intensity"] = peakIntensity;
        peakStep.parameters = peakParams;

        m_patternSteps.append(peakStep);

        // Cooldown/Release phase
        double finalIntensity = completeCooldown ? 0.0 : cooldownIntensity;

        PatternStep cooldownStep;
        cooldownStep.pressurePercent = finalIntensity;
        cooldownStep.durationMs = cooldownDuration;
        cooldownStep.action = completeCooldown ? "release" : "vacuum";
        cooldownStep.description = QString("Cooldown Phase (Cycle %1)").arg(cycle + 1);

        QJsonObject cooldownParams;
        cooldownParams["phase"] = "cooldown";
        cooldownParams["cycle"] = cycle + 1;
        cooldownParams["curve_type"] = cooldownCurve;
        cooldownParams["complete_release"] = completeCooldown;
        cooldownParams["final_intensity"] = finalIntensity;
        cooldownStep.parameters = cooldownParams;

        m_patternSteps.append(cooldownStep);

        // Add delay between cycles (except for last cycle)
        if (cycle < edgeCycles - 1 && cycleDelay > 0) {
            PatternStep delayStep;
            delayStep.pressurePercent = finalIntensity;
            delayStep.durationMs = cycleDelay;
            delayStep.action = "hold";
            delayStep.description = QString("Cycle Delay %1->%2").arg(cycle + 1).arg(cycle + 2);

            QJsonObject delayParams;
            delayParams["phase"] = "delay";
            delayParams["from_cycle"] = cycle + 1;
            delayParams["to_cycle"] = cycle + 2;
            delayStep.parameters = delayParams;

            m_patternSteps.append(delayStep);
        }
    }

    // Update the step list display
    updateStepList();
}

void CustomPatternEditor::previewIntensityCurve()
{
    if (!m_curvePreviewScene) return;

    m_curvePreviewScene->clear();

    QString curveType = m_intensityCurveTypeCombo->currentText();
    double exponent = m_curveExponentSpin->value();

    // Draw curve preview
    QPen curvePen(QColor(0, 120, 215), 2);
    QPen gridPen(QColor(200, 200, 200), 1);

    // Set scene rect
    QRectF sceneRect(0, 0, 300, 150);
    m_curvePreviewScene->setSceneRect(sceneRect);

    // Draw grid
    for (int i = 0; i <= 10; ++i) {
        double x = i * 30;
        m_curvePreviewScene->addLine(x, 0, x, 150, gridPen);

        double y = i * 15;
        m_curvePreviewScene->addLine(0, y, 300, y, gridPen);
    }

    // Draw curve
    QPainterPath curvePath;
    curvePath.moveTo(0, 150);

    for (int x = 0; x <= 300; ++x) {
        double progress = static_cast<double>(x) / 300.0;
        double intensity = calculateIntensityCurve(progress, curveType, exponent);
        double y = 150 - (intensity * 150);

        if (x == 0) {
            curvePath.moveTo(x, y);
        } else {
            curvePath.lineTo(x, y);
        }
    }

    m_curvePreviewScene->addPath(curvePath, curvePen);

    // Add labels
    QFont labelFont("Arial", 8);
    m_curvePreviewScene->addText("0%", labelFont)->setPos(-15, 135);
    m_curvePreviewScene->addText("100%", labelFont)->setPos(-25, -5);
    m_curvePreviewScene->addText("Time →", labelFont)->setPos(250, 160);
    m_curvePreviewScene->addText("Intensity ↑", labelFont)->setPos(-50, 70);
}

void CustomPatternEditor::validateEdgingParameters()
{
    if (!m_buildupIntensitySlider) return;

    QStringList warnings;
    QStringList errors;

    // Validate build-up parameters
    if (m_buildupIntensitySpin->value() >= m_peakIntensitySpin->value()) {
        warnings << "Build-up intensity should be lower than peak intensity";
    }

    if (m_buildupDurationSpin->value() < 3000) {
        warnings << "Build-up duration may be too short for effective edging";
    }

    // Validate peak parameters
    if (m_peakIntensitySpin->value() > 95) {
        warnings << "Peak intensity above 95% may be too intense";
    }

    if (m_holdDurationSpin->value() < 1000) {
        warnings << "Hold duration may be too short";
    }

    // Validate cooldown parameters
    if (m_cooldownDurationSpin->value() < 2000) {
        warnings << "Cooldown duration may be too short for recovery";
    }

    // Validate cycle parameters
    if (!m_infiniteCyclesCheck->isChecked() && m_edgeCyclesSpin->value() > 10) {
        warnings << "High number of cycles may be exhausting";
    }

    // Validate sensitivity parameters
    if (m_autoEdgeDetectionCheck->isChecked()) {
        if (m_sensitivityThresholdSpin->value() < 70) {
            warnings << "Low sensitivity threshold may trigger false edges";
        }

        if (m_responseTimeSpin->value() < 200) {
            errors << "Response time too fast - may cause instability";
        }
    }

    // Update validation display
    QString validationText;
    if (!errors.isEmpty()) {
        validationText += "ERRORS:\n" + errors.join("\n") + "\n\n";
    }
    if (!warnings.isEmpty()) {
        validationText += "WARNINGS:\n" + warnings.join("\n");
    }
    if (errors.isEmpty() && warnings.isEmpty()) {
        validationText = "Edging parameters are valid.";
    }

    if (m_validationResults) {
        m_validationResults->setPlainText(validationText);
        if (!errors.isEmpty()) {
            m_validationResults->setStyleSheet("color: red;");
        } else if (!warnings.isEmpty()) {
            m_validationResults->setStyleSheet("color: orange;");
        } else {
            m_validationResults->setStyleSheet("color: green;");
        }
    }
}

QJsonObject CustomPatternEditor::getEdgingParameters() const
{
    QJsonObject params;

    if (!m_buildupIntensitySlider) return params;

    // Build-up parameters
    QJsonObject buildupParams;
    buildupParams["intensity"] = m_buildupIntensitySpin->value();
    buildupParams["duration"] = m_buildupDurationSpin->value();
    buildupParams["curve_type"] = m_buildupCurveCombo->currentText();
    buildupParams["gradual"] = m_gradualBuildupCheck->isChecked();
    buildupParams["steps"] = m_buildupStepsSpin->value();
    params["buildup"] = buildupParams;

    // Peak parameters
    QJsonObject peakParams;
    peakParams["intensity"] = m_peakIntensitySpin->value();
    peakParams["hold_duration"] = m_holdDurationSpin->value();
    peakParams["variable"] = m_variablePeakCheck->isChecked();
    peakParams["variation"] = m_peakVariationSpin->value();
    params["peak"] = peakParams;

    // Cooldown parameters
    QJsonObject cooldownParams;
    cooldownParams["duration"] = m_cooldownDurationSpin->value();
    cooldownParams["curve_type"] = m_cooldownCurveCombo->currentText();
    cooldownParams["complete_release"] = m_completeCooldownCheck->isChecked();
    cooldownParams["min_intensity"] = m_cooldownIntensitySpin->value();
    params["cooldown"] = cooldownParams;

    // Cycle parameters
    QJsonObject cycleParams;
    cycleParams["count"] = m_edgeCyclesSpin->value();
    cycleParams["infinite"] = m_infiniteCyclesCheck->isChecked();
    cycleParams["delay"] = m_cycleDelaySpin->value();
    cycleParams["increasing_intensity"] = m_increasingIntensityCheck->isChecked();
    cycleParams["intensity_increment"] = m_intensityIncrementSpin->value();
    params["cycles"] = cycleParams;

    // Sensitivity parameters
    QJsonObject sensitivityParams;
    sensitivityParams["auto_detection"] = m_autoEdgeDetectionCheck->isChecked();
    sensitivityParams["threshold"] = m_sensitivityThresholdSpin->value();
    sensitivityParams["detection_window"] = m_detectionWindowSpin->value();
    sensitivityParams["adaptive"] = m_adaptiveSensitivityCheck->isChecked();
    sensitivityParams["response_time"] = m_responseTimeSpin->value();
    params["sensitivity"] = sensitivityParams;

    // Intensity curve parameters
    QJsonObject curveParams;
    curveParams["type"] = m_intensityCurveTypeCombo->currentText();
    curveParams["exponent"] = m_curveExponentSpin->value();
    curveParams["custom"] = m_customCurveCheck->isChecked();
    params["intensity_curve"] = curveParams;

    return params;
}

void CustomPatternEditor::setEdgingParameters(const QJsonObject& params)
{
    if (!m_buildupIntensitySlider) return;

    // Build-up parameters
    if (params.contains("buildup")) {
        QJsonObject buildupParams = params["buildup"].toObject();
        m_buildupIntensitySpin->setValue(buildupParams["intensity"].toDouble(70.0));
        m_buildupDurationSpin->setValue(buildupParams["duration"].toInt(15000));
        m_buildupCurveCombo->setCurrentText(buildupParams["curve_type"].toString("Exponential"));
        m_gradualBuildupCheck->setChecked(buildupParams["gradual"].toBool(true));
        m_buildupStepsSpin->setValue(buildupParams["steps"].toInt(8));
    }

    // Peak parameters
    if (params.contains("peak")) {
        QJsonObject peakParams = params["peak"].toObject();
        m_peakIntensitySpin->setValue(peakParams["intensity"].toDouble(85.0));
        m_holdDurationSpin->setValue(peakParams["hold_duration"].toInt(3000));
        m_variablePeakCheck->setChecked(peakParams["variable"].toBool(false));
        m_peakVariationSpin->setValue(peakParams["variation"].toDouble(5.0));
    }

    // Cooldown parameters
    if (params.contains("cooldown")) {
        QJsonObject cooldownParams = params["cooldown"].toObject();
        m_cooldownDurationSpin->setValue(cooldownParams["duration"].toInt(5000));
        m_cooldownCurveCombo->setCurrentText(cooldownParams["curve_type"].toString("Exponential"));
        m_completeCooldownCheck->setChecked(cooldownParams["complete_release"].toBool(true));
        m_cooldownIntensitySpin->setValue(cooldownParams["min_intensity"].toDouble(10.0));
    }

    // Cycle parameters
    if (params.contains("cycles")) {
        QJsonObject cycleParams = params["cycles"].toObject();
        m_edgeCyclesSpin->setValue(cycleParams["count"].toInt(3));
        m_infiniteCyclesCheck->setChecked(cycleParams["infinite"].toBool(false));
        m_cycleDelaySpin->setValue(cycleParams["delay"].toInt(5000));
        m_increasingIntensityCheck->setChecked(cycleParams["increasing_intensity"].toBool(false));
        m_intensityIncrementSpin->setValue(cycleParams["intensity_increment"].toDouble(3.0));
    }

    // Sensitivity parameters
    if (params.contains("sensitivity")) {
        QJsonObject sensitivityParams = params["sensitivity"].toObject();
        m_autoEdgeDetectionCheck->setChecked(sensitivityParams["auto_detection"].toBool(false));
        m_sensitivityThresholdSpin->setValue(sensitivityParams["threshold"].toDouble(80.0));
        m_detectionWindowSpin->setValue(sensitivityParams["detection_window"].toInt(2000));
        m_adaptiveSensitivityCheck->setChecked(sensitivityParams["adaptive"].toBool(false));
        m_responseTimeSpin->setValue(sensitivityParams["response_time"].toInt(500));
    }

    // Intensity curve parameters
    if (params.contains("intensity_curve")) {
        QJsonObject curveParams = params["intensity_curve"].toObject();
        m_intensityCurveTypeCombo->setCurrentText(curveParams["type"].toString("Exponential"));
        m_curveExponentSpin->setValue(curveParams["exponent"].toDouble(1.5));
        m_customCurveCheck->setChecked(curveParams["custom"].toBool(false));
    }

    // Update UI
    syncEdgingSliders();
    previewIntensityCurve();
}

void CustomPatternEditor::resetEdgingToDefaults()
{
    if (!m_buildupIntensitySlider) return;

    // Reset to default values
    m_buildupIntensitySpin->setValue(70.0);
    m_buildupDurationSpin->setValue(15000);
    m_buildupCurveCombo->setCurrentText("Exponential");
    m_gradualBuildupCheck->setChecked(true);
    m_buildupStepsSpin->setValue(8);

    m_peakIntensitySpin->setValue(85.0);
    m_holdDurationSpin->setValue(3000);
    m_variablePeakCheck->setChecked(false);
    m_peakVariationSpin->setValue(5.0);

    m_cooldownDurationSpin->setValue(5000);
    m_cooldownCurveCombo->setCurrentText("Exponential");
    m_completeCooldownCheck->setChecked(true);
    m_cooldownIntensitySpin->setValue(10.0);

    m_edgeCyclesSpin->setValue(3);
    m_infiniteCyclesCheck->setChecked(false);
    m_cycleDelaySpin->setValue(5000);
    m_increasingIntensityCheck->setChecked(false);
    m_intensityIncrementSpin->setValue(3.0);

    m_autoEdgeDetectionCheck->setChecked(false);
    m_sensitivityThresholdSpin->setValue(80.0);
    m_detectionWindowSpin->setValue(2000);
    m_adaptiveSensitivityCheck->setChecked(false);
    m_responseTimeSpin->setValue(500);

    m_intensityCurveTypeCombo->setCurrentText("Exponential");
    m_curveExponentSpin->setValue(1.5);
    m_customCurveCheck->setChecked(false);

    syncEdgingSliders();
    previewIntensityCurve();
}

void CustomPatternEditor::enableEdgingControls(bool enabled)
{
    if (!m_buildupIntensitySlider) return;

    // Enable/disable sensitivity controls based on auto-detection
    m_sensitivityThresholdSlider->setEnabled(enabled);
    m_sensitivityThresholdSpin->setEnabled(enabled);
    m_detectionWindowSlider->setEnabled(enabled);
    m_detectionWindowSpin->setEnabled(enabled);
    m_adaptiveSensitivityCheck->setEnabled(enabled);
    m_responseTimeSlider->setEnabled(enabled);
    m_responseTimeSpin->setEnabled(enabled);
}

double CustomPatternEditor::calculateIntensityCurve(double progress, const QString& curveType, double exponent) const
{
    // Clamp progress to [0, 1]
    progress = qBound(0.0, progress, 1.0);

    if (curveType == "Linear") {
        return progress;
    } else if (curveType == "Exponential") {
        return std::pow(progress, exponent);
    } else if (curveType == "Logarithmic") {
        return std::log(1.0 + progress * (std::exp(exponent) - 1.0)) / exponent;
    } else if (curveType == "S-Curve") {
        // Sigmoid curve
        double x = (progress - 0.5) * 6.0; // Scale to [-3, 3]
        return 1.0 / (1.0 + std::exp(-x * exponent));
    } else if (curveType == "Sine Wave") {
        return std::sin(progress * M_PI / 2.0);
    } else {
        // Default to linear
        return progress;
    }
}