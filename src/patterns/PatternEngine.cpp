#include "PatternEngine.h"
#include "PatternDefinitions.h"
#include "../hardware/HardwareManager.h"
#include "../safety/AntiDetachmentMonitor.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <QJsonDocument>
#include <QFile>
#include <cmath>
#include <algorithm>

// Constants
const double PatternEngine::DEFAULT_INTENSITY = 100.0;
const double PatternEngine::DEFAULT_SPEED_MULTIPLIER = 1.0;
const double PatternEngine::DEFAULT_PRESSURE_OFFSET = 0.0;
const double PatternEngine::MIN_PRESSURE_PERCENT = 10.0;
const double PatternEngine::MAX_PRESSURE_PERCENT = 90.0;
const int PatternEngine::MIN_STEP_DURATION_MS = 100;
const int PatternEngine::MAX_STEP_DURATION_MS = 60000;

PatternEngine::PatternEngine(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_antiDetachmentMonitor(nullptr)
    , m_state(STOPPED)
    , m_currentPatternType(PULSE)
    , m_currentStep(0)
    , m_patternStartTime(0)
    , m_stepStartTime(0)
    , m_pausedTime(0)
    , m_totalPausedTime(0)
    , m_stepTimer(new QTimer(this))
    , m_safetyTimer(new QTimer(this))
    , m_emergencyStop(false)
    , m_infiniteLoop(false)
    , m_completedCycles(0)
    , m_intensity(DEFAULT_INTENSITY)
    , m_speedMultiplier(DEFAULT_SPEED_MULTIPLIER)
    , m_pressureOffset(DEFAULT_PRESSURE_OFFSET)
    , m_minPressure(MIN_PRESSURE_PERCENT)
    , m_maxPressure(MAX_PRESSURE_PERCENT)
    , m_patternDefinitions(std::make_unique<PatternDefinitions>())
{
    // Set up timers
    m_stepTimer->setSingleShot(true);
    connect(m_stepTimer, &QTimer::timeout, this, &PatternEngine::onStepTimer);
    
    m_safetyTimer->setInterval(SAFETY_CHECK_INTERVAL_MS);
    connect(m_safetyTimer, &QTimer::timeout, this, &PatternEngine::onSafetyCheck);
    
    // Load default patterns
    m_patternDefinitions->loadDefaultPatterns();
    
    qDebug() << "Pattern engine initialized";
}

PatternEngine::~PatternEngine()
{
    stopPattern();
}

bool PatternEngine::startPattern(const QString& patternName, const QJsonObject& parameters)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == RUNNING) {
        qWarning() << "Pattern already running, stopping current pattern first";
        stopPattern();
    }
    
    if (!m_hardware || !m_hardware->isReady()) {
        qCritical() << "Hardware not ready for pattern execution";
        emit patternError("Hardware not ready");
        return false;
    }
    
    if (!initializePattern(patternName, parameters)) {
        return false;
    }
    
    try {
        setState(STARTING);
        
        m_currentPatternName = patternName;
        m_currentStep = 0;
        m_patternStartTime = QDateTime::currentMSecsSinceEpoch();
        m_totalPausedTime = 0;
        m_emergencyStop = false;
        
        // Start safety monitoring
        m_safetyTimer->start();

        // Start anti-detachment monitoring if available
        if (m_antiDetachmentMonitor && !m_antiDetachmentMonitor->isActive()) {
            if (m_antiDetachmentMonitor->initialize()) {
                m_antiDetachmentMonitor->startMonitoring();
                qDebug() << "Anti-detachment monitoring started with pattern";
            } else {
                qWarning() << "Failed to initialize anti-detachment monitoring";
            }
        }

        // Execute first step
        executeNextStep();
        
        setState(RUNNING);
        emit patternStarted(patternName);
        
        qDebug() << "Pattern started:" << patternName;
        return true;
        
    } catch (const std::exception& e) {
        QString error = QString("Failed to start pattern %1: %2").arg(patternName, e.what());
        qCritical() << error;
        emit patternError(error);
        setState(ERROR);
        return false;
    }
}

