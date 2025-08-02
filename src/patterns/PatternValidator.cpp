#include "PatternValidator.h"
#include <QDebug>
#include <QJsonDocument>
#include <cmath>

// Constants are now defined as constexpr in the header

const QStringList PatternValidator::VALID_ACTIONS = {
    "pressure", "pause", "ramp", "hold", "release"
};

const QStringList PatternValidator::REQUIRED_STEP_FIELDS = {
    "action", "duration", "pressure"
};

const QStringList PatternValidator::REQUIRED_PATTERN_FIELDS = {
    "name", "type", "steps"
};

PatternValidator::PatternValidator(QObject *parent)
    : QObject(parent)
    , m_validationLevel(COMPREHENSIVE)
    , m_maxPressure(DEFAULT_MAX_PRESSURE)
    , m_maxDuration(DEFAULT_MAX_DURATION)
    , m_maxPressureGradient(DEFAULT_MAX_GRADIENT)
    , m_minPressure(DEFAULT_MIN_PRESSURE)
    , m_minDuration(DEFAULT_MIN_DURATION)
    , m_maxSteps(DEFAULT_MAX_STEPS)
    , m_maxTotalDuration(DEFAULT_MAX_TOTAL_DURATION)
    , m_maxComplexity(DEFAULT_MAX_COMPLEXITY)
    , m_warningPressureThreshold(WARNING_PRESSURE_THRESHOLD)
    , m_warningDurationThreshold(WARNING_DURATION_THRESHOLD)
    , m_warningGradientThreshold(WARNING_GRADIENT_THRESHOLD)
{
    qDebug() << "PatternValidator initialized with safety limits";
}

PatternValidator::~PatternValidator()
{
}

PatternValidator::ValidationReport PatternValidator::validatePattern(const QJsonObject& patternData, ValidationLevel level)
{
    ValidationReport report;
    report.overallResult = VALID;
    report.isSafeForExecution = true;
    
    // Check required fields
    for (const QString& field : REQUIRED_PATTERN_FIELDS) {
        if (!patternData.contains(field)) {
            addIssue(report, CRITICAL, "Missing Field", 
                    QString("Required field '%1' is missing").arg(field),
                    QString("Add the required field '%1' to the pattern").arg(field));
        }
    }
    
    // Validate pattern parameters
    if (patternData.contains("parameters")) {
        validateBasicParameters(patternData["parameters"].toObject(), report);
    }
    
    // Validate steps
    if (patternData.contains("steps")) {
        QJsonArray steps = patternData["steps"].toArray();
        
        if (level >= SAFETY) {
            validatePressureLimits(steps, report);
            validateTimingConstraints(steps, report);
            validateSafetyConstraints(steps, report);
        }
        
        if (level >= PERFORMANCE) {
            validatePerformanceImpact(steps, report);
        }
        
        if (level >= COMPREHENSIVE) {
            validatePressureGradients(steps, report);
            validatePatternCoherence(patternData, report);
        }
    }
    
    // Determine overall result
    report.overallResult = determineOverallResult(report.issues);
    report.isSafeForExecution = (report.overallResult != CRITICAL && report.overallResult != ERROR);
    
    // Generate statistics and recommendations
    if (patternData.contains("steps")) {
        report.statistics = generateStatistics(patternData["steps"].toArray());
    }
    report.recommendations = generateRecommendations(report.issues);
    
    emit validationCompleted(report);
    
    return report;
}

bool PatternValidator::isPatternSafe(const QJsonObject& patternData)
{
    ValidationReport report = validatePattern(patternData, SAFETY);
    return report.isSafeForExecution;
}

bool PatternValidator::areStepsSafe(const QJsonArray& steps)
{
    ValidationReport report;
    validatePressureLimits(steps, report);
    validateTimingConstraints(steps, report);
    validateSafetyConstraints(steps, report);
    
    ValidationResult result = determineOverallResult(report.issues);
    return (result != CRITICAL && result != ERROR);
}

