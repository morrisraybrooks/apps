#include "ExecutionModeSelector.h"
#include "TouchButton.h"
#include "styles/ModernMedicalStyle.h"
#include "../VacuumController.h"
#include "../control/OrgasmControlAlgorithm.h"

#include <QDebug>
#include <QMessageBox>

ExecutionModeSelector::ExecutionModeSelector(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_algorithm(nullptr)
    , m_mainLayout(nullptr)
    , m_modeGroup(nullptr)
    , m_modeGrid(nullptr)
    , m_modeButtonGroup(nullptr)
    , m_parameterGroup(nullptr)
    , m_parameterStack(nullptr)
    , m_targetCyclesSpinBox(nullptr)
    , m_targetOrgasmsSpinBox(nullptr)
    , m_maxDurationSpinBox(nullptr)
    , m_multiOrgasmTargetSpinBox(nullptr)
    , m_recoveryTimeSpinBox(nullptr)
    , m_denialDurationSpinBox(nullptr)
    , m_milkingDurationSpinBox(nullptr)
    , m_failureModeCombo(nullptr)
    , m_controlGroup(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_selectedMode(Mode::Manual)
    , m_sessionActive(false)
{
    if (m_controller) {
        m_algorithm = m_controller->getOrgasmControlAlgorithm();
    }
    
    setupUI();
    
    // Connect to algorithm signals if available
    if (m_algorithm) {
        connect(m_algorithm, &OrgasmControlAlgorithm::stateChanged,
                this, &ExecutionModeSelector::onAlgorithmStateChanged);
        connect(m_algorithm, &OrgasmControlAlgorithm::modeChanged,
                this, &ExecutionModeSelector::onAlgorithmStateChanged);
    }
}

ExecutionModeSelector::~ExecutionModeSelector() = default;

void ExecutionModeSelector::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    setupModeButtons();
    setupParameterPanels();
    setupControlButtons();
    
    // Select default mode
    selectMode(Mode::Manual);
}

void ExecutionModeSelector::setupModeButtons()
{
    m_modeGroup = new QGroupBox(tr("Execution Mode"), this);
    m_modeGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    m_modeGrid = new QGridLayout(m_modeGroup);
    m_modeGrid->setSpacing(15);
    
    m_modeButtonGroup = new QButtonGroup(this);
    m_modeButtonGroup->setExclusive(true);
    
    // Create mode buttons
    struct ModeInfo {
        Mode mode;
        QString name;
        QString description;
    };
    
    QList<ModeInfo> modes = {
        {Mode::Manual, tr("Manual"), tr("Direct control\nNo automation")},
        {Mode::AdaptiveEdging, tr("Adaptive Edging"), tr("Build to edge\nThen back off")},
        {Mode::ForcedOrgasm, tr("Forced Orgasm"), tr("Push through\nto completion")},
        {Mode::MultiOrgasm, tr("Multi-Orgasm"), tr("Multiple peaks\nwith recovery")},
        {Mode::Denial, tr("Denial"), tr("Tease without\nallowing release")},
        {Mode::Milking, tr("Milking"), tr("Sustained zone\nfor extraction")}
    };
    
    int row = 0, col = 0;
    for (const auto& info : modes) {
        TouchButton* button = createModeButton(info.mode, info.name, info.description);
        m_modeButtons[info.mode] = button;
        m_modeButtonGroup->addButton(button, static_cast<int>(info.mode));
        m_modeGrid->addWidget(button, row, col);
        
        col++;
        if (col >= GRID_COLUMNS) {
            col = 0;
            row++;
        }
    }
    
    connect(m_modeButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &ExecutionModeSelector::onModeButtonClicked);
    
    m_mainLayout->addWidget(m_modeGroup);
}

TouchButton* ExecutionModeSelector::createModeButton(Mode mode, const QString& name, const QString& description)
{
    TouchButton* button = new TouchButton(name, this);
    button->setCheckable(true);
    button->setMinimumSize(MODE_BUTTON_WIDTH, MODE_BUTTON_HEIGHT);
    button->setToolTip(description);
    
    // Style based on mode type
    QString baseColor;
    switch (mode) {
        case Mode::Manual:
            baseColor = "#607D8B"; // Blue-grey
            break;
        case Mode::AdaptiveEdging:
            baseColor = "#FF9800"; // Orange
            break;
        case Mode::ForcedOrgasm:
            baseColor = "#E91E63"; // Pink
            break;
        case Mode::MultiOrgasm:
            baseColor = "#9C27B0"; // Purple
            break;
        case Mode::Denial:
            baseColor = "#2196F3"; // Blue
            break;
        case Mode::Milking:
            baseColor = "#4CAF50"; // Green
            break;
    }
    
    button->setStyleSheet(QString(
        "TouchButton { background-color: %1; color: white; border-radius: 10px; font-size: 16px; font-weight: bold; }"
        "TouchButton:checked { background-color: %2; border: 3px solid white; }"
        "TouchButton:hover { background-color: %3; }"
    ).arg(baseColor, baseColor, baseColor));

    return button;
}

void ExecutionModeSelector::setupParameterPanels()
{
    m_parameterGroup = new QGroupBox(tr("Session Parameters"), this);
    m_parameterGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());

    QVBoxLayout* paramLayout = new QVBoxLayout(m_parameterGroup);

    m_parameterStack = new QStackedWidget(this);

    // Create parameter panels for each mode
    m_parameterPanels[Mode::Manual] = createManualPanel();
    m_parameterPanels[Mode::AdaptiveEdging] = createAdaptiveEdgingPanel();
    m_parameterPanels[Mode::ForcedOrgasm] = createForcedOrgasmPanel();
    m_parameterPanels[Mode::MultiOrgasm] = createMultiOrgasmPanel();
    m_parameterPanels[Mode::Denial] = createDenialPanel();
    m_parameterPanels[Mode::Milking] = createMilkingPanel();

    for (auto it = m_parameterPanels.begin(); it != m_parameterPanels.end(); ++it) {
        m_parameterStack->addWidget(it.value());
    }

    paramLayout->addWidget(m_parameterStack);
    m_mainLayout->addWidget(m_parameterGroup);
}

