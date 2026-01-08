#include "CameraManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>
#include <algorithm>
#include <numeric>

CameraManager::CameraManager(QObject* parent)
    : QObject(parent)
    , m_combinedMotionMagnitude(0.0)
    , m_combinedMotionLevel(CameraMotionSensor::MotionLevel::NONE)
    , m_combinedStillnessScore(1.0)
    , m_bodyWeight(0.6)
    , m_cupWeight(0.4)
    , m_orgasmDetectionEnabled(false)
    , m_orgasmState(OrgasmState::NONE)
    , m_orgasmConfidence(0.0)
    , m_orgasmOnsetTime(0)
    , m_recordingConsent(false)
    , m_privacyMode(true)
{
    m_orgasmAnalysisTimer = new QTimer(this);
    m_orgasmAnalysisTimer->setInterval(33);  // ~30 Hz analysis
    connect(m_orgasmAnalysisTimer, &QTimer::timeout, this, &CameraManager::onOrgasmAnalysisTick);
    
    m_cupMotionHistory.reserve(ORGASM_HISTORY_SIZE);
}

CameraManager::~CameraManager()
{
    shutdown();
}

bool CameraManager::initialize()
{
    // Auto-detect cameras if not configured
    CameraConfig bodyConfig;
    bodyConfig.deviceIndex = 0;
    bodyConfig.type = CameraMotionSensor::CameraType::USB_WEBCAM;
    bodyConfig.motionWeight = m_bodyWeight;
    
    CameraConfig cupConfig;
    cupConfig.deviceIndex = 1;
    cupConfig.type = CameraMotionSensor::CameraType::USB_WEBCAM;
    cupConfig.motionWeight = m_cupWeight;
    
    bool bodyOk = initializeBodyCamera(bodyConfig);
    bool cupOk = initializeCupCamera(cupConfig);
    
    return bodyOk || cupOk;  // At least one camera needed
}

bool CameraManager::initializeBodyCamera(const CameraConfig& config)
{
    QMutexLocker locker(&m_mutex);
    
    m_bodyConfig = config;
    m_bodyCamera = std::make_unique<CameraMotionSensor>(
        config.type, CameraMotionSensor::CameraRole::BODY_CAMERA, this);
    
    if (config.deviceIndex >= 0) {
        m_bodyCamera->setDeviceIndex(config.deviceIndex);
    }
    if (!config.deviceUrl.isEmpty()) {
        m_bodyCamera->setDeviceUrl(config.deviceUrl);
    }
    if (!config.roi.isEmpty()) {
        m_bodyCamera->setRegionOfInterest(config.roi);
    }
    
    bool success = m_bodyCamera->initialize();
    connectCameraSignals(m_bodyCamera.get(), CameraMotionSensor::CameraRole::BODY_CAMERA);
    
    emit cameraInitialized(CameraMotionSensor::CameraRole::BODY_CAMERA, success);
    return success;
}

bool CameraManager::initializeCupCamera(const CameraConfig& config)
{
    QMutexLocker locker(&m_mutex);
    
    m_cupConfig = config;
    m_cupCamera = std::make_unique<CameraMotionSensor>(
        config.type, CameraMotionSensor::CameraRole::CUP_CAMERA, this);
    
    if (config.deviceIndex >= 0) {
        m_cupCamera->setDeviceIndex(config.deviceIndex);
    }
    if (!config.deviceUrl.isEmpty()) {
        m_cupCamera->setDeviceUrl(config.deviceUrl);
    }
    if (!config.roi.isEmpty()) {
        m_cupCamera->setRegionOfInterest(config.roi);
    }
    
    bool success = m_cupCamera->initialize();
    connectCameraSignals(m_cupCamera.get(), CameraMotionSensor::CameraRole::CUP_CAMERA);
    
    emit cameraInitialized(CameraMotionSensor::CameraRole::CUP_CAMERA, success);
    return success;
}

void CameraManager::shutdown()
{
    m_orgasmAnalysisTimer->stop();
    
    if (m_bodyCamera) {
        m_bodyCamera->stopCapture();
        m_bodyCamera.reset();
    }
    if (m_cupCamera) {
        m_cupCamera->stopCapture();
        m_cupCamera.reset();
    }
}

bool CameraManager::isReady() const
{
    return hasBodyCamera() || hasCupCamera();
}

