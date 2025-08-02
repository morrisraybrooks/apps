#include "PatternTemplateManager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants
const QString PatternTemplateManager::DEFAULT_TEMPLATES_FILE = "pattern_templates.json";
const QStringList PatternTemplateManager::VALID_ACTIONS = {
    "pressure", "pause", "ramp", "hold", "release"
};
const QStringList PatternTemplateManager::VALID_CATEGORIES = {
    "Pulse", "Wave", "Air Pulse", "Milking", "Constant", "Special", "Custom"
};
const double PatternTemplateManager::MIN_PRESSURE_PERCENT = 0.0;
const double PatternTemplateManager::MAX_PRESSURE_PERCENT = 100.0;
const int PatternTemplateManager::MIN_DURATION_MS = 100;
const int PatternTemplateManager::MAX_DURATION_MS = 60000;

PatternTemplateManager::PatternTemplateManager(QObject *parent)
    : QObject(parent)
    , m_templatesFilePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + DEFAULT_TEMPLATES_FILE)
{
    initializeTemplateManager();
}

PatternTemplateManager::~PatternTemplateManager()
{
    saveTemplates();
}

void PatternTemplateManager::initializeTemplateManager()
{
    // Create data directory if it doesn't exist
    QDir dataDir = QFileInfo(m_templatesFilePath).dir();
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    // Load existing templates or create built-in ones
    if (!loadTemplates()) {
        qDebug() << "Creating built-in templates";
        createBuiltInTemplates();
        saveTemplates();
    }
    
    qDebug() << "PatternTemplateManager initialized with" << m_templates.size() << "templates";
}

bool PatternTemplateManager::loadTemplates()
{
    return loadTemplatesFromFile(m_templatesFilePath);
}

bool PatternTemplateManager::saveTemplates()
{
    return saveTemplatesToFile(m_templatesFilePath);
}

bool PatternTemplateManager::loadTemplatesFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open templates file:" << filePath;
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in templates file:" << error.errorString();
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray templatesArray = root["templates"].toArray();
    
    m_templates.clear();
    m_categorizedTemplates.clear();
    
    for (const auto& templateValue : templatesArray) {
        QJsonObject templateObj = templateValue.toObject();
        
        TemplateInfo info;
        info.name = templateObj["name"].toString();
        info.category = templateObj["category"].toString();
        info.description = templateObj["description"].toString();
        info.author = templateObj["author"].toString();
        info.version = templateObj["version"].toString();
        info.isBuiltIn = templateObj["isBuiltIn"].toBool();
        info.parameters = templateObj["parameters"].toObject();
        info.steps = templateObj["steps"].toArray();
        
        QJsonArray tagsArray = templateObj["tags"].toArray();
        for (const auto& tagValue : tagsArray) {
            info.tags.append(tagValue.toString());
        }
        
        info.isValid = validateTemplate(info);
        
        m_templates[info.name] = info;
        m_categorizedTemplates[info.category].append(info.name);
    }
    
    emit templatesLoaded(m_templates.size());
    return true;
}

