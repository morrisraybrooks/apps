#ifndef PATTERNTEMPLATEMANAGER_H
#define PATTERNTEMPLATEMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QStringList>

/**
 * @brief Pattern template management system
 * 
 * This system provides:
 * - Predefined pattern templates for common use cases
 * - Template categorization and organization
 * - Custom template creation and storage
 * - Template validation and verification
 * - Template import/export functionality
 * - Template parameter customization
 */
class PatternTemplateManager : public QObject
{
    Q_OBJECT

public:
    struct TemplateInfo {
        QString name;
        QString category;
        QString description;
        QString author;
        QString version;
        QStringList tags;
        QJsonObject parameters;
        QJsonArray steps;
        bool isBuiltIn;
        bool isValid;
        
        TemplateInfo() : isBuiltIn(false), isValid(false) {}
    };

    explicit PatternTemplateManager(QObject *parent = nullptr);
    ~PatternTemplateManager();

    // Template management
    bool loadTemplates();
    bool saveTemplates();
    bool loadTemplatesFromFile(const QString& filePath);
    bool saveTemplatesToFile(const QString& filePath);
    
    // Template access
    QStringList getTemplateNames() const;
    QStringList getTemplateCategories() const;
    QStringList getTemplatesByCategory(const QString& category) const;
    TemplateInfo getTemplate(const QString& name) const;
    bool hasTemplate(const QString& name) const;
    
    // Template creation
    bool createTemplate(const QString& name, const QString& category, 
                       const QString& description, const QJsonObject& parameters,
                       const QJsonArray& steps, const QStringList& tags = QStringList());
    bool updateTemplate(const QString& name, const TemplateInfo& templateInfo);
    bool removeTemplate(const QString& name);
    bool duplicateTemplate(const QString& sourceName, const QString& newName);
    
    // Template validation
    bool validateTemplate(const TemplateInfo& templateInfo) const;
    QString getValidationError() const { return m_lastValidationError; }
    
    // Template generation
    QJsonObject generatePatternFromTemplate(const QString& templateName, 
                                           const QJsonObject& customParameters = QJsonObject()) const;
    QJsonArray generateStepsFromTemplate(const QString& templateName,
                                        const QJsonObject& customParameters = QJsonObject()) const;
    
    // Built-in templates
    void createBuiltInTemplates();
    void resetToBuiltInTemplates();
    
    // Import/Export
    bool exportTemplate(const QString& templateName, const QString& filePath) const;
    bool importTemplate(const QString& filePath);
    bool exportAllTemplates(const QString& filePath) const;
    bool importTemplatesFromFile(const QString& filePath);

signals:
    void templateAdded(const QString& templateName);
    void templateRemoved(const QString& templateName);
    void templateUpdated(const QString& templateName);
    void templatesLoaded(int count);

private:
    void initializeTemplateManager();
    bool validateTemplateParameters(const QJsonObject& parameters) const;
    bool validateTemplateSteps(const QJsonArray& steps) const;
    QJsonObject mergeParameters(const QJsonObject& templateParams, const QJsonObject& customParams) const;
    
    // Built-in template creators
    TemplateInfo createBasicPulseTemplate() const;
    TemplateInfo createBasicWaveTemplate() const;
    TemplateInfo createBasicAirPulseTemplate() const;
    TemplateInfo createBasicMilkingTemplate() const;
    TemplateInfo createBasicConstantTemplate() const;
    TemplateInfo createAdvancedEdgingTemplate() const;
    TemplateInfo createGentleStartTemplate() const;
    TemplateInfo createIntenseBuildupTemplate() const;
    TemplateInfo createRelaxationTemplate() const;
    TemplateInfo createEnduranceTemplate() const;
    
    // Template storage
    QMap<QString, TemplateInfo> m_templates;
    QMap<QString, QStringList> m_categorizedTemplates;
    
    // Configuration
    QString m_templatesFilePath;
    QString m_lastValidationError;
    
    // Constants
    static const QString DEFAULT_TEMPLATES_FILE;
    static const QStringList VALID_ACTIONS;
    static const QStringList VALID_CATEGORIES;
    static const double MIN_PRESSURE_PERCENT;
    static const double MAX_PRESSURE_PERCENT;
    static const int MIN_DURATION_MS;
    static const int MAX_DURATION_MS;
};

#endif // PATTERNTEMPLATEMANAGER_H
