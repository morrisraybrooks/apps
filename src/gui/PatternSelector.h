
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
class QAbstractButton;

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
    struct PatternStep {
        double pressurePercent;
        int durationMs;
        QString action;
        QString description;
        QJsonObject parameters;

        PatternStep() : pressurePercent(0.0), durationMs(0) {}
    };

    struct PatternInfo {
        QString name;
        QString type;
        QString description;
        QString category;
        double basePressure;
        double speed;
        double intensity;
        QList<PatternStep> steps;
        QJsonObject parameters;

        PatternInfo() : basePressure(50.0), speed(1.0), intensity(50.0) {}
        PatternInfo(const QString& n, const QString& t, const QString& d, const QString& c)
            : name(n), type(t), description(d), category(c), basePressure(50.0), speed(1.0), intensity(50.0) {}
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

public Q_SLOTS:
    void selectPattern(const QString& patternName);
    void selectCategory(const QString& category);
    void resetToDefaults();
    void onPatternCreated(const QString& patternName, const QJsonObject& patternData);
    void onPatternModified(const QString& patternName, const QJsonObject& patternData);

Q_SIGNALS:
    void patternSelected(const QString& patternName);
    void parametersChanged(const QString& patternName, const QJsonObject& parameters);
    void previewRequested(const QString& patternName);
    void patternCreated(const QString& patternName);
    void patternModified(const QString& patternName);
    void patternEditorRequested(const QString& patternName = QString());

private Q_SLOTS:
    void onPatternButtonClicked(QAbstractButton* button);
    void onCategoryChanged();
    void onParameterChanged();
    void onPreviewClicked();
    void onCustomizeClicked();
    void onCreateNewPatternClicked();

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
    void savePatternToConfig(const PatternInfo& pattern);
    
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
    static const int PATTERN_BUTTON_WIDTH = 120;
    static const int PATTERN_BUTTON_HEIGHT = 80;
    static const int GRID_COLUMNS = 4;
    static const int PARAMETER_CONTROL_HEIGHT = 60;
    static const int PREVIEW_PANEL_HEIGHT = 200;
};

#endif // PATTERNSELECTOR_H
