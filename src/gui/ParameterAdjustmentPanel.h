#ifndef PARAMETERADJUSTMENTPANEL_H
#define PARAMETERADJUSTMENTPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDial>
#include <QProgressBar>
#include <QTimer>
#include <QJsonObject>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

// Forward declarations
class VacuumController;
class PatternEngine;
class TouchButton;

/**
 * @brief Real-time parameter adjustment controls
 * 
 * This widget provides comprehensive real-time control over pattern parameters:
 * - Intensity adjustment (0-100%)
 * - Speed/timing control (0.1x to 3.0x)
 * - Pressure offset adjustment (-20% to +20%)
 * - Pulse duration control
 * - Pattern-specific parameters
 * - Visual feedback and safety limits
 * - Touch-optimized controls for 50-inch displays
 */
class ParameterAdjustmentPanel : public QWidget
{
    Q_OBJECT

public:
    struct ParameterLimits {
        double minIntensity = 0.0;
        double maxIntensity = 100.0;
        double minSpeed = 0.1;
        double maxSpeed = 3.0;
        double minPressureOffset = -20.0;
        double maxPressureOffset = 20.0;
        int minPulseDuration = 100;
        int maxPulseDuration = 10000;
        double safetyLimit = 90.0;
        
        ParameterLimits() = default;
    };

    explicit ParameterAdjustmentPanel(VacuumController* controller, QWidget *parent = nullptr);
    ~ParameterAdjustmentPanel();

    // Parameter access
    double getIntensity() const { return m_currentIntensity; }
    double getSpeed() const { return m_currentSpeed; }
    double getPressureOffset() const { return m_currentPressureOffset; }
    int getPulseDuration() const { return m_currentPulseDuration; }
    QJsonObject getAllParameters() const;
    
    // Parameter control
    void setIntensity(double intensity);
    void setSpeed(double speed);
    void setPressureOffset(double offset);
    void setPulseDuration(int duration);
    void setParameters(const QJsonObject& parameters);
    
    // Safety and limits
    void setParameterLimits(const ParameterLimits& limits);
    ParameterLimits getParameterLimits() const { return m_limits; }
    void setSafetyMode(bool enabled);
    bool isSafetyMode() const { return m_safetyMode; }
    
    // Pattern-specific controls
    void setPatternType(const QString& patternType);
    void enablePatternSpecificControls(bool enabled);
    void resetToDefaults();

public slots:
    void onPatternChanged(const QString& patternName);
    void onPatternStarted();
    void onPatternStopped();
    void updateRealTimeValues();

signals:
    void intensityChanged(double intensity);
    void speedChanged(double speed);
    void pressureOffsetChanged(double offset);
    void pulseDurationChanged(int duration);
    void parametersChanged(const QJsonObject& parameters);
    void safetyLimitExceeded(const QString& parameter, double value);

private slots:
    void onIntensitySliderChanged(int value);
    void onSpeedSliderChanged(int value);
    void onPressureOffsetSliderChanged(int value);
    void onPulseDurationChanged(int value);
    void onResetButtonClicked();
    void onSafetyModeToggled(bool enabled);
    void onPresetButtonClicked();
    void onRealTimeUpdateTimer();

private:
    void setupUI();
    void setupIntensityControls();
    void setupSpeedControls();
    void setupPressureControls();
    void setupTimingControls();
    void setupPatternSpecificControls();
    void setupPresetControls();
    void setupSafetyControls();
    void connectSignals();
    
    void updateParameterDisplay();
    void updateSafetyIndicators();
    void animateParameterChange(QWidget* control);
    void validateParameters();
    void applyParameterLimits();
    
    // UI creation helpers
    QWidget* createParameterGroup(const QString& title, QWidget* control, const QString& unit = QString());
    QSlider* createTouchSlider(Qt::Orientation orientation = Qt::Horizontal);
    QLabel* createValueLabel(const QString& initialText = "0");
    TouchButton* createPresetButton(const QString& text, const QJsonObject& preset);
    
    // Controller interface
    VacuumController* m_controller;
    PatternEngine* m_patternEngine;
    
    // Main layout
    QVBoxLayout* m_mainLayout;
    
    // Control groups
    QGroupBox* m_intensityGroup;
    QGroupBox* m_speedGroup;
    QGroupBox* m_pressureGroup;
    QGroupBox* m_timingGroup;
    QGroupBox* m_patternSpecificGroup;
    QGroupBox* m_presetGroup;
    QGroupBox* m_safetyGroup;
    
    // Intensity controls
    QSlider* m_intensitySlider;
    QDial* m_intensityDial;
    QLabel* m_intensityValueLabel;
    QProgressBar* m_intensityProgressBar;
    
    // Speed controls
    QSlider* m_speedSlider;
    QLabel* m_speedValueLabel;
    QLabel* m_speedMultiplierLabel;
    
    // Pressure controls
    QSlider* m_pressureOffsetSlider;
    QLabel* m_pressureOffsetValueLabel;
    QLabel* m_pressureRangeLabel;
    
    // Timing controls
    QSpinBox* m_pulseDurationSpin;
    QSlider* m_pulseDurationSlider;
    QLabel* m_pulseDurationValueLabel;
    
    // Pattern-specific controls
    QWidget* m_patternSpecificWidget;
    QVBoxLayout* m_patternSpecificLayout;
    QMap<QString, QWidget*> m_patternControls;
    
    // Preset controls
    TouchButton* m_gentlePresetButton;
    TouchButton* m_moderatePresetButton;
    TouchButton* m_intensePresetButton;
    TouchButton* m_customPresetButton;
    TouchButton* m_resetButton;
    
    // Safety controls
    QLabel* m_safetyStatusLabel;
    TouchButton* m_safetyModeButton;
    QProgressBar* m_safetyLimitBar;
    
    // Current parameter values
    double m_currentIntensity;
    double m_currentSpeed;
    double m_currentPressureOffset;
    int m_currentPulseDuration;
    QString m_currentPatternType;
    
    // Configuration
    ParameterLimits m_limits;
    bool m_safetyMode;
    bool m_realTimeUpdates;
    bool m_patternRunning;
    
    // Real-time updates
    QTimer* m_updateTimer;
    
    // Animation effects
    QPropertyAnimation* m_parameterAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    
    // Preset configurations
    QMap<QString, QJsonObject> m_presetConfigurations;
    
    // Constants
    static const int SLIDER_RESOLUTION = 1000;
    static const int UPDATE_INTERVAL_MS = 100;
    static const int ANIMATION_DURATION_MS = 300;
    static constexpr double DEFAULT_INTENSITY = 70.0;
    static constexpr double DEFAULT_SPEED = 1.0;
    static constexpr double DEFAULT_PRESSURE_OFFSET = 0.0;
    static const int DEFAULT_PULSE_DURATION = 1000;
};

#endif // PARAMETERADJUSTMENTPANEL_H
