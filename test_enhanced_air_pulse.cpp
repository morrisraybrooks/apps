#include "src/VacuumController.h"
#include <QCoreApplication>
#include <QTimer>
#include <QJsonObject>
#include <QDebug>
#include <iostream>

class AirPulseDemo : public QObject
{
    Q_OBJECT

public:
    AirPulseDemo(QObject* parent = nullptr) : QObject(parent), controller(nullptr) {}

    void runDemo()
    {
        std::cout << "\n=== Enhanced Air Pulse Pattern Demo ===" << std::endl;
        std::cout << "Single-chamber vacuum system with anti-detachment integration" << std::endl;
        std::cout << "V-shaped cup with built-in drainage channels" << std::endl;
        std::cout << "Full vulvar area coverage and stimulation\n" << std::endl;

        // Initialize controller
        controller = new VacuumController(this);
        
        // Connect signals for monitoring
        connect(controller, &VacuumController::systemStateChanged,
                this, &AirPulseDemo::onSystemStateChanged);
        connect(controller, &VacuumController::pressureUpdated,
                this, &AirPulseDemo::onPressureUpdated);
        connect(controller, &VacuumController::antiDetachmentActivated,
                this, &AirPulseDemo::onAntiDetachmentActivated);
        connect(controller, &VacuumController::patternStarted,
                this, &AirPulseDemo::onPatternStarted);
        connect(controller, &VacuumController::patternStopped,
                this, &AirPulseDemo::onPatternStopped);

        if (!controller->initialize()) {
            std::cerr << "Failed to initialize vacuum controller!" << std::endl;
            QCoreApplication::quit();
            return;
        }

        // Configure anti-detachment system
        controller->setAntiDetachmentThreshold(50.0);  // 50 mmHg threshold
        controller->setMaxPressure(100.0);             // 100 mmHg maximum

        std::cout << "System initialized successfully!" << std::endl;
        std::cout << "Anti-detachment threshold: " << controller->getAntiDetachmentThreshold() << " mmHg" << std::endl;
        std::cout << "Maximum pressure: " << controller->getMaxPressure() << " mmHg\n" << std::endl;

        // Start demo sequence
        demoStep = 0;
        runNextDemo();
    }

private slots:
    void onSystemStateChanged(VacuumController::SystemState state)
    {
        const char* stateNames[] = {"STOPPED", "STARTING", "RUNNING", "PAUSED", "ERROR", "EMERGENCY_STOP"};
        std::cout << "System state: " << stateNames[state] << std::endl;
    }

    void onPressureUpdated(double avl, double tank)
    {
        static int updateCount = 0;
        if (++updateCount % 10 == 0) {  // Log every 10th update
            std::cout << "Pressure - AVL: " << avl << " mmHg, Tank: " << tank << " mmHg" << std::endl;
        }
    }

    void onAntiDetachmentActivated()
    {
        std::cout << "🛡️  ANTI-DETACHMENT ACTIVATED - Automatically maintaining seal integrity!" << std::endl;
    }

    void onPatternStarted(const QString& patternName)
    {
        std::cout << "✅ Pattern started: " << patternName.toStdString() << std::endl;
    }

    void onPatternStopped()
    {
        std::cout << "⏹️  Pattern stopped\n" << std::endl;
        
        // Wait 2 seconds then run next demo
        QTimer::singleShot(2000, this, &AirPulseDemo::runNextDemo);
    }

    void runNextDemo()
    {
        switch (demoStep) {
        case 0:
            demoTherapeuticPulse();
            break;
        case 1:
            demoEnhancedAirPulse();
            break;
        case 2:
            demoHighFrequencyPulse();
            break;
        case 3:
            demoProgressiveIntensity();
            break;
        default:
            std::cout << "🎉 Demo completed successfully!" << std::endl;
            QCoreApplication::quit();
            return;
        }
        demoStep++;
    }

private:
    void demoTherapeuticPulse()
    {
        std::cout << "--- Demo 1: Therapeutic Blood Flow Pattern ---" << std::endl;
        std::cout << "Purpose: Blood flow enhancement and tissue engorgement" << std::endl;
        std::cout << "Parameters:" << std::endl;
        std::cout << "  - Baseline pressure: 20 mmHg (maintains seal)" << std::endl;
        std::cout << "  - Therapeutic pressure: 35 mmHg (promotes blood flow)" << std::endl;
        std::cout << "  - Frequency: 4 Hz (gentle therapeutic rate)" << std::endl;
        std::cout << "  - Duration: 10 seconds (shortened for demo)" << std::endl;

        QJsonObject params;
        params["type"] = "therapeutic_pulse";
        params["baseline_pressure_mmhg"] = 20.0;
        params["therapeutic_pressure_mmhg"] = 35.0;
        params["frequency_hz"] = 4.0;
        params["session_duration_ms"] = 10000;  // 10 seconds for demo
        params["include_warmup"] = true;
        params["include_cooldown"] = true;

        controller->startPattern("Therapeutic Blood Flow", params);

        // Stop after 10 seconds
        QTimer::singleShot(10000, controller, &VacuumController::stopPattern);
    }

