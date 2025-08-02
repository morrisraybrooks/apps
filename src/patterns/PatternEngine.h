#ifndef PATTERNENGINE_H
#define PATTERNENGINE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStateMachine>
#include <QState>
#include <QFinalState>
#include <memory>

// Forward declarations
class HardwareManager;
class PatternDefinitions;

/**
 * @brief Pattern execution engine for vacuum controller
 * 
 * This engine executes the 15+ vacuum patterns with precise timing control:
 * - Pulse patterns (Slow, Medium, Fast)
 * - Wave patterns with gradual pressure changes
 * - Air pulse patterns with release phases
 * - Milking patterns with rhythmic motion
 * - Constant patterns with variations
 * - Special patterns like Edging
 * 
 * Features:
 * - State machine-based execution
 * - Real-time parameter adjustment
 * - Precise timing control
 * - Safety integration
 * - Pattern interpolation and smoothing
 */
class PatternEngine : public QObject
{
    Q_OBJECT

public:
    enum PatternState {
        STOPPED,
        STARTING,
        RUNNING,
        PAUSED,
        STOPPING,
        ERROR
    };
    Q_ENUM(PatternState)

    enum PatternType {
        PULSE,
        WAVE,
        AIR_PULSE,
        MILKING,
        CONSTANT,
        EDGING,
        CUSTOM
    };
    Q_ENUM(PatternType)

    struct PatternStep {
        double pressurePercent;
        int durationMs;
        QString action;  // "vacuum", "release", "hold"
        QJsonObject parameters;
        
        PatternStep() : pressurePercent(0.0), durationMs(0) {}
        PatternStep(double pressure, int duration, const QString& act = "vacuum")
            : pressurePercent(pressure), durationMs(duration), action(act) {}
    };

    explicit PatternEngine(HardwareManager* hardware, QObject *parent = nullptr);
    ~PatternEngine();

    // Pattern control
    bool startPattern(const QString& patternName, const QJsonObject& parameters);
    void stopPattern();
    void pausePattern();
    void resumePattern();
    void emergencyStop();
    
    // Pattern management
    bool loadPattern(const QString& patternName, const QJsonObject& patternData);
    bool loadPatternsFromFile(const QString& filePath);
    QStringList getAvailablePatterns() const;
    
    // Current pattern info
    QString getCurrentPattern() const { return m_currentPatternName; }
    PatternState getState() const { return m_state; }
    PatternType getCurrentPatternType() const { return m_currentPatternType; }
    
    // Execution status
    int getCurrentStep() const { return m_currentStep; }
    int getTotalSteps() const { return m_patternSteps.size(); }
    double getProgress() const;
    qint64 getElapsedTime() const;
    qint64 getRemainingTime() const;
    
    // Real-time parameter adjustment
    void setIntensity(double intensityPercent);  // 0-100%
    void setSpeed(double speedMultiplier);       // 0.1-3.0
    void setPressureOffset(double offsetPercent); // -20 to +20%
    
    double getIntensity() const { return m_intensity; }
    double getSpeed() const { return m_speedMultiplier; }
    double getPressureOffset() const { return m_pressureOffset; }
    
    // Safety limits
    void setMaxPressure(double maxPressure);
    void setSafetyLimits(double minPressure, double maxPressure);
    
    // Pattern creation
    bool createCustomPattern(const QString& name, const QList<PatternStep>& steps);
    bool savePattern(const QString& patternName, const QString& filePath);

public slots:
    void setPatternParameters(const QJsonObject& parameters);

signals:
    void patternStarted(const QString& patternName);
    void patternStopped();
    void patternPaused();
    void patternResumed();
    void patternCompleted();
    void patternError(const QString& error);
    void stateChanged(PatternState newState);
    void stepChanged(int currentStep, int totalSteps);
    void progressUpdated(double progress);
    void pressureTargetChanged(double targetPressure);

private slots:
    void executeNextStep();
    void onStepTimer();
    void onSafetyCheck();

private:
    void setState(PatternState newState);
    bool initializePattern(const QString& patternName, const QJsonObject& parameters);
    void buildPatternSteps(const QJsonObject& patternData);
    void executeStep(const PatternStep& step);
    void applyPressureTarget(double targetPressure);
    void performSafetyCheck();
    
    // Pattern type handlers
    void buildPulsePattern(const QJsonObject& params);
    void buildWavePattern(const QJsonObject& params);
    void buildAirPulsePattern(const QJsonObject& params);
    void buildMilkingPattern(const QJsonObject& params);
    void buildConstantPattern(const QJsonObject& params);
    void buildEdgingPattern(const QJsonObject& params);
    
    // Utility functions
    double applyIntensityAndOffset(double basePressure);
    int applySpeedMultiplier(int baseDuration);
    double interpolatePressure(double startPressure, double endPressure, double progress);
    bool validatePatternData(const QJsonObject& patternData);
    
    // Hardware interface
    HardwareManager* m_hardware;
    
    // Pattern execution state
    PatternState m_state;
    QString m_currentPatternName;
    PatternType m_currentPatternType;
    QList<PatternStep> m_patternSteps;
    int m_currentStep;
    qint64 m_patternStartTime;
    qint64 m_stepStartTime;
    qint64 m_pausedTime;
    qint64 m_totalPausedTime;
    
    // Execution control
    QTimer* m_stepTimer;
    QTimer* m_safetyTimer;
    bool m_emergencyStop;
    
    // Real-time adjustments
    double m_intensity;          // 0-100%
    double m_speedMultiplier;    // 0.1-3.0
    double m_pressureOffset;     // -20 to +20%
    
    // Safety limits
    double m_minPressure;
    double m_maxPressure;
    
    // Pattern storage
    QMap<QString, QJsonObject> m_loadedPatterns;
    std::unique_ptr<PatternDefinitions> m_patternDefinitions;
    
    // Thread safety
    mutable QMutex m_stateMutex;
    
    // Constants
    static const int SAFETY_CHECK_INTERVAL_MS = 100;  // 10Hz safety checks
    static const double DEFAULT_INTENSITY;            // 100%
    static const double DEFAULT_SPEED_MULTIPLIER;     // 1.0
    static const double DEFAULT_PRESSURE_OFFSET;      // 0%
    static const double MIN_PRESSURE_PERCENT;         // 10%
    static const double MAX_PRESSURE_PERCENT;         // 90%
    static const int MIN_STEP_DURATION_MS;            // 100ms
    static const int MAX_STEP_DURATION_MS;            // 60000ms (1 minute)
};

#endif // PATTERNENGINE_H
