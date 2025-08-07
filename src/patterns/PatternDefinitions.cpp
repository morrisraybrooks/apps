#include "PatternDefinitions.h"
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PatternDefinitions::PatternDefinitions(QObject *parent)
    : QObject(parent)
{
    initializePatterns();
}

PatternDefinitions::~PatternDefinitions()
{
}

void PatternDefinitions::initializePatterns()
{
    // Clear existing patterns
    m_patterns.clear();
    
    // Create all predefined patterns
    createPulsePatterns();
    createWavePatterns();
    createAirPulsePatterns();
    createMilkingPatterns();
    createConstantPatterns();
    createSpecialPatterns();
    createTherapeuticPatterns();
    
    qDebug() << "Initialized" << m_patterns.size() << "vacuum patterns";
}

void PatternDefinitions::loadDefaultPatterns()
{
    // This method loads default patterns - only initialize if not already done
    if (m_patterns.isEmpty()) {
        initializePatterns();
    } else {
        qDebug() << "Patterns already loaded, skipping re-initialization";
    }
}

QStringList PatternDefinitions::getPatternNames() const
{
    return m_patterns.keys();
}

QStringList PatternDefinitions::getPatternsByType(const QString& type) const
{
    QStringList result;
    
    for (auto it = m_patterns.begin(); it != m_patterns.end(); ++it) {
        if (it.value().type.contains(type, Qt::CaseInsensitive)) {
            result.append(it.key());
        }
    }
    
    return result;
}

PatternDefinitions::PatternInfo PatternDefinitions::getPattern(const QString& name) const
{
    return m_patterns.value(name, PatternInfo());
}

bool PatternDefinitions::hasPattern(const QString& name) const
{
    return m_patterns.contains(name);
}

bool PatternDefinitions::isValidPattern(const QString& name) const
{
    return hasPattern(name) && m_patterns[name].isValid;
}

bool PatternDefinitions::validatePatternParameters(const QJsonObject& parameters) const
{
    // Basic parameter validation
    if (parameters.contains("base_pressure_percent")) {
        double pressure = parameters["base_pressure_percent"].toDouble();
        if (pressure < 0.0 || pressure > 100.0) {
            return false;
        }
    }

    if (parameters.contains("intensity")) {
        double intensity = parameters["intensity"].toDouble();
        if (intensity < 0.0 || intensity > 100.0) {
            return false;
        }
    }

    if (parameters.contains("duration_ms")) {
        int duration = parameters["duration_ms"].toInt();
        if (duration < 100 || duration > 60000) {
            return false;
        }
    }

    return true;
}

QString PatternDefinitions::getPatternDescription(const QString& name) const
{
    if (hasPattern(name)) {
        return m_patterns[name].description;
    }
    return QString();
}

QString PatternDefinitions::getPatternType(const QString& name) const
{
    if (hasPattern(name)) {
        return m_patterns[name].type;
    }
    return QString();
}

QString PatternDefinitions::getPatternSpeed(const QString& name) const
{
    if (hasPattern(name)) {
        return m_patterns[name].speed;
    }
    return QString();
}

QString PatternDefinitions::getPatternCategory(const QString& name) const
{
    if (hasPattern(name)) {
        return m_patterns[name].category;
    }
    return QString();
}

void PatternDefinitions::createPulsePatterns()
{
    // Slow Pulse Pattern
    PatternInfo slowPulse;
    slowPulse.name = "Slow Pulse";
    slowPulse.type = "Pulse";
    slowPulse.description = "2-second pulses with 2-second pauses at 60% pressure";
    slowPulse.basePressure = 60.0;
    slowPulse.speed = 1.0;
    slowPulse.intensity = 60.0;
    
    PatternStep step1;
    step1.pressurePercent = 60.0;
    step1.durationMs = 2000;
    step1.action = "vacuum";
    step1.description = "Vacuum on";
    
    PatternStep step2;
    step2.pressurePercent = 0.0;
    step2.durationMs = 2000;
    step2.action = "release";
    step2.description = "Vacuum off";
    
    slowPulse.steps.append(step1);
    slowPulse.steps.append(step2);
    m_patterns["Slow Pulse"] = slowPulse;
    
    // medium Pulse Pattern
    PatternInfo mediumPulse;
    mediumPulse.name = "medium Pulse";
    mediumPulse.type = "Pulse";
    mediumPulse.description = "1-second pulses with 1-second pauses at 70% pressure";
    mediumPulse.basePressure = 70.0;
    mediumPulse.speed = 1.0;
    mediumPulse.intensity = 70.0;

    step1.pressurePercent = 70.0;
    step1.durationMs = 1000;
    step2.pressurePercent = 0.0;
    step2.durationMs = 1000;

    mediumPulse.steps.append(step1);
    mediumPulse.steps.append(step2);
    m_patterns["medium Pulse"] = mediumPulse;
    
    // Fast Pulse Pattern
    PatternInfo fastPulse;
    fastPulse.name = "Fast Pulse";
    fastPulse.type = "Pulse";
    fastPulse.description = "0.5-second pulses with 0.5-second pauses at 75% pressure";
    fastPulse.basePressure = 75.0;
    fastPulse.speed = 1.0;
    fastPulse.intensity = 75.0;
    
    step1.pressurePercent = 75.0;
    step1.durationMs = 500;
    step2.pressurePercent = 0.0;
    step2.durationMs = 500;
    
    fastPulse.steps.append(step1);
    fastPulse.steps.append(step2);
    m_patterns["Fast Pulse"] = fastPulse;
}

