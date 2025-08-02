#include "PatternSelector.h"
#include "components/TouchButton.h"
#include "../VacuumController.h"
#include <QDebug>
#include <QJsonDocument>
#include <QFile>
#include <QMessageBox>
#include <QApplication>

PatternSelector::PatternSelector(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_mainLayout(new QVBoxLayout(this))
    , m_patternButtonGroup(new QButtonGroup(this))
    , m_configFilePath("config/patterns.json")
{
    setupUI();
    loadPatterns();
    
    // Set default selection
    if (!m_patterns.isEmpty()) {
        selectPattern(m_patterns.firstKey());
    }
}

PatternSelector::~PatternSelector()
{
}

void PatternSelector::setupUI()
{
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    setupCategorySelector();
    setupPatternGrid();
    setupParameterPanel();
    setupPreviewPanel();
    
    // Make pattern button group exclusive
    m_patternButtonGroup->setExclusive(true);
    
    // Connect signals
    connect(m_patternButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &PatternSelector::onPatternButtonClicked);
}

void PatternSelector::setupCategorySelector()
{
    m_categoryGroup = new QGroupBox("Pattern Categories");
    m_categoryGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");

    QVBoxLayout* categoryLayout = new QVBoxLayout(m_categoryGroup);

    m_categoryCombo = new QComboBox();
    m_categoryCombo->setMinimumHeight(50);
    m_categoryCombo->setStyleSheet(
        "QComboBox {"
        "    font-size: 14pt;"
        "    padding: 10px;"
        "    border: 2px solid #ddd;"
        "    border-radius: 8px;"
        "    background-color: white;"
        "}"
        "QComboBox:focus {"
        "    border-color: #2196F3;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 30px;"
        "}"
    );

    // Add pattern categories
    m_categoryCombo->addItem("All Patterns");
    m_categoryCombo->addItem("Pulse Patterns");
    m_categoryCombo->addItem("Wave Patterns");
    m_categoryCombo->addItem("Air Pulse Patterns");
    m_categoryCombo->addItem("Milking Patterns");
    m_categoryCombo->addItem("Constant Patterns");
    m_categoryCombo->addItem("Special Patterns");

    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PatternSelector::onCategoryChanged);

    categoryLayout->addWidget(m_categoryCombo);
    m_mainLayout->addWidget(m_categoryGroup);
}

void PatternSelector::setupPatternGrid()
{
    m_patternGroup = new QGroupBox("Available Patterns");
    m_patternGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    
    QVBoxLayout* patternGroupLayout = new QVBoxLayout(m_patternGroup);
    
    // Create scroll area for patterns
    m_patternScrollArea = new QScrollArea();
    m_patternScrollArea->setWidgetResizable(true);
    m_patternScrollArea->setMinimumHeight(300);
    m_patternScrollArea->setStyleSheet("QScrollArea { border: 1px solid #ddd; border-radius: 5px; }");
    
    m_patternWidget = new QWidget();
    m_patternGrid = new QGridLayout(m_patternWidget);
    m_patternGrid->setSpacing(10);
    
    m_patternScrollArea->setWidget(m_patternWidget);
    patternGroupLayout->addWidget(m_patternScrollArea);
    
    m_mainLayout->addWidget(m_patternGroup);
}

void PatternSelector::setupParameterPanel()
{
    m_parameterGroup = new QGroupBox("Pattern Parameters");
    m_parameterGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    
    QVBoxLayout* parameterGroupLayout = new QVBoxLayout(m_parameterGroup);
    
    // Create scroll area for parameters
    m_parameterScrollArea = new QScrollArea();
    m_parameterScrollArea->setWidgetResizable(true);
    m_parameterScrollArea->setMaximumHeight(200);
    m_parameterScrollArea->setStyleSheet("QScrollArea { border: 1px solid #ddd; border-radius: 5px; }");
    
    m_parameterWidget = new QWidget();
    m_parameterLayout = new QVBoxLayout(m_parameterWidget);
    m_parameterLayout->setSpacing(10);
    
    m_parameterScrollArea->setWidget(m_parameterWidget);
    parameterGroupLayout->addWidget(m_parameterScrollArea);
    
    m_mainLayout->addWidget(m_parameterGroup);
}

