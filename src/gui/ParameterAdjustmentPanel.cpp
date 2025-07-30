#include "ParameterAdjustmentPanel.h"
#include "components/TouchButton.h"
#include "../VacuumController.h"
#include "../patterns/PatternEngine.h"
#include <QDebug>
#include <QApplication>
#include <QStyle>

ParameterAdjustmentPanel::ParameterAdjustmentPanel(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_patternEngine(nullptr)
    , m_mainLayout(new QVBoxLayout(this))
    , m_currentIntensity(DEFAULT_INTENSITY)
    , m_currentSpeed(DEFAULT_SPEED)
    , m_currentPressureOffset(DEFAULT_PRESSURE_OFFSET)
    , m_currentPulseDuration(DEFAULT_PULSE_DURATION)
    , m_safetyMode(true)
    , m_realTimeUpdates(true)
    , m_patternRunning(false)
    , m_updateTimer(new QTimer(this))
    , m_parameterAnimation(new QPropertyAnimation(this))
    , m_opacityEffect(new QGraphicsOpacityEffect(this))
{
    setupUI();
    connectSignals();
    
    // Initialize preset configurations
    setupPresetConfigurations();
    
    // Start real-time update timer
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &ParameterAdjustmentPanel::onRealTimeUpdateTimer);
    m_updateTimer->start();
    
    // Get pattern engine reference
    if (m_controller) {
        m_patternEngine = m_controller->getPatternEngine();
    }
}

ParameterAdjustmentPanel::~ParameterAdjustmentPanel()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void ParameterAdjustmentPanel::setupUI()
{
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    setupIntensityControls();
    setupSpeedControls();
    setupPressureControls();
    setupTimingControls();
    setupPatternSpecificControls();
    setupPresetControls();
    setupSafetyControls();
    
    m_mainLayout->addStretch();
}

void ParameterAdjustmentPanel::setupIntensityControls()
{
    m_intensityGroup = new QGroupBox("Intensity Control");
    m_intensityGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    
    QVBoxLayout* intensityLayout = new QVBoxLayout(m_intensityGroup);
    
    // Create intensity slider
    m_intensitySlider = createTouchSlider();
    m_intensitySlider->setRange(0, SLIDER_RESOLUTION);
    m_intensitySlider->setValue(static_cast<int>(m_currentIntensity * SLIDER_RESOLUTION / 100.0));
    m_intensitySlider->setMinimumHeight(60);
    
    // Create intensity dial for fine control
    m_intensityDial = new QDial();
    m_intensityDial->setRange(0, SLIDER_RESOLUTION);
    m_intensityDial->setValue(static_cast<int>(m_currentIntensity * SLIDER_RESOLUTION / 100.0));
    m_intensityDial->setMinimumSize(120, 120);
    m_intensityDial->setStyleSheet(
        "QDial {"
        "    background-color: #f5f5f5;"
        "    border: 3px solid #2196F3;"
        "    border-radius: 60px;"
        "}"
    );
    
    // Create value display
    m_intensityValueLabel = createValueLabel(QString("%1%").arg(m_currentIntensity, 0, 'f', 1));
    m_intensityValueLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #2196F3;");
    
    // Create progress bar
    m_intensityProgressBar = new QProgressBar();
    m_intensityProgressBar->setRange(0, 100);
    m_intensityProgressBar->setValue(static_cast<int>(m_currentIntensity));
    m_intensityProgressBar->setMinimumHeight(30);
    m_intensityProgressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #ddd;"
        "    border-radius: 15px;"
        "    text-align: center;"
        "    font-size: 12pt;"
        "    font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #2196F3);"
        "    border-radius: 13px;"
        "}"
    );
    
    // Layout intensity controls
    QHBoxLayout* intensityControlLayout = new QHBoxLayout();
    intensityControlLayout->addWidget(m_intensityDial);
    
    QVBoxLayout* intensityRightLayout = new QVBoxLayout();
    intensityRightLayout->addWidget(m_intensityValueLabel);
    intensityRightLayout->addWidget(m_intensitySlider);
    intensityRightLayout->addWidget(m_intensityProgressBar);
    
    intensityControlLayout->addLayout(intensityRightLayout);
    intensityLayout->addLayout(intensityControlLayout);
    
    m_mainLayout->addWidget(m_intensityGroup);
}

