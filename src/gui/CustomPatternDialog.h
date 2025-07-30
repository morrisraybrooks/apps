#ifndef CUSTOMPATTERNDIALOG_H
#define CUSTOMPATTERNDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSlider>
#include <QTableWidget>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>

// Forward declarations
class VacuumController;
class TouchButton;
class PatternEngine;

/**
 * @brief Custom pattern creation and editing dialog
 * 
 * This dialog provides comprehensive pattern creation capabilities:
 * - Visual pattern designer with drag-and-drop
 * - Step-by-step pattern builder
 * - Real-time pattern preview
 * - Pattern validation and testing
 * - Template-based pattern creation
 * - Pattern import/export
 * - Advanced timing and pressure controls
 */
class CustomPatternDialog : public QDialog
{
    Q_OBJECT

public:
    struct PatternStep {
        double pressurePercent;
        int durationMs;
        QString action;         // "vacuum", "release", "hold", "ramp"
        QString description;
        QJsonObject parameters;
        
        PatternStep() : pressurePercent(0.0), durationMs(1000), action("vacuum") {}
        PatternStep(double pressure, int duration, const QString& act = "vacuum", const QString& desc = "")
            : pressurePercent(pressure), durationMs(duration), action(act), description(desc) {}
    };

    explicit CustomPatternDialog(VacuumController* controller, QWidget *parent = nullptr);
    ~CustomPatternDialog();

    // Pattern management
    void loadPattern(const QString& patternName);
    void createNewPattern();
    bool savePattern();
    bool savePatternAs();
    
    // Pattern data
    QJsonObject getPatternData() const;
    void setPatternData(const QJsonObject& data);
    QList<PatternStep> getPatternSteps() const;
    void setPatternSteps(const QList<PatternStep>& steps);

public slots:
    void previewPattern();
    void stopPreview();
    void testPattern();
    void validatePattern();

signals:
    void patternCreated(const QString& patternName, const QJsonObject& patternData);
    void patternModified(const QString& patternName, const QJsonObject& patternData);

private slots:
    void onPatternTypeChanged();
    void onStepAdded();
    void onStepRemoved();
    void onStepModified();
    void onStepSelected();
    void onPreviewTimer();
    void onTemplateSelected();
    void onImportPattern();
    void onExportPattern();
    void onResetPattern();
    void onApplyClicked();
    void onCancelClicked();
    void onOkClicked();

private:
    void setupUI();
    void setupBasicInfoTab();
    void setupStepEditorTab();
    void setupVisualDesignerTab();
    void setupPreviewTab();
    void setupAdvancedTab();
    void connectSignals();
    
    void updateStepTable();
    void updateVisualDesigner();
    void updatePreviewChart();
    void updatePatternInfo();
    
    void addStepToTable(const PatternStep& step, int row = -1);
    void removeStepFromTable(int row);
    PatternStep getStepFromTable(int row);
    void setStepInTable(int row, const PatternStep& step);
    
    void createPatternFromTemplate(const QString& templateName);
    bool validatePatternData();
    QString generatePatternDescription();
    
    // Controller interface
    VacuumController* m_controller;
    PatternEngine* m_patternEngine;
    
    // Main UI
    QTabWidget* m_tabWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_buttonLayout;
    
    // Buttons
    TouchButton* m_previewButton;
    TouchButton* m_testButton;
    TouchButton* m_validateButton;
    TouchButton* m_applyButton;
    TouchButton* m_cancelButton;
    TouchButton* m_okButton;
    
    // Basic Info Tab
    QWidget* m_basicInfoTab;
    QLineEdit* m_patternNameEdit;
    QComboBox* m_patternTypeCombo;
    QComboBox* m_patternSpeedCombo;
    QTextEdit* m_patternDescriptionEdit;
    QComboBox* m_patternCategoryCombo;
    
    // Step Editor Tab
    QWidget* m_stepEditorTab;
    QTableWidget* m_stepTable;
    QGroupBox* m_stepDetailsGroup;
    QDoubleSpinBox* m_stepPressureSpin;
    QSpinBox* m_stepDurationSpin;
    QComboBox* m_stepActionCombo;
    QLineEdit* m_stepDescriptionEdit;
    TouchButton* m_addStepButton;
    TouchButton* m_removeStepButton;
    TouchButton* m_moveUpButton;
    TouchButton* m_moveDownButton;
    
    // Visual Designer Tab
    QWidget* m_visualDesignerTab;
    QGraphicsView* m_designerView;
    QGraphicsScene* m_designerScene;
    QSlider* m_timeScaleSlider;
    QSlider* m_pressureScaleSlider;
    TouchButton* m_addPointButton;
    TouchButton* m_removePointButton;
    TouchButton* m_smoothButton;
    
    // Preview Tab
    QWidget* m_previewTab;
    QGraphicsView* m_previewChart;
    QGraphicsScene* m_previewScene;
    QLabel* m_previewStatusLabel;
    QLabel* m_patternDurationLabel;
    QLabel* m_patternStepsLabel;
    QSlider* m_previewPositionSlider;
    TouchButton* m_playButton;
    TouchButton* m_pauseButton;
    TouchButton* m_stopButton;
    
    // Advanced Tab
    QWidget* m_advancedTab;
    QGroupBox* m_timingGroup;
    QSpinBox* m_minStepDurationSpin;
    QSpinBox* m_maxStepDurationSpin;
    QDoubleSpinBox* m_rampRateSpin;
    QGroupBox* m_pressureGroup;
    QDoubleSpinBox* m_minPressureSpin;
    QDoubleSpinBox* m_maxPressureSpin;
    QDoubleSpinBox* m_pressureToleranceSpin;
    QGroupBox* m_safetyGroup;
    QDoubleSpinBox* m_maxPressureLimitSpin;
    QSpinBox* m_emergencyStopDelaySpin;
    QCheckBox* m_enableSafetyChecksBox;
    
    // Templates and Import/Export
    QGroupBox* m_templatesGroup;
    QListWidget* m_templatesList;
    TouchButton* m_loadTemplateButton;
    TouchButton* m_saveTemplateButton;
    TouchButton* m_importButton;
    TouchButton* m_exportButton;
    TouchButton* m_resetButton;
    
    // Pattern data
    QString m_currentPatternName;
    QList<PatternStep> m_patternSteps;
    QJsonObject m_patternMetadata;
    bool m_patternModified;
    
    // Preview state
    QTimer* m_previewTimer;
    int m_previewPosition;
    bool m_previewPlaying;
    bool m_previewTesting;
    
    // Visual designer state
    QList<QPointF> m_designerPoints;
    int m_selectedPoint;
    
    // Constants
    static const int DEFAULT_STEP_DURATION = 1000;     // 1 second
    static const double DEFAULT_PRESSURE = 50.0;       // 50%
    static const int MIN_STEP_DURATION = 100;          // 100ms
    static const int MAX_STEP_DURATION = 60000;        // 60 seconds
    static const double MIN_PRESSURE = 0.0;            // 0%
    static const double MAX_PRESSURE = 100.0;          // 100%
    static const int PREVIEW_UPDATE_INTERVAL = 100;    // 100ms
    static const int MAX_PATTERN_STEPS = 100;          // Maximum steps per pattern
};

#endif // CUSTOMPATTERNDIALOG_H