bool CameraManager::calibrateBothCameras(int durationMs)
{
    bool success = true;
    int totalSteps = 0;
    int completedSteps = 0;
    
    if (hasBodyCamera()) totalSteps++;
    if (hasCupCamera()) totalSteps++;
    
    if (totalSteps == 0) return false;
    
    if (hasBodyCamera()) {
        emit calibrationProgress(0);
        if (!m_bodyCamera->calibrate(durationMs / 2)) {
            success = false;
        }
        completedSteps++;
        emit calibrationProgress((completedSteps * 100) / totalSteps);
    }
    
    if (hasCupCamera()) {
        if (!m_cupCamera->calibrate(durationMs / 2)) {
            success = false;
        }
        completedSteps++;
        emit calibrationProgress(100);
    }
    
    emit calibrationComplete(success);
    return success;
}

bool CameraManager::isCalibrated() const
{
    bool calibrated = true;
    if (hasBodyCamera()) calibrated &= m_bodyCamera->isCalibrated();
    if (hasCupCamera()) calibrated &= m_cupCamera->isCalibrated();
    return calibrated;
}

void CameraManager::connectCameraSignals(CameraMotionSensor* camera, CameraMotionSensor::CameraRole role)
{
    if (!camera) return;

    if (role == CameraMotionSensor::CameraRole::BODY_CAMERA) {
        connect(camera, &CameraMotionSensor::motionDetected,
                this, &CameraManager::onBodyCameraMotion);
        connect(camera, &CameraMotionSensor::violationDetected,
                this, &CameraManager::onBodyCameraViolation);
    } else {
        connect(camera, &CameraMotionSensor::motionDetected,
                this, &CameraManager::onCupCameraMotion);
        connect(camera, &CameraMotionSensor::violationDetected,
                this, &CameraManager::onCupCameraViolation);
    }

    connect(camera, &CameraMotionSensor::sensorError,
            this, [this, role](const QString& error) {
        emit cameraError(role, error);
    });

    connect(camera, &CameraMotionSensor::frameReady,
            this, [this, role](const QImage& frame) {
        emit frameReady(role, frame);
    });
}

// Motion event handlers
void CameraManager::onBodyCameraMotion(CameraMotionSensor::MotionLevel level, double magnitude)
{
    Q_UNUSED(level)
    Q_UNUSED(magnitude)
    updateCombinedMotion();
}

void CameraManager::onCupCameraMotion(CameraMotionSensor::MotionLevel level, double magnitude)
{
    Q_UNUSED(level)

    // Store cup motion for orgasm pattern analysis
    if (m_orgasmDetectionEnabled) {
        QMutexLocker locker(&m_mutex);
        m_cupMotionHistory.append(magnitude);
        if (m_cupMotionHistory.size() > ORGASM_HISTORY_SIZE) {
            m_cupMotionHistory.removeFirst();
        }
    }

    updateCombinedMotion();
}

void CameraManager::onBodyCameraViolation(CameraMotionSensor::MotionLevel level, double intensity)
{
    emit violationDetected(CameraMotionSensor::CameraRole::BODY_CAMERA, level, intensity);
}

void CameraManager::onCupCameraViolation(CameraMotionSensor::MotionLevel level, double intensity)
{
    emit violationDetected(CameraMotionSensor::CameraRole::CUP_CAMERA, level, intensity);
}

