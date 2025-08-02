#ifndef PATTERNVALIDATOR_H
#define PATTERNVALIDATOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

/**
 * @brief Pattern validation and safety checking system
 * 
 * This system provides:
 * - Comprehensive pattern safety validation
 * - Parameter range checking
 * - Timing constraint validation
 * - Pressure gradient analysis
 * - Safety limit enforcement
 * - Pattern complexity analysis
 * - Performance impact assessment
 */
class PatternValidator : public QObject
{
    Q_OBJECT

public:
    enum ValidationLevel {
        BASIC,          // Basic parameter validation
        SAFETY,         // Safety-focused validation
        PERFORMANCE,    // Performance impact validation
        COMPREHENSIVE   // All validation checks
    };

    enum ValidationResult {
        VALID,          // Pattern is valid
        WARNING,        // Pattern has warnings but is usable
        ERROR,          // Pattern has errors and should not be used
        CRITICAL        // Pattern has critical safety issues
    };

    struct ValidationIssue {
        ValidationResult severity;
        QString category;
        QString message;
        QString suggestion;
        QJsonObject context;
        
        ValidationIssue() : severity(VALID) {}
        ValidationIssue(ValidationResult sev, const QString& cat, const QString& msg, 
                       const QString& sug = QString(), const QJsonObject& ctx = QJsonObject())
            : severity(sev), category(cat), message(msg), suggestion(sug), context(ctx) {}
    };

    struct ValidationReport {
        ValidationResult overallResult;
        QList<ValidationIssue> issues;
        QJsonObject statistics;
        QStringList recommendations;
        bool isSafeForExecution;
        
        ValidationReport() : overallResult(VALID), isSafeForExecution(true) {}
    };

    explicit PatternValidator(QObject *parent = nullptr);
    ~PatternValidator();

    // Validation methods
    ValidationReport validatePattern(const QJsonObject& patternData, ValidationLevel level = COMPREHENSIVE);
    ValidationReport validatePatternSteps(const QJsonArray& steps, ValidationLevel level = COMPREHENSIVE);
    ValidationReport validatePatternParameters(const QJsonObject& parameters, ValidationLevel level = COMPREHENSIVE);
    
    // Quick validation
    bool isPatternSafe(const QJsonObject& patternData);
    bool areStepsSafe(const QJsonArray& steps);
    bool areParametersSafe(const QJsonObject& parameters);
    
    // Configuration
    void setSafetyLimits(double maxPressure, double maxDuration, double maxGradient);
    void setPerformanceLimits(int maxSteps, int maxTotalDuration, double maxComplexity);
    void setValidationLevel(ValidationLevel level);
    
    // Safety limits
    double getMaxPressure() const { return m_maxPressure; }
    double getMaxDuration() const { return m_maxDuration; }
    double getMaxPressureGradient() const { return m_maxPressureGradient; }
    
    // Validation statistics
    QJsonObject getValidationStatistics(const QJsonObject& patternData);
    double calculatePatternComplexity(const QJsonArray& steps);
    double calculatePressureVariability(const QJsonArray& steps);
    int calculateTotalDuration(const QJsonArray& steps);

signals:
    void validationCompleted(const ValidationReport& report);
    void criticalIssueFound(const ValidationIssue& issue);

private:
    // Validation methods
    void validateBasicParameters(const QJsonObject& parameters, ValidationReport& report);
    void validateStepParameters(const QJsonObject& step, int stepIndex, ValidationReport& report);
    void validatePressureLimits(const QJsonArray& steps, ValidationReport& report);
    void validateTimingConstraints(const QJsonArray& steps, ValidationReport& report);
    void validatePressureGradients(const QJsonArray& steps, ValidationReport& report);
    void validateSafetyConstraints(const QJsonArray& steps, ValidationReport& report);
    void validatePerformanceImpact(const QJsonArray& steps, ValidationReport& report);
    void validatePatternCoherence(const QJsonObject& patternData, ValidationReport& report);
    
    // Analysis methods
    double calculateStepComplexity(const QJsonObject& step);
    double calculatePressureGradient(const QJsonObject& step1, const QJsonObject& step2);
    bool isValidAction(const QString& action);
    bool isValidPressureRange(double pressure);
    bool isValidDurationRange(int duration);
    
    // Utility methods
    void addIssue(ValidationReport& report, ValidationResult severity, const QString& category,
                 const QString& message, const QString& suggestion = QString(),
                 const QJsonObject& context = QJsonObject());
    ValidationResult determineOverallResult(const QList<ValidationIssue>& issues);
    QJsonObject generateStatistics(const QJsonArray& steps);
    QStringList generateRecommendations(const QList<ValidationIssue>& issues);
    
    // Configuration
    ValidationLevel m_validationLevel;
    
    // Safety limits
    double m_maxPressure;           // Maximum pressure (%)
    double m_maxDuration;           // Maximum step duration (ms)
    double m_maxPressureGradient;   // Maximum pressure change rate (%/s)
    double m_minPressure;           // Minimum pressure (%)
    double m_minDuration;           // Minimum step duration (ms)
    
    // Performance limits
    int m_maxSteps;                 // Maximum number of steps
    int m_maxTotalDuration;         // Maximum total pattern duration (ms)
    double m_maxComplexity;         // Maximum pattern complexity score
    
    // Validation thresholds
    double m_warningPressureThreshold;     // Pressure warning threshold
    double m_warningDurationThreshold;     // Duration warning threshold
    double m_warningGradientThreshold;     // Gradient warning threshold
    
    // Constants
    static constexpr double DEFAULT_MAX_PRESSURE = 90.0;          // 90.0%
    static constexpr double DEFAULT_MIN_PRESSURE = 0.0;          // 0.0%
    static constexpr int DEFAULT_MAX_DURATION = 60000;             // 60000ms (1 minute)
    static constexpr int DEFAULT_MIN_DURATION = 100;             // 100ms
    static constexpr double DEFAULT_MAX_GRADIENT = 50.0;          // 50.0%/s
    static constexpr int DEFAULT_MAX_STEPS = 100;                // 100 steps
    static constexpr int DEFAULT_MAX_TOTAL_DURATION = 3600000;       // 3600000ms (1 hour)
    static constexpr double DEFAULT_MAX_COMPLEXITY = 10.0;        // 10.0
    static constexpr double WARNING_PRESSURE_THRESHOLD = 80.0;    // 80.0%
    static constexpr int WARNING_DURATION_THRESHOLD = 30000;       // 30000ms (30 seconds)
    static constexpr double WARNING_GRADIENT_THRESHOLD = 30.0;    // 30.0%/s
    
    static const QStringList VALID_ACTIONS;
    static const QStringList REQUIRED_STEP_FIELDS;
    static const QStringList REQUIRED_PATTERN_FIELDS;
};

#endif // PATTERNVALIDATOR_H
