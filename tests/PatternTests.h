#ifndef PATTERNTESTS_H
#define PATTERNTESTS_H

#include <QObject>
#include <QTest>
#include <QJsonObject>

/**
 * @brief Pattern testing class for vacuum controller
 * 
 * Tests all vacuum pattern functionality including:
 * - Pulse patterns with configurable timing
 * - Wave patterns with pressure modulation
 * - Constant pressure patterns
 * - Air pulse patterns for stimulation
 * - Milking patterns with rhythmic action
 * - Pattern transitions and safety
 * - Parameter validation
 * - Concurrent pattern handling
 */
class PatternTests : public QObject
{
    Q_OBJECT

public:
    explicit PatternTests(QObject *parent = nullptr);

private slots:
    // Test framework setup/teardown
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Individual pattern tests
    void testPulsePattern();
    void testWavePattern();
    void testConstantPattern();
    void testAirPulsePattern();
    void testMilkingPattern();
    
    // Pattern management tests
    void testPatternTransitions();
    void testPatternSafety();
    void testPatternValidation();
    void testConcurrentPatterns();

private:
    // Helper methods
    QJsonObject createTestPattern(const QString& type, int duration, double pressure);
    bool verifyPatternExecution(const QString& patternName, int expectedDuration);
    void monitorPatternBehavior(const QString& patternName, int monitorTime);
    
    // Test constants
    static constexpr double MAX_SAFE_PRESSURE = -100.0;  // mmHg
    static constexpr double MIN_SAFE_PRESSURE = -10.0;   // mmHg
    static constexpr int MIN_PATTERN_DURATION = 100;     // ms
    static constexpr int MAX_PATTERN_DURATION = 300000;  // ms (5 minutes)
    
    // Pattern timing tolerances
    static constexpr int TIMING_TOLERANCE_MS = 50;
    static constexpr double PRESSURE_TOLERANCE = 5.0;    // mmHg
};

#endif // PATTERNTESTS_H