void CameraManager::updateCombinedMotion()
{
    double bodyMag = hasBodyCamera() ? m_bodyCamera->getMotionMagnitude() : 0.0;
    double cupMag = hasCupCamera() ? m_cupCamera->getMotionMagnitude() : 0.0;

    double totalWeight = 0.0;
    double weightedSum = 0.0;

    if (hasBodyCamera()) {
        weightedSum += bodyMag * m_bodyWeight;
        totalWeight += m_bodyWeight;
    }
    if (hasCupCamera()) {
        weightedSum += cupMag * m_cupWeight;
        totalWeight += m_cupWeight;
    }

    m_combinedMotionMagnitude = (totalWeight > 0) ? (weightedSum / totalWeight) : 0.0;

    // Determine combined motion level
    if (m_combinedMotionMagnitude < 0.1) {
        m_combinedMotionLevel = CameraMotionSensor::MotionLevel::NONE;
    } else if (m_combinedMotionMagnitude < 0.3) {
        m_combinedMotionLevel = CameraMotionSensor::MotionLevel::MINIMAL;
    } else if (m_combinedMotionMagnitude < 0.5) {
        m_combinedMotionLevel = CameraMotionSensor::MotionLevel::MODERATE;
    } else if (m_combinedMotionMagnitude < 0.7) {
        m_combinedMotionLevel = CameraMotionSensor::MotionLevel::SIGNIFICANT;
    } else {
        m_combinedMotionLevel = CameraMotionSensor::MotionLevel::EXTREME;
    }

    // Calculate stillness score (inverse of motion)
    double prevStillness = m_combinedStillnessScore;
    m_combinedStillnessScore = 1.0 - std::min(1.0, m_combinedMotionMagnitude);

    emit combinedMotionDetected(m_combinedMotionLevel, m_combinedMotionMagnitude);

    bool wasStill = prevStillness > 0.8;
    bool isStill = m_combinedStillnessScore > 0.8;
    if (wasStill != isStill) {
        emit combinedStillnessChanged(isStill, m_combinedStillnessScore);
    }
}

double CameraManager::getCombinedMotionMagnitude() const
{
    return m_combinedMotionMagnitude;
}

CameraMotionSensor::MotionLevel CameraManager::getCombinedMotionLevel() const
{
    return m_combinedMotionLevel;
}

double CameraManager::getCombinedStillnessScore() const
{
    return m_combinedStillnessScore;
}

// Orgasm detection
void CameraManager::enableOrgasmDetection(bool enable)
{
    m_orgasmDetectionEnabled = enable;
    if (enable) {
        m_orgasmAnalysisTimer->start();
    } else {
        m_orgasmAnalysisTimer->stop();
        m_orgasmState = OrgasmState::NONE;
        m_orgasmConfidence = 0.0;
    }
}

void CameraManager::onOrgasmAnalysisTick()
{
    if (!m_orgasmDetectionEnabled || !hasCupCamera()) return;
    analyzeOrgasmIndicators();
}

void CameraManager::analyzeOrgasmIndicators()
{
    QMutexLocker locker(&m_mutex);

    if (m_cupMotionHistory.size() < 10) return;

    // Analyze motion patterns for orgasm indicators:
    // 1. Rhythmic contractions (periodic motion spikes)
    // 2. Increasing intensity buildup
    // 3. Sustained high-intensity motion
    // 4. Post-orgasm relaxation

    int historySize = m_cupMotionHistory.size();
    double recentAvg = 0.0;
    double olderAvg = 0.0;
    int recentCount = std::min(15, historySize / 2);
    int olderCount = historySize - recentCount;

    for (int i = historySize - recentCount; i < historySize; ++i) {
        recentAvg += m_cupMotionHistory[i];
    }
    recentAvg /= recentCount;

    for (int i = 0; i < olderCount; ++i) {
        olderAvg += m_cupMotionHistory[i];
    }
    olderAvg /= olderCount;

    // Detect rhythmic patterns (variance in recent motion)
    double variance = 0.0;
    for (int i = historySize - recentCount; i < historySize; ++i) {
        double diff = m_cupMotionHistory[i] - recentAvg;
        variance += diff * diff;
    }
    variance /= recentCount;
    double rhythmScore = std::min(1.0, variance * 10.0);

    // Detect intensity buildup
    double buildupScore = (recentAvg > olderAvg * 1.5) ? std::min(1.0, (recentAvg - olderAvg) * 2.0) : 0.0;

    // Detect sustained high intensity
    double sustainedScore = (recentAvg > 0.6) ? std::min(1.0, recentAvg) : 0.0;

    // Combined confidence
    double newConfidence = (rhythmScore * 0.3 + buildupScore * 0.4 + sustainedScore * 0.3);

    // Smooth confidence changes
    m_orgasmConfidence = m_orgasmConfidence * 0.7 + newConfidence * 0.3;

    // State machine for orgasm detection
    OrgasmState prevState = m_orgasmState;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    switch (m_orgasmState) {
        case OrgasmState::NONE:
            if (m_orgasmConfidence > 0.3 && buildupScore > 0.2) {
                m_orgasmState = OrgasmState::PRE_ORGASM;
            }
            break;

        case OrgasmState::PRE_ORGASM:
            if (m_orgasmConfidence > 0.6) {
                m_orgasmState = OrgasmState::ORGASM_ONSET;
                m_orgasmOnsetTime = now;
            } else if (m_orgasmConfidence < 0.15) {
                m_orgasmState = OrgasmState::NONE;
            }
            break;

        case OrgasmState::ORGASM_ONSET:
            if (m_orgasmConfidence > 0.8) {
                m_orgasmState = OrgasmState::ORGASM_PEAK;
            } else if (m_orgasmConfidence < 0.4) {
                m_orgasmState = OrgasmState::POST_ORGASM;
            }
            break;

        case OrgasmState::ORGASM_PEAK:
            if (m_orgasmConfidence < 0.5) {
                m_orgasmState = OrgasmState::POST_ORGASM;
                qint64 duration = now - m_orgasmOnsetTime;
                emit orgasmDetected(m_orgasmConfidence, duration);
            }
            break;

        case OrgasmState::POST_ORGASM:
            if (m_orgasmConfidence < 0.1) {
                m_orgasmState = OrgasmState::NONE;
            }
            break;
    }

    if (prevState != m_orgasmState) {
        emit orgasmStateChanged(m_orgasmState, m_orgasmConfidence);
    }
}