void PatternDefinitions::createWavePatterns()
{
    // Slow Wave Pattern
    PatternInfo slowWave;
    slowWave.name = "Slow Wave Pattern";
    slowWave.type = "Wave";
    slowWave.description = "10-second gradual pressure waves (30-70% range)";
    slowWave.basePressure = 50.0;
    slowWave.speed = 1.0;
    slowWave.intensity = 70.0;

    // Create wave steps
    for (int i = 0; i <= 10; ++i) {
        PatternStep step;
        double angle = (i * 36.0) * M_PI / 180.0; // 36 degrees per step
        step.pressurePercent = 50.0 + 20.0 * sin(angle); // 30-70% range
        step.durationMs = 1000; // 1 second per step
        step.action = "ramp";
        step.description = QString("Wave step %1").arg(i + 1);
        slowWave.steps.append(step);
    }

    m_patterns["Slow Wave Pattern"] = slowWave;
    
    // medium Wave Pattern
    PatternInfo mediumWave;
    mediumWave.name = "medium Wave Pattern";
    mediumWave.type = "Wave";
    mediumWave.description = "5-second pressure waves (40-80% range)";
    mediumWave.basePressure = 60.0;
    mediumWave.speed = 1.0;
    mediumWave.intensity = 80.0;

    for (int i = 0; i <= 10; ++i) {
        PatternStep step;
        double angle = (i * 36.0) * M_PI / 180.0;
        step.pressurePercent = 60.0 + 20.0 * sin(angle); // 40-80% range
        step.durationMs = 500; // 0.5 second per step
        step.action = "ramp";
        step.description = QString("Wave step %1").arg(i + 1);
        mediumWave.steps.append(step);
    }

    m_patterns["medium Wave Pattern"] = mediumWave;
    
    // Fast Wave Pattern
    PatternInfo fastWave;
    fastWave.name = "Fast Wave Pattern";
    fastWave.type = "Wave";
    fastWave.description = "2-second pressure waves (50-85% range)";
    fastWave.basePressure = 67.5;
    fastWave.speed = 1.0;
    fastWave.intensity = 85.0;

    for (int i = 0; i <= 10; ++i) {
        PatternStep step;
        double angle = (i * 36.0) * M_PI / 180.0;
        step.pressurePercent = 67.5 + 17.5 * sin(angle); // 50-85% range
        step.durationMs = 200; // 0.2 second per step
        step.action = "ramp";
        step.description = QString("Wave step %1").arg(i + 1);
        fastWave.steps.append(step);
    }

    m_patterns["Fast Wave Pattern"] = fastWave;
}

void PatternDefinitions::createAirPulsePatterns()
{
    // Slow Air Pulse Pattern
    PatternInfo slowAirPulse;
    slowAirPulse.name = "Slow Air Pulse";
    slowAirPulse.type = "Air Pulse";
    slowAirPulse.description = "3-second vacuum with 2-second air release";
    slowAirPulse.basePressure = 65.0;
    slowAirPulse.speed = 1.0;
    slowAirPulse.intensity = 65.0;
    
    PatternStep vacStep;
    vacStep.pressurePercent = 65.0;
    vacStep.durationMs = 3000;
    vacStep.action = "vacuum";
    vacStep.description = "Vacuum phase";
    
    PatternStep airStep;
    airStep.pressurePercent = -10.0; // Slight positive pressure
    airStep.durationMs = 2000;
    airStep.action = "air_release";
    airStep.description = "Air release phase";
    
    slowAirPulse.steps.append(vacStep);
    slowAirPulse.steps.append(airStep);
    m_patterns["Slow Air Pulse"] = slowAirPulse;
    
    // medium Air Pulse Pattern
    PatternInfo mediumAirPulse;
    mediumAirPulse.name = "medium Air Pulse";
    mediumAirPulse.type = "Air Pulse";
    mediumAirPulse.description = "2-second vacuum with 1.5-second air release";
    mediumAirPulse.basePressure = 70.0;
    mediumAirPulse.speed = 1.0;
    mediumAirPulse.intensity = 70.0;

    vacStep.pressurePercent = 70.0;
    vacStep.durationMs = 2000;
    airStep.pressurePercent = -10.0;
    airStep.durationMs = 1500;

    mediumAirPulse.steps.append(vacStep);
    mediumAirPulse.steps.append(airStep);
    m_patterns["medium Air Pulse"] = mediumAirPulse;
    
    // Fast Air Pulse Pattern
    PatternInfo fastAirPulse;
    fastAirPulse.name = "Fast Air Pulse";
    fastAirPulse.type = "Air Pulse";
    fastAirPulse.description = "1-second vacuum with 1-second air release";
    fastAirPulse.basePressure = 75.0;
    fastAirPulse.speed = 1.0;
    fastAirPulse.intensity = 75.0;
    
    vacStep.pressurePercent = 75.0;
    vacStep.durationMs = 1000;
    airStep.pressurePercent = -10.0;
    airStep.durationMs = 1000;
    
    fastAirPulse.steps.append(vacStep);
    fastAirPulse.steps.append(airStep);
    m_patterns["Fast Air Pulse"] = fastAirPulse;
}

