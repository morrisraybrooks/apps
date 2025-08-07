/**
 * @file enhanced_air_pulse_usage.cpp
 * @brief Example usage of enhanced air pulse patterns with anti-detachment integration
 * 
 * This example demonstrates how to use the enhanced single-chamber air pulse
 * patterns with integrated anti-detachment monitoring for safe and effective
 * vacuum therapy across the entire vulvar area.
 */

#include "../src/VacuumController.h"
#include "../src/patterns/PatternEngine.h"
#include "../src/safety/AntiDetachmentMonitor.h"
#include <QApplication>
#include <QJsonObject>
#include <QDebug>

class EnhancedAirPulseExample : public QObject
{
    Q_OBJECT

public:
    EnhancedAirPulseExample(QObject* parent = nullptr) : QObject(parent) {}

    void demonstrateEnhancedAirPulse()
    {
        // Initialize vacuum controller
        VacuumController controller;
        
        if (!controller.initialize()) {
            qCritical() << "Failed to initialize vacuum controller";
            return;
        }

        // Configure anti-detachment system
        controller.setAntiDetachmentThreshold(50.0);  // 50 mmHg threshold
        controller.setMaxPressure(100.0);             // 100 mmHg maximum

        qDebug() << "=== Enhanced Single-Chamber Air Pulse Demo ===";
        qDebug() << "Cup Design: V-shaped boomerang with built-in drainage channels";
        qDebug() << "Coverage: Full vulvar area (labia majora and minora)";
        qDebug() << "Anti-detachment threshold:" << controller.getAntiDetachmentThreshold() << "mmHg";

        // Example 1: Gentle Therapeutic Air Pulse
        demonstrateTherapeuticPulse(controller);

        // Example 2: Medium Intensity Air Pulse with Progressive Intensity
        demonstrateProgressiveAirPulse(controller);

        // Example 3: High-Frequency Stimulation Pattern
        demonstrateHighFrequencyPattern(controller);

        // Example 4: Custom Pattern with Anti-Detachment Integration
        demonstrateCustomPattern(controller);
    }

private slots:
    void onAntiDetachmentTriggered(double avlPressure)
    {
        qWarning() << QString("ANTI-DETACHMENT ACTIVATED - AVL Pressure: %1 mmHg")
                      .arg(avlPressure, 0, 'f', 1);
        qDebug() << "System automatically increasing vacuum to maintain seal integrity";
    }

    void onSealIntegrityWarning(double avlPressure)
    {
        qDebug() << QString("Seal integrity warning - AVL Pressure: %1 mmHg")
                    .arg(avlPressure, 0, 'f', 1);
    }

    void onPatternCompleted()
    {
        qDebug() << "Pattern completed successfully";
    }

private:
    void demonstrateTherapeuticPulse(VacuumController& controller)
    {
        qDebug() << "\n--- Therapeutic Blood Flow Pattern ---";
        
        QJsonObject therapeuticParams;
        therapeuticParams["type"] = "therapeutic_pulse";
        therapeuticParams["baseline_pressure_mmhg"] = 20.0;
        therapeuticParams["therapeutic_pressure_mmhg"] = 35.0;
        therapeuticParams["frequency_hz"] = 4.0;
        therapeuticParams["session_duration_ms"] = 180000;  // 3 minutes
        therapeuticParams["include_warmup"] = true;
        therapeuticParams["include_cooldown"] = true;

        qDebug() << "Parameters:";
        qDebug() << "  - Baseline pressure: 20 mmHg (maintains seal)";
        qDebug() << "  - Therapeutic pressure: 35 mmHg (promotes blood flow)";
        qDebug() << "  - Frequency: 4 Hz (gentle therapeutic rate)";
        qDebug() << "  - Duration: 3 minutes with warmup/cooldown";

        // Connect signals for monitoring
        connectPatternSignals(controller);

        // Start pattern
        controller.startPattern("Therapeutic Blood Flow", therapeuticParams);
        
        // Simulate running for demonstration
        qDebug() << "Pattern started - monitoring anti-detachment system...";
    }