// Configuration
void CameraManager::setSensitivity(CameraMotionSensor::SensitivityPreset preset)
{
    if (hasBodyCamera()) m_bodyCamera->setSensitivity(preset);
    if (hasCupCamera()) m_cupCamera->setSensitivity(preset);
}

void CameraManager::setBodyCameraWeight(double weight)
{
    m_bodyWeight = std::clamp(weight, 0.0, 1.0);
}

void CameraManager::setCupCameraWeight(double weight)
{
    m_cupWeight = std::clamp(weight, 0.0, 1.0);
}

// Privacy controls
void CameraManager::setRecordingConsent(bool consent)
{
    m_recordingConsent = consent;
    qDebug() << "Camera recording consent:" << (consent ? "granted" : "revoked");
}

void CameraManager::setPrivacyMode(bool enabled)
{
    m_privacyMode = enabled;
    // In privacy mode, frames are processed but not stored/transmitted
}

// Session control
void CameraManager::startSession()
{
    if (hasBodyCamera()) m_bodyCamera->startCapture();
    if (hasCupCamera()) m_cupCamera->startCapture();

    if (m_orgasmDetectionEnabled) {
        m_orgasmAnalysisTimer->start();
    }
}

void CameraManager::endSession()
{
    m_orgasmAnalysisTimer->stop();
    if (hasBodyCamera()) m_bodyCamera->stopCapture();
    if (hasCupCamera()) m_cupCamera->stopCapture();
}

void CameraManager::resetSession()
{
    resetViolations();
    m_cupMotionHistory.clear();
    m_orgasmState = OrgasmState::NONE;
    m_orgasmConfidence = 0.0;
}

// Violation tracking
int CameraManager::getTotalViolationCount() const
{
    int total = 0;
    if (hasBodyCamera()) total += m_bodyCamera->getViolationCount();
    if (hasCupCamera()) total += m_cupCamera->getViolationCount();
    return total;
}

int CameraManager::getTotalWarningCount() const
{
    int total = 0;
    if (hasBodyCamera()) total += m_bodyCamera->getWarningCount();
    if (hasCupCamera()) total += m_cupCamera->getWarningCount();
    return total;
}

void CameraManager::resetViolations()
{
    if (hasBodyCamera()) m_bodyCamera->resetViolations();
    if (hasCupCamera()) m_cupCamera->resetViolations();
}

// Frame access
QImage CameraManager::getBodyCameraFrame() const
{
    return hasBodyCamera() ? m_bodyCamera->getCurrentFrame() : QImage();
}

QImage CameraManager::getCupCameraFrame() const
{
    return hasCupCamera() ? m_cupCamera->getCurrentFrame() : QImage();
}

QImage CameraManager::getBodyCameraVisualization() const
{
    return hasBodyCamera() ? m_bodyCamera->getVisualizationFrame() : QImage();
}

QImage CameraManager::getCupCameraVisualization() const
{
    return hasCupCamera() ? m_cupCamera->getVisualizationFrame() : QImage();
}