void PatternDefinitions::createMilkingPatterns()
{
    // Slow Milking Pattern
    PatternInfo slowMilking;
    slowMilking.name = "Slow Milking";
    slowMilking.type = "Milking";
    slowMilking.description = "3-second strokes with 2-second release, 7 cycles";
    slowMilking.basePressure = 60.0;
    slowMilking.speed = 1.0;
    slowMilking.intensity = 60.0;
    
    for (int cycle = 0; cycle < 7; ++cycle) {
        // Stroke phase
        PatternStep strokeStep;
        strokeStep.pressurePercent = 60.0;
        strokeStep.durationMs = 3000;
        strokeStep.action = "vacuum";
        strokeStep.description = QString("Stroke %1").arg(cycle + 1);
        
        // Release phase
        PatternStep releaseStep;
        releaseStep.pressurePercent = 20.0;
        releaseStep.durationMs = 2000;
        releaseStep.action = "release";
        releaseStep.description = QString("Release %1").arg(cycle + 1);
        
        slowMilking.steps.append(strokeStep);
        slowMilking.steps.append(releaseStep);
    }
    
    m_patterns["Slow Milking"] = slowMilking;
    
    // medium Milking Pattern
    PatternInfo mediumMilking;
    mediumMilking.name = "medium Milking";
    mediumMilking.type = "Milking";
    mediumMilking.description = "2-second strokes with 1.5-second release, 8 cycles";
    mediumMilking.basePressure = 65.0;
    mediumMilking.speed = 1.0;
    mediumMilking.intensity = 65.0;

    for (int cycle = 0; cycle < 8; ++cycle) {
        PatternStep strokeStep;
        strokeStep.pressurePercent = 65.0;
        strokeStep.durationMs = 2000;
        strokeStep.action = "vacuum";
        strokeStep.description = QString("Stroke %1").arg(cycle + 1);

        PatternStep releaseStep;
        releaseStep.pressurePercent = 25.0;
        releaseStep.durationMs = 1500;
        releaseStep.action = "release";
        releaseStep.description = QString("Release %1").arg(cycle + 1);

        mediumMilking.steps.append(strokeStep);
        mediumMilking.steps.append(releaseStep);
    }

    m_patterns["medium Milking"] = mediumMilking;
    
    // Fast Milking Pattern
    PatternInfo fastMilking;
    fastMilking.name = "Fast Milking";
    fastMilking.type = "Milking";
    fastMilking.description = "1.5-second strokes with 1-second release, 10 cycles";
    fastMilking.basePressure = 70.0;
    fastMilking.speed = 1.0;
    fastMilking.intensity = 70.0;
    
    for (int cycle = 0; cycle < 10; ++cycle) {
        PatternStep strokeStep;
        strokeStep.pressurePercent = 70.0;
        strokeStep.durationMs = 1500;
        strokeStep.action = "vacuum";
        strokeStep.description = QString("Stroke %1").arg(cycle + 1);
        
        PatternStep releaseStep;
        releaseStep.pressurePercent = 30.0;
        releaseStep.durationMs = 1000;
        releaseStep.action = "release";
        releaseStep.description = QString("Release %1").arg(cycle + 1);
        
        fastMilking.steps.append(strokeStep);
        fastMilking.steps.append(releaseStep);
    }
    
    m_patterns["Fast Milking"] = fastMilking;
}