    void demonstrateProgressiveAirPulse(VacuumController& controller)
    {
        qDebug() << "\n--- Progressive Air Pulse Pattern ---";
        
        QJsonObject airPulseParams;
        airPulseParams["type"] = "air_pulse";
        airPulseParams["frequency_hz"] = 8.0;
        airPulseParams["base_pressure_mmhg"] = 28.0;
        airPulseParams["pulse_amplitude_mmhg"] = 15.0;
        airPulseParams["duty_cycle_percent"] = 35.0;
        airPulseParams["cycle_count"] = 30;
        airPulseParams["progressive_intensity"] = true;

        qDebug() << "Parameters:";
        qDebug() << "  - Frequency: 8 Hz (125ms cycles)";
        qDebug() << "  - Base pressure: 28 mmHg (seal maintenance)";
        qDebug() << "  - Pulse amplitude: 15 mmHg (stimulation intensity)";
        qDebug() << "  - Duty cycle: 35% (44ms suction, 81ms baseline)";
        qDebug() << "  - Progressive intensity: Builds from 50% to 100%";

        connectPatternSignals(controller);
        controller.startPattern("Enhanced Single Chamber Air Pulse", airPulseParams);
    }

    void demonstrateHighFrequencyPattern(VacuumController& controller)
    {
        qDebug() << "\n--- High-Frequency Stimulation Pattern ---";
        
        QJsonObject highFreqParams;
        highFreqParams["type"] = "air_pulse";
        highFreqParams["frequency_hz"] = 12.0;
        highFreqParams["base_pressure_mmhg"] = 30.0;
        highFreqParams["pulse_amplitude_mmhg"] = 18.0;
        highFreqParams["duty_cycle_percent"] = 40.0;
        highFreqParams["cycle_count"] = 40;
        highFreqParams["progressive_intensity"] = true;

        qDebug() << "Parameters:";
        qDebug() << "  - Frequency: 12 Hz (83ms cycles)";
        qDebug() << "  - Base pressure: 30 mmHg";
        qDebug() << "  - Pulse amplitude: 18 mmHg (up to 48 mmHg peak)";
        qDebug() << "  - Duty cycle: 40% (33ms suction, 50ms baseline)";
        qDebug() << "  - High-intensity stimulation across entire vulvar area";

        connectPatternSignals(controller);
        controller.startPattern("High Frequency Air Pulse", highFreqParams);
    }

    void demonstrateCustomPattern(VacuumController& controller)
    {
        qDebug() << "\n--- Custom Anti-Detachment Aware Pattern ---";
        
        // This pattern demonstrates how the anti-detachment system
        // automatically maintains seal integrity during operation
        
        QJsonObject customParams;
        customParams["type"] = "air_pulse";
        customParams["frequency_hz"] = 10.0;
        customParams["base_pressure_mmhg"] = 25.0;  // Lower base pressure
        customParams["pulse_amplitude_mmhg"] = 20.0; // Higher amplitude
        customParams["duty_cycle_percent"] = 30.0;
        customParams["cycle_count"] = 25;
        customParams["progressive_intensity"] = false;

        qDebug() << "This pattern uses lower base pressure to test anti-detachment:";
        qDebug() << "  - Base pressure: 25 mmHg (closer to detachment threshold)";
        qDebug() << "  - Pulse amplitude: 20 mmHg (45 mmHg peak)";
        qDebug() << "  - Anti-detachment will activate if pressure drops below 50 mmHg";
        qDebug() << "  - System will automatically increase vacuum to maintain seal";

        connectPatternSignals(controller);
        controller.startPattern("Anti-Detachment Test Pattern", customParams);
    }

    void connectPatternSignals(VacuumController& controller)
    {
        // Connect to pattern engine signals
        connect(&controller, &VacuumController::patternStarted,
                [](const QString& name) {
                    qDebug() << "Pattern started:" << name;
                });

        connect(&controller, &VacuumController::patternStopped,
                this, &EnhancedAirPulseExample::onPatternCompleted);

        // Connect to anti-detachment signals
        connect(&controller, &VacuumController::antiDetachmentActivated,
                this, &EnhancedAirPulseExample::onAntiDetachmentTriggered);

        // Connect to pressure monitoring
        connect(&controller, &VacuumController::pressureUpdated,
                [](double avl, double tank) {
                    static int updateCount = 0;
                    if (++updateCount % 20 == 0) {  // Log every 20th update
                        qDebug() << QString("Pressure - AVL: %1 mmHg, Tank: %2 mmHg")
                                    .arg(avl, 0, 'f', 1).arg(tank, 0, 'f', 1);
                    }
                });
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "Enhanced Air Pulse Pattern Demo";
    qDebug() << "==============================";
    qDebug() << "Single-chamber vacuum system with anti-detachment integration";
    qDebug() << "V-shaped cup with built-in drainage channels";
    qDebug() << "Full vulvar area coverage and stimulation";

    EnhancedAirPulseExample example;
    example.demonstrateEnhancedAirPulse();

    return app.exec();
}

#include "enhanced_air_pulse_usage.moc"