void PatternValidator::validatePressureLimits(const QJsonArray& steps, ValidationReport& report)
{
    for (int i = 0; i < steps.size(); ++i) {
        QJsonObject step = steps[i].toObject();
        
        if (step.contains("pressure")) {
            double pressure = step["pressure"].toDouble();
            
            if (pressure < m_minPressure) {
                addIssue(report, ERROR, "Pressure Limit", 
                        QString("Step %1: Pressure %2% is below minimum %3%")
                        .arg(i+1).arg(pressure).arg(m_minPressure),
                        QString("Increase pressure to at least %1%").arg(m_minPressure));
            }
            
            if (pressure > m_maxPressure) {
                addIssue(report, CRITICAL, "Pressure Limit", 
                        QString("Step %1: Pressure %2% exceeds maximum %3%")
                        .arg(i+1).arg(pressure).arg(m_maxPressure),
                        QString("Reduce pressure to maximum %1%").arg(m_maxPressure));
            }
            
            if (pressure > m_warningPressureThreshold) {
                addIssue(report, WARNING, "Pressure Warning", 
                        QString("Step %1: High pressure %2% detected")
                        .arg(i+1).arg(pressure),
                        "Consider reducing pressure for safety");
            }
        }
    }
}

void PatternValidator::validateTimingConstraints(const QJsonArray& steps, ValidationReport& report)
{
    int totalDuration = 0;
    
    for (int i = 0; i < steps.size(); ++i) {
        QJsonObject step = steps[i].toObject();
        
        if (step.contains("duration")) {
            int duration = step["duration"].toInt();
            totalDuration += duration;
            
            if (duration < m_minDuration) {
                addIssue(report, ERROR, "Timing Constraint", 
                        QString("Step %1: Duration %2ms is below minimum %3ms")
                        .arg(i+1).arg(duration).arg(m_minDuration),
                        QString("Increase duration to at least %1ms").arg(m_minDuration));
            }
            
            if (duration > m_maxDuration) {
                addIssue(report, WARNING, "Timing Constraint", 
                        QString("Step %1: Long duration %2ms detected")
                        .arg(i+1).arg(duration),
                        "Consider breaking into shorter steps");
            }
        }
    }
    
    if (totalDuration > m_maxTotalDuration) {
        addIssue(report, WARNING, "Total Duration", 
                QString("Total pattern duration %1ms exceeds recommended maximum %2ms")
                .arg(totalDuration).arg(m_maxTotalDuration),
                "Consider shortening the pattern");
    }
}

void PatternValidator::validateSafetyConstraints(const QJsonArray& steps, ValidationReport& report)
{
    // Check for rapid pressure changes that could be unsafe
    for (int i = 1; i < steps.size(); ++i) {
        QJsonObject prevStep = steps[i-1].toObject();
        QJsonObject currStep = steps[i].toObject();
        
        if (prevStep.contains("pressure") && currStep.contains("pressure")) {
            double prevPressure = prevStep["pressure"].toDouble();
            double currPressure = currStep["pressure"].toDouble();
            double pressureChange = std::abs(currPressure - prevPressure);
            
            if (pressureChange > 50.0) { // More than 50% change
                addIssue(report, WARNING, "Safety Constraint", 
                        QString("Steps %1-%2: Large pressure change %3%")
                        .arg(i).arg(i+1).arg(pressureChange),
                        "Consider adding intermediate steps for gradual change");
            }
        }
    }
    
    // Check for too many steps
    if (steps.size() > m_maxSteps) {
        addIssue(report, WARNING, "Complexity", 
                QString("Pattern has %1 steps, exceeding recommended maximum %2")
                .arg(steps.size()).arg(m_maxSteps),
                "Consider simplifying the pattern");
    }
}

void PatternValidator::addIssue(ValidationReport& report, ValidationResult severity, 
                               const QString& category, const QString& message, 
                               const QString& suggestion, const QJsonObject& context)
{
    ValidationIssue issue(severity, category, message, suggestion, context);
    report.issues.append(issue);
    
    if (severity == CRITICAL) {
        emit criticalIssueFound(issue);
    }
}

PatternValidator::ValidationResult PatternValidator::determineOverallResult(const QList<ValidationIssue>& issues)
{
    ValidationResult worst = VALID;
    
    for (const auto& issue : issues) {
        if (issue.severity > worst) {
            worst = issue.severity;
        }
    }
    
    return worst;
}