void PatternDefinitions::createConstantPatterns()
{
    // Single Cycle Automated Orgasm Pattern
    PatternInfo singleOrgasm;
    singleOrgasm.name = "Single Automated Orgasm";
    singleOrgasm.type = "Automated Orgasm";
    singleOrgasm.description = "Complete 5-minute arousal-to-climax cycle with 4 physiological phases";
    singleOrgasm.basePressure = 75.0;
    singleOrgasm.speed = 1.0;
    singleOrgasm.intensity = 75.0;

    // Phase 1: Initial Sensitivity (0-30 seconds) - Gentle ramp-up
    PatternStep phase1;
    phase1.pressurePercent = 35.0;  // Start very gentle
    phase1.durationMs = 10000;      // 10 seconds gentle start
    phase1.action = "gentle_ramp";
    phase1.description = "Phase 1: Initial sensitivity - gentle start";
    phase1.parameters["ramp_to"] = 55.0;  // Ramp to moderate level
    phase1.parameters["anti_detachment_priority"] = true;
    singleOrgasm.steps.append(phase1);

    PatternStep phase1b;
    phase1b.pressurePercent = 55.0;  // Moderate level
    phase1b.durationMs = 20000;      // 20 seconds to complete 30-second phase
    phase1b.action = "steady_moderate";
    phase1b.description = "Phase 1b: Settling into moderate stimulation";
    phase1b.parameters["variation"] = 5.0;
    phase1b.parameters["variation_period"] = 3000;
    singleOrgasm.steps.append(phase1b);

    // Phase 2: Adaptation Period (30 seconds - 2 minutes) - Consistent moderate
    PatternStep phase2;
    phase2.pressurePercent = 60.0;
    phase2.durationMs = 90000;       // 90 seconds (1.5 minutes)
    phase2.action = "adaptation_steady";
    phase2.description = "Phase 2: Adaptation - consistent moderate intensity";
    phase2.parameters["variation"] = 8.0;
    phase2.parameters["variation_period"] = 4000;
    phase2.parameters["maintain_seal"] = true;
    singleOrgasm.steps.append(phase2);

    // Phase 3: Arousal Build-up (2-4 minutes) - Gradual intensity increase
    PatternStep phase3a;
    phase3a.pressurePercent = 60.0;
    phase3a.durationMs = 60000;      // 60 seconds gradual ramp
    phase3a.action = "arousal_buildup";
    phase3a.description = "Phase 3a: Early arousal buildup";
    phase3a.parameters["ramp_to"] = 75.0;
    phase3a.parameters["variation"] = 10.0;
    phase3a.parameters["variation_period"] = 2500;
    singleOrgasm.steps.append(phase3a);

    PatternStep phase3b;
    phase3b.pressurePercent = 75.0;
    phase3b.durationMs = 60000;      // 60 seconds continued buildup
    phase3b.action = "arousal_intensify";
    phase3b.description = "Phase 3b: Intensifying arousal";
    phase3b.parameters["ramp_to"] = 85.0;
    phase3b.parameters["variation"] = 12.0;
    phase3b.parameters["variation_period"] = 2000;
    phase3b.parameters["enhanced_anti_detachment"] = true;
    singleOrgasm.steps.append(phase3b);

    // Phase 4: Pre-climax Tension (4-5 minutes) - Maintain precise stimulation
    PatternStep phase4;
    phase4.pressurePercent = 85.0;
    phase4.durationMs = 60000;       // 60 seconds climax phase
    phase4.action = "climax_maintain";
    phase4.description = "Phase 4: Pre-climax tension - precise stimulation";
    phase4.parameters["variation"] = 8.0;
    phase4.parameters["variation_period"] = 1500;
    phase4.parameters["maximum_anti_detachment"] = true;
    phase4.parameters["climax_mode"] = true;
    singleOrgasm.steps.append(phase4);

    m_patterns["Single Automated Orgasm"] = singleOrgasm;

    // Triple Cycle Automated Orgasm Pattern
    PatternInfo tripleOrgasm;
    tripleOrgasm.name = "Triple Automated Orgasm";
    tripleOrgasm.type = "Multi-Cycle Orgasm";
    tripleOrgasm.description = "Three consecutive 5-minute orgasm cycles with recovery periods";
    tripleOrgasm.basePressure = 75.0;
    tripleOrgasm.speed = 1.0;
    tripleOrgasm.intensity = 75.0;

    // Cycle 1: Full intensity progression
    for (const auto& step : singleOrgasm.steps) {
        PatternStep cycle1Step = step;
        cycle1Step.description = "Cycle 1: " + step.description;
        tripleOrgasm.steps.append(cycle1Step);
    }

    // Recovery Period 1: Gentle cooldown
    PatternStep recovery1;
    recovery1.pressurePercent = 30.0;
    recovery1.durationMs = 45000;    // 45 seconds recovery
    recovery1.action = "post_climax_recovery";
    recovery1.description = "Recovery 1: Post-climax sensitivity reduction";
    recovery1.parameters["variation"] = 5.0;
    recovery1.parameters["variation_period"] = 5000;
    recovery1.parameters["gentle_mode"] = true;
    tripleOrgasm.steps.append(recovery1);

    // Cycle 2: Slightly reduced initial sensitivity
    for (const auto& step : singleOrgasm.steps) {
        PatternStep cycle2Step = step;
        cycle2Step.description = "Cycle 2: " + step.description;
        // Reduce initial sensitivity for second cycle
        if (cycle2Step.action == "gentle_ramp") {
            cycle2Step.pressurePercent = 40.0;  // Start slightly higher
            cycle2Step.parameters["ramp_to"] = 60.0;
        }
        tripleOrgasm.steps.append(cycle2Step);
    }

    // Recovery Period 2: Longer cooldown
    PatternStep recovery2;
    recovery2.pressurePercent = 25.0;
    recovery2.durationMs = 60000;    // 60 seconds recovery
    recovery2.action = "post_climax_recovery";
    recovery2.description = "Recovery 2: Extended post-climax recovery";
    recovery2.parameters["variation"] = 3.0;
    recovery2.parameters["variation_period"] = 6000;
    recovery2.parameters["gentle_mode"] = true;
    tripleOrgasm.steps.append(recovery2);

    // Cycle 3: Adapted progression for final climax
    for (const auto& step : singleOrgasm.steps) {
        PatternStep cycle3Step = step;
        cycle3Step.description = "Cycle 3: " + step.description;
        // Further adapt for third cycle
        if (cycle3Step.action == "gentle_ramp") {
            cycle3Step.pressurePercent = 45.0;  // Start higher still
            cycle3Step.parameters["ramp_to"] = 65.0;
        } else if (cycle3Step.action == "climax_maintain") {
            cycle3Step.durationMs = 75000;  // Longer final climax phase
        }
        tripleOrgasm.steps.append(cycle3Step);
    }

    // Final cooldown
    PatternStep finalCooldown;
    finalCooldown.pressurePercent = 20.0;
    finalCooldown.durationMs = 90000;  // 90 seconds final recovery
    finalCooldown.action = "final_recovery";
    finalCooldown.description = "Final cooldown: Complete session recovery";
    finalCooldown.parameters["variation"] = 2.0;
    finalCooldown.parameters["variation_period"] = 8000;
    finalCooldown.parameters["gentle_mode"] = true;
    tripleOrgasm.steps.append(finalCooldown);

    m_patterns["Triple Automated Orgasm"] = tripleOrgasm;

    // Continuous Orgasm Marathon Pattern - Endless cycling
    PatternInfo continuousOrgasm;
    continuousOrgasm.name = "Continuous Orgasm Marathon";
    continuousOrgasm.type = "Continuous Orgasm";
    continuousOrgasm.description = "Endless orgasm cycles - runs continuously until manually stopped";
    continuousOrgasm.basePressure = 75.0;
    continuousOrgasm.speed = 1.0;
    continuousOrgasm.intensity = 75.0;

    // Create a repeating cycle pattern that loops indefinitely
    // Each cycle is optimized for continuous operation with shorter recovery periods

    // Phase 1: Quick Sensitivity Check (0-15 seconds) - Shortened for continuous operation
    PatternStep continuousPhase1;
    continuousPhase1.pressurePercent = 40.0;  // Start slightly higher for continuous mode
    continuousPhase1.durationMs = 5000;       // 5 seconds quick start
    continuousPhase1.action = "continuous_gentle_ramp";
    continuousPhase1.description = "Continuous Phase 1: Quick sensitivity adaptation";
    continuousPhase1.parameters["ramp_to"] = 60.0;
    continuousPhase1.parameters["continuous_mode"] = true;
    continuousOrgasm.steps.append(continuousPhase1);

    PatternStep continuousPhase1b;
    continuousPhase1b.pressurePercent = 60.0;
    continuousPhase1b.durationMs = 10000;     // 10 seconds settling
    continuousPhase1b.action = "continuous_steady_moderate";
    continuousPhase1b.description = "Continuous Phase 1b: Quick settling";
    continuousPhase1b.parameters["variation"] = 6.0;
    continuousPhase1b.parameters["variation_period"] = 2500;
    continuousOrgasm.steps.append(continuousPhase1b);

    // Phase 2: Shortened Adaptation (15-45 seconds) - Reduced for continuous flow
    PatternStep continuousPhase2;
    continuousPhase2.pressurePercent = 65.0;  // Higher base for continuous mode
    continuousPhase2.durationMs = 30000;      // 30 seconds (half of single cycle)
    continuousPhase2.action = "continuous_adaptation";
    continuousPhase2.description = "Continuous Phase 2: Rapid adaptation";
    continuousPhase2.parameters["variation"] = 10.0;
    continuousPhase2.parameters["variation_period"] = 3000;
    continuousPhase2.parameters["maintain_seal"] = true;
    continuousOrgasm.steps.append(continuousPhase2);

    // Phase 3: Accelerated Buildup (45 seconds - 2 minutes) - Faster progression
    PatternStep continuousPhase3a;
    continuousPhase3a.pressurePercent = 65.0;
    continuousPhase3a.durationMs = 30000;     // 30 seconds rapid ramp
    continuousPhase3a.action = "continuous_arousal_buildup";
    continuousPhase3a.description = "Continuous Phase 3a: Accelerated arousal buildup";
    continuousPhase3a.parameters["ramp_to"] = 80.0;
    continuousPhase3a.parameters["variation"] = 12.0;
    continuousPhase3a.parameters["variation_period"] = 2000;
    continuousOrgasm.steps.append(continuousPhase3a);

    PatternStep continuousPhase3b;
    continuousPhase3b.pressurePercent = 80.0;
    continuousPhase3b.durationMs = 45000;     // 45 seconds intensification
    continuousPhase3b.action = "continuous_arousal_intensify";
    continuousPhase3b.description = "Continuous Phase 3b: Rapid intensification";
    continuousPhase3b.parameters["ramp_to"] = 88.0;
    continuousPhase3b.parameters["variation"] = 15.0;
    continuousPhase3b.parameters["variation_period"] = 1500;
    continuousPhase3b.parameters["enhanced_anti_detachment"] = true;
    continuousOrgasm.steps.append(continuousPhase3b);

    // Phase 4: Extended Climax (2-3.5 minutes) - Longer climax for continuous pleasure
    PatternStep continuousPhase4;
    continuousPhase4.pressurePercent = 88.0;
    continuousPhase4.durationMs = 90000;      // 90 seconds extended climax
    continuousPhase4.action = "continuous_climax_maintain";
    continuousPhase4.description = "Continuous Phase 4: Extended climax maintenance";
    continuousPhase4.parameters["variation"] = 10.0;
    continuousPhase4.parameters["variation_period"] = 1200;
    continuousPhase4.parameters["maximum_anti_detachment"] = true;
    continuousPhase4.parameters["continuous_climax_mode"] = true;
    continuousOrgasm.steps.append(continuousPhase4);

    // Brief Recovery/Transition (3.5-4 minutes) - Minimal recovery for continuous flow
    PatternStep continuousRecovery;
    continuousRecovery.pressurePercent = 45.0;  // Higher than normal recovery
    continuousRecovery.durationMs = 30000;      // 30 seconds brief recovery
    continuousRecovery.action = "continuous_brief_recovery";
    continuousRecovery.description = "Continuous Recovery: Brief transition for next cycle";
    continuousRecovery.parameters["variation"] = 8.0;
    continuousRecovery.parameters["variation_period"] = 4000;
    continuousRecovery.parameters["prepare_next_cycle"] = true;
    continuousOrgasm.steps.append(continuousRecovery);

    // Mark this pattern as repeating/looping
    continuousOrgasm.parameters["infinite_loop"] = true;
    continuousOrgasm.parameters["cycle_duration_minutes"] = 4.0;  // 4-minute cycles
    continuousOrgasm.parameters["auto_repeat"] = true;

    m_patterns["Continuous Orgasm Marathon"] = continuousOrgasm;

    // Legacy patterns for compatibility
    createLegacyConstantPatterns();
}