void PatternSelector::setupPreviewPanel()
{
    m_previewGroup = new QGroupBox("Pattern Preview");
    m_previewGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    m_previewGroup->setFixedHeight(PREVIEW_PANEL_HEIGHT);
    
    QVBoxLayout* previewLayout = new QVBoxLayout(m_previewGroup);
    
    // Pattern info labels
    m_patternNameLabel = new QLabel("No pattern selected");
    m_patternNameLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #333;");
    m_patternNameLabel->setAlignment(Qt::AlignCenter);
    
    m_patternTypeLabel = new QLabel("");
    m_patternTypeLabel->setStyleSheet("font-size: 14pt; color: #666;");
    m_patternTypeLabel->setAlignment(Qt::AlignCenter);
    
    m_patternDescriptionLabel = new QLabel("");
    m_patternDescriptionLabel->setStyleSheet("font-size: 12pt; color: #333;");
    m_patternDescriptionLabel->setWordWrap(true);
    m_patternDescriptionLabel->setAlignment(Qt::AlignCenter);
    
    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_previewButton = new TouchButton("Preview Pattern");
    m_previewButton->setButtonType(TouchButton::Primary);
    m_previewButton->setMinimumSize(150, 50);
    
    m_customizeButton = new TouchButton("Customize");
    m_customizeButton->setButtonType(TouchButton::Normal);
    m_customizeButton->setMinimumSize(120, 50);
    
    connect(m_previewButton, &TouchButton::clicked, this, &PatternSelector::onPreviewClicked);
    connect(m_customizeButton, &TouchButton::clicked, this, &PatternSelector::onCustomizeClicked);
    
    buttonLayout->addWidget(m_previewButton);
    buttonLayout->addWidget(m_customizeButton);
    buttonLayout->addStretch();
    
    // Layout preview panel
    previewLayout->addWidget(m_patternNameLabel);
    previewLayout->addWidget(m_patternTypeLabel);
    previewLayout->addWidget(m_patternDescriptionLabel);
    previewLayout->addStretch();
    previewLayout->addLayout(buttonLayout);
    
    m_mainLayout->addWidget(m_previewGroup);
}

void PatternSelector::loadPatterns()
{
    loadPatternsFromConfig();
    populateCategorySelector();
    populatePatternGrid();
}

