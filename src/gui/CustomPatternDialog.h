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
#include <QCheckBox>
#include <QSlider>
#include <QTableWidget>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QFormLayout>

// Forward declarations
class VacuumController;
class TouchButton;
class PatternEngine;

class CustomPatternDialog : public QDialog
{
    Q_OBJECT

public:
    struct PatternStep {
        double pressurePercent;
        int durationMs;
        QString action;
        QString description;
        QJsonObject parameters;
        
        PatternStep() : pressurePercent(0.0), durationMs(1000), action("vacuum") {}
        PatternStep(double pressure, int duration, const QString& act = "vacuum", const QString& desc = "")
            : pressurePercent(pressure), durationMs(duration), action(act), description(desc) {}
    };

    explicit CustomPatternDialog(VacuumController* controller, QWidget *parent = nullptr);
    ~CustomPatternDialog();

    void loadPattern(const QString& patternName);
    void createNewPattern();
    bool savePattern();
    bool savePatternAs();
    
    QJsonObject getPatternData() const;
    void setPatternData(const QJsonObject& data);
    QList<PatternStep> getPatternSteps() const;
    void setPatternSteps(const QList<PatternStep>& steps);

public Q_SLOTS:
    void previewPattern();
    void stopPreview();
    void testPattern();
    void validatePattern();

Q_SIGNALS:
    void patternCreated(const QString& patternName, const QJsonObject& patternData);
    void patternModified(const QString& patternName, const QJsonObject& patternData);

private Q_SLOTS:
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
    void onTabChanged(int index);
    void onPatternNameChanged();
    void onParameterChanged();
    void onStepSelectionChanged(int row);
    void onPreviewClicked();
    void onTestClicked();
    void onSaveClicked();
    void onLoadTemplateClicked();

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

    QJsonObject stepToJson(const PatternStep& step) const;
    PatternStep jsonToStep(const QJsonObject& json) const;

    void updateStepList();
    void updatePreview();
    void applyTouchOptimizedStyles();
    void initializeDefaultPattern();
    void addDefaultStep();

    void addPatternStep();
    void removePatternStep();
    void moveStepUp();
    void moveStepDown();
    void duplicateStep();
    void clearAllSteps();

    void loadTemplate(const QString& templateName);
    void exportPattern();
    void importPattern();

    VacuumController* m_controller;
    PatternEngine* m_patternEngine;
    
    QTabWidget* m_tabWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_buttonLayout;
    
    TouchButton* m_previewButton;
    TouchButton* m_testButton;
    TouchButton* m_validateButton;
    TouchButton* m_applyButton;
    TouchButton* m_cancelButton;
    TouchButton* m_okButton;
    TouchButton* m_saveButton;
    
    QWidget* m_basicInfoTab;
    QLineEdit* m_patternNameEdit;
    QComboBox* m_patternTypeCombo;
    QComboBox* m_patternSpeedCombo;
    QTextEdit* m_patternDescriptionEdit;
    QComboBox* m_patternCategoryCombo;
    QDoubleSpinBox* m_basePressureSpin;
    QDoubleSpinBox* m_speedSpin;
    QDoubleSpinBox* m_intensitySpin;
    
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
    QListWidget* m_stepsList;
    TouchButton* m_duplicateStepButton;
    TouchButton* m_clearStepsButton;
    
    QWidget* m_visualDesignerTab;
    QGraphicsView* m_designerView;
    QGraphicsScene* m_designerScene;
    QSlider* m_timeScaleSlider;
    QSlider* m_pressureScaleSlider;
    TouchButton* m_addPointButton;
    TouchButton* m_removePointButton;
    TouchButton* m_smoothButton;
    
    QWidget* m_previewTab;
    QLabel* m_previewChart;
    QGraphicsScene* m_previewScene;
    QLabel* m_previewStatusLabel;
    QLabel* m_patternDurationLabel;
    QLabel* m_patternStepsLabel;
    QSlider* m_previewPositionSlider;
    TouchButton* m_playButton;
    TouchButton* m_pauseButton;
    TouchButton* m_stopButton;
    QLabel* m_totalDurationLabel;
    QLabel* m_totalStepsLabel;
    QLabel* m_avgPressureLabel;
    QLabel* m_maxPressureLabel;
    
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
    
    QGroupBox* m_templatesGroup;
    QListWidget* m_templatesList;
    TouchButton* m_loadTemplateButton;
    TouchButton* m_saveTemplateButton;
    TouchButton* m_importButton;
    TouchButton* m_exportButton;
    TouchButton* m_resetButton;
    QComboBox* m_templateCombo;
    QTextEdit* m_validationResults;
    QCheckBox* m_loopPatternCheck;
    QSpinBox* m_loopCountSpin;
    QCheckBox* m_autoStartCheck;
    QComboBox* m_priorityCombo;
    
    QString m_currentPatternName;
    QList<PatternStep> m_patternSteps;
    QJsonObject m_patternMetadata;
    bool m_patternModified;
    
    QTimer* m_previewTimer;
    int m_previewPosition;
    bool m_previewPlaying;
    bool m_previewTesting;
    
    QList<QPointF> m_designerPoints;
    int m_selectedPoint;
    int m_currentTab;
    
    static const int DEFAULT_STEP_DURATION = 1000;
    static constexpr double DEFAULT_PRESSURE = 50.0;
    static const int MIN_STEP_DURATION = 100;
    static const int MAX_STEP_DURATION = 60000;
    static constexpr double MIN_PRESSURE = 0.0;
    static constexpr double MAX_PRESSURE = 100.0;
    static const int PREVIEW_UPDATE_INTERVAL = 100;
    static const int MAX_PATTERN_STEPS = 100;
    static const int SPACING_NORMAL = 10;
    static const int BUTTON_MIN_WIDTH = 150;
    static const int BUTTON_MIN_HEIGHT = 40;
};

#endif // CUSTOMPATTERNDIALOG_H