void PatternEngine::stopPattern()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == STOPPED) return;
    
    setState(STOPPING);
    
    // Stop timers
    m_stepTimer->stop();
    m_safetyTimer->stop();

    // Stop anti-detachment monitoring
    if (m_antiDetachmentMonitor && m_antiDetachmentMonitor->isActive()) {
        m_antiDetachmentMonitor->stopMonitoring();
        qDebug() << "Anti-detachment monitoring stopped with pattern";
    }

    // Set hardware to safe state
    if (m_hardware) {
        try {
            m_hardware->setPumpSpeed(0.0);
            m_hardware->setPumpEnabled(false);
            m_hardware->setSOL2(true);  // Open vent valve
            m_hardware->setSOL3(true);  // Open tank vent valve
        } catch (const std::exception& e) {
            qWarning() << "Error setting hardware to safe state:" << e.what();
        }
    }
    
    // Clear pattern data
    m_patternSteps.clear();
    m_currentStep = 0;
    m_currentPatternName.clear();
    
    setState(STOPPED);
    emit patternStopped();
    
    qDebug() << "Pattern stopped";
}

void PatternEngine::pausePattern()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state != RUNNING) return;
    
    m_pausedTime = QDateTime::currentMSecsSinceEpoch();
    m_stepTimer->stop();
    
    // Set hardware to hold state
    if (m_hardware) {
        m_hardware->setPumpSpeed(0.0);
    }
    
    setState(PAUSED);
    emit patternPaused();
    
    qDebug() << "Pattern paused";
}

void PatternEngine::resumePattern()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state != PAUSED) return;
    
    // Calculate paused duration
    qint64 pauseDuration = QDateTime::currentMSecsSinceEpoch() - m_pausedTime;
    m_totalPausedTime += pauseDuration;
    
    // Resume step timer with remaining time
    if (m_currentStep < m_patternSteps.size()) {
        executeNextStep();
    }
    
    setState(RUNNING);
    emit patternResumed();
    
    qDebug() << "Pattern resumed";
}

void PatternEngine::emergencyStop()
{
    QMutexLocker locker(&m_stateMutex);
    
    qCritical() << "PATTERN ENGINE EMERGENCY STOP";
    
    m_emergencyStop = true;
    
    // Immediately stop all timers
    m_stepTimer->stop();
    m_safetyTimer->stop();
    
    // Set hardware to emergency safe state
    if (m_hardware) {
        m_hardware->emergencyStop();
    }
    
    setState(ERROR);
    emit patternError("Emergency stop activated");
}

bool PatternEngine::initializePattern(const QString& patternName, const QJsonObject& parameters)
{
    if (!m_patternDefinitions->isValidPattern(patternName)) {
        QString error = QString("Invalid pattern: %1").arg(patternName);
        emit patternError(error);
        return false;
    }
    
    PatternDefinitions::PatternInfo patternInfo = m_patternDefinitions->getPattern(patternName);
    if (!patternInfo.isValid) {
        QString error = QString("Failed to load pattern: %1").arg(patternName);
        emit patternError(error);
        return false;
    }
    
    // Build pattern steps based on type
    buildPatternSteps(parameters);
    
    if (m_patternSteps.isEmpty()) {
        QString error = QString("No steps generated for pattern: %1").arg(patternName);
        emit patternError(error);
        return false;
    }
    
    // Determine pattern type
    QString typeStr = patternInfo.type.toLower();
    if (typeStr == "pulse") m_currentPatternType = PULSE;
    else if (typeStr == "wave") m_currentPatternType = WAVE;
    else if (typeStr == "air_pulse") m_currentPatternType = AIR_PULSE;
    else if (typeStr == "milking") m_currentPatternType = MILKING;
    else if (typeStr == "constant") m_currentPatternType = CONSTANT;
    else if (typeStr == "automated orgasm") m_currentPatternType = AUTOMATED_ORGASM;
    else if (typeStr == "multi-cycle orgasm") m_currentPatternType = MULTI_CYCLE_ORGASM;
    else if (typeStr == "continuous orgasm") m_currentPatternType = CONTINUOUS_ORGASM;
    else if (typeStr == "edging") m_currentPatternType = EDGING;
    else m_currentPatternType = CUSTOM;
    
    return true;
}

void PatternEngine::buildPatternSteps(const QJsonObject& patternData)
{
    m_patternSteps.clear();
    
    QString type = patternData["type"].toString().toLower();
    
    if (type == "pulse") {
        buildPulsePattern(patternData);
    } else if (type == "wave") {
        buildWavePattern(patternData);
    } else if (type == "air_pulse") {
        buildAirPulsePattern(patternData);
    } else if (type == "milking") {
        buildMilkingPattern(patternData);
    } else if (type == "constant") {
        buildConstantPattern(patternData);
    } else if (type == "automated orgasm" || type == "multi-cycle orgasm") {
        buildAutomatedOrgasmPattern(patternData);
    } else if (type == "continuous orgasm") {
        buildContinuousOrgasmPattern(patternData);
    } else if (type == "edging") {
        buildEdgingPattern(patternData);
    } else if (type == "therapeutic_pulse") {
        buildTherapeuticPulsePattern(patternData);
    }
}

