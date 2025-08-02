
#include "PatternSelector.h"
#include "components/TouchButton.h"
#include "CustomPatternDialog.h"
#include "../VacuumController.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QAbstractButton>

PatternSelector::PatternSelector(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_mainLayout(new QVBoxLayout(this))
    , m_patternButtonGroup(new QButtonGroup(this))
    , m_configFilePath("config/patterns.json")
{
    qDebug() << "PatternSelector constructor called.";
    setupUI();
    loadPatterns();
    
    // Set default selection
    if (!m_patterns.isEmpty()) {
        selectPattern(m_patterns.firstKey());
    } else {
        qWarning() << "No patterns loaded, default selection not set.";
    }
}

PatternSelector::~PatternSelector()
{
}

void PatternSelector::setupUI()
{
    qDebug() << "setupUI called.";
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    setupCategorySelector();
    setupPatternGrid();
    setupParameterPanel();
    setupPreviewPanel();

    // Add "Create New Pattern" button
    TouchButton* createNewButton = new TouchButton("+ Create New Pattern");
    createNewButton->setButtonType(TouchButton::Success);
    createNewButton->setMinimumHeight(50);
    connect(createNewButton, &TouchButton::clicked, this, &PatternSelector::onCreateNewPatternClicked);
    m_mainLayout->addWidget(createNewButton, 0, Qt::AlignBottom);

    // Ensure the widget has proper size for 50-inch display
    this->setMinimumSize(600, 400); // Appropriate size for 50-inch display
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setStyleSheet("QWidget { background-color: transparent; }");
    
    // Make pattern button group exclusive
    m_patternButtonGroup->setExclusive(true);
    
    // Connect signals
    connect(m_patternButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &PatternSelector::onPatternButtonClicked);
}

void PatternSelector::setupCategorySelector()
{
    qDebug() << "setupCategorySelector called.";
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

    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PatternSelector::onCategoryChanged);

    categoryLayout->addWidget(m_categoryCombo);
    m_mainLayout->addWidget(m_categoryGroup, 0);
}

void PatternSelector::setupPatternGrid()
{
    qDebug() << "setupPatternGrid called.";
    m_patternGroup = new QGroupBox("Available Patterns");
    m_patternGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    
    QVBoxLayout* patternGroupLayout = new QVBoxLayout(m_patternGroup);
    
    // Create scroll area for patterns
    m_patternScrollArea = new QScrollArea();
    m_patternScrollArea->setWidgetResizable(true);
    m_patternScrollArea->setMinimumHeight(200);
    m_patternScrollArea->setMaximumHeight(350);
    m_patternScrollArea->setStyleSheet("QScrollArea { border: 1px solid #ddd; border-radius: 5px; background-color: white; }");
    
    m_patternWidget = new QWidget();
    m_patternGrid = new QGridLayout(m_patternWidget);
    m_patternGrid->setSpacing(5);
    
    m_patternScrollArea->setWidget(m_patternWidget);
    patternGroupLayout->addWidget(m_patternScrollArea);
    
    m_mainLayout->addWidget(m_patternGroup, 1);
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
    
    m_mainLayout->addWidget(m_parameterGroup, 0);
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
    
    m_mainLayout->addWidget(m_previewGroup, 0);
}

void PatternSelector::loadPatterns()
{
    qDebug() << "loadPatterns called.";
    loadPatternsFromConfig();
    populateCategorySelector();
    populatePatternGrid();
}

