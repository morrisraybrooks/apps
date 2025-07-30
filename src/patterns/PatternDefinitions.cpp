#include "PatternDefinitions.h"
#include <QDebug>

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
    
    qDebug() << "Initialized" << m_patterns.size() << "vacuum patterns";
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
    
    // Medium Pulse Pattern
    PatternInfo mediumPulse;
    mediumPulse.name = "Medium Pulse";
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
    m_patterns["Medium Pulse"] = mediumPulse;
    
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
    slowWave.name = "Slow Wave";
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
    
    m_patterns["Slow Wave"] = slowWave;
    
    // Medium Wave Pattern
    PatternInfo mediumWave;
    mediumWave.name = "Medium Wave";
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
    
    m_patterns["Medium Wave"] = mediumWave;
    
    // Fast Wave Pattern
    PatternInfo fastWave;
    fastWave.name = "Fast Wave";
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
    
    m_patterns["Fast Wave"] = fastWave;
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
    
    // Medium Air Pulse Pattern
    PatternInfo mediumAirPulse;
    mediumAirPulse.name = "Medium Air Pulse";
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
    m_patterns["Medium Air Pulse"] = mediumAirPulse;
    
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
    
    // Medium Milking Pattern
    PatternInfo mediumMilking;
    mediumMilking.name = "Medium Milking";
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
    
    m_patterns["Medium Milking"] = mediumMilking;
    
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
    // Slow Constant Pattern
    PatternInfo slowConstant;
    slowConstant.name = "Slow Constant";
    slowConstant.type = "Constant";
    slowConstant.description = "70% base pressure with ±15% variation over 3 seconds";
    slowConstant.basePressure = 70.0;
    slowConstant.speed = 1.0;
    slowConstant.intensity = 70.0;
    
    PatternStep constantStep;
    constantStep.pressurePercent = 70.0;
    constantStep.durationMs = 3000;
    constantStep.action = "constant";
    constantStep.description = "Constant pressure with variation";
    constantStep.parameters["variation"] = 15.0;
    constantStep.parameters["variation_period"] = 3000;
    
    slowConstant.steps.append(constantStep);
    m_patterns["Slow Constant"] = slowConstant;
    
    // Medium Constant Pattern
    PatternInfo mediumConstant;
    mediumConstant.name = "Medium Constant";
    mediumConstant.type = "Constant";
    mediumConstant.description = "75% base pressure with ±10% variation over 2 seconds";
    mediumConstant.basePressure = 75.0;
    mediumConstant.speed = 1.0;
    mediumConstant.intensity = 75.0;
    
    constantStep.pressurePercent = 75.0;
    constantStep.durationMs = 2000;
    constantStep.parameters["variation"] = 10.0;
    constantStep.parameters["variation_period"] = 2000;
    
    mediumConstant.steps.append(constantStep);
    m_patterns["Medium Constant"] = mediumConstant;
    
    // Fast Constant Pattern
    PatternInfo fastConstant;
    fastConstant.name = "Fast Constant";
    fastConstant.type = "Constant";
    fastConstant.description = "80% base pressure with ±5% variation over 1 second";
    fastConstant.basePressure = 80.0;
    fastConstant.speed = 1.0;
    fastConstant.intensity = 80.0;
    
    constantStep.pressurePercent = 80.0;
    constantStep.durationMs = 1000;
    constantStep.parameters["variation"] = 5.0;
    constantStep.parameters["variation_period"] = 1000;
    
    fastConstant.steps.append(constantStep);
    m_patterns["Fast Constant"] = fastConstant;
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