void PatternSelector::loadPatternsFromConfig()
{
    QFile file(m_configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open patterns config file:" << m_configFilePath;
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON in patterns config file";
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonObject vacuumPatterns = root["vacuum_patterns"].toObject();
    
    // Load patterns from each category
    QStringList categoryKeys = {"pulse_patterns", "wave_patterns", "air_pulse_patterns", 
                               "milking_patterns", "constant_patterns", "special_patterns"};
    
    for (const QString& categoryKey : categoryKeys) {
        QJsonArray patterns = vacuumPatterns[categoryKey].toArray();
        QString categoryName = categoryKey;
        categoryName.replace("_", " ");
        categoryName = categoryName.split(" ").first();
        categoryName[0] = categoryName[0].toUpper();
        
        for (const auto& patternValue : patterns) {
            QJsonObject patternObj = patternValue.toObject();
            
            PatternInfo info;
            info.name = patternObj["name"].toString();
            info.type = patternObj["type"].toString();
            info.speed = patternObj["speed"].toString();
            info.description = patternObj["description"].toString();
            info.category = categoryName;
            info.parameters = patternObj;
            
            m_patterns[info.name] = info;
            
            // Add to category list
            if (!m_categories.contains(categoryName)) {
                m_categories.append(categoryName);
            }
        }
    }
    
    qDebug() << "Loaded" << m_patterns.size() << "patterns in" << m_categories.size() << "categories";
}

void PatternSelector::populateCategorySelector()
{
    m_categoryCombo->clear();
    m_categoryCombo->addItem("All Categories", "all");
    
    for (const QString& category : m_categories) {
        m_categoryCombo->addItem(category, category);
    }
}

void PatternSelector::populatePatternGrid()
{
    // Clear existing buttons
    for (auto button : m_patternButtonGroup->buttons()) {
        m_patternButtonGroup->removeButton(button);
        button->deleteLater();
    }
    
    // Clear grid layout
    QLayoutItem* item;
    while ((item = m_patternGrid->takeAt(0)) != nullptr) {
        delete item;
    }
    
    // Get patterns for current category
    QList<PatternInfo> patternsToShow;
    QString currentCategory = m_categoryCombo->currentData().toString();
    
    for (const auto& pattern : m_patterns) {
        if (currentCategory == "all" || pattern.category == currentCategory) {
            patternsToShow.append(pattern);
        }
    }
    
    // Create pattern buttons
    int row = 0, col = 0;
    for (const auto& pattern : patternsToShow) {
        TouchButton* button = createPatternButton(pattern);
        m_patternButtonGroup->addButton(button);
        m_patternGrid->addWidget(button, row, col);
        
        col++;
        if (col >= GRID_COLUMNS) {
            col = 0;
            row++;
        }
    }
    
    // Add stretch to fill remaining space
    m_patternGrid->setRowStretch(row + 1, 1);
}

TouchButton* PatternSelector::createPatternButton(const PatternInfo& pattern)
{
    TouchButton* button = new TouchButton();
    button->setText(pattern.name);
    button->setMinimumSize(PATTERN_BUTTON_WIDTH, PATTERN_BUTTON_HEIGHT);
    button->setCheckable(true);
    button->setProperty("patternName", pattern.name);
    
    // Set button type based on pattern type
    if (pattern.type == "pulse") {
        button->setButtonType(TouchButton::Primary);
    } else if (pattern.type == "wave") {
        button->setButtonType(TouchButton::Success);
    } else if (pattern.type == "air_pulse") {
        button->setButtonType(TouchButton::Warning);
    } else if (pattern.type == "milking") {
        button->setButtonType(TouchButton::Normal);
    } else if (pattern.type == "constant") {
        button->setButtonType(TouchButton::Primary);
    } else if (pattern.type == "edging") {
        button->setButtonType(TouchButton::Danger);
    }
    
    // Set tooltip with description
    button->setToolTip(pattern.description);
    
    return button;
}

void PatternSelector::selectPattern(const QString& patternName)
{
    if (!m_patterns.contains(patternName)) {
        qWarning() << "Pattern not found:" << patternName;
        return;
    }
    
    m_selectedPattern = patternName;
    
    // Update button selection
    for (auto button : m_patternButtonGroup->buttons()) {
        TouchButton* touchButton = qobject_cast<TouchButton*>(button);
        if (touchButton && touchButton->property("patternName").toString() == patternName) {
            touchButton->setChecked(true);
            break;
        }
    }
    
    // Update parameter panel and preview
    updateParameterPanel();
    updatePreviewPanel();
    
    emit patternSelected(patternName);
}

void PatternSelector::selectCategory(const QString& category)
{
    // Find the category in the combo box and select it
    for (int i = 0; i < m_categoryCombo->count(); ++i) {
        if (m_categoryCombo->itemText(i) == category) {
            m_categoryCombo->setCurrentIndex(i);
            onCategoryChanged();
            break;
        }
    }
}

void PatternSelector::updateParameterPanel()
{
    // Clear existing controls
    for (auto control : m_parameterControls) {
        control->deleteLater();
    }
    m_parameterControls.clear();
    
    // Clear layout
    QLayoutItem* item;
    while ((item = m_parameterLayout->takeAt(0)) != nullptr) {
        delete item;
    }
    
    if (m_selectedPattern.isEmpty() || !m_patterns.contains(m_selectedPattern)) {
        return;
    }
    
    const PatternInfo& pattern = m_patterns[m_selectedPattern];
    
    // Create controls for each parameter
    QJsonObject params = pattern.parameters;
    QStringList paramKeys = params.keys();
    
    for (const QString& key : paramKeys) {
        if (key == "name" || key == "type" || key == "speed" || key == "description") {
            continue; // Skip metadata
        }
        
        QJsonValue value = params[key];
        QWidget* control = createParameterControl(key, value);
        
        if (control) {
            m_parameterControls[key] = control;
            m_parameterLayout->addWidget(control);
        }
    }
    
    m_parameterLayout->addStretch();
}

QWidget* PatternSelector::createParameterControl(const QString& name, const QJsonValue& value)
{
    QWidget* container = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // Create label
    QLabel* label = new QLabel(name + ":");
    label->setMinimumWidth(150);
    label->setStyleSheet("font-size: 12pt; font-weight: bold;");
    layout->addWidget(label);
    
    // Create control based on value type
    if (value.isDouble()) {
        QSpinBox* spinBox = new QSpinBox();
        spinBox->setMinimumHeight(PARAMETER_CONTROL_HEIGHT);
        spinBox->setRange(0, 10000);
        spinBox->setValue(static_cast<int>(value.toDouble()));
        spinBox->setStyleSheet("font-size: 12pt;");
        
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &PatternSelector::onParameterChanged);
        
        layout->addWidget(spinBox);
    } else if (value.isString()) {
        QComboBox* comboBox = new QComboBox();
        comboBox->setMinimumHeight(PARAMETER_CONTROL_HEIGHT);
        comboBox->addItem(value.toString());
        comboBox->setStyleSheet("font-size: 12pt;");
        
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PatternSelector::onParameterChanged);
        
        layout->addWidget(comboBox);
    } else {
        QLabel* valueLabel = new QLabel(value.toVariant().toString());
        valueLabel->setStyleSheet("font-size: 12pt; color: #666;");
        layout->addWidget(valueLabel);
    }
    
    layout->addStretch();
    return container;
}