void PatternDefinitions::createLegacyConstantPatterns()
{
    // Keep original constant patterns for backward compatibility

    // Slow Constant Orgasm Pattern (Legacy)
    PatternInfo slowConstantOrgasm;
    slowConstantOrgasm.name = "Slow Constant Orgasm (Legacy)";
    slowConstantOrgasm.type = "Constant Orgasm";
    slowConstantOrgasm.description = "70% base pressure with ±15% variation over 3 seconds, designed for sustained pleasure";
    slowConstantOrgasm.basePressure = 70.0;
    slowConstantOrgasm.speed = 1.0;
    slowConstantOrgasm.intensity = 70.0;

    PatternStep constantStep;
    constantStep.pressurePercent = 70.0;
    constantStep.durationMs = 3000;
    constantStep.action = "constant_orgasm";
    constantStep.description = "Constant orgasmic pressure with variation";
    constantStep.parameters["variation"] = 15.0;
    constantStep.parameters["variation_period"] = 3000;
    constantStep.parameters["orgasm_mode"] = true;

    slowConstantOrgasm.steps.append(constantStep);
    m_patterns["Slow Constant Orgasm (Legacy)"] = slowConstantOrgasm;

    // Medium Constant Orgasm Pattern (Legacy)
    PatternInfo mediumConstantOrgasm;
    mediumConstantOrgasm.name = "Medium Constant Orgasm (Legacy)";
    mediumConstantOrgasm.type = "Constant Orgasm";
    mediumConstantOrgasm.description = "75% base pressure with ±10% variation over 2 seconds, designed for sustained pleasure";
    mediumConstantOrgasm.basePressure = 75.0;
    mediumConstantOrgasm.speed = 1.0;
    mediumConstantOrgasm.intensity = 75.0;

    constantStep.pressurePercent = 75.0;
    constantStep.durationMs = 2000;
    constantStep.action = "constant_orgasm";
    constantStep.description = "Medium constant orgasmic pressure with variation";
    constantStep.parameters["variation"] = 10.0;
    constantStep.parameters["variation_period"] = 2000;
    constantStep.parameters["orgasm_mode"] = true;

    mediumConstantOrgasm.steps.append(constantStep);
    m_patterns["Medium Constant Orgasm (Legacy)"] = mediumConstantOrgasm;

    // Fast Constant Orgasm Pattern (Legacy)
    PatternInfo fastConstantOrgasm;
    fastConstantOrgasm.name = "Fast Constant Orgasm (Legacy)";
    fastConstantOrgasm.type = "Constant Orgasm";
    fastConstantOrgasm.description = "80% base pressure with ±5% variation over 1 second, designed for sustained pleasure";
    fastConstantOrgasm.basePressure = 80.0;
    fastConstantOrgasm.speed = 1.0;
    fastConstantOrgasm.intensity = 80.0;

    constantStep.pressurePercent = 80.0;
    constantStep.durationMs = 1000;
    constantStep.action = "constant_orgasm";
    constantStep.description = "Fast constant orgasmic pressure with variation";
    constantStep.parameters["variation"] = 5.0;
    constantStep.parameters["variation_period"] = 1000;
    constantStep.parameters["orgasm_mode"] = true;

    fastConstantOrgasm.steps.append(constantStep);
    m_patterns["Fast Constant Orgasm (Legacy)"] = fastConstantOrgasm;
}

