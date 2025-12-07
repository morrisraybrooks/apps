#include "PatternTests.h"
#include "TestFramework.h"
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

PatternTests::PatternTests(QObject *parent)
    : QObject(parent)
{
}

void PatternTests::initTestCase()
{
    TestFramework::initialize();
    TestFramework::enableMockHardware(true);
    qDebug() << "Pattern Tests initialized";
}

void PatternTests::cleanupTestCase()
{
    TestFramework::cleanup();
    qDebug() << "Pattern Tests cleanup completed";
}

void PatternTests::init()
{
    TestFramework::resetHardwareState();
    TestFramework::stopAllPatterns();
}

void PatternTests::cleanup()
{
    TestFramework::stopAllOperations();
}

void PatternTests::testPulsePattern()
{
    qDebug() << "Testing pulse pattern...";
    
    // Create pulse pattern parameters
    QJsonObject pulseParams;
    pulseParams["type"] = "pulse";
    pulseParams["duration_ms"] = 2000;
    pulseParams["pressure_mmhg"] = -60;
    pulseParams["pulse_width_ms"] = 200;
    pulseParams["pulse_interval_ms"] = 400;
    
    // Start pulse pattern
    QVERIFY(TestFramework::startPattern("test_pulse", pulseParams));
    
    // Verify pattern is running
    QVERIFY(TestFramework::isPatternRunning("test_pulse"));
    
    // Check initial state
    QTest::qWait(100); // Allow pattern to start
    QVERIFY(TestFramework::getPumpState());
    
    // Monitor pulse cycles
    int pulseCycles = 0;
    for (int i = 0; i < 10; ++i) {
        QTest::qWait(100);
        if (TestFramework::getSolenoidState(1)) { // SOL1 active during pulse
            pulseCycles++;
        }
    }
    
    QVERIFY2(pulseCycles >= 2, "Should detect at least 2 pulse cycles");
    
    // Stop pattern
    QVERIFY(TestFramework::stopPattern("test_pulse"));
    QVERIFY(!TestFramework::isPatternRunning("test_pulse"));
}

void PatternTests::testWavePattern()
{
    qDebug() << "Testing wave pattern...";
    
    QJsonObject waveParams;
    waveParams["type"] = "wave";
    waveParams["duration_ms"] = 3000;
    waveParams["min_pressure_mmhg"] = -20;
    waveParams["max_pressure_mmhg"] = -80;
    waveParams["wave_period_ms"] = 1000;
    
    QVERIFY(TestFramework::startPattern("test_wave", waveParams));
    QVERIFY(TestFramework::isPatternRunning("test_wave"));
    
    // Monitor pressure changes over time
    QList<double> pressureReadings;
    for (int i = 0; i < 20; ++i) {
        QTest::qWait(150);
        double pressure = TestFramework::readPressureSensor(1);
        pressureReadings.append(pressure);
    }
    
    // Verify wave characteristics
    double minPressure = *std::min_element(pressureReadings.begin(), pressureReadings.end());
    double maxPressure = *std::max_element(pressureReadings.begin(), pressureReadings.end());
    
    QVERIFY2(minPressure <= -15, "Wave should reach minimum pressure");
    QVERIFY2(maxPressure >= -85, "Wave should reach maximum pressure");
    
    TestFramework::stopPattern("test_wave");
}

void PatternTests::testConstantPattern()
{
    qDebug() << "Testing constant pattern...";
    
    QJsonObject constantParams;
    constantParams["type"] = "constant";
    constantParams["duration_ms"] = 1500;
    constantParams["pressure_mmhg"] = -50;
    
    QVERIFY(TestFramework::startPattern("test_constant", constantParams));
    
    // Allow pattern to stabilize
    QTest::qWait(300);
    
    // Check pressure stability
    QList<double> pressureReadings;
    for (int i = 0; i < 10; ++i) {
        QTest::qWait(100);
        pressureReadings.append(TestFramework::readPressureSensor(1));
    }
    
    // Calculate pressure variance
    double avgPressure = 0;
    for (double p : pressureReadings) {
        avgPressure += p;
    }
    avgPressure /= pressureReadings.size();
    
    double variance = 0;
    for (double p : pressureReadings) {
        variance += (p - avgPressure) * (p - avgPressure);
    }
    variance /= pressureReadings.size();
    
    QVERIFY2(variance < 25.0, "Constant pattern should have low pressure variance");
    QVERIFY2(qAbs(avgPressure - (-50)) < 10, "Average pressure should be close to target");
    
    TestFramework::stopPattern("test_constant");
}

