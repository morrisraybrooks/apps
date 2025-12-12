#ifndef ORGASMCONTROLALGORITHMTESTS_H
#define ORGASMCONTROLALGORITHMTESTS_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <memory>

#include "TestFramework.h"
#include "control/OrgasmControlAlgorithm.h"
#include "hardware/HardwareManager.h"

/**
 * @brief Comprehensive OrgasmControlAlgorithm test suite
 *
 * This test suite validates the core control algorithm functionality:
 * - Initialization and state management
 * - Arousal level calculation and thresholds
 * - Mode transitions (edging, forced orgasm, milking, denial)
 * - Edge detection and back-off logic
 * - Orgasm detection and handling
 * - Safety checks and emergency stop
 * - Heart rate sensor integration
 * - Fluid sensor integration
 * - Milking mode with PID control
 * - Point of no return detection
 */
class OrgasmControlAlgorithmTests : public TestSuite
{
    Q_OBJECT

public:
    explicit OrgasmControlAlgorithmTests(QObject* parent = nullptr);

    // TestSuite interface
    QStringList testNames() const override;
    TestResult runTest(const QString& testName) override;

    // TestSuite interface
    bool setup() override;
    void cleanup() override;

private:
    // Initialization tests
    TestResult testInitialization();
    TestResult testDefaultThresholds();
    TestResult testSetThresholds();
    
    // State management tests
    TestResult testStartAdaptiveEdging();
    TestResult testStartForcedOrgasm();
    TestResult testStartDenial();
    TestResult testStartMilking();
    TestResult testStop();
    TestResult testEmergencyStop();
    
    // Arousal calculation tests
    TestResult testArousalLevelCalculation();
    TestResult testArousalSmoothing();
    TestResult testArousalStateTransitions();
    
    // Edge detection tests
    TestResult testEdgeDetection();
    TestResult testBackOffBehavior();
    TestResult testPointOfNoReturnDetection();
    
    // Orgasm detection tests  
    TestResult testOrgasmDetection();
    TestResult testUnexpectedOrgasmDuringEdging();
    
    // Milking mode tests
    TestResult testMilkingZoneTracking();
    TestResult testMilkingPIDControl();
    TestResult testDangerZoneReduction();
    
    // Configuration tests
    TestResult testSetTENSEnabled();
    TestResult testSetAntiEscapeEnabled();
    TestResult testMilkingThresholdValidation();
    
    // Safety tests
    TestResult testHighPressureLimit();
    TestResult testCalibrationValidation();
    TestResult testSensorErrorHandling();
    
    // Signal emission tests
    TestResult testSignalEmissions();

    // Test objects (raw pointers, managed by Qt parent)
    HardwareManager* m_hardwareManager;
    OrgasmControlAlgorithm* m_algorithm;
};

#endif // ORGASMCONTROLALGORITHMTESTS_H