void PatternEngine::buildPulsePattern(const QJsonObject& params)
{
    int pulseDuration = params["pulse_duration_ms"].toInt(1000);
    int pauseDuration = params["pause_duration_ms"].toInt(1000);
    double pressure = params["pressure_percent"].toDouble(60.0);
    
    // Create repeating pulse pattern (10 cycles for demonstration)
    for (int i = 0; i < 10; ++i) {
        m_patternSteps.append(PatternStep(pressure, pulseDuration, "vacuum"));
        m_patternSteps.append(PatternStep(0.0, pauseDuration, "release"));
    }
}

void PatternEngine::buildWavePattern(const QJsonObject& params)
{
    int period = params["wave_period_ms"].toInt(5000);
    double minPressure = params["min_pressure_percent"].toDouble(30.0);
    double maxPressure = params["max_pressure_percent"].toDouble(70.0);
    
    // Create wave pattern with 20 steps per cycle
    int stepDuration = period / 20;
    for (int i = 0; i < 20; ++i) {
        double angle = (i * 2.0 * M_PI) / 20.0;
        double pressure = minPressure + (maxPressure - minPressure) * (sin(angle) + 1.0) / 2.0;
        m_patternSteps.append(PatternStep(pressure, stepDuration, "vacuum"));
    }
}

void PatternEngine::buildAirPulsePattern(const QJsonObject& params)
{
    // Enhanced air pulse for single-chamber therapeutic system
    double frequency = params["frequency_hz"].toDouble(8.0);
    double basePressure = params["base_pressure_mmhg"].toDouble(25.0);
    double pulseAmplitude = params["pulse_amplitude_mmhg"].toDouble(15.0);
    double dutyCycle = params["duty_cycle_percent"].toDouble(35.0) / 100.0;
    int totalCycles = params["cycle_count"].toInt(20);
    bool progressiveIntensity = params["progressive_intensity"].toBool(false);

    // Calculate timing from frequency
    int cycleDurationMs = static_cast<int>(1000.0 / frequency);
    int suctionDurationMs = static_cast<int>(cycleDurationMs * dutyCycle);
    int releaseDurationMs = cycleDurationMs - suctionDurationMs;

    // Convert mmHg to percentage (assuming 100 mmHg = 100%)
    double basePressurePercent = basePressure;
    double maxPressurePercent = basePressure + pulseAmplitude;

    // Create therapeutic air pulse pattern
    for (int i = 0; i < totalCycles; ++i) {
        double intensityMultiplier = 1.0;
        if (progressiveIntensity) {
            // Gradually increase intensity over first 50% of cycles
            double progress = static_cast<double>(i) / totalCycles;
            if (progress < 0.5) {
                intensityMultiplier = 0.5 + progress; // Start at 50%, reach 100% at halfway
            }
        }

        double currentMaxPressure = basePressurePercent + (pulseAmplitude * intensityMultiplier);

        // Suction phase: Apply vacuum for blood flow and stimulation
        m_patternSteps.append(PatternStep(currentMaxPressure, suctionDurationMs, "therapeutic_suction"));

        // Release phase: Return to baseline (not zero - maintains seal and drainage)
        m_patternSteps.append(PatternStep(basePressurePercent, releaseDurationMs, "maintain_baseline"));
    }
}

void PatternEngine::buildMilkingPattern(const QJsonObject& params)
{
    int strokeDuration = params["stroke_duration_ms"].toInt(2000);
    int releaseDuration = params["release_duration_ms"].toInt(1500);
    double pressure = params["pressure_percent"].toDouble(75.0);
    int strokeCount = params["stroke_count"].toInt(7);
    
    // Create milking pattern
    for (int i = 0; i < strokeCount; ++i) {
        m_patternSteps.append(PatternStep(pressure, strokeDuration, "vacuum"));
        m_patternSteps.append(PatternStep(pressure * 0.3, releaseDuration, "release"));
    }
}

void PatternEngine::buildConstantPattern(const QJsonObject& params)
{
    double basePressure = params["base_pressure_percent"].toDouble(70.0);
    double variation = params["variation_percent"].toDouble(15.0);
    int variationPeriod = params["variation_period_ms"].toInt(3000);
    
    // Create constant pattern with variations (30 seconds total)
    int steps = 30000 / variationPeriod;
    for (int i = 0; i < steps; ++i) {
        double variationAmount = variation * sin(i * 2.0 * M_PI / steps);
        double pressure = basePressure + variationAmount;
        m_patternSteps.append(PatternStep(pressure, variationPeriod, "vacuum"));
    }
}