void PatternTests::testAirPulsePattern()
{
    qDebug() << "Testing air pulse pattern...";
    
    QJsonObject airPulseParams;
    airPulseParams["type"] = "air_pulse";
    airPulseParams["duration_ms"] = 2000;
    airPulseParams["vacuum_pressure_mmhg"] = -60;
    airPulseParams["air_pulse_duration_ms"] = 150;
    airPulseParams["cycle_time_ms"] = 800;
    
    QVERIFY(TestFramework::startPattern("test_air_pulse", airPulseParams));
    
    // Monitor solenoid switching for air pulses
    int airPulseCycles = 0;
    bool lastSol2State = false;
    
    for (int i = 0; i < 25; ++i) {
        QTest::qWait(100);
        bool currentSol2State = TestFramework::getSolenoidState(2); // SOL2 for air pulse
        
        if (currentSol2State && !lastSol2State) {
            airPulseCycles++;
        }
        lastSol2State = currentSol2State;
    }
    
    QVERIFY2(airPulseCycles >= 2, "Should detect air pulse cycles");
    
    TestFramework::stopPattern("test_air_pulse");
}

void PatternTests::testMilkingPattern()
{
    qDebug() << "Testing milking pattern...";
    
    QJsonObject milkingParams;
    milkingParams["type"] = "milking";
    milkingParams["duration_ms"] = 3000;
    milkingParams["base_pressure_mmhg"] = -40;
    milkingParams["peak_pressure_mmhg"] = -80;
    milkingParams["milk_duration_ms"] = 300;
    milkingParams["rest_duration_ms"] = 200;
    
    QVERIFY(TestFramework::startPattern("test_milking", milkingParams));
    
    // Monitor pressure variations for milking action
    QList<double> pressureReadings;
    QList<bool> pumpStates;
    
    for (int i = 0; i < 30; ++i) {
        QTest::qWait(100);
        pressureReadings.append(TestFramework::readPressureSensor(1));
        pumpStates.append(TestFramework::getPumpState());
    }
    
    // Verify milking characteristics
    int pumpCycles = 0;
    bool lastPumpState = false;
    
    for (bool pumpState : pumpStates) {
        if (pumpState && !lastPumpState) {
            pumpCycles++;
        }
        lastPumpState = pumpState;
    }
    
    QVERIFY2(pumpCycles >= 3, "Should detect multiple milking cycles");
    
    TestFramework::stopPattern("test_milking");
}

void PatternTests::testPatternTransitions()
{
    qDebug() << "Testing pattern transitions...";
    
    // Start with pulse pattern
    QJsonObject pulseParams;
    pulseParams["type"] = "pulse";
    pulseParams["duration_ms"] = 1000;
    pulseParams["pressure_mmhg"] = -50;
    pulseParams["pulse_width_ms"] = 200;
    pulseParams["pulse_interval_ms"] = 400;
    
    QVERIFY(TestFramework::startPattern("pattern1", pulseParams));
    QVERIFY(TestFramework::isPatternRunning("pattern1"));
    
    QTest::qWait(300);
    
    // Transition to constant pattern
    QJsonObject constantParams;
    constantParams["type"] = "constant";
    constantParams["duration_ms"] = 1000;
    constantParams["pressure_mmhg"] = -70;
    
    QVERIFY(TestFramework::stopPattern("pattern1"));
    QVERIFY(TestFramework::startPattern("pattern2", constantParams));
    
    QVERIFY(!TestFramework::isPatternRunning("pattern1"));
    QVERIFY(TestFramework::isPatternRunning("pattern2"));
    
    QTest::qWait(300);
    
    // Verify smooth transition (no hardware glitches)
    QVERIFY(TestFramework::getPumpState()); // Pump should remain on
    
    TestFramework::stopPattern("pattern2");
}