void PatternSelector::updatePreviewPanel()
{
    if (m_selectedPattern.isEmpty() || !m_patterns.contains(m_selectedPattern)) {
        m_patternNameLabel->setText("No pattern selected");
        m_patternTypeLabel->setText("");
        m_patternDescriptionLabel->setText("");
        return;
    }
    
    const PatternInfo& pattern = m_patterns[m_selectedPattern];
    
    m_patternNameLabel->setText(pattern.name);
    m_patternTypeLabel->setText(QString("%1 Pattern (%2 Speed)")
                               .arg(pattern.type.toUpper())
                               .arg(pattern.speed.toUpper()));
    m_patternDescriptionLabel->setText(pattern.description);
}

void PatternSelector::onPatternButtonClicked()
{
    TouchButton* button = qobject_cast<TouchButton*>(sender());
    if (button) {
        QString patternName = button->property("patternName").toString();
        selectPattern(patternName);
    }
}

void PatternSelector::onCategoryChanged()
{
    populatePatternGrid();
}

void PatternSelector::onParameterChanged()
{
    if (!m_selectedPattern.isEmpty()) {
        emit parametersChanged(m_selectedPattern, getCurrentParameters());
    }
}

void PatternSelector::onPreviewClicked()
{
    if (!m_selectedPattern.isEmpty()) {
        emit previewRequested(m_selectedPattern);
    }
}

void PatternSelector::onCustomizeClicked()
{
    QMessageBox::information(this, "Customize Pattern", 
                           "Pattern customization interface will be implemented in a future version.");
}

QJsonObject PatternSelector::getCurrentParameters() const
{
    if (m_selectedPattern.isEmpty() || !m_patterns.contains(m_selectedPattern)) {
        return QJsonObject();
    }
    
    QJsonObject params = m_patterns[m_selectedPattern].parameters;
    
    // Update with current control values
    for (auto it = m_parameterControls.begin(); it != m_parameterControls.end(); ++it) {
        const QString& key = it.key();
        QWidget* control = it.value();
        
        QSpinBox* spinBox = control->findChild<QSpinBox*>();
        if (spinBox) {
            params[key] = spinBox->value();
            continue;
        }
        
        QComboBox* comboBox = control->findChild<QComboBox*>();
        if (comboBox) {
            params[key] = comboBox->currentText();
            continue;
        }
    }
    
    return params;
}

PatternSelector::PatternInfo PatternSelector::getSelectedPatternInfo() const
{
    if (m_selectedPattern.isEmpty() || !m_patterns.contains(m_selectedPattern)) {
        return PatternInfo();
    }
    
    return m_patterns[m_selectedPattern];
}

QStringList PatternSelector::getAvailablePatterns() const
{
    return m_patterns.keys();
}

QStringList PatternSelector::getPatternCategories() const
{
    return m_categories;
}

void PatternSelector::resetToDefaults()
{
    // Reset all pattern parameters to their default values
    // Clear current pattern selection
    m_selectedPattern.clear();
    m_selectedCategory.clear();
    updateParameterPanel();
    updatePreviewPanel();
}
