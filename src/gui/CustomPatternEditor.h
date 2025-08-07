#ifndef CUSTOMPATTERNEDITOR_H
#define CUSTOMPATTERNEDITOR_H

#include <QWidget>
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

class CustomPatternEditor : public QWidget
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

    explicit CustomPatternEditor(VacuumController* controller, QWidget *parent = nullptr);
    ~CustomPatternEditor();

    void loadPattern(const QString& patternName);
    void createNewPattern();
    bool savePattern();
    bool savePatternAs();

    QJsonObject getPatternData() const;
    void setPatternData(const QJsonObject& data);
    QList<PatternStep> getPatternSteps() const;
    void setPatternSteps(const QList<PatternStep>& steps);

    // Editor control methods
    void showEditor();
    void hideEditor();
    void resetEditor();

public Q_SLOTS:
    void previewPattern();
    void stopPreview();
    void testPattern();
    void validatePattern();

Q_SIGNALS:
    void patternCreated(const QString& patternName, const QJsonObject& patternData);
    void patternModified(const QString& patternName, const QJsonObject& patternData);
    void editorClosed();
    void backToPatternSelector();

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
    void onSaveClicked();
    void onBackClicked();
    void onResetClicked();
    void onTabChanged(int index);
    void onPatternNameChanged();
    void onParameterChanged();
    void onStepSelectionChanged(int row);
    void onPreviewClicked();
    void onTestClicked();
    void onLoadTemplateClicked();

    // Edging pattern control slots
    void onEdgingParameterChanged();
    void onBuildupParameterChanged();
    void onPeakParameterChanged();
    void onCooldownParameterChanged();
    void onCycleParameterChanged();
    void onSensitivityParameterChanged();
    void onIntensityCurveChanged();
    void onCurvePreviewClicked();
    void onResetCurveClicked();
    void onAutoDetectionToggled(bool enabled);

private:
    void setupUI();
    void setupBasicInfoTab();
    void setupStepEditorTab();
    void setupVisualDesignerTab();
    void setupPreviewTab();
    void setupAdvancedTab();
    void setupEdgingTab();
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

    // Edging pattern helper methods
    void updateEdgingControls();
    void syncEdgingSliders();
    void generateEdgingSteps();
    void previewIntensityCurve();
    void validateEdgingParameters();
    QJsonObject getEdgingParameters() const;
    void setEdgingParameters(const QJsonObject& params);
    void resetEdgingToDefaults();
    void enableEdgingControls(bool enabled);
    double calculateIntensityCurve(double progress, const QString& curveType, double exponent) const;

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

    // Enhanced Edging Pattern Controls
    QWidget* m_edgingTab;
    QGroupBox* m_edgingControlsGroup;
    QGroupBox* m_buildupGroup;
    QGroupBox* m_peakGroup;
    QGroupBox* m_cooldownGroup;
    QGroupBox* m_cycleGroup;
    QGroupBox* m_sensitivityGroup;
    QGroupBox* m_intensityCurveGroup;

    // Build-up controls
    QSlider* m_buildupIntensitySlider;
    QDoubleSpinBox* m_buildupIntensitySpin;
    QSlider* m_buildupDurationSlider;
    QSpinBox* m_buildupDurationSpin;
    QComboBox* m_buildupCurveCombo;
    QCheckBox* m_gradualBuildupCheck;
    QSlider* m_buildupStepsSlider;
    QSpinBox* m_buildupStepsSpin;

    // Peak/Hold controls
    QSlider* m_peakIntensitySlider;
    QDoubleSpinBox* m_peakIntensitySpin;
    QSlider* m_holdDurationSlider;
    QSpinBox* m_holdDurationSpin;
    QCheckBox* m_variablePeakCheck;
    QSlider* m_peakVariationSlider;
    QDoubleSpinBox* m_peakVariationSpin;

    // Cooldown/Release controls
    QSlider* m_cooldownDurationSlider;
    QSpinBox* m_cooldownDurationSpin;
    QComboBox* m_cooldownCurveCombo;
    QCheckBox* m_completeCooldownCheck;
    QSlider* m_cooldownIntensitySlider;
    QDoubleSpinBox* m_cooldownIntensitySpin;

    // Cycle controls
    QSpinBox* m_edgeCyclesSpin;
    QCheckBox* m_infiniteCyclesCheck;
    QSlider* m_cycleDelaySlider;
    QSpinBox* m_cycleDelaySpin;
    QCheckBox* m_increasingIntensityCheck;
    QSlider* m_intensityIncrementSlider;
    QDoubleSpinBox* m_intensityIncrementSpin;

    // Sensitivity/Auto-detection controls
    QCheckBox* m_autoEdgeDetectionCheck;
    QSlider* m_sensitivityThresholdSlider;
    QDoubleSpinBox* m_sensitivityThresholdSpin;
    QSlider* m_detectionWindowSlider;
    QSpinBox* m_detectionWindowSpin;
    QCheckBox* m_adaptiveSensitivityCheck;
    QSlider* m_responseTimeSlider;
    QSpinBox* m_responseTimeSpin;

    // Intensity curve controls
    QComboBox* m_intensityCurveTypeCombo;
    QSlider* m_curveExponentSlider;
    QDoubleSpinBox* m_curveExponentSpin;
    QCheckBox* m_customCurveCheck;
    QGraphicsView* m_curvePreviewView;
    QGraphicsScene* m_curvePreviewScene;
    TouchButton* m_resetCurveButton;
    TouchButton* m_previewCurveButton;

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

#endif // CUSTOMPATTERNEDITOR_H