void PatternDefinitions::createSpecialPatterns()
{
    // Edging Pattern
    PatternInfo edging;
    edging.name = "Edging";
    edging.type = "Special";
    edging.description = "15-second buildup to 85%, 5-second release, 3-second hold, repeated 3 times";
    edging.basePressure = 85.0;
    edging.speed = 1.0;
    edging.intensity = 85.0;
    
    for (int cycle = 0; cycle < 3; ++cycle) {
        // Buildup phase - gradual increase to 85%
        for (int i = 0; i < 15; ++i) {
            PatternStep buildupStep;
            buildupStep.pressurePercent = 20.0 + (65.0 * i / 14.0); // 20% to 85%
            buildupStep.durationMs = 1000;
            buildupStep.action = "ramp";
            buildupStep.description = QString("Buildup %1 step %2").arg(cycle + 1).arg(i + 1);
            edging.steps.append(buildupStep);
        }
        
        // Release phase
        PatternStep releaseStep;
        releaseStep.pressurePercent = 10.0;
        releaseStep.durationMs = 5000;
        releaseStep.action = "release";
        releaseStep.description = QString("Release %1").arg(cycle + 1);
        edging.steps.append(releaseStep);
        
        // Hold phase
        PatternStep holdStep;
        holdStep.pressurePercent = 30.0;
        holdStep.durationMs = 3000;
        holdStep.action = "hold";
        holdStep.description = QString("Hold %1").arg(cycle + 1);
        edging.steps.append(holdStep);
    }
    
    m_patterns["Edging"] = edging;
}