void PatternEngine::buildEdgingPattern(const QJsonObject& params)
{
    int buildupDuration = params["buildup_duration_ms"].toInt(15000);
    double peakPressure = params["peak_pressure_percent"].toDouble(85.0);
    int releaseDuration = params["release_duration_ms"].toInt(5000);
    int holdDuration = params["hold_duration_ms"].toInt(3000);
    int cycles = params["cycles"].toInt(3);
    
    // Create edging pattern
    for (int cycle = 0; cycle < cycles; ++cycle) {
        // Buildup phase
        int buildupSteps = buildupDuration / 1000;  // 1 second per step
        for (int i = 0; i < buildupSteps; ++i) {
            double progress = static_cast<double>(i) / buildupSteps;
            double pressure = peakPressure * progress;
            m_patternSteps.append(PatternStep(pressure, 1000, "vacuum"));
        }
        
        // Release phase
        m_patternSteps.append(PatternStep(0.0, releaseDuration, "release"));
        
        // Hold phase
        m_patternSteps.append(PatternStep(peakPressure * 0.2, holdDuration, "hold"));
    }
}

void PatternEngine::buildTherapeuticPulsePattern(const QJsonObject& params)
{
    // Optimized for blood flow, engorgement, and therapeutic benefits
    double baselinePressure = params["baseline_pressure_mmhg"].toDouble(20.0);
    double therapeuticPressure = params["therapeutic_pressure_mmhg"].toDouble(35.0);
    double frequency = params["frequency_hz"].toDouble(4.0);  // Slower for therapy
    int sessionDuration = params["session_duration_ms"].toInt(300000);  // 5 minutes
    bool warmupPhase = params["include_warmup"].toBool(true);
    bool cooldownPhase = params["include_cooldown"].toBool(true);

    int cycleDurationMs = static_cast<int>(1000.0 / frequency);
    int suctionDurationMs = static_cast<int>(cycleDurationMs * 0.6);  // 60% duty cycle for therapy
    int releaseDurationMs = cycleDurationMs - suctionDurationMs;

    int totalCycles = sessionDuration / cycleDurationMs;
    int warmupCycles = warmupPhase ? totalCycles / 10 : 0;  // 10% warmup
    int cooldownCycles = cooldownPhase ? totalCycles / 10 : 0;  // 10% cooldown
    int mainCycles = totalCycles - warmupCycles - cooldownCycles;

    // Warmup phase: gradually increase from baseline to therapeutic pressure
    for (int i = 0; i < warmupCycles; ++i) {
        double progress = static_cast<double>(i) / warmupCycles;
        double currentPressure = baselinePressure + (therapeuticPressure - baselinePressure) * progress;

        m_patternSteps.append(PatternStep(currentPressure, suctionDurationMs, "therapeutic_warmup"));
        m_patternSteps.append(PatternStep(baselinePressure, releaseDurationMs, "maintain_baseline"));
    }

    // Main therapeutic phase: consistent therapeutic pressure
    for (int i = 0; i < mainCycles; ++i) {
        m_patternSteps.append(PatternStep(therapeuticPressure, suctionDurationMs, "therapeutic_main"));
        m_patternSteps.append(PatternStep(baselinePressure, releaseDurationMs, "maintain_baseline"));
    }

    // Cooldown phase: gradually decrease to baseline
    for (int i = 0; i < cooldownCycles; ++i) {
        double progress = static_cast<double>(i) / cooldownCycles;
        double currentPressure = therapeuticPressure - (therapeuticPressure - baselinePressure) * progress;

        m_patternSteps.append(PatternStep(currentPressure, suctionDurationMs, "therapeutic_cooldown"));
        m_patternSteps.append(PatternStep(baselinePressure, releaseDurationMs, "maintain_baseline"));
    }
}