bool PatternTemplateManager::saveTemplatesToFile(const QString& filePath)
{
    QJsonObject root;
    QJsonArray templatesArray;
    
    for (auto it = m_templates.begin(); it != m_templates.end(); ++it) {
        const TemplateInfo& info = it.value();
        
        QJsonObject templateObj;
        templateObj["name"] = info.name;
        templateObj["category"] = info.category;
        templateObj["description"] = info.description;
        templateObj["author"] = info.author;
        templateObj["version"] = info.version;
        templateObj["isBuiltIn"] = info.isBuiltIn;
        templateObj["parameters"] = info.parameters;
        templateObj["steps"] = info.steps;
        
        QJsonArray tagsArray;
        for (const QString& tag : info.tags) {
            tagsArray.append(tag);
        }
        templateObj["tags"] = tagsArray;
        
        templatesArray.append(templateObj);
    }
    
    root["templates"] = templatesArray;
    root["version"] = "1.0";
    root["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write templates file:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

QStringList PatternTemplateManager::getTemplateNames() const
{
    return m_templates.keys();
}

QStringList PatternTemplateManager::getTemplateCategories() const
{
    return m_categorizedTemplates.keys();
}

QStringList PatternTemplateManager::getTemplatesByCategory(const QString& category) const
{
    return m_categorizedTemplates.value(category, QStringList());
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::getTemplate(const QString& name) const
{
    return m_templates.value(name, TemplateInfo());
}

bool PatternTemplateManager::hasTemplate(const QString& name) const
{
    return m_templates.contains(name);
}

bool PatternTemplateManager::createTemplate(const QString& name, const QString& category, 
                                          const QString& description, const QJsonObject& parameters,
                                          const QJsonArray& steps, const QStringList& tags)
{
    if (m_templates.contains(name)) {
        m_lastValidationError = QString("Template '%1' already exists").arg(name);
        return false;
    }
    
    TemplateInfo info;
    info.name = name;
    info.category = category;
    info.description = description;
    info.author = "User";
    info.version = "1.0";
    info.tags = tags;
    info.parameters = parameters;
    info.steps = steps;
    info.isBuiltIn = false;
    info.isValid = validateTemplate(info);
    
    if (!info.isValid) {
        return false;
    }
    
    m_templates[name] = info;
    m_categorizedTemplates[category].append(name);
    
    emit templateAdded(name);
    return true;
}

bool PatternTemplateManager::validateTemplate(const TemplateInfo& templateInfo) const
{
    m_lastValidationError.clear();
    
    // Check required fields
    if (templateInfo.name.isEmpty()) {
        m_lastValidationError = "Template name cannot be empty";
        return false;
    }
    
    if (templateInfo.category.isEmpty()) {
        m_lastValidationError = "Template category cannot be empty";
        return false;
    }
    
    if (!VALID_CATEGORIES.contains(templateInfo.category)) {
        m_lastValidationError = QString("Invalid category: %1").arg(templateInfo.category);
        return false;
    }
    
    // Validate parameters
    if (!validateTemplateParameters(templateInfo.parameters)) {
        return false;
    }
    
    // Validate steps
    if (!validateTemplateSteps(templateInfo.steps)) {
        return false;
    }
    
    return true;
}

bool PatternTemplateManager::validateTemplateParameters(const QJsonObject& parameters) const
{
    // Check intensity
    if (parameters.contains("intensity")) {
        double intensity = parameters["intensity"].toDouble();
        if (intensity < 0.0 || intensity > 100.0) {
            m_lastValidationError = QString("Invalid intensity: %1 (must be 0-100)").arg(intensity);
            return false;
        }
    }
    
    // Check speed
    if (parameters.contains("speed")) {
        double speed = parameters["speed"].toDouble();
        if (speed < 0.1 || speed > 5.0) {
            m_lastValidationError = QString("Invalid speed: %1 (must be 0.1-5.0)").arg(speed);
            return false;
        }
    }
    
    return true;
}

bool PatternTemplateManager::validateTemplateSteps(const QJsonArray& steps) const
{
    if (steps.isEmpty()) {
        m_lastValidationError = "Template must have at least one step";
        return false;
    }
    
    for (int i = 0; i < steps.size(); ++i) {
        QJsonObject step = steps[i].toObject();
        
        // Check required fields
        if (!step.contains("action")) {
            m_lastValidationError = QString("Step %1: Missing 'action' field").arg(i + 1);
            return false;
        }
        
        QString action = step["action"].toString();
        if (!VALID_ACTIONS.contains(action)) {
            m_lastValidationError = QString("Step %1: Invalid action '%2'").arg(i + 1).arg(action);
            return false;
        }
        
        // Check duration
        if (step.contains("duration")) {
            int duration = step["duration"].toInt();
            if (duration < MIN_DURATION_MS || duration > MAX_DURATION_MS) {
                m_lastValidationError = QString("Step %1: Invalid duration %2ms (must be %3-%4ms)")
                                       .arg(i + 1).arg(duration).arg(MIN_DURATION_MS).arg(MAX_DURATION_MS);
                return false;
            }
        }
        
        // Check pressure
        if (step.contains("pressure")) {
            double pressure = step["pressure"].toDouble();
            if (pressure < MIN_PRESSURE_PERCENT || pressure > MAX_PRESSURE_PERCENT) {
                m_lastValidationError = QString("Step %1: Invalid pressure %2% (must be %3-%4%)")
                                       .arg(i + 1).arg(pressure).arg(MIN_PRESSURE_PERCENT).arg(MAX_PRESSURE_PERCENT);
                return false;
            }
        }
    }
    
    return true;
}

void PatternTemplateManager::createBuiltInTemplates()
{
    m_templates.clear();
    m_categorizedTemplates.clear();
    
    // Create basic templates
    m_templates["Basic Pulse"] = createBasicPulseTemplate();
    m_templates["Basic Wave"] = createBasicWaveTemplate();
    m_templates["Basic Air Pulse"] = createBasicAirPulseTemplate();
    m_templates["Basic Milking"] = createBasicMilkingTemplate();
    m_templates["Basic Constant"] = createBasicConstantTemplate();
    
    // Create advanced templates
    m_templates["Advanced Edging"] = createAdvancedEdgingTemplate();
    m_templates["Gentle Start"] = createGentleStartTemplate();
    m_templates["Intense Buildup"] = createIntenseBuildupTemplate();
    m_templates["Relaxation"] = createRelaxationTemplate();
    m_templates["Endurance"] = createEnduranceTemplate();
    
    // Categorize templates
    for (auto it = m_templates.begin(); it != m_templates.end(); ++it) {
        const TemplateInfo& info = it.value();
        m_categorizedTemplates[info.category].append(info.name);
    }
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createBasicPulseTemplate() const
{
    TemplateInfo info;
    info.name = "Basic Pulse";
    info.category = "Pulse";
    info.description = "Simple pulsing pattern with adjustable intensity";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "basic" << "pulse" << "beginner";
    info.isBuiltIn = true;
    info.isValid = true;
    
    // Parameters
    QJsonObject params;
    params["intensity"] = 50.0;
    params["speed"] = 1.0;
    params["pulseDuration"] = 1000;
    params["pauseDuration"] = 500;
    info.parameters = params;
    
    // Steps
    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "pressure";
    step1["duration"] = 1000;
    step1["pressure"] = 50.0;
    steps.append(step1);
    
    QJsonObject step2;
    step2["action"] = "pause";
    step2["duration"] = 500;
    step2["pressure"] = 0.0;
    steps.append(step2);
    
    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createBasicWaveTemplate() const
{
    TemplateInfo info;
    info.name = "Basic Wave";
    info.category = "Wave";
    info.description = "Smooth wave pattern with gradual pressure changes";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "basic" << "wave" << "smooth";
    info.isBuiltIn = true;
    info.isValid = true;
    
    // Parameters
    QJsonObject params;
    params["intensity"] = 60.0;
    params["speed"] = 1.0;
    params["minPressure"] = 20.0;
    params["maxPressure"] = 80.0;
    params["period"] = 4000;
    info.parameters = params;
    
    // Steps - create a wave pattern
    QJsonArray steps;
    for (int i = 0; i < 8; ++i) {
        QJsonObject step;
        step["action"] = "ramp";
        step["duration"] = 500;
        
        // Calculate wave pressure using sine function
        double angle = (i * M_PI) / 4.0; // 8 steps for full wave
        double pressure = 50.0 + 30.0 * sin(angle);
        step["pressure"] = pressure;
        
        steps.append(step);
    }
    
    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createBasicAirPulseTemplate() const
{
    TemplateInfo info;
    info.name = "Basic Air Pulse";
    info.category = "Air Pulse";
    info.description = "Air pulse pattern with pressure and release cycles";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "basic" << "air" << "pulse";
    info.isBuiltIn = true;
    info.isValid = true;
    
    // Parameters
    QJsonObject params;
    params["intensity"] = 70.0;
    params["speed"] = 1.0;
    params["pulseDuration"] = 800;
    params["releaseDuration"] = 1200;
    info.parameters = params;
    
    // Steps
    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "pressure";
    step1["duration"] = 800;
    step1["pressure"] = 70.0;
    steps.append(step1);
    
    QJsonObject step2;
    step2["action"] = "release";
    step2["duration"] = 1200;
    step2["pressure"] = 10.0;
    steps.append(step2);
    
    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createBasicMilkingTemplate() const
{
    TemplateInfo info;
    info.name = "Basic Milking";
    info.category = "Milking";
    info.description = "Basic milking pattern with rhythmic pressure cycles";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "basic" << "milking" << "rhythmic";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 60.0;
    params["speed"] = 1.0;
    params["cycles"] = 10;
    info.parameters = params;

    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "ramp";
    step1["duration"] = 1000;
    step1["pressure"] = 60.0;
    steps.append(step1);

    QJsonObject step2;
    step2["action"] = "hold";
    step2["duration"] = 500;
    step2["pressure"] = 60.0;
    steps.append(step2);

    QJsonObject step3;
    step3["action"] = "release";
    step3["duration"] = 800;
    step3["pressure"] = 20.0;
    steps.append(step3);

    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createBasicConstantTemplate() const
{
    TemplateInfo info;
    info.name = "Basic Constant";
    info.category = "Constant";
    info.description = "Constant pressure pattern";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "basic" << "constant" << "steady";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 50.0;
    params["speed"] = 1.0;
    info.parameters = params;

    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "hold";
    step1["duration"] = 5000;
    step1["pressure"] = 50.0;
    steps.append(step1);

    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createAdvancedEdgingTemplate() const
{
    TemplateInfo info;
    info.name = "Advanced Edging";
    info.category = "Edging";
    info.description = "Advanced edging pattern with variable intensity";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "advanced" << "edging" << "variable";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 80.0;
    params["speed"] = 1.2;
    params["buildupTime"] = 3000;
    params["holdTime"] = 1000;
    info.parameters = params;

    QJsonArray steps;
    // Build up
    QJsonObject step1;
    step1["action"] = "ramp";
    step1["duration"] = 3000;
    step1["pressure"] = 80.0;
    steps.append(step1);

    // Hold
    QJsonObject step2;
    step2["action"] = "hold";
    step2["duration"] = 1000;
    step2["pressure"] = 80.0;
    steps.append(step2);

    // Quick release
    QJsonObject step3;
    step3["action"] = "release";
    step3["duration"] = 500;
    step3["pressure"] = 10.0;
    steps.append(step3);

    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createGentleStartTemplate() const
{
    TemplateInfo info;
    info.name = "Gentle Start";
    info.category = "Gentle";
    info.description = "Gentle starting pattern with gradual buildup";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "gentle" << "gradual" << "start";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 30.0;
    params["speed"] = 0.8;
    params["buildupTime"] = 5000;
    info.parameters = params;

    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "ramp";
    step1["duration"] = 5000;
    step1["pressure"] = 30.0;
    steps.append(step1);

    QJsonObject step2;
    step2["action"] = "hold";
    step2["duration"] = 2000;
    step2["pressure"] = 30.0;
    steps.append(step2);

    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createIntenseBuildupTemplate() const
{
    TemplateInfo info;
    info.name = "Intense Buildup";
    info.category = "Intense";
    info.description = "Intense pattern with rapid buildup";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "intense" << "buildup" << "rapid";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 90.0;
    params["speed"] = 1.5;
    params["buildupTime"] = 2000;
    info.parameters = params;

    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "ramp";
    step1["duration"] = 2000;
    step1["pressure"] = 90.0;
    steps.append(step1);

    QJsonObject step2;
    step2["action"] = "hold";
    step2["duration"] = 1500;
    step2["pressure"] = 90.0;
    steps.append(step2);

    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createRelaxationTemplate() const
{
    TemplateInfo info;
    info.name = "Relaxation";
    info.category = "Relaxation";
    info.description = "Gentle relaxation pattern with low pressure";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "relaxation" << "gentle" << "low";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 25.0;
    params["speed"] = 0.6;
    info.parameters = params;

    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "hold";
    step1["duration"] = 8000;
    step1["pressure"] = 25.0;
    steps.append(step1);

    info.steps = steps;
    return info;
}

PatternTemplateManager::TemplateInfo PatternTemplateManager::createEnduranceTemplate() const
{
    TemplateInfo info;
    info.name = "Endurance";
    info.category = "Endurance";
    info.description = "Long duration endurance pattern";
    info.author = "System";
    info.version = "1.0";
    info.tags = QStringList() << "endurance" << "long" << "sustained";
    info.isBuiltIn = true;
    info.isValid = true;

    QJsonObject params;
    params["intensity"] = 45.0;
    params["speed"] = 0.9;
    params["duration"] = 30000; // 30 seconds
    info.parameters = params;

    QJsonArray steps;
    QJsonObject step1;
    step1["action"] = "ramp";
    step1["duration"] = 5000;
    step1["pressure"] = 45.0;
    steps.append(step1);

    QJsonObject step2;
    step2["action"] = "hold";
    step2["duration"] = 25000;
    step2["pressure"] = 45.0;
    steps.append(step2);

    info.steps = steps;
    return info;
}