void ParameterAdjustmentPanel::setupSpeedControls()
{
    m_speedGroup = new QGroupBox("Speed Control");
    m_speedGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #FF9800; }");
    
    QVBoxLayout* speedLayout = new QVBoxLayout(m_speedGroup);
    
    // Create speed slider
    m_speedSlider = createTouchSlider();
    m_speedSlider->setRange(static_cast<int>(m_limits.minSpeed * SLIDER_RESOLUTION), 
                           static_cast<int>(m_limits.maxSpeed * SLIDER_RESOLUTION));
    m_speedSlider->setValue(static_cast<int>(m_currentSpeed * SLIDER_RESOLUTION));
    m_speedSlider->setMinimumHeight(60);
    
    // Create value labels
    m_speedValueLabel = createValueLabel(QString("%1x").arg(m_currentSpeed, 0, 'f', 1));
    m_speedValueLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #FF9800;");
    
    m_speedMultiplierLabel = new QLabel("Pattern Speed Multiplier");
    m_speedMultiplierLabel->setStyleSheet("font-size: 12pt; color: #666;");
    m_speedMultiplierLabel->setAlignment(Qt::AlignCenter);
    
    // Layout speed controls
    QHBoxLayout* speedControlLayout = new QHBoxLayout();
    speedControlLayout->addWidget(new QLabel("Slow"));
    speedControlLayout->addWidget(m_speedSlider);
    speedControlLayout->addWidget(new QLabel("Fast"));
    
    speedLayout->addWidget(m_speedValueLabel);
    speedLayout->addLayout(speedControlLayout);
    speedLayout->addWidget(m_speedMultiplierLabel);
    
    m_mainLayout->addWidget(m_speedGroup);
}

void ParameterAdjustmentPanel::setupPressureControls()
{
    m_pressureGroup = new QGroupBox("Pressure Adjustment");
    m_pressureGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #9C27B0; }");
    
    QVBoxLayout* pressureLayout = new QVBoxLayout(m_pressureGroup);
    
    // Create pressure offset slider
    m_pressureOffsetSlider = createTouchSlider();
    m_pressureOffsetSlider->setRange(static_cast<int>(m_limits.minPressureOffset * SLIDER_RESOLUTION / 100.0), 
                                    static_cast<int>(m_limits.maxPressureOffset * SLIDER_RESOLUTION / 100.0));
    m_pressureOffsetSlider->setValue(static_cast<int>(m_currentPressureOffset * SLIDER_RESOLUTION / 100.0));
    m_pressureOffsetSlider->setMinimumHeight(60);
    
    // Create value labels
    m_pressureOffsetValueLabel = createValueLabel(QString("%1%").arg(m_currentPressureOffset, 0, 'f', 1));
    m_pressureOffsetValueLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #9C27B0;");
    
    m_pressureRangeLabel = new QLabel("Pressure Offset from Base Pattern");
    m_pressureRangeLabel->setStyleSheet("font-size: 12pt; color: #666;");
    m_pressureRangeLabel->setAlignment(Qt::AlignCenter);
    
    // Layout pressure controls
    QHBoxLayout* pressureControlLayout = new QHBoxLayout();
    pressureControlLayout->addWidget(new QLabel("-20%"));
    pressureControlLayout->addWidget(m_pressureOffsetSlider);
    pressureControlLayout->addWidget(new QLabel("+20%"));
    
    pressureLayout->addWidget(m_pressureOffsetValueLabel);
    pressureLayout->addLayout(pressureControlLayout);
    pressureLayout->addWidget(m_pressureRangeLabel);
    
    m_mainLayout->addWidget(m_pressureGroup);
}

void ParameterAdjustmentPanel::setupTimingControls()
{
    m_timingGroup = new QGroupBox("Timing Control");
    m_timingGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #607D8B; }");
    
    QVBoxLayout* timingLayout = new QVBoxLayout(m_timingGroup);
    
    // Create pulse duration controls
    QHBoxLayout* durationControlLayout = new QHBoxLayout();
    
    QLabel* durationLabel = new QLabel("Pulse Duration:");
    durationLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_pulseDurationSpin = new QSpinBox();
    m_pulseDurationSpin->setRange(m_limits.minPulseDuration, m_limits.maxPulseDuration);
    m_pulseDurationSpin->setValue(m_currentPulseDuration);
    m_pulseDurationSpin->setSuffix(" ms");
    m_pulseDurationSpin->setMinimumHeight(50);
    m_pulseDurationSpin->setStyleSheet("font-size: 14pt; padding: 5px;");
    
    m_pulseDurationSlider = createTouchSlider();
    m_pulseDurationSlider->setRange(m_limits.minPulseDuration, m_limits.maxPulseDuration);
    m_pulseDurationSlider->setValue(m_currentPulseDuration);
    m_pulseDurationSlider->setMinimumHeight(60);
    
    m_pulseDurationValueLabel = createValueLabel(QString("%1 ms").arg(m_currentPulseDuration));
    m_pulseDurationValueLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #607D8B;");
    
    durationControlLayout->addWidget(durationLabel);
    durationControlLayout->addWidget(m_pulseDurationSpin);
    durationControlLayout->addStretch();
    
    timingLayout->addLayout(durationControlLayout);
    timingLayout->addWidget(m_pulseDurationValueLabel);
    timingLayout->addWidget(m_pulseDurationSlider);
    
    m_mainLayout->addWidget(m_timingGroup);
}