    void demoEnhancedAirPulse()
    {
        std::cout << "--- Demo 2: Enhanced Air Pulse Pattern ---" << std::endl;
        std::cout << "Purpose: High-frequency stimulation across entire vulvar area" << std::endl;
        std::cout << "Parameters:" << std::endl;
        std::cout << "  - Frequency: 8 Hz (125ms cycles)" << std::endl;
        std::cout << "  - Base pressure: 28 mmHg (seal maintenance)" << std::endl;
        std::cout << "  - Pulse amplitude: 15 mmHg (up to 43 mmHg peak)" << std::endl;
        std::cout << "  - Duty cycle: 35% (44ms suction, 81ms baseline)" << std::endl;

        QJsonObject params;
        params["type"] = "air_pulse";
        params["frequency_hz"] = 8.0;
        params["base_pressure_mmhg"] = 28.0;
        params["pulse_amplitude_mmhg"] = 15.0;
        params["duty_cycle_percent"] = 35.0;
        params["cycle_count"] = 20;  // 20 cycles for demo
        params["progressive_intensity"] = false;

        controller->startPattern("Enhanced Air Pulse", params);

        // Stop after 8 seconds
        QTimer::singleShot(8000, controller, &VacuumController::stopPattern);
    }

    void demoHighFrequencyPulse()
    {
        std::cout << "--- Demo 3: High-Frequency Stimulation ---" << std::endl;
        std::cout << "Purpose: Intense stimulation similar to commercial air pulse toys" << std::endl;
        std::cout << "Parameters:" << std::endl;
        std::cout << "  - Frequency: 12 Hz (83ms cycles)" << std::endl;
        std::cout << "  - Base pressure: 30 mmHg" << std::endl;
        std::cout << "  - Pulse amplitude: 18 mmHg (up to 48 mmHg peak)" << std::endl;
        std::cout << "  - Duty cycle: 40% (33ms suction, 50ms baseline)" << std::endl;

        QJsonObject params;
        params["type"] = "air_pulse";
        params["frequency_hz"] = 12.0;
        params["base_pressure_mmhg"] = 30.0;
        params["pulse_amplitude_mmhg"] = 18.0;
        params["duty_cycle_percent"] = 40.0;
        params["cycle_count"] = 25;  // 25 cycles for demo
        params["progressive_intensity"] = false;

        controller->startPattern("High Frequency Air Pulse", params);

        // Stop after 6 seconds
        QTimer::singleShot(6000, controller, &VacuumController::stopPattern);
    }

    void demoProgressiveIntensity()
    {
        std::cout << "--- Demo 4: Progressive Intensity Pattern ---" << std::endl;
        std::cout << "Purpose: Gradually building intensity for enhanced experience" << std::endl;
        std::cout << "Parameters:" << std::endl;
        std::cout << "  - Frequency: 10 Hz" << std::endl;
        std::cout << "  - Base pressure: 25 mmHg" << std::endl;
        std::cout << "  - Pulse amplitude: 20 mmHg (up to 45 mmHg peak)" << std::endl;
        std::cout << "  - Progressive: Builds from 50% to 100% intensity" << std::endl;

        QJsonObject params;
        params["type"] = "air_pulse";
        params["frequency_hz"] = 10.0;
        params["base_pressure_mmhg"] = 25.0;
        params["pulse_amplitude_mmhg"] = 20.0;
        params["duty_cycle_percent"] = 35.0;
        params["cycle_count"] = 30;  // 30 cycles for demo
        params["progressive_intensity"] = true;

        controller->startPattern("Progressive Air Pulse", params);

        // Stop after 10 seconds
        QTimer::singleShot(10000, controller, &VacuumController::stopPattern);
    }

    VacuumController* controller;
    int demoStep;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    AirPulseDemo demo;
    demo.runDemo();

    return app.exec();
}

#include "test_enhanced_air_pulse.moc"