void PatternDefinitions::createTherapeuticPatterns()
{
    // Therapeutic Blood Flow Pattern
    PatternInfo therapeuticFlow;
    therapeuticFlow.name = "Therapeutic Blood Flow";
    therapeuticFlow.type = "Therapeutic";
    therapeuticFlow.description = "Optimized for blood circulation and tissue engorgement across entire vulvar area";
    therapeuticFlow.basePressure = 25.0;
    therapeuticFlow.speed = 1.0;
    therapeuticFlow.intensity = 75.0;
    therapeuticFlow.category = "Therapeutic";

    // Warmup phase
    for (int i = 0; i < 5; ++i) {
        PatternStep warmupStep;
        warmupStep.pressurePercent = 15.0 + (10.0 * i / 4.0); // 15% to 25%
        warmupStep.durationMs = 2000;
        warmupStep.action = "therapeutic_warmup";
        warmupStep.description = QString("Warmup phase %1").arg(i + 1);
        therapeuticFlow.steps.append(warmupStep);

        PatternStep baselineStep;
        baselineStep.pressurePercent = 15.0;
        baselineStep.durationMs = 1000;
        baselineStep.action = "maintain_baseline";
        baselineStep.description = "Baseline maintenance";
        therapeuticFlow.steps.append(baselineStep);
    }

    // Main therapeutic phase
    for (int i = 0; i < 20; ++i) {
        PatternStep mainStep;
        mainStep.pressurePercent = 35.0;
        mainStep.durationMs = 1500;
        mainStep.action = "therapeutic_main";
        mainStep.description = "Therapeutic pressure";
        therapeuticFlow.steps.append(mainStep);

        PatternStep baselineStep;
        baselineStep.pressurePercent = 20.0;
        baselineStep.durationMs = 750;
        baselineStep.action = "maintain_baseline";
        baselineStep.description = "Baseline maintenance";
        therapeuticFlow.steps.append(baselineStep);
    }

    m_patterns["Therapeutic Blood Flow"] = therapeuticFlow;

    // Enhanced Air Pulse for Single Chamber
    PatternInfo enhancedAirPulse;
    enhancedAirPulse.name = "Enhanced Single Chamber Air Pulse";
    enhancedAirPulse.type = "Enhanced Air Pulse";
    enhancedAirPulse.description = "High-frequency air pulse optimized for single-chamber uniform pressure distribution";
    enhancedAirPulse.basePressure = 28.0;
    enhancedAirPulse.speed = 1.0;
    enhancedAirPulse.intensity = 85.0;
    enhancedAirPulse.category = "Air Pulse";

    // Progressive intensity air pulse
    for (int i = 0; i < 30; ++i) {
        double intensityMultiplier = 0.5 + (0.5 * std::min(1.0, static_cast<double>(i) / 15.0));

        PatternStep pulseStep;
        pulseStep.pressurePercent = 28.0 + (17.0 * intensityMultiplier); // 28-45 mmHg range
        pulseStep.durationMs = 40;
        pulseStep.action = "therapeutic_suction";
        pulseStep.description = QString("Air pulse %1").arg(i + 1);
        enhancedAirPulse.steps.append(pulseStep);

        PatternStep baselineStep;
        baselineStep.pressurePercent = 25.0;
        baselineStep.durationMs = 85;
        baselineStep.action = "maintain_baseline";
        baselineStep.description = "Baseline maintenance";
        enhancedAirPulse.steps.append(baselineStep);
    }

    m_patterns["Enhanced Single Chamber Air Pulse"] = enhancedAirPulse;
}