void ParameterAdjustmentPanel::setupPatternSpecificControls()
{
    m_patternSpecificGroup = new QGroupBox("Pattern-Specific Controls");
    m_patternSpecificGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #795548; }");
    m_patternSpecificGroup->setVisible(false); // Hidden by default
    
    m_patternSpecificLayout = new QVBoxLayout(m_patternSpecificGroup);
    
    m_patternSpecificWidget = new QWidget();
    m_patternSpecificLayout->addWidget(m_patternSpecificWidget);
    
    m_mainLayout->addWidget(m_patternSpecificGroup);
}

void ParameterAdjustmentPanel::setupPresetControls()
{
    m_presetGroup = new QGroupBox("Quick Presets");
    m_presetGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #4CAF50; }");
    
    QHBoxLayout* presetLayout = new QHBoxLayout(m_presetGroup);
    
    // Create preset buttons
    m_gentlePresetButton = new TouchButton("Gentle");
    m_gentlePresetButton->setButtonType(TouchButton::Success);
    m_gentlePresetButton->setMinimumSize(100, 60);
    
    m_moderatePresetButton = new TouchButton("Moderate");
    m_moderatePresetButton->setButtonType(TouchButton::Primary);
    m_moderatePresetButton->setMinimumSize(100, 60);
    
    m_intensePresetButton = new TouchButton("Intense");
    m_intensePresetButton->setButtonType(TouchButton::Warning);
    m_intensePresetButton->setMinimumSize(100, 60);
    
    m_customPresetButton = new TouchButton("Custom");
    m_customPresetButton->setButtonType(TouchButton::Normal);
    m_customPresetButton->setMinimumSize(100, 60);
    
    m_resetButton = new TouchButton("Reset");
    m_resetButton->setButtonType(TouchButton::Danger);
    m_resetButton->setMinimumSize(100, 60);
    
    presetLayout->addWidget(m_gentlePresetButton);
    presetLayout->addWidget(m_moderatePresetButton);
    presetLayout->addWidget(m_intensePresetButton);
    presetLayout->addWidget(m_customPresetButton);
    presetLayout->addStretch();
    presetLayout->addWidget(m_resetButton);
    
    m_mainLayout->addWidget(m_presetGroup);
}

void ParameterAdjustmentPanel::setupSafetyControls()
{
    m_safetyGroup = new QGroupBox("Safety Monitor");
    m_safetyGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #f44336; }");
    
    QVBoxLayout* safetyLayout = new QVBoxLayout(m_safetyGroup);
    
    // Safety status
    m_safetyStatusLabel = new QLabel("Safety Mode: ACTIVE");
    m_safetyStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #4CAF50;");
    m_safetyStatusLabel->setAlignment(Qt::AlignCenter);
    
    // Safety mode toggle
    m_safetyModeButton = new TouchButton("Safety Mode: ON");
    m_safetyModeButton->setButtonType(TouchButton::Success);
    m_safetyModeButton->setCheckable(true);
    m_safetyModeButton->setChecked(m_safetyMode);
    m_safetyModeButton->setMinimumSize(150, 50);
    
    // Safety limit indicator
    m_safetyLimitBar = new QProgressBar();
    m_safetyLimitBar->setRange(0, 100);
    m_safetyLimitBar->setValue(static_cast<int>(m_currentIntensity));
    m_safetyLimitBar->setFormat("Safety Limit: %p%");
    m_safetyLimitBar->setMinimumHeight(30);
    m_safetyLimitBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #ddd;"
        "    border-radius: 15px;"
        "    text-align: center;"
        "    font-size: 12pt;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #4CAF50;"
        "    border-radius: 13px;"
        "}"
    );
    
    QHBoxLayout* safetyButtonLayout = new QHBoxLayout();
    safetyButtonLayout->addWidget(m_safetyModeButton);
    safetyButtonLayout->addStretch();
    
    safetyLayout->addWidget(m_safetyStatusLabel);
    safetyLayout->addLayout(safetyButtonLayout);
    safetyLayout->addWidget(m_safetyLimitBar);
    
    m_mainLayout->addWidget(m_safetyGroup);
}