void PatternEngine::buildAutomatedOrgasmPattern(const QJsonObject& params)
{
    // Build automated orgasm patterns based on physiological response phases
    // This handles both single and multi-cycle patterns

    QString patternName = params["name"].toString();
    bool isMultiCycle = patternName.contains("Triple") || params["cycles"].toInt(1) > 1;
    int cycles = isMultiCycle ? params["cycles"].toInt(3) : 1;

    for (int cycle = 0; cycle < cycles; ++cycle) {
        // Adjust intensities based on cycle number (sensitivity adaptation)
        double sensitivityMultiplier = 1.0 + (cycle * 0.15);  // Increase by 15% each cycle
        double initialIntensity = 35.0 + (cycle * 10.0);      // Start higher each cycle

        // Phase 1: Initial Sensitivity (0-30 seconds) - Gentle ramp-up
        // 10 seconds gentle start
        double startPressure = initialIntensity;
        double rampTarget = 55.0 + (cycle * 5.0);
        int rampSteps = 5;  // 2-second steps
        for (int i = 0; i < rampSteps; ++i) {
            double progress = static_cast<double>(i) / rampSteps;
            double pressure = startPressure + (rampTarget - startPressure) * progress;
            m_patternSteps.append(PatternStep(pressure, 2000, "gentle_ramp"));
        }

        // 20 seconds moderate steady
        double moderatePressure = rampTarget;
        for (int i = 0; i < 10; ++i) {  // 2-second steps
            double variation = 5.0 * sin(i * 0.6);  // Gentle variation
            m_patternSteps.append(PatternStep(moderatePressure + variation, 2000, "steady_moderate"));
        }

        // Phase 2: Adaptation Period (30 seconds - 2 minutes) - Consistent moderate
        double adaptationPressure = 60.0 + (cycle * 5.0);
        int adaptationSteps = 45;  // 2-second steps for 90 seconds
        for (int i = 0; i < adaptationSteps; ++i) {
            double variation = 8.0 * sin(i * 0.4);  // Slow variation
            m_patternSteps.append(PatternStep(adaptationPressure + variation, 2000, "adaptation_steady"));
        }

        // Phase 3: Arousal Build-up (2-4 minutes) - Gradual intensity increase
        // Phase 3a: Early buildup (60 seconds)
        double buildupStart = 60.0 + (cycle * 5.0);
        double buildupMid = 75.0 + (cycle * 5.0);
        int buildup1Steps = 30;  // 2-second steps
        for (int i = 0; i < buildup1Steps; ++i) {
            double progress = static_cast<double>(i) / buildup1Steps;
            double pressure = buildupStart + (buildupMid - buildupStart) * progress;
            double variation = 10.0 * sin(i * 0.5);
            m_patternSteps.append(PatternStep(pressure + variation, 2000, "arousal_buildup"));
        }

        // Phase 3b: Intensifying buildup (60 seconds)
        double buildupEnd = 85.0 + (cycle * 3.0);  // Cap at reasonable level
        int buildup2Steps = 30;  // 2-second steps
        for (int i = 0; i < buildup2Steps; ++i) {
            double progress = static_cast<double>(i) / buildup2Steps;
            double pressure = buildupMid + (buildupEnd - buildupMid) * progress;
            double variation = 12.0 * sin(i * 0.6);
            m_patternSteps.append(PatternStep(pressure + variation, 2000, "arousal_intensify"));
        }

        // Phase 4: Pre-climax Tension (4-5 minutes) - Maintain precise stimulation
        double climaxPressure = std::min(85.0 + (cycle * 3.0), 90.0);  // Cap at 90%
        int climaxDuration = (cycle == cycles - 1) ? 75000 : 60000;  // Longer final climax
        int climaxSteps = climaxDuration / 1500;  // 1.5-second steps for precision
        for (int i = 0; i < climaxSteps; ++i) {
            double variation = 8.0 * sin(i * 0.8);  // Faster variation for climax
            m_patternSteps.append(PatternStep(climaxPressure + variation, 1500, "climax_maintain"));
        }

        // Recovery period between cycles (except after last cycle)
        if (cycle < cycles - 1) {
            double recoveryPressure = 30.0 - (cycle * 5.0);  // Gentler each time
            recoveryPressure = std::max(recoveryPressure, 20.0);  // Minimum 20%
            int recoveryDuration = (cycle == 0) ? 45000 : 60000;  // Longer recovery after first
            int recoverySteps = recoveryDuration / 5000;  // 5-second steps
            for (int i = 0; i < recoverySteps; ++i) {
                double variation = 3.0 * sin(i * 0.3);  // Very gentle variation
                m_patternSteps.append(PatternStep(recoveryPressure + variation, 5000, "post_climax_recovery"));
            }
        }
    }

    // Final cooldown for multi-cycle patterns
    if (isMultiCycle) {
        double cooldownPressure = 20.0;
        int cooldownSteps = 18;  // 90 seconds / 5 seconds per step
        for (int i = 0; i < cooldownSteps; ++i) {
            double variation = 2.0 * sin(i * 0.2);  // Minimal variation
            m_patternSteps.append(PatternStep(cooldownPressure + variation, 5000, "final_recovery"));
        }
    }
}