QWidget* ExecutionModeSelector::createManualPanel()
{
    QWidget* panel = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(panel);

    QLabel* label = new QLabel(tr("Manual mode provides direct control.\n"
                                   "Use pattern selector for vacuum patterns.\n"
                                   "No automatic arousal management."), panel);
    label->setStyleSheet("font-size: 14px; color: #666;");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch();

    return panel;
}

QWidget* ExecutionModeSelector::createAdaptiveEdgingPanel()
{
    QWidget* panel = new QWidget(this);
    QGridLayout* layout = new QGridLayout(panel);
    layout->setSpacing(15);

    // Target cycles
    QLabel* cyclesLabel = new QLabel(tr("Target Edge Cycles:"), panel);
    cyclesLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_targetCyclesSpinBox = new QSpinBox(panel);
    m_targetCyclesSpinBox->setRange(1, 20);
    m_targetCyclesSpinBox->setValue(5);
    m_targetCyclesSpinBox->setSuffix(tr(" cycles"));
    m_targetCyclesSpinBox->setMinimumHeight(40);
    connect(m_targetCyclesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(cyclesLabel, 0, 0);
    layout->addWidget(m_targetCyclesSpinBox, 0, 1);

    QLabel* descLabel = new QLabel(tr("Build arousal to edge threshold, then back off.\n"
                                       "Repeat for specified number of cycles."), panel);
    descLabel->setStyleSheet("font-size: 12px; color: #666;");
    layout->addWidget(descLabel, 1, 0, 1, 2);
    layout->setRowStretch(2, 1);

    return panel;
}

QWidget* ExecutionModeSelector::createForcedOrgasmPanel()
{
    QWidget* panel = new QWidget(this);
    QGridLayout* layout = new QGridLayout(panel);
    layout->setSpacing(15);

    // Target orgasms
    QLabel* orgasmsLabel = new QLabel(tr("Target Orgasms:"), panel);
    orgasmsLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_targetOrgasmsSpinBox = new QSpinBox(panel);
    m_targetOrgasmsSpinBox->setRange(1, 10);
    m_targetOrgasmsSpinBox->setValue(3);
    m_targetOrgasmsSpinBox->setMinimumHeight(40);
    connect(m_targetOrgasmsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(orgasmsLabel, 0, 0);
    layout->addWidget(m_targetOrgasmsSpinBox, 0, 1);

    // Max duration
    QLabel* durationLabel = new QLabel(tr("Max Duration:"), panel);
    durationLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_maxDurationSpinBox = new QSpinBox(panel);
    m_maxDurationSpinBox->setRange(5, 60);
    m_maxDurationSpinBox->setValue(30);
    m_maxDurationSpinBox->setSuffix(tr(" min"));
    m_maxDurationSpinBox->setMinimumHeight(40);
    connect(m_maxDurationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(durationLabel, 1, 0);
    layout->addWidget(m_maxDurationSpinBox, 1, 1);
    layout->setRowStretch(2, 1);

    return panel;
}

QWidget* ExecutionModeSelector::createMultiOrgasmPanel()
{
    QWidget* panel = new QWidget(this);
    QGridLayout* layout = new QGridLayout(panel);
    layout->setSpacing(15);

    // Target orgasms
    QLabel* targetLabel = new QLabel(tr("Target Orgasms:"), panel);
    targetLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_multiOrgasmTargetSpinBox = new QSpinBox(panel);
    m_multiOrgasmTargetSpinBox->setRange(2, 10);
    m_multiOrgasmTargetSpinBox->setValue(3);
    m_multiOrgasmTargetSpinBox->setMinimumHeight(40);
    connect(m_multiOrgasmTargetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(targetLabel, 0, 0);
    layout->addWidget(m_multiOrgasmTargetSpinBox, 0, 1);

    // Recovery time
    QLabel* recoveryLabel = new QLabel(tr("Recovery Time:"), panel);
    recoveryLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_recoveryTimeSpinBox = new QSpinBox(panel);
    m_recoveryTimeSpinBox->setRange(10, 120);
    m_recoveryTimeSpinBox->setValue(30);
    m_recoveryTimeSpinBox->setSuffix(tr(" sec"));
    m_recoveryTimeSpinBox->setMinimumHeight(40);
    connect(m_recoveryTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(recoveryLabel, 1, 0);
    layout->addWidget(m_recoveryTimeSpinBox, 1, 1);
    layout->setRowStretch(2, 1);

    return panel;
}

QWidget* ExecutionModeSelector::createDenialPanel()
{
    QWidget* panel = new QWidget(this);
    QGridLayout* layout = new QGridLayout(panel);
    layout->setSpacing(15);

    // Duration
    QLabel* durationLabel = new QLabel(tr("Denial Duration:"), panel);
    durationLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_denialDurationSpinBox = new QSpinBox(panel);
    m_denialDurationSpinBox->setRange(5, 60);
    m_denialDurationSpinBox->setValue(10);
    m_denialDurationSpinBox->setSuffix(tr(" min"));
    m_denialDurationSpinBox->setMinimumHeight(40);
    connect(m_denialDurationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(durationLabel, 0, 0);
    layout->addWidget(m_denialDurationSpinBox, 0, 1);

    QLabel* descLabel = new QLabel(tr("Tease and build arousal without allowing orgasm.\n"
                                       "Backs off when approaching threshold."), panel);
    descLabel->setStyleSheet("font-size: 12px; color: #666;");
    layout->addWidget(descLabel, 1, 0, 1, 2);
    layout->setRowStretch(2, 1);

    return panel;
}

QWidget* ExecutionModeSelector::createMilkingPanel()
{
    QWidget* panel = new QWidget(this);
    QGridLayout* layout = new QGridLayout(panel);
    layout->setSpacing(15);

    // Duration
    QLabel* durationLabel = new QLabel(tr("Milking Duration:"), panel);
    durationLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_milkingDurationSpinBox = new QSpinBox(panel);
    m_milkingDurationSpinBox->setRange(10, 60);
    m_milkingDurationSpinBox->setValue(30);
    m_milkingDurationSpinBox->setSuffix(tr(" min"));
    m_milkingDurationSpinBox->setMinimumHeight(40);
    connect(m_milkingDurationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(durationLabel, 0, 0);
    layout->addWidget(m_milkingDurationSpinBox, 0, 1);

    // Failure mode
    QLabel* failureLabel = new QLabel(tr("On Orgasm Failure:"), panel);
    failureLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_failureModeCombo = new QComboBox(panel);
    m_failureModeCombo->addItem(tr("Stop Session"), 0);
    m_failureModeCombo->addItem(tr("Ruin Orgasm"), 1);
    m_failureModeCombo->addItem(tr("Punish (TENS)"), 2);
    m_failureModeCombo->addItem(tr("Continue"), 3);
    m_failureModeCombo->setMinimumHeight(40);
    connect(m_failureModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExecutionModeSelector::onParameterChanged);

    layout->addWidget(failureLabel, 1, 0);
    layout->addWidget(m_failureModeCombo, 1, 1);

    QLabel* descLabel = new QLabel(tr("Maintain arousal in milking zone for fluid extraction.\n"
                                       "Failure mode determines response to accidental orgasm."), panel);
    descLabel->setStyleSheet("font-size: 12px; color: #666;");
    layout->addWidget(descLabel, 2, 0, 1, 2);
    layout->setRowStretch(3, 1);

    return panel;
}

void ExecutionModeSelector::setupControlButtons()
{
    m_controlGroup = new QGroupBox(tr("Session Control"), this);
    m_controlGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());

    QHBoxLayout* controlLayout = new QHBoxLayout(m_controlGroup);
    controlLayout->setSpacing(20);

    m_startButton = new TouchButton(tr("Start Session"), this);
    m_startButton->setMinimumSize(200, 60);
    m_startButton->setStyleSheet(
        "TouchButton { background-color: #4CAF50; color: white; border-radius: 10px; font-size: 18px; font-weight: bold; }"
        "TouchButton:hover { background-color: #45a049; }"
        "TouchButton:disabled { background-color: #cccccc; }"
    );
    connect(m_startButton, &TouchButton::clicked, this, &ExecutionModeSelector::onStartClicked);

    m_stopButton = new TouchButton(tr("Stop Session"), this);
    m_stopButton->setMinimumSize(200, 60);
    m_stopButton->setEnabled(false);
    m_stopButton->setStyleSheet(
        "TouchButton { background-color: #f44336; color: white; border-radius: 10px; font-size: 18px; font-weight: bold; }"
        "TouchButton:hover { background-color: #da190b; }"
        "TouchButton:disabled { background-color: #cccccc; }"
    );
    connect(m_stopButton, &TouchButton::clicked, this, &ExecutionModeSelector::onStopClicked);

    controlLayout->addStretch();
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addStretch();

    m_mainLayout->addWidget(m_controlGroup);
}

void ExecutionModeSelector::onModeButtonClicked(QAbstractButton* button)
{
    int modeId = m_modeButtonGroup->id(button);
    selectMode(static_cast<Mode>(modeId));
}

void ExecutionModeSelector::selectMode(Mode mode)
{
    m_selectedMode = mode;

    // Update parameter panel
    if (m_parameterPanels.contains(mode)) {
        m_parameterStack->setCurrentWidget(m_parameterPanels[mode]);
    }

    // Update button states
    highlightSelectedMode();

    emit modeSelected(mode);
    qDebug() << "Mode selected:" << getSelectedModeName();
}

void ExecutionModeSelector::highlightSelectedMode()
{
    for (auto it = m_modeButtons.begin(); it != m_modeButtons.end(); ++it) {
        it.value()->setChecked(it.key() == m_selectedMode);
    }
}

QString ExecutionModeSelector::getSelectedModeName() const
{
    switch (m_selectedMode) {
        case Mode::Manual: return tr("Manual");
        case Mode::AdaptiveEdging: return tr("Adaptive Edging");
        case Mode::ForcedOrgasm: return tr("Forced Orgasm");
        case Mode::MultiOrgasm: return tr("Multi-Orgasm");
        case Mode::Denial: return tr("Denial");
        case Mode::Milking: return tr("Milking");
        default: return tr("Unknown");
    }
}

QJsonObject ExecutionModeSelector::getSessionParameters() const
{
    QJsonObject params;
    params["mode"] = static_cast<int>(m_selectedMode);
    params["modeName"] = getSelectedModeName();

    switch (m_selectedMode) {
        case Mode::AdaptiveEdging:
            params["targetCycles"] = m_targetCyclesSpinBox ? m_targetCyclesSpinBox->value() : 5;
            break;
        case Mode::ForcedOrgasm:
            params["targetOrgasms"] = m_targetOrgasmsSpinBox ? m_targetOrgasmsSpinBox->value() : 3;
            params["maxDurationMs"] = m_maxDurationSpinBox ? m_maxDurationSpinBox->value() * 60000 : 1800000;
            break;
        case Mode::MultiOrgasm:
            params["targetOrgasms"] = m_multiOrgasmTargetSpinBox ? m_multiOrgasmTargetSpinBox->value() : 3;
            params["recoveryTimeMs"] = m_recoveryTimeSpinBox ? m_recoveryTimeSpinBox->value() * 1000 : 30000;
            break;
        case Mode::Denial:
            params["durationMs"] = m_denialDurationSpinBox ? m_denialDurationSpinBox->value() * 60000 : 600000;
            break;
        case Mode::Milking:
            params["durationMs"] = m_milkingDurationSpinBox ? m_milkingDurationSpinBox->value() * 60000 : 1800000;
            params["failureMode"] = m_failureModeCombo ? m_failureModeCombo->currentData().toInt() : 0;
            break;
        default:
            break;
    }

    return params;
}

void ExecutionModeSelector::setSessionParameters(const QJsonObject& parameters)
{
    if (parameters.contains("mode")) {
        selectMode(static_cast<Mode>(parameters["mode"].toInt()));
    }

    if (parameters.contains("targetCycles") && m_targetCyclesSpinBox) {
        m_targetCyclesSpinBox->setValue(parameters["targetCycles"].toInt());
    }
    if (parameters.contains("targetOrgasms") && m_targetOrgasmsSpinBox) {
        m_targetOrgasmsSpinBox->setValue(parameters["targetOrgasms"].toInt());
    }
    if (parameters.contains("maxDurationMs") && m_maxDurationSpinBox) {
        m_maxDurationSpinBox->setValue(parameters["maxDurationMs"].toInt() / 60000);
    }
    if (parameters.contains("durationMs") && m_denialDurationSpinBox) {
        m_denialDurationSpinBox->setValue(parameters["durationMs"].toInt() / 60000);
    }
    if (parameters.contains("failureMode") && m_failureModeCombo) {
        int index = m_failureModeCombo->findData(parameters["failureMode"].toInt());
        if (index >= 0) m_failureModeCombo->setCurrentIndex(index);
    }
}

void ExecutionModeSelector::onParameterChanged()
{
    emit parametersChanged(getSessionParameters());
}

void ExecutionModeSelector::onStartClicked()
{
    if (!m_algorithm) {
        QMessageBox::warning(this, tr("Error"), tr("Orgasm control algorithm not available."));
        return;
    }

    QJsonObject params = getSessionParameters();

    // Start the appropriate mode on the algorithm
    switch (m_selectedMode) {
        case Mode::Manual:
            // Manual mode - no automatic control
            break;
        case Mode::AdaptiveEdging:
            m_algorithm->startAdaptiveEdging(params["targetCycles"].toInt(5));
            break;
        case Mode::ForcedOrgasm:
            m_algorithm->startForcedOrgasm(params["targetOrgasms"].toInt(3),
                                           params["maxDurationMs"].toInt(1800000));
            break;
        case Mode::MultiOrgasm:
            m_algorithm->startForcedOrgasm(params["targetOrgasms"].toInt(3),
                                           params["maxDurationMs"].toInt(1800000));
            break;
        case Mode::Denial:
            m_algorithm->startDenial(params["durationMs"].toInt(600000));
            break;
        case Mode::Milking:
            m_algorithm->startMilking(params["durationMs"].toInt(1800000),
                                      params["failureMode"].toInt(0));
            break;
    }

    m_sessionActive = true;
    updateControlButtons();
    emit sessionStartRequested(m_selectedMode, params);
}

void ExecutionModeSelector::onStopClicked()
{
    if (m_algorithm) {
        m_algorithm->stop();
    }

    m_sessionActive = false;
    updateControlButtons();
    emit sessionStopRequested();
}

void ExecutionModeSelector::updateControlButtons()
{
    m_startButton->setEnabled(!m_sessionActive);
    m_stopButton->setEnabled(m_sessionActive);

    // Disable mode selection during active session
    for (auto button : m_modeButtons) {
        button->setEnabled(!m_sessionActive);
    }
}

void ExecutionModeSelector::onAlgorithmStateChanged()
{
    if (!m_algorithm) return;

    auto state = m_algorithm->getState();
    m_sessionActive = (state != OrgasmControlAlgorithm::ControlState::STOPPED &&
                       state != OrgasmControlAlgorithm::ControlState::ERROR);
    updateControlButtons();
}

void ExecutionModeSelector::resetToDefaults()
{
    if (m_targetCyclesSpinBox) m_targetCyclesSpinBox->setValue(5);
    if (m_targetOrgasmsSpinBox) m_targetOrgasmsSpinBox->setValue(3);
    if (m_maxDurationSpinBox) m_maxDurationSpinBox->setValue(30);
    if (m_multiOrgasmTargetSpinBox) m_multiOrgasmTargetSpinBox->setValue(3);
    if (m_recoveryTimeSpinBox) m_recoveryTimeSpinBox->setValue(30);
    if (m_denialDurationSpinBox) m_denialDurationSpinBox->setValue(10);
    if (m_milkingDurationSpinBox) m_milkingDurationSpinBox->setValue(30);
    if (m_failureModeCombo) m_failureModeCombo->setCurrentIndex(0);

    selectMode(Mode::Manual);
}

void ExecutionModeSelector::updateParameterPanel()
{
    // Already handled by stacked widget
}