QJsonObject PatternValidator::generateStatistics(const QJsonArray& steps)
{
    QJsonObject stats;
    
    stats["stepCount"] = steps.size();
    stats["totalDuration"] = calculateTotalDuration(steps);
    stats["complexity"] = calculatePatternComplexity(steps);
    stats["pressureVariability"] = calculatePressureVariability(steps);
    
    return stats;
}

double PatternValidator::calculatePatternComplexity(const QJsonArray& steps)
{
    if (steps.isEmpty()) return 0.0;
    
    double complexity = 0.0;
    
    // Base complexity from number of steps
    complexity += steps.size() * 0.1;
    
    // Add complexity for pressure variations
    complexity += calculatePressureVariability(steps) * 0.5;
    
    // Add complexity for timing variations
    double avgDuration = calculateTotalDuration(steps) / static_cast<double>(steps.size());
    for (const auto& stepValue : steps) {
        QJsonObject step = stepValue.toObject();
        if (step.contains("duration")) {
            double duration = step["duration"].toDouble();
            complexity += std::abs(duration - avgDuration) / avgDuration * 0.1;
        }
    }
    
    return complexity;
}

double PatternValidator::calculatePressureVariability(const QJsonArray& steps)
{
    if (steps.size() < 2) return 0.0;
    
    QList<double> pressures;
    for (const auto& stepValue : steps) {
        QJsonObject step = stepValue.toObject();
        if (step.contains("pressure")) {
            pressures.append(step["pressure"].toDouble());
        }
    }
    
    if (pressures.size() < 2) return 0.0;
    
    // Calculate standard deviation
    double mean = 0.0;
    for (double pressure : pressures) {
        mean += pressure;
    }
    mean /= pressures.size();
    
    double variance = 0.0;
    for (double pressure : pressures) {
        variance += std::pow(pressure - mean, 2);
    }
    variance /= pressures.size();
    
    return std::sqrt(variance);
}

int PatternValidator::calculateTotalDuration(const QJsonArray& steps)
{
    int total = 0;
    for (const auto& stepValue : steps) {
        QJsonObject step = stepValue.toObject();
        if (step.contains("duration")) {
            total += step["duration"].toInt();
        }
    }
    return total;
}

QStringList PatternValidator::generateRecommendations(const QList<ValidationIssue>& issues)
{
    QStringList recommendations;
    
    bool hasPressureIssues = false;
    bool hasTimingIssues = false;
    bool hasComplexityIssues = false;
    
    for (const auto& issue : issues) {
        if (issue.category.contains("Pressure")) hasPressureIssues = true;
        if (issue.category.contains("Timing")) hasTimingIssues = true;
        if (issue.category.contains("Complexity")) hasComplexityIssues = true;
    }
    
    if (hasPressureIssues) {
        recommendations << "Review pressure settings to ensure they are within safe limits";
        recommendations << "Consider gradual pressure changes to improve safety";
    }
    
    if (hasTimingIssues) {
        recommendations << "Optimize timing parameters for better performance";
        recommendations << "Consider breaking long steps into shorter segments";
    }
    
    if (hasComplexityIssues) {
        recommendations << "Simplify the pattern to reduce complexity";
        recommendations << "Consider using predefined pattern templates";
    }
    
    if (recommendations.isEmpty()) {
        recommendations << "Pattern validation passed - no specific recommendations";
    }
    
    return recommendations;
}

void PatternValidator::validateBasicParameters(const QJsonObject& parameters, ValidationReport& report)
{
    // Validate intensity
    if (parameters.contains("intensity")) {
        double intensity = parameters["intensity"].toDouble();
        if (intensity < 0.0 || intensity > 100.0) {
            addIssue(report, ERROR, "Parameter Range", 
                    QString("Intensity %1% is outside valid range 0-100%").arg(intensity),
                    "Set intensity between 0% and 100%");
        }
    }
    
    // Validate speed multiplier
    if (parameters.contains("speed")) {
        double speed = parameters["speed"].toDouble();
        if (speed < 0.1 || speed > 5.0) {
            addIssue(report, WARNING, "Parameter Range", 
                    QString("Speed multiplier %1 is outside recommended range 0.1-5.0").arg(speed),
                    "Consider using speed multiplier between 0.1 and 5.0");
        }
    }
}