void PatternTests::testPatternSafety()
{
    qDebug() << "Testing pattern safety limits...";
    
    // Test overpressure protection
    QJsonObject dangerousParams;
    dangerousParams["type"] = "constant";
    dangerousParams["duration_ms"] = 1000;
    dangerousParams["pressure_mmhg"] = -150; // Exceeds safety limit
    
    // Should reject dangerous pattern
    QVERIFY(!TestFramework::startPattern("dangerous", dangerousParams));
    
    // Test emergency stop during pattern
    QJsonObject safeParams;
    safeParams["type"] = "pulse";
    safeParams["duration_ms"] = 2000;
    safeParams["pressure_mmhg"] = -60;
    safeParams["pulse_width_ms"] = 200;
    safeParams["pulse_interval_ms"] = 400;
    
    QVERIFY(TestFramework::startPattern("safe_pattern", safeParams));
    QTest::qWait(200);
    
    // Trigger emergency stop
    TestFramework::triggerEmergencyStop();
    
    // Pattern should stop immediately
    QVERIFY(!TestFramework::isPatternRunning("safe_pattern"));
    QVERIFY(!TestFramework::getPumpState());
    
    TestFramework::resetEmergencyStop();
}

void PatternTests::testPatternValidation()
{
    qDebug() << "Testing pattern parameter validation...";
    
    // Test invalid pattern type
    QJsonObject invalidType;
    invalidType["type"] = "invalid_type";
    invalidType["duration_ms"] = 1000;
    
    QVERIFY(!TestFramework::startPattern("invalid", invalidType));
    
    // Test missing required parameters
    QJsonObject missingParams;
    missingParams["type"] = "pulse";
    // Missing duration_ms, pressure_mmhg, etc.
    
    QVERIFY(!TestFramework::startPattern("missing", missingParams));
    
    // Test invalid duration
    QJsonObject invalidDuration;
    invalidDuration["type"] = "constant";
    invalidDuration["duration_ms"] = -100; // Negative duration
    invalidDuration["pressure_mmhg"] = -50;
    
    QVERIFY(!TestFramework::startPattern("invalid_duration", invalidDuration));
    
    // Test valid parameters
    QJsonObject validParams;
    validParams["type"] = "constant";
    validParams["duration_ms"] = 1000;
    validParams["pressure_mmhg"] = -50;
    
    QVERIFY(TestFramework::startPattern("valid", validParams));
    TestFramework::stopPattern("valid");
}

void PatternTests::testConcurrentPatterns()
{
    qDebug() << "Testing concurrent pattern handling...";
    
    QJsonObject pattern1;
    pattern1["type"] = "pulse";
    pattern1["duration_ms"] = 2000;
    pattern1["pressure_mmhg"] = -50;
    pattern1["pulse_width_ms"] = 200;
    pattern1["pulse_interval_ms"] = 400;
    
    QJsonObject pattern2;
    pattern2["type"] = "constant";
    pattern2["duration_ms"] = 1500;
    pattern2["pressure_mmhg"] = -60;
    
    // Start first pattern
    QVERIFY(TestFramework::startPattern("concurrent1", pattern1));
    QVERIFY(TestFramework::isPatternRunning("concurrent1"));
    
    // Attempt to start second pattern (should replace first)
    QVERIFY(TestFramework::startPattern("concurrent2", pattern2));
    
    // Only one pattern should be running
    QVERIFY(!TestFramework::isPatternRunning("concurrent1"));
    QVERIFY(TestFramework::isPatternRunning("concurrent2"));
    
    TestFramework::stopPattern("concurrent2");
}

// Test runner for this specific test class
QTEST_MAIN(PatternTests)
