/**
 * @file automated_orgasm_demo.cpp
 * @brief Demonstration of automated orgasm patterns with physiological phase progression
 * 
 * This example shows how to use the new automated orgasm patterns that replicate
 * natural arousal and climax cycles with integrated anti-detachment monitoring.
 * 
 * Features demonstrated:
 * - Single 5-minute automated orgasm cycle
 * - Triple consecutive orgasm cycles with recovery periods
 * - Physiological phase progression (4 phases)
 * - Enhanced anti-detachment integration
 * - Sensitivity adaptation between cycles
 */

#include "../src/VacuumController.h"
#include "../src/patterns/PatternEngine.h"
#include "../src/safety/AntiDetachmentMonitor.h"
#include <QApplication>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>

class AutomatedOrgasmDemo : public QObject
{
    Q_OBJECT

public:
    AutomatedOrgasmDemo(QObject* parent = nullptr) : QObject(parent), demoStep(0) {}

    void runDemo()
    {
        qDebug() << "\n=== Automated Orgasm Pattern Demonstration ===";
        qDebug() << "This demo showcases the new physiological phase-based orgasm patterns";
        qDebug() << "designed to replicate natural arousal and climax cycles.\n";

        // Initialize vacuum controller
        controller = new VacuumController(this);
        if (!controller->initialize()) {
            qCritical() << "Failed to initialize vacuum controller";
            return;
        }

        // Start demonstration sequence
        QTimer::singleShot(1000, this, &AutomatedOrgasmDemo::nextDemo);
    }

private slots:
    void nextDemo()
    {
        switch (demoStep++) {
        case 0:
            demonstrateSingleOrgasmCycle();
            break;
        case 1:
            QTimer::singleShot(8000, this, &AutomatedOrgasmDemo::nextDemo);
            break;
        case 2:
            demonstrateTripleOrgasmCycle();
            break;
        case 3:
            QTimer::singleShot(15000, this, &AutomatedOrgasmDemo::nextDemo);
            break;
        case 4:
            demonstratePhaseProgression();
            break;
        default:
            qDebug() << "\n=== Demo Complete ===";
            QCoreApplication::quit();
            break;
        }
    }

    void demonstrateSingleOrgasmCycle()
    {
        qDebug() << "\n--- Single Automated Orgasm Cycle ---";
        qDebug() << "Duration: 5 minutes";
        qDebug() << "Phases: Initial Sensitivity → Adaptation → Arousal Buildup → Climax";
        qDebug() << "Features: Gentle ramp-up, enhanced anti-detachment during climax";

        QJsonObject params;
        params["type"] = "automated_orgasm";
        params["name"] = "Single Automated Orgasm";
        params["cycles"] = 1;
        params["enhanced_anti_detachment"] = true;

        controller->startPattern("Single Automated Orgasm", params);

        // Stop after 8 seconds for demo (normally 5 minutes)
        QTimer::singleShot(8000, controller, &VacuumController::stopPattern);
    }

    void demonstrateTripleOrgasmCycle()
    {
        qDebug() << "\n--- Triple Automated Orgasm Cycle ---";
        qDebug() << "Duration: ~18 minutes total";
        qDebug() << "Cycles: 3 complete orgasm cycles with recovery periods";
        qDebug() << "Features: Sensitivity adaptation, progressive intensity adjustment";
        qDebug() << "Recovery: 45s → 60s → 90s final cooldown";

        QJsonObject params;
        params["type"] = "multi-cycle_orgasm";
        params["name"] = "Triple Automated Orgasm";
        params["cycles"] = 3;
        params["sensitivity_adaptation"] = true;
        params["enhanced_anti_detachment"] = true;

        controller->startPattern("Triple Automated Orgasm", params);

        // Stop after 15 seconds for demo (normally 18 minutes)
        QTimer::singleShot(15000, controller, &VacuumController::stopPattern);
    }

    void demonstratePhaseProgression()
    {
        qDebug() << "\n--- Physiological Phase Progression Details ---";
        qDebug() << "";
        qDebug() << "Phase 1: Initial Sensitivity (0-30 seconds)";
        qDebug() << "  - Start: 35% pressure (gentle)";
        qDebug() << "  - Ramp to: 55% over 10 seconds";
        qDebug() << "  - Purpose: Avoid overwhelming initial sensitivity";
        qDebug() << "";
        qDebug() << "Phase 2: Adaptation Period (30 seconds - 2 minutes)";
        qDebug() << "  - Pressure: 60% with ±8% variation";
        qDebug() << "  - Purpose: Allow body adaptation to stimulation";
        qDebug() << "";
        qDebug() << "Phase 3: Arousal Build-up (2-4 minutes)";
        qDebug() << "  - Start: 60% → End: 85% pressure";
        qDebug() << "  - Progressive intensity increase";
        qDebug() << "  - Enhanced anti-detachment monitoring";
        qDebug() << "";
        qDebug() << "Phase 4: Pre-climax Tension (4-5 minutes)";
        qDebug() << "  - Pressure: 85% with ±8% variation";
        qDebug() << "  - Maximum anti-detachment sensitivity";
        qDebug() << "  - Precise stimulation maintenance";
        qDebug() << "";
        qDebug() << "Multi-cycle Adaptations:";
        qDebug() << "  - Cycle 1: Full sensitivity progression";
        qDebug() << "  - Cycle 2: Higher starting intensity (reduced sensitivity)";
        qDebug() << "  - Cycle 3: Extended climax phase, adapted progression";
        qDebug() << "";
        qDebug() << "Anti-detachment Integration:";
        qDebug() << "  - Enhanced mode during climax phases";
        qDebug() << "  - Faster response times (25ms vs 100ms)";
        qDebug() << "  - Gentle mode during recovery periods";
        qDebug() << "  - Maintains consistent contact throughout cycle";

        // Demonstrate a short phase progression
        QJsonObject params;
        params["type"] = "automated_orgasm";
        params["name"] = "Phase Progression Demo";
        params["cycles"] = 1;
        params["demo_mode"] = true;  // Shortened phases for demonstration

        controller->startPattern("Single Automated Orgasm", params);

        // Stop after 10 seconds
        QTimer::singleShot(10000, controller, &VacuumController::stopPattern);
    }

private:
    VacuumController* controller;
    int demoStep;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "Automated Orgasm Pattern Demo";
    qDebug() << "============================";
    qDebug() << "";
    qDebug() << "This demonstration shows the new automated orgasm patterns";
    qDebug() << "designed to replicate natural physiological response cycles.";
    qDebug() << "";
    qDebug() << "Key Features:";
    qDebug() << "• 4-phase physiological progression";
    qDebug() << "• Single-click activation (1 click = 1 complete cycle)";
    qDebug() << "• Multi-cycle support (up to 3 consecutive cycles)";
    qDebug() << "• Enhanced anti-detachment integration";
    qDebug() << "• Sensitivity adaptation between cycles";
    qDebug() << "• Post-climax recovery periods";
    qDebug() << "";

    AutomatedOrgasmDemo demo;
    demo.runDemo();

    return app.exec();
}

#include "automated_orgasm_demo.moc"