bool PatternDefinitions::validatePulsePattern(const QJsonObject& params) const
{
    if (!params.contains("pulseDuration") || !params.contains("pauseDuration")) {
        return false;
    }

    int pulseDuration = params["pulseDuration"].toInt();
    int pauseDuration = params["pauseDuration"].toInt();

    return isValidDuration(pulseDuration) && isValidDuration(pauseDuration);
}

bool PatternDefinitions::validateWavePattern(const QJsonObject& params) const
{
    if (!params.contains("period") || !params.contains("minPressure") || !params.contains("maxPressure")) {
        return false;
    }

    int period = params["period"].toInt();
    double minPressure = params["minPressure"].toDouble();
    double maxPressure = params["maxPressure"].toDouble();

    return isValidDuration(period) &&
           isValidPressurePercent(minPressure) &&
           isValidPressurePercent(maxPressure) &&
           minPressure < maxPressure;
}

bool PatternDefinitions::validateAirPulsePattern(const QJsonObject& params) const
{
    if (!params.contains("pulseDuration") || !params.contains("releaseDuration")) {
        return false;
    }

    int pulseDuration = params["pulseDuration"].toInt();
    int releaseDuration = params["releaseDuration"].toInt();

    return isValidDuration(pulseDuration) && isValidDuration(releaseDuration);
}

bool PatternDefinitions::validateMilkingPattern(const QJsonObject& params) const
{
    if (!params.contains("strokeDuration") || !params.contains("releaseDuration") || !params.contains("strokeCount")) {
        return false;
    }

    int strokeDuration = params["strokeDuration"].toInt();
    int releaseDuration = params["releaseDuration"].toInt();
    int strokeCount = params["strokeCount"].toInt();

    return isValidDuration(strokeDuration) &&
           isValidDuration(releaseDuration) &&
           strokeCount > 0 && strokeCount <= 20;
}

bool PatternDefinitions::validateConstantPattern(const QJsonObject& params) const
{
    if (!params.contains("basePressure") || !params.contains("variation")) {
        return false;
    }

    double basePressure = params["basePressure"].toDouble();
    double variation = params["variation"].toDouble();

    return isValidPressurePercent(basePressure) &&
           variation >= 0.0 && variation <= 20.0;
}

bool PatternDefinitions::validateEdgingPattern(const QJsonObject& params) const
{
    if (!params.contains("buildupDuration") || !params.contains("peakPressure") || !params.contains("cycles")) {
        return false;
    }

    int buildupDuration = params["buildupDuration"].toInt();
    double peakPressure = params["peakPressure"].toDouble();
    int cycles = params["cycles"].toInt();

    return isValidDuration(buildupDuration) &&
           isValidPressurePercent(peakPressure) &&
           cycles > 0 && cycles <= 10;
}

bool PatternDefinitions::isValidPressurePercent(double pressure) const
{
    return pressure >= 0.0 && pressure <= 100.0;
}

bool PatternDefinitions::isValidDuration(int duration) const
{
    return duration >= 100 && duration <= 60000; // 100ms to 60 seconds
}

bool PatternDefinitions::isValidSpeed(const QString& speed) const
{
    return speed == "slow" || speed == "medium" || speed == "fast";
}