void PatternValidator::validatePatternCoherence(const QJsonObject& patternData, ValidationReport& report)
{
    // Check if pattern name matches its behavior
    QString name = patternData["name"].toString().toLower();
    QJsonArray steps = patternData["steps"].toArray();
    
    if (name.contains("gentle") || name.contains("soft")) {
        // Gentle patterns should have lower pressures
        for (const auto& stepValue : steps) {
            QJsonObject step = stepValue.toObject();
            if (step.contains("pressure")) {
                double pressure = step["pressure"].toDouble();
                if (pressure > 60.0) {
                    addIssue(report, WARNING, "Pattern Coherence", 
                            QString("Pattern named '%1' has high pressure %2%")
                            .arg(patternData["name"].toString()).arg(pressure),
                            "Consider reducing pressure for gentle patterns");
                }
            }
        }
    }
    
    if (name.contains("intense") || name.contains("strong")) {
        // Intense patterns should have higher pressures
        bool hasHighPressure = false;
        for (const auto& stepValue : steps) {
            QJsonObject step = stepValue.toObject();
            if (step.contains("pressure")) {
                double pressure = step["pressure"].toDouble();
                if (pressure > 70.0) {
                    hasHighPressure = true;
                    break;
                }
            }
        }
        
        if (!hasHighPressure) {
            addIssue(report, WARNING, "Pattern Coherence", 
                    QString("Pattern named '%1' has no high pressure steps")
                    .arg(patternData["name"].toString()),
                    "Consider adding higher pressure steps for intense patterns");
        }
    }
}

void PatternValidator::validatePerformanceImpact(const QJsonArray& steps, ValidationReport& report)
{
    double complexity = calculatePatternComplexity(steps);
    
    if (complexity > m_maxComplexity) {
        addIssue(report, WARNING, "Performance Impact", 
                QString("Pattern complexity %1 exceeds recommended maximum %2")
                .arg(complexity, 0, 'f', 2).arg(m_maxComplexity),
                "Simplify pattern to improve performance");
    }
    
    // Check for rapid step changes that could impact performance
    for (int i = 1; i < steps.size(); ++i) {
        QJsonObject prevStep = steps[i-1].toObject();
        QJsonObject currStep = steps[i].toObject();
        
        if (prevStep.contains("duration") && currStep.contains("duration")) {
            int prevDuration = prevStep["duration"].toInt();
            int currDuration = currStep["duration"].toInt();
            
            if (prevDuration < 200 && currDuration < 200) {
                addIssue(report, WARNING, "Performance Impact", 
                        QString("Steps %1-%2: Consecutive short durations may impact performance")
                        .arg(i).arg(i+1),
                        "Consider combining short steps or increasing duration");
            }
        }
    }
}

void PatternValidator::validatePressureGradients(const QJsonArray& steps, ValidationReport& report)
{
    if (steps.size() < 2) {
        return; // Need at least 2 steps to check gradients
    }

    for (int i = 1; i < steps.size(); ++i) {
        QJsonObject prevStep = steps[i-1].toObject();
        QJsonObject currStep = steps[i].toObject();

        if (prevStep.contains("pressure") && currStep.contains("pressure")) {
            double prevPressure = prevStep["pressure"].toDouble();
            double currPressure = currStep["pressure"].toDouble();
            double gradient = qAbs(currPressure - prevPressure);

            // Check for excessive pressure gradients
            if (gradient > 50.0) { // More than 50 mmHg change
                addIssue(report, WARNING, "Pressure Gradient",
                        QString("Step %1: Large pressure change (%2 mmHg)")
                        .arg(i+1).arg(gradient, 0, 'f', 1),
                        "Consider adding intermediate steps for smoother transitions");
            }

            // Check for very rapid pressure changes with short durations
            if (gradient > 30.0 && currStep.contains("duration")) {
                int duration = currStep["duration"].toInt();
                if (duration < 500) { // Less than 500ms
                    addIssue(report, ERROR, "Pressure Gradient",
                            QString("Step %1: Rapid pressure change (%2 mmHg in %3ms)")
                            .arg(i+1).arg(gradient, 0, 'f', 1).arg(duration),
                            "Increase step duration or reduce pressure change");
                }
            }
        }
    }
}