void PatternSelector::loadPatternsFromConfig()
{
    qDebug() << "loadPatternsFromConfig called. Config file path:" << m_configFilePath;
    QFile file(m_configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open patterns config file:" << m_configFilePath << ", error: " << file.errorString();
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
        QStringList parts = categoryName.split(" ");
        for (int i = 0; i < parts.size(); ++i) {
            parts[i] = parts[i].at(0).toUpper() + parts[i].mid(1);
        }
        categoryName = parts.join(" ");
        
        for (const auto& patternValue : patterns) {
            QJsonObject patternObj = patternValue.toObject();
            
            PatternInfo info;
            info.name = patternObj["name"].toString();
            info.type = patternObj["type"].toString();
            info.speed = patternObj["speed"].toDouble(1.0);
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
    qDebug() << "populateCategorySelector called.";
    m_categoryCombo->clear();
    m_categoryCombo->addItem("All Categories", "all");
    
    for (const QString& category : m_categories) {
        qDebug() << "Adding category to combo box:" << category;
        m_categoryCombo->addItem(category, category);
    }
}

void PatternSelector::populatePatternGrid()
{
    qDebug() << "populatePatternGrid() called";

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

    qDebug() << "PopulatePatternGrid: currentCategory =" << currentCategory;
    qDebug() << "PopulatePatternGrid: total patterns =" << m_patterns.size();

    for (const auto& pattern : m_patterns) {
        if (currentCategory == "all" || pattern.category == currentCategory) {
            patternsToShow.append(pattern);
        }
    }

    qDebug() << "PopulatePatternGrid: patterns to show =" << patternsToShow.size();
    
    // Create pattern buttons
    int row = 0, col = 0;
    for (const auto& pattern : patternsToShow) {
        TouchButton* button = createPatternButton(pattern);
        m_patternButtonGroup->addButton(button);
        m_patternGrid->addWidget(button, row, col);
        qDebug() << "Added pattern button:" << pattern.name << "at row" << row << "col" << col;

        col++;
        if (col >= GRID_COLUMNS) {
            col = 0;
            row++;
        }
    }

    // Add stretch to fill remaining space
    m_patternGrid->setRowStretch(row + 1, 1);
    qDebug() << "Pattern grid populated.";
}

TouchButton* PatternSelector::createPatternButton(const PatternInfo& pattern)
{
    TouchButton* button = new TouchButton();
    button->setText(pattern.name);
    button->setMinimumSize(PATTERN_BUTTON_WIDTH, PATTERN_BUTTON_HEIGHT);
    button->setCheckable(true);
    button->setProperty("patternName", pattern.name);
    
    // Set consistent button type for all patterns
    button->setButtonType(TouchButton::Normal);
    
    // Set tooltip with description
    button->setToolTip(pattern.description);
    
    // Add stylesheet for selection feedback
    button->setStyleSheet(
        "TouchButton:checked {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: 2px solid #1976D2;"
        "}"
    );
    
    return button;
}

void PatternSelector::selectPattern(const QString& patternName)
{
    qDebug() << "selectPattern called with pattern:" << patternName;
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
        } else {
            touchButton->setChecked(false);
        }
    }
    
    // Update parameter panel and preview
    updateParameterPanel();
    updatePreviewPanel();
    
    emit patternSelected(patternName);
}

void PatternSelector::selectCategory(const QString& category)
{
    qDebug() << "selectCategory called with category:" << category;
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
    m_patternTypeLabel->setText(QString("%1 Pattern (Speed: %2x)")
                               .arg(pattern.type.toUpper())
                               .arg(pattern.speed, 0, 'f', 1));
    m_patternDescriptionLabel->setText(pattern.description);
}

void PatternSelector::onPatternButtonClicked(QAbstractButton* button)
{
    if (button) {
        QString patternName = button->property("patternName").toString();
        selectPattern(patternName);
    }
}

void PatternSelector::onCategoryChanged()
{
    qDebug() << "onCategoryChanged called.";
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
    if (!m_controller) {
        QMessageBox::warning(this, "Error", "Controller not available for pattern customization.");
        return;
    }

    // Create and show custom pattern dialog
    CustomPatternDialog* dialog = new CustomPatternDialog(m_controller, this);

    // If a pattern is selected, load it for editing
    if (!m_selectedPattern.isEmpty()) {
        dialog->loadPattern(m_selectedPattern);
    }

    // Connect signals to handle pattern creation/modification
    connect(dialog, &CustomPatternDialog::patternCreated,
            this, &PatternSelector::onPatternCreated);
    connect(dialog, &CustomPatternDialog::patternModified,
            this, &PatternSelector::onPatternModified);

    // Show dialog
    if (dialog->exec() == QDialog::Accepted) {
        // Pattern was saved successfully
        // Reload patterns to include any new/modified patterns
        loadPatterns();

        // If a new pattern was created, select it
        QString newPatternName = dialog->getPatternData()["name"].toString();
        if (!newPatternName.isEmpty() && m_patterns.contains(newPatternName)) {
            selectPattern(newPatternName);
        }
    }

    dialog->deleteLater();
}

void PatternSelector::onCreateNewPatternClicked()
{
    if (!m_controller) {
        QMessageBox::warning(this, "Error", "Controller not available for pattern creation.");
        return;
    }

    // Create and show custom pattern dialog for new pattern creation
    CustomPatternDialog* dialog = new CustomPatternDialog(m_controller, this);

    // Start with a fresh new pattern
    dialog->createNewPattern();

    // Connect signals to handle pattern creation
    connect(dialog, &CustomPatternDialog::patternCreated,
            this, &PatternSelector::onPatternCreated);

    // Show dialog
    if (dialog->exec() == QDialog::Accepted) {
        // Pattern was created successfully
        // Reload patterns to include the new pattern
        loadPatterns();

        // Select the new pattern
        QString newPatternName = dialog->getPatternData()["name"].toString();
        if (!newPatternName.isEmpty() && m_patterns.contains(newPatternName)) {
            selectPattern(newPatternName);
        }
    }

    dialog->deleteLater();
}