void PatternEngine::buildContinuousOrgasmPattern(const QJsonObject& params)
{
    // Build continuous orgasm pattern that loops indefinitely
    // Optimized 4-minute cycles for continuous operation

    m_infiniteLoop = params["infinite_loop"].toBool(true);
    m_completedCycles = 0;

    // Phase 1: Quick Sensitivity Adaptation (0-15 seconds)
    // 5 seconds quick ramp
    double startPressure = 40.0;  // Higher start for continuous mode
    double rampTarget = 60.0;
    int rampSteps = 3;  // Faster ramp for continuous operation
    for (int i = 0; i < rampSteps; ++i) {
        double progress = static_cast<double>(i) / rampSteps;
        double pressure = startPressure + (rampTarget - startPressure) * progress;
        m_patternSteps.append(PatternStep(pressure, 1500, "continuous_gentle_ramp"));
    }

    // 10 seconds quick settling
    for (int i = 0; i < 5; ++i) {  // 2-second steps
        double variation = 6.0 * sin(i * 0.8);
        m_patternSteps.append(PatternStep(60.0 + variation, 2000, "continuous_steady_moderate"));
    }

    // Phase 2: Rapid Adaptation (15-45 seconds) - 30 seconds total
    double adaptationPressure = 65.0;  // Higher for continuous mode
    int adaptationSteps = 15;  // 2-second steps for 30 seconds
    for (int i = 0; i < adaptationSteps; ++i) {
        double variation = 10.0 * sin(i * 0.5);
        m_patternSteps.append(PatternStep(adaptationPressure + variation, 2000, "continuous_adaptation"));
    }

    // Phase 3: Accelerated Buildup (45 seconds - 2 minutes) - 75 seconds total
    // Phase 3a: Rapid buildup (30 seconds)
    double buildupStart = 65.0;
    double buildupMid = 80.0;
    int buildup1Steps = 15;  // 2-second steps
    for (int i = 0; i < buildup1Steps; ++i) {
        double progress = static_cast<double>(i) / buildup1Steps;
        double pressure = buildupStart + (buildupMid - buildupStart) * progress;
        double variation = 12.0 * sin(i * 0.6);
        m_patternSteps.append(PatternStep(pressure + variation, 2000, "continuous_arousal_buildup"));
    }

    // Phase 3b: Rapid intensification (45 seconds)
    double buildupEnd = 88.0;  // Higher peak for continuous mode
    int buildup2Steps = 23;  // ~2-second steps
    for (int i = 0; i < buildup2Steps; ++i) {
        double progress = static_cast<double>(i) / buildup2Steps;
        double pressure = buildupMid + (buildupEnd - buildupMid) * progress;
        double variation = 15.0 * sin(i * 0.7);
        m_patternSteps.append(PatternStep(pressure + variation, 2000, "continuous_arousal_intensify"));
    }

    // Phase 4: Extended Climax (2-3.5 minutes) - 90 seconds
    double climaxPressure = 88.0;
    int climaxSteps = 75;  // 1.2-second steps for precision during extended climax
    for (int i = 0; i < climaxSteps; ++i) {
        double variation = 10.0 * sin(i * 0.9);  // Faster variation for continuous climax
        m_patternSteps.append(PatternStep(climaxPressure + variation, 1200, "continuous_climax_maintain"));
    }

    // Brief Recovery/Transition (3.5-4 minutes) - 30 seconds
    double recoveryPressure = 45.0;  // Higher than normal recovery for continuous flow
    int recoverySteps = 6;  // 5-second steps
    for (int i = 0; i < recoverySteps; ++i) {
        double variation = 8.0 * sin(i * 0.4);
        m_patternSteps.append(PatternStep(recoveryPressure + variation, 5000, "continuous_brief_recovery"));
    }

    qDebug() << QString("Continuous orgasm pattern built: %1 steps, %2 minute cycles")
                .arg(m_patternSteps.size()).arg(4.0);
}

void PatternEngine::executeNextStep()
{
    if (m_currentStep >= m_patternSteps.size()) {
        // Check if this is an infinite loop pattern (continuous orgasm)
        if (m_infiniteLoop && m_currentPatternType == CONTINUOUS_ORGASM) {
            // Reset to beginning for continuous cycling
            m_currentStep = 0;
            m_completedCycles++;

            qDebug() << QString("Continuous Orgasm: Starting cycle %1").arg(m_completedCycles + 1);
            emit cycleCompleted(m_completedCycles);

            // Continue with first step of next cycle
            const PatternStep& step = m_patternSteps[m_currentStep];
            executeStep(step);

            int adjustedDuration = applySpeedMultiplier(step.durationMs);
            m_stepTimer->start(adjustedDuration);
            m_stepStartTime = QDateTime::currentMSecsSinceEpoch();

            emit stepChanged(m_currentStep, m_patternSteps.size());
            emit progressUpdated(getProgress());
            return;
        } else {
            // Pattern completed normally
            setState(STOPPED);
            emit patternCompleted();
            return;
        }
    }
    
    const PatternStep& step = m_patternSteps[m_currentStep];
    executeStep(step);
    
    // Set timer for next step
    int adjustedDuration = applySpeedMultiplier(step.durationMs);
    m_stepTimer->start(adjustedDuration);
    m_stepStartTime = QDateTime::currentMSecsSinceEpoch();
    
    emit stepChanged(m_currentStep, m_patternSteps.size());
    emit progressUpdated(getProgress());
}

