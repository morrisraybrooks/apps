#ifndef PATTERNSELECTOR_H
#define PATTERNSELECTOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QButtonGroup>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

// Forward declarations
class VacuumController;
class TouchButton;

/**
 * @brief Pattern selection and configuration widget
 * 
 * This widget provides comprehensive pattern selection for the vacuum controller:
 * - 15+ predefined vacuum patterns organized by category
 * - Real-time parameter adjustment
 * - Pattern preview and description
 * - Touch-optimized interface for 50-inch displays
 * - Custom pattern creation capabilities
 */
class PatternSelector : public QWidget
{
    Q_OBJECT

public:
    struct PatternInfo {
        QString name;
        QString type;
        QString speed;
        QString description;
        QJsonObject parameters;
        QString category;
        
        PatternInfo() = default;
        PatternInfo(const QString& n, const QString& t, const QString& s, const QString& d, const QJsonObject& p, const QString& c)
            : name(n), type(t), speed(s), description(d), parameters(p), category(c) {}
    };

    explicit PatternSelector(VacuumController* controller, QWidget *parent = nullptr);
    ~PatternSelector();

    // Pattern selection
    QString getSelectedPattern() const { return m_selectedPattern; }
    PatternInfo getSelectedPatternInfo() const;
    
    // Pattern management
    void loadPatterns();
    void refreshPatterns();
    QStringList getAvailablePatterns() const;
    QStringList getPatternCategories() const;
    
    // Parameter access
    QJsonObject getCurrentParameters() const;
    void setPatternParameters(const QString& patternName, const QJsonObject& parameters);

public slots:
    void selectPattern(const QString& patternName);
    void selectCategory(const QString& category);
    void resetToDefaults();

signals:
    void patternSelected(const QString& patternName);
    void parametersChanged(const QString& patternName, const QJsonObject& parameters);
    void previewRequested(const QString& patternName);

private slots:
    void onPatternButtonClicked();
    void onCategoryChanged();
    void onParameterChanged();
    void onPreviewClicked();
    void onCustomizeClicked();

private:
    void setupUI();
    void setupCategorySelector();
    void setupPatternGrid();
    void setupParameterPanel();
    void setupPreviewPanel();
    
    void loadPatternsFromConfig();
    void populateCategorySelector();
    void populatePatternGrid();
    void updateParameterPanel();
    void updatePreviewPanel();
    
    TouchButton* createPatternButton(const PatternInfo& pattern);
    QWidget* createParameterControl(const QString& name, const QJsonValue& value);
    
    void highlightSelectedPattern();
    void showPatternDescription(const PatternInfo& pattern);
    
    // Controller interface
    VacuumController* m_controller;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Category selection
    QGroupBox* m_categoryGroup;
    QComboBox* m_categoryCombo;
    
    // Pattern grid
    QGroupBox* m_patternGroup;
    QScrollArea* m_patternScrollArea;
    QWidget* m_patternWidget;
    QGridLayout* m_patternGrid;
    QButtonGroup* m_patternButtonGroup;
    
    // Parameter panel
    QGroupBox* m_parameterGroup;
    QScrollArea* m_parameterScrollArea;
    QWidget* m_parameterWidget;
    QVBoxLayout* m_parameterLayout;
    QMap<QString, QWidget*> m_parameterControls;
    
    // Preview panel
    QGroupBox* m_previewGroup;
    QLabel* m_patternNameLabel;
    QLabel* m_patternDescriptionLabel;
    QLabel* m_patternTypeLabel;
    TouchButton* m_previewButton;
    TouchButton* m_customizeButton;
    
    // Pattern data
    QMap<QString, PatternInfo> m_patterns;
    QStringList m_categories;
    QString m_selectedPattern;
    QString m_selectedCategory;
    
    // Configuration
    QString m_configFilePath;
    
    // Constants for touch interface
    static const int PATTERN_BUTTON_WIDTH = 180;
    static const int PATTERN_BUTTON_HEIGHT = 120;
    static const int GRID_COLUMNS = 3;
    static const int PARAMETER_CONTROL_HEIGHT = 60;
    static const int PREVIEW_PANEL_HEIGHT = 200;
};

#endif // PATTERNSELECTOR_H
