#include "PatternEngine.h"
#include "PatternDefinitions.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <QJsonDocument>
#include <QFile>
#include <cmath>

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

bool PatternEngine::startPattern(const QString& patternName)
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
    
    if (!initializePattern(patternName)) {
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

bool PatternEngine::initializePattern(const QString& patternName)
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
    buildPatternSteps(patternInfo.parameters);
    
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
    } else if (type == "edging") {
        buildEdgingPattern(patternData);
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
    int pulseDuration = params["pulse_duration_ms"].toInt(1200);
    int releaseDuration = params["air_release_duration_ms"].toInt(1800);
    double pressure = params["pressure_percent"].toDouble(75.0);
    
    // Create air pulse pattern (8 cycles)
    for (int i = 0; i < 8; ++i) {
        m_patternSteps.append(PatternStep(pressure, pulseDuration, "vacuum"));
        m_patternSteps.append(PatternStep(0.0, releaseDuration, "air_release"));
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

void PatternEngine::executeNextStep()
{
    if (m_currentStep >= m_patternSteps.size()) {
        // Pattern completed
        setState(STOPPED);
        emit patternCompleted();
        return;
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
        
        // Apply pressure target
        applyPressureTarget(adjustedPressure);
        
        emit pressureTargetChanged(adjustedPressure);
        
    } catch (const std::exception& e) {
        qWarning() << "Error executing step:" << e.what();
    }
}

void PatternEngine::applyPressureTarget(double targetPressure)
{
    if (!m_hardware) return;
    
    // Convert pressure percentage to pump speed
    double pumpSpeed = std::max(0.0, std::min(100.0, targetPressure));
    
    if (pumpSpeed > 0) {
        m_hardware->setPumpEnabled(true);
        m_hardware->setPumpSpeed(pumpSpeed);
        m_hardware->setSOL1(true);   // Open vacuum line
        m_hardware->setSOL2(false);  // Close vent valve
    } else {
        m_hardware->setPumpSpeed(0.0);
        m_hardware->setSOL1(false);  // Close vacuum line
        m_hardware->setSOL2(true);   // Open vent valve for release
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