void PatternEngine::executeStep(const PatternStep& step)
{
    if (!m_hardware) return;

    try {
        double adjustedPressure = applyIntensityAndOffset(step.pressurePercent);

        // Enhanced anti-detachment handling for automated orgasm patterns
        if (m_currentPatternType == AUTOMATED_ORGASM || m_currentPatternType == MULTI_CYCLE_ORGASM ||
            m_currentPatternType == CONTINUOUS_ORGASM) {
            // Set anti-detachment sensitivity based on phase
            if (step.action == "climax_maintain" || step.action == "arousal_intensify" ||
                step.action == "continuous_climax_maintain" || step.action == "continuous_arousal_intensify") {
                // Maximum anti-detachment sensitivity during critical phases
                if (m_antiDetachmentMonitor) {
                    m_antiDetachmentMonitor->setResponseDelay(25);  // Faster response (25ms)
                    m_antiDetachmentMonitor->setMaxVacuumIncrease(30.0);  // Higher correction (30%)
                }
            } else if (step.action == "post_climax_recovery" || step.action == "final_recovery" ||
                       step.action == "continuous_brief_recovery") {
                // Gentle anti-detachment during recovery
                if (m_antiDetachmentMonitor) {
                    m_antiDetachmentMonitor->setResponseDelay(150);  // Slower response (150ms)
                    m_antiDetachmentMonitor->setMaxVacuumIncrease(15.0);  // Gentler correction (15%)
                }
            }
        }

        // Apply pressure target
        applyPressureTarget(adjustedPressure);

        emit pressureTargetChanged(adjustedPressure);

        // Log phase transitions for automated orgasm patterns
        if (m_currentPatternType == AUTOMATED_ORGASM || m_currentPatternType == MULTI_CYCLE_ORGASM ||
            m_currentPatternType == CONTINUOUS_ORGASM) {
            static QString lastAction;
            if (step.action != lastAction) {
                qDebug() << QString("Automated Orgasm Phase: %1 - Pressure: %2%")
                            .arg(step.action).arg(adjustedPressure, 0, 'f', 1);
                lastAction = step.action;
            }
        }

    } catch (const std::exception& e) {
        qWarning() << "Error executing step:" << e.what();
    }
}

void PatternEngine::applyPressureTarget(double targetPressure)
{
    if (!m_hardware) return;

    // Check if anti-detachment is currently active
    bool antiDetachmentActive = isAntiDetachmentActive();

    // Convert pressure percentage to pump speed
    double pumpSpeed = std::max(0.0, std::min(100.0, targetPressure));

    if (pumpSpeed > 0) {
        m_hardware->setPumpEnabled(true);
        m_hardware->setPumpSpeed(pumpSpeed);

        // Only control SOL1 if anti-detachment is not active
        // Anti-detachment system takes priority for SOL1 control
        if (!antiDetachmentActive) {
            m_hardware->setSOL1(true);   // Open vacuum line
        }
        m_hardware->setSOL2(false);  // Close vent valve
    } else {
        // For therapeutic patterns, maintain minimal vacuum for seal
        // instead of complete release
        if (targetPressure > 0) {
            // Maintain baseline pressure for seal integrity
            m_hardware->setPumpEnabled(true);
            m_hardware->setPumpSpeed(targetPressure);

            // Only control SOL1 if anti-detachment is not active
            if (!antiDetachmentActive) {
                m_hardware->setSOL1(true);   // Keep vacuum line open
            }
            m_hardware->setSOL2(false);  // Keep vent valve closed
        } else {
            // Complete release only when explicitly set to 0
            // And only if anti-detachment is not preventing it
            if (!antiDetachmentActive) {
                m_hardware->setPumpSpeed(0.0);
                m_hardware->setSOL1(false);  // Close vacuum line
                m_hardware->setSOL2(true);   // Open vent valve for release
            } else {
                // Anti-detachment active - maintain minimal pressure
                qDebug() << "Anti-detachment active - maintaining minimal pressure instead of full release";
                m_hardware->setPumpSpeed(20.0);  // Minimal pressure to maintain seal
            }
        }
    }
}

