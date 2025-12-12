#ifndef EXECUTIONMODESELECTOR_H
#define EXECUTIONMODESELECTOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QStackedWidget>
#include <QJsonObject>
#include <memory>

// Forward declarations
class VacuumController;
class OrgasmControlAlgorithm;
class TouchButton;
class QAbstractButton;

/**
 * @brief Execution mode selection widget for OrgasmControlAlgorithm
 * 
 * This widget provides mode selection for the orgasm control algorithm:
 * - 6 execution modes: MANUAL, ADAPTIVE_EDGING, FORCED_ORGASM, MULTI_ORGASM, DENIAL, MILKING
 * - Mode-specific parameter configuration dialogs
 * - Real-time mode switching with parameter validation
 * - Touch-optimized interface for 50-inch displays
 */
class ExecutionModeSelector : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Execution modes matching OrgasmControlAlgorithm::Mode
     */
    enum class Mode {
        Manual = 0,
        AdaptiveEdging = 1,
        ForcedOrgasm = 2,
        MultiOrgasm = 3,
        Denial = 4,
        Milking = 5
    };
    Q_ENUM(Mode)

    explicit ExecutionModeSelector(VacuumController* controller, QWidget *parent = nullptr);
    ~ExecutionModeSelector();

    // Mode selection
    Mode getSelectedMode() const { return m_selectedMode; }
    QString getSelectedModeName() const;
    
    // Parameter access
    QJsonObject getSessionParameters() const;
    void setSessionParameters(const QJsonObject& parameters);

public Q_SLOTS:
    void selectMode(Mode mode);
    void resetToDefaults();
    void onAlgorithmStateChanged();

Q_SIGNALS:
    void modeSelected(Mode mode);
    void sessionStartRequested(Mode mode, const QJsonObject& parameters);
    void sessionStopRequested();
    void parametersChanged(const QJsonObject& parameters);

private Q_SLOTS:
    void onModeButtonClicked(QAbstractButton* button);
    void onParameterChanged();
    void onStartClicked();
    void onStopClicked();

private:
    void setupUI();
    void setupModeButtons();
    void setupParameterPanels();
    void setupControlButtons();
    
    void updateParameterPanel();
    void updateControlButtons();
    void highlightSelectedMode();
    
    TouchButton* createModeButton(Mode mode, const QString& name, const QString& description);
    QWidget* createManualPanel();
    QWidget* createAdaptiveEdgingPanel();
    QWidget* createForcedOrgasmPanel();
    QWidget* createMultiOrgasmPanel();
    QWidget* createDenialPanel();
    QWidget* createMilkingPanel();
    
    // Controller interface
    VacuumController* m_controller;
    OrgasmControlAlgorithm* m_algorithm;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Mode selection
    QGroupBox* m_modeGroup;
    QGridLayout* m_modeGrid;
    QButtonGroup* m_modeButtonGroup;
    QMap<Mode, TouchButton*> m_modeButtons;
    
    // Parameter panels (stacked for each mode)
    QGroupBox* m_parameterGroup;
    QStackedWidget* m_parameterStack;
    QMap<Mode, QWidget*> m_parameterPanels;
    
    // Mode-specific parameter controls
    // Adaptive Edging
    QSpinBox* m_targetCyclesSpinBox;
    
    // Forced Orgasm
    QSpinBox* m_targetOrgasmsSpinBox;
    QSpinBox* m_maxDurationSpinBox;
    
    // Multi-Orgasm
    QSpinBox* m_multiOrgasmTargetSpinBox;
    QSpinBox* m_recoveryTimeSpinBox;
    
    // Denial
    QSpinBox* m_denialDurationSpinBox;
    
    // Milking
    QSpinBox* m_milkingDurationSpinBox;
    QComboBox* m_failureModeCombo;
    
    // Control buttons
    QGroupBox* m_controlGroup;
    TouchButton* m_startButton;
    TouchButton* m_stopButton;
    
    // State
    Mode m_selectedMode;
    bool m_sessionActive;
    
    // Constants for touch interface
    static const int MODE_BUTTON_WIDTH = 180;
    static const int MODE_BUTTON_HEIGHT = 100;
    static const int GRID_COLUMNS = 3;
};

#endif // EXECUTIONMODESELECTOR_H

