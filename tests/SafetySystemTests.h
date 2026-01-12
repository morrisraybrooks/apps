#ifndef SAFETYSYSTEMTESTS_H
#define SAFETYSYSTEMTESTS_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QMap>
#include <QVariant>
#include <memory>

#include "VacuumController.h"
#include "safety/SafetyManager.h"
#include "safety/AntiDetachmentMonitor.h"
#include "safety/AdaptiveSealMonitor.h"
#include "safety/EmergencyStop.h"
#include "hardware/HardwareManager.h"
#include "TestFramework.h"

/**
 * @brief Comprehensive safety system test suite
 *
 * This test suite validates all safety-critical functionality:
 * - Anti-detachment monitoring and response
 * - Emergency stop functionality
 * - Overpressure protection
 * - Sensor failure detection
 * - Hardware failure response
 * - Safety system integration
 * - Fail-safe behavior validation
 */
class SafetySystemTests : public TestSuite
{
    Q_OBJECT

public:
    explicit SafetySystemTests(QObject* parent = nullptr);

    // TestSuite interface
    QStringList testNames() const override;
    TestResult runTest(const QString& testName) override;

    // TestSuite interface
    bool setup() override;
    void cleanup() override;

private:
    // Test implementations
    TestResult testSafetyManagerInitialization();
    TestResult testEmergencyStopActivation();
    TestResult testAntiDetachmentMonitoring();

        // New safety behavior tests
        TestResult testSealMaintainedSafeStateOnEmergencyStop();
        TestResult testFullVentOnTissueDamageRiskOverpressure();
        TestResult testFullVentOnRunawayPumpWithInvalidSensors();

        // AdaptiveSealMonitor tests
        TestResult testAdaptiveSealMonitorInitialization();
        TestResult testAdaptiveThresholdCalculation();
        TestResult testMultiPathLeakDetection();
        TestResult testProgressiveResealRecovery();
        TestResult testArousalIntegration();
        TestResult testPhasePriorityAdjustment();

    // Test objects (raw pointers, managed by Qt parent)
    SafetyManager* m_safetyManager;
    HardwareManager* m_hardwareManager;
    AntiDetachmentMonitor* m_antiDetachmentMonitor;
    AdaptiveSealMonitor* m_adaptiveSealMonitor;
    EmergencyStop* m_emergencyStop;
};

#endif // SAFETYSYSTEMTESTS_H