void PatternEngine::onStepTimer()
{
    m_currentStep++;
    executeNextStep();
}

void PatternEngine::onSafetyCheck()
{
    performSafetyCheck();
}

void PatternEngine::performSafetyCheck()
{
    if (!m_hardware || m_emergencyStop) return;
    
    try {
        // Check pressure limits
        double avlPressure = m_hardware->readAVLPressure();
        double tankPressure = m_hardware->readTankPressure();
        
        // Convert mmHg to percentage for comparison
        double maxPressureMmHg = 100.0;  // Assuming 100 mmHg max
        double avlPercent = (avlPressure / maxPressureMmHg) * 100.0;
        double tankPercent = (tankPressure / maxPressureMmHg) * 100.0;
        
        if (avlPercent > m_maxPressure || tankPercent > m_maxPressure) {
            emergencyStop();
            emit patternError("Pressure limit exceeded during pattern execution");
        }
        
    } catch (const std::exception& e) {
        qWarning() << "Safety check error:" << e.what();
    }
}

double PatternEngine::applyIntensityAndOffset(double basePressure)
{
    // Apply intensity scaling
    double adjustedPressure = basePressure * (m_intensity / 100.0);
    
    // Apply offset
    adjustedPressure += m_pressureOffset;
    
    // Clamp to safe limits
    return std::max(m_minPressure, std::min(m_maxPressure, adjustedPressure));
}

int PatternEngine::applySpeedMultiplier(int baseDuration)
{
    int adjustedDuration = static_cast<int>(baseDuration / m_speedMultiplier);
    return std::max(MIN_STEP_DURATION_MS, std::min(MAX_STEP_DURATION_MS, adjustedDuration));
}

double PatternEngine::getProgress() const
{
    if (m_patternSteps.isEmpty()) return 0.0;
    return static_cast<double>(m_currentStep) / m_patternSteps.size() * 100.0;
}

qint64 PatternEngine::getElapsedTime() const
{
    if (m_patternStartTime == 0) return 0;
    return QDateTime::currentMSecsSinceEpoch() - m_patternStartTime - m_totalPausedTime;
}

void PatternEngine::setState(PatternState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

void PatternEngine::setIntensity(double intensityPercent)
{
    m_intensity = std::max(0.0, std::min(100.0, intensityPercent));
}

void PatternEngine::setSpeed(double speedMultiplier)
{
    m_speedMultiplier = std::max(0.1, std::min(3.0, speedMultiplier));
}

void PatternEngine::setPressureOffset(double offsetPercent)
{
    m_pressureOffset = std::max(-20.0, std::min(20.0, offsetPercent));
}

void PatternEngine::setPatternParameters(const QJsonObject& parameters)
{
    // Set pattern parameters - could update intensity, speed, etc.
    if (parameters.contains("intensity")) {
        setIntensity(parameters["intensity"].toDouble());
    }
    if (parameters.contains("speed")) {
        setSpeed(parameters["speed"].toDouble());
    }
    if (parameters.contains("pressure_offset")) {
        setPressureOffset(parameters["pressure_offset"].toDouble());
    }
}

PatternDefinitions* PatternEngine::getPatternDefinitions() const
{
    return m_patternDefinitions.get();
}

void PatternEngine::setAntiDetachmentMonitor(AntiDetachmentMonitor* monitor)
{
    if (m_antiDetachmentMonitor) {
        // Disconnect previous monitor signals
        disconnect(m_antiDetachmentMonitor, nullptr, this, nullptr);
    }

    m_antiDetachmentMonitor = monitor;

    if (m_antiDetachmentMonitor) {
        // Connect anti-detachment signals
        connect(m_antiDetachmentMonitor, &AntiDetachmentMonitor::detachmentDetected,
                this, &PatternEngine::antiDetachmentTriggered);
        connect(m_antiDetachmentMonitor, &AntiDetachmentMonitor::detachmentWarning,
                this, &PatternEngine::sealIntegrityWarning);
        connect(m_antiDetachmentMonitor, &AntiDetachmentMonitor::systemError,
                this, [this](const QString& error) {
                    qCritical() << "Anti-detachment system error during pattern:" << error;
                    emergencyStop();
                });

        qDebug() << "Anti-detachment monitor integrated with pattern engine";
    }
}

bool PatternEngine::isAntiDetachmentActive() const
{
    return m_antiDetachmentMonitor && m_antiDetachmentMonitor->isSOL1Active();
}