void PatternSelector::onPatternCreated(const QString& patternName, const QJsonObject& patternData)
{
    qDebug() << "New pattern created:" << patternName;

    // Add the new pattern to our pattern map
    PatternInfo newPattern;
    newPattern.name = patternName;
    newPattern.type = patternData["type"].toString();
    newPattern.description = patternData["description"].toString();
    newPattern.basePressure = patternData.value("base_pressure").toDouble(50.0);
    newPattern.speed = patternData.value("speed").toDouble(1.0);
    newPattern.intensity = patternData.value("intensity").toDouble(50.0);

    // Parse pattern steps
    QJsonArray stepsArray = patternData["steps"].toArray();
    for (const QJsonValue& stepValue : stepsArray) {
        QJsonObject stepObj = stepValue.toObject();
        PatternStep step;
        step.pressurePercent = stepObj["pressure_percent"].toDouble();
        step.durationMs = stepObj["duration_ms"].toInt();
        step.action = stepObj["action"].toString();
        step.description = stepObj["description"].toString();
        step.parameters = stepObj["parameters"].toObject().toVariantMap();
        newPattern.steps.append(step);
    }

    m_patterns[patternName] = newPattern;

    // Save the new pattern to configuration
    savePatternToConfig(newPattern);

    // Refresh the UI
    populatePatternGrid();

    // Select the new pattern
    selectPattern(patternName);

    emit patternCreated(patternName);
}

void PatternSelector::onPatternModified(const QString& patternName, const QJsonObject& patternData)
{
    qDebug() << "Pattern modified:" << patternName;

    // Update the existing pattern
    if (m_patterns.contains(patternName)) {
        PatternInfo& pattern = m_patterns[patternName];
        pattern.type = patternData["type"].toString();
        pattern.description = patternData["description"].toString();
        pattern.basePressure = patternData.value("base_pressure").toDouble(pattern.basePressure);
        pattern.speed = patternData.value("speed").toDouble(pattern.speed);
        pattern.intensity = patternData.value("intensity").toDouble(pattern.intensity);

        // Update pattern steps
        pattern.steps.clear();
        QJsonArray stepsArray = patternData["steps"].toArray();
        for (const QJsonValue& stepValue : stepsArray) {
            QJsonObject stepObj = stepValue.toObject();
            PatternStep step;
            step.pressurePercent = stepObj["pressure_percent"].toDouble();
            step.durationMs = stepObj["duration_ms"].toInt();
            step.action = stepObj["action"].toString();
            step.description = stepObj["description"].toString();
            step.parameters = stepObj["parameters"].toObject().toVariantMap();
            pattern.steps.append(step);
        }

        // Save the modified pattern to configuration
        savePatternToConfig(pattern);

        // Refresh the UI
        populatePatternGrid();
        updatePreviewPanel();

        emit patternModified(patternName);
    }
}

void PatternSelector::savePatternToConfig(const PatternInfo& pattern)
{
    // This is a simplified implementation - in a real application,
    // you would save to a proper configuration file or database

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/custom_patterns.json";

    // Create directory if it doesn't exist
    QDir().mkpath(QFileInfo(configPath).absolutePath());

    // Load existing patterns
    QJsonObject patternsObj;
    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        if (doc.isObject()) {
            patternsObj = doc.object();
        }
        configFile.close();
    }

    // Add/update the pattern
    QJsonObject patternObj;
    patternObj["name"] = pattern.name;
    patternObj["type"] = pattern.type;
    patternObj["description"] = pattern.description;
    patternObj["base_pressure"] = pattern.basePressure;
    patternObj["speed"] = pattern.speed;
    patternObj["intensity"] = pattern.intensity;

    // Add steps
    QJsonArray stepsArray;
    for (const PatternStep& step : pattern.steps) {
        QJsonObject stepObj;
        stepObj["pressure_percent"] = step.pressurePercent;
        stepObj["duration_ms"] = step.durationMs;
        stepObj["action"] = step.action;
        stepObj["description"] = step.description;
        stepObj["parameters"] = QJsonObject::fromVariantMap(step.parameters);
        stepsArray.append(stepObj);
    }
    patternObj["steps"] = stepsArray;

    patternsObj[pattern.name] = patternObj;

    // Save back to file
    if (configFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(patternsObj);
        configFile.write(doc.toJson());
        configFile.close();

        qDebug() << "Pattern saved to config:" << pattern.name;
    } else {
        qWarning() << "Failed to save pattern to config:" << pattern.name;
    }
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
