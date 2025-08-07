#ifndef PATTERNDEFINITIONS_H
#define PATTERNDEFINITIONS_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QStringList>

/**
 * @brief Pattern definitions and management
 * 
 * This class manages the 15+ vacuum patterns defined in the configuration:
 * - Loads patterns from JSON configuration
 * - Validates pattern parameters
 * - Provides pattern metadata and descriptions
 * - Handles pattern categories and organization
 */
class PatternDefinitions : public QObject
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
        QString speed;
        QString description;
        QString category;
        QJsonObject parameters;
        QList<PatternStep> steps;
        double basePressure;
        double intensity;
        bool isValid;

        PatternInfo() : basePressure(0.0), intensity(0.0), isValid(false) {}
    };

    explicit PatternDefinitions(QObject *parent = nullptr);
    ~PatternDefinitions();

    // Pattern loading
    bool loadFromFile(const QString& filePath);
    bool loadFromJson(const QJsonObject& jsonData);
    void loadDefaultPatterns();
    
    // Pattern access
    PatternInfo getPattern(const QString& name) const;
    QJsonObject getPatternParameters(const QString& name) const;
    QStringList getPatternNames() const;
    QStringList getPatternsByType(const QString& type) const;
    QStringList getAllPatternNames() const;
    QStringList getPatternsByCategory(const QString& category) const;
    QStringList getAllCategories() const;
    
    // Pattern validation
    bool isValidPattern(const QString& name) const;
    bool hasPattern(const QString& name) const;
    bool validatePatternParameters(const QJsonObject& parameters) const;
    QString getValidationError() const { return m_lastError; }
    
    // Pattern metadata
    QString getPatternDescription(const QString& name) const;
    QString getPatternType(const QString& name) const;
    QString getPatternSpeed(const QString& name) const;
    QString getPatternCategory(const QString& name) const;
    
    // Pattern creation helpers
    QJsonObject createPulsePattern(const QString& speed, int pulseDuration, int pauseDuration, double pressure);
    QJsonObject createWavePattern(const QString& speed, int period, double minPressure, double maxPressure);
    QJsonObject createAirPulsePattern(const QString& speed, int pulseDuration, int releaseDuration, double pressure);
    QJsonObject createMilkingPattern(const QString& speed, int strokeDuration, int releaseDuration, double pressure, int strokeCount);
    QJsonObject createConstantPattern(const QString& speed, double basePressure, double variation, int variationPeriod);
    QJsonObject createEdgingPattern(int buildupDuration, double peakPressure, int releaseDuration, int holdDuration, int cycles);

Q_SIGNALS:
    void patternsLoaded(int count);
    void patternLoadError(const QString& error);

private:
    void initializePatterns();
    void initializeDefaultPatterns();
    void createPulsePatterns();
    void createConstantPatterns();
    void createLegacyConstantPatterns();
    void createSpecialPatterns();
    void createAirPulsePatterns();
    void createMilkingPatterns();
    void createWavePatterns();
    void createTherapeuticPatterns();
    bool validatePulsePattern(const QJsonObject& params) const;
    bool validateWavePattern(const QJsonObject& params) const;
    bool validateAirPulsePattern(const QJsonObject& params) const;
    bool validateMilkingPattern(const QJsonObject& params) const;
    bool validateConstantPattern(const QJsonObject& params) const;
    bool validateEdgingPattern(const QJsonObject& params) const;
    
    bool isValidPressurePercent(double pressure) const;
    bool isValidDuration(int duration) const;
    bool isValidSpeed(const QString& speed) const;
    
    // Pattern storage
    QMap<QString, PatternInfo> m_patterns;
    QMap<QString, QStringList> m_categorizedPatterns;
    
    // Error tracking
    QString m_lastError;
    
    // Constants
    static const double MIN_PRESSURE_PERCENT;
    static const double MAX_PRESSURE_PERCENT;
    static const int MIN_DURATION_MS;
    static const int MAX_DURATION_MS;
    static const QStringList VALID_SPEEDS;
    static const QStringList VALID_TYPES;
    
    // Default pattern definitions
    static const QJsonObject DEFAULT_SLOW_PULSE;
    static const QJsonObject DEFAULT_MEDIUM_PULSE;
    static const QJsonObject DEFAULT_FAST_PULSE;
    static const QJsonObject DEFAULT_SLOW_WAVE;
    static const QJsonObject DEFAULT_MEDIUM_WAVE;
    static const QJsonObject DEFAULT_FAST_WAVE;
    static const QJsonObject DEFAULT_SLOW_AIR_PULSE;
    static const QJsonObject DEFAULT_MEDIUM_AIR_PULSE;
    static const QJsonObject DEFAULT_FAST_AIR_PULSE;
    static const QJsonObject DEFAULT_SLOW_MILKING;
    static const QJsonObject DEFAULT_MEDIUM_MILKING;
    static const QJsonObject DEFAULT_FAST_MILKING;
    static const QJsonObject DEFAULT_SLOW_CONSTANT;
    static const QJsonObject DEFAULT_MEDIUM_CONSTANT;
    static const QJsonObject DEFAULT_FAST_CONSTANT;
    static const QJsonObject DEFAULT_EDGING;
};

#endif // PATTERNDEFINITIONS_H
