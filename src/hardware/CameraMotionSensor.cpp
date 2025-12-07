#include "CameraMotionSensor.h"
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>

// OpenCV includes
#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
#endif

CameraMotionSensor::CameraMotionSensor(CameraType type, CameraRole role, QObject* parent)
    : QObject(parent)
    , m_cameraType(type)
    , m_cameraRole(role)
    , m_initialized(false)
    , m_calibrated(false)
    , m_sessionActive(false)
    , m_frameWidth(DEFAULT_FRAME_WIDTH)
    , m_frameHeight(DEFAULT_FRAME_HEIGHT)
    , m_frameRate(DEFAULT_FRAME_RATE)
    , m_detectionMethod(DetectionMethod::FRAME_DIFFERENCE)
    , m_motionThreshold(DEFAULT_MOTION_THRESHOLD)
    , m_areaThreshold(DEFAULT_AREA_THRESHOLD)
    , m_minMotionArea(100)
    , m_motionMagnitude(0.0)
    , m_motionLevel(MotionLevel::STILL)
    , m_stillnessScore(100.0)
    , m_motionAreaPercent(0.0)
    , m_thresholdStill(0.5)
    , m_thresholdMinor(2.0)
    , m_thresholdModerate(5.0)
    , m_violationCount(0)
    , m_warningCount(0)
    , m_lastViolationTime(0)
    , m_violationDebounceMs(500)
    , m_recording(false)
    , m_recordingConsent(false)
    , m_privacyMode(false)
    , m_calibrationTimer(new QTimer(this))
    , m_calibrationFramesNeeded(CALIBRATION_FRAMES)
    , m_calibrationFramesCaptured(0)
    , m_captureTimer(new QTimer(this))
    , m_simulatedMotion(0.0)
{
    connect(m_captureTimer, &QTimer::timeout, this, &CameraMotionSensor::onCaptureTimer);
    connect(m_calibrationTimer, &QTimer::timeout, this, &CameraMotionSensor::onCalibrationTimer);

#ifdef HAVE_OPENCV
    m_currentFrame = std::make_unique<cv::Mat>();
    m_previousFrame = std::make_unique<cv::Mat>();
    m_motionMask = std::make_unique<cv::Mat>();
    m_backgroundModel = std::make_unique<cv::Mat>();
#endif
}

CameraMotionSensor::~CameraMotionSensor()
{
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool CameraMotionSensor::initialize(int deviceIndex)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        shutdown();
    }

#ifdef HAVE_OPENCV
    if (m_cameraType == CameraType::SIMULATED) {
        m_initialized = true;
        m_simulationTimer.start();
        qDebug() << "CameraMotionSensor: Initialized in simulation mode";
        return true;
    }

    m_capture = std::make_unique<cv::VideoCapture>();

    // Try to open the camera
    if (!m_capture->open(deviceIndex)) {
        emit cameraError("Failed to open camera device " + QString::number(deviceIndex));
        return false;
    }

    // Set resolution
    m_capture->set(cv::CAP_PROP_FRAME_WIDTH, m_frameWidth);
    m_capture->set(cv::CAP_PROP_FRAME_HEIGHT, m_frameHeight);
    m_capture->set(cv::CAP_PROP_FPS, m_frameRate);

    // Verify camera is working
    cv::Mat testFrame;
    if (!m_capture->read(testFrame) || testFrame.empty()) {
        emit cameraError("Camera opened but failed to capture test frame");
        m_capture.reset();
        return false;
    }

    m_initialized = true;

    // Start capture timer
    int intervalMs = 1000 / m_frameRate;
    m_captureTimer->start(intervalMs);

    qDebug() << "CameraMotionSensor: Initialized camera" << deviceIndex
             << "at" << m_frameWidth << "x" << m_frameHeight << "@" << m_frameRate << "fps";
    return true;
#else
    // No OpenCV - use simulation mode
    m_cameraType = CameraType::SIMULATED;
    m_initialized = true;
    m_simulationTimer.start();
    qDebug() << "CameraMotionSensor: OpenCV not available, using simulation mode";
    return true;
#endif
}

bool CameraMotionSensor::initializeFromUrl(const QString& url)
{
    QMutexLocker locker(&m_mutex);

#ifdef HAVE_OPENCV
    if (m_initialized) {
        shutdown();
    }

    m_capture = std::make_unique<cv::VideoCapture>();

    if (!m_capture->open(url.toStdString())) {
        emit cameraError("Failed to open camera URL: " + url);
        return false;
    }

    m_initialized = true;
    m_cameraType = CameraType::IP_CAMERA;

    int intervalMs = 1000 / m_frameRate;
    m_captureTimer->start(intervalMs);

    qDebug() << "CameraMotionSensor: Initialized IP camera from" << url;
    return true;
#else
    Q_UNUSED(url)
    emit cameraError("OpenCV not available for IP camera support");
    return false;
#endif
}

void CameraMotionSensor::shutdown()
{
    QMutexLocker locker(&m_mutex);

    m_captureTimer->stop();
    m_calibrationTimer->stop();

    if (m_recording) {
        stopRecording();
    }

#ifdef HAVE_OPENCV

// ============================================================================
// Configuration
// ============================================================================

void CameraMotionSensor::setResolution(int width, int height)
{
    m_frameWidth = width;
    m_frameHeight = height;

#ifdef HAVE_OPENCV
    if (m_capture && m_capture->isOpened()) {
        m_capture->set(cv::CAP_PROP_FRAME_WIDTH, width);
        m_capture->set(cv::CAP_PROP_FRAME_HEIGHT, height);
    }
#endif
}

void CameraMotionSensor::setFrameRate(int fps)
{
    m_frameRate = qBound(1, fps, 120);

#ifdef HAVE_OPENCV
    if (m_capture && m_capture->isOpened()) {
        m_capture->set(cv::CAP_PROP_FPS, m_frameRate);
    }
#endif

    if (m_captureTimer->isActive()) {
        m_captureTimer->setInterval(1000 / m_frameRate);
    }
}

void CameraMotionSensor::setRegionOfInterest(const QRect& roi)
{
    m_roi = roi;
}

void CameraMotionSensor::setDetectionMethod(DetectionMethod method)
{
    m_detectionMethod = method;
}

void CameraMotionSensor::setSensitivity(SensitivityPreset preset)
{
    switch (preset) {
        case SensitivityPreset::LENIENT:
            m_thresholdStill = 2.0;
            m_thresholdMinor = 5.0;
            m_thresholdModerate = 10.0;
            m_motionThreshold = 40.0;
            break;
        case SensitivityPreset::NORMAL:
            m_thresholdStill = 0.5;
            m_thresholdMinor = 2.0;
            m_thresholdModerate = 5.0;
            m_motionThreshold = 25.0;
            break;
        case SensitivityPreset::STRICT:
            m_thresholdStill = 0.2;
            m_thresholdMinor = 1.0;
            m_thresholdModerate = 3.0;
            m_motionThreshold = 15.0;
            break;
        case SensitivityPreset::EXTREME:
            m_thresholdStill = 0.1;
            m_thresholdMinor = 0.5;
            m_thresholdModerate = 1.5;
            m_motionThreshold = 10.0;
            break;
    }
}

void CameraMotionSensor::setCustomThresholds(double motionThreshold, double areaThreshold)
{
    m_motionThreshold = motionThreshold;
    m_areaThreshold = areaThreshold;
}

void CameraMotionSensor::setMinMotionArea(int pixels)
{
    m_minMotionArea = pixels;
}

// ============================================================================
// Calibration
// ============================================================================

bool CameraMotionSensor::calibrateBackground(int durationMs)
{
    if (!m_initialized) {
        emit cameraError("Cannot calibrate: camera not initialized");
        return false;
    }

    m_calibrationFramesCaptured = 0;
    m_calibrationFramesNeeded = (durationMs * m_frameRate) / 1000;

    m_calibrationTimer->start(1000 / m_frameRate);

    qDebug() << "CameraMotionSensor: Starting background calibration for" << durationMs << "ms";
    return true;
}

void CameraMotionSensor::resetCalibration()
{
    m_calibrated = false;
#ifdef HAVE_OPENCV
    if (m_backgroundModel) {
        *m_backgroundModel = cv::Mat();
    }
#endif
}

// ============================================================================
// Motion Readings
// ============================================================================

double CameraMotionSensor::getMotionMagnitude() const
{
    QMutexLocker locker(&m_mutex);
    return m_motionMagnitude;
}

CameraMotionSensor::MotionLevel CameraMotionSensor::getMotionLevel() const
{
    QMutexLocker locker(&m_mutex);
    return m_motionLevel;
}

double CameraMotionSensor::getStillnessScore() const
{
    QMutexLocker locker(&m_mutex);
    return m_stillnessScore;
}

QPointF CameraMotionSensor::getMotionCenter() const
{
    QMutexLocker locker(&m_mutex);
    return m_motionCenter;
}

double CameraMotionSensor::getMotionArea() const
{
    QMutexLocker locker(&m_mutex);
    return m_motionAreaPercent;
}

void CameraMotionSensor::resetViolations()
{
    QMutexLocker locker(&m_mutex);
    m_violationCount = 0;
    m_warningCount = 0;
    m_lastViolationTime = 0;
}


// ============================================================================
// Frame Access
// ============================================================================

QImage CameraMotionSensor::getCurrentFrame() const
{
    QMutexLocker locker(&m_mutex);

#ifdef HAVE_OPENCV
    if (m_currentFrame && !m_currentFrame->empty()) {
        return matToQImage(*m_currentFrame);
    }
#endif

    return QImage();
}

QImage CameraMotionSensor::getMotionMask() const
{
    QMutexLocker locker(&m_mutex);

#ifdef HAVE_OPENCV
    if (m_motionMask && !m_motionMask->empty()) {
        return matToQImage(*m_motionMask);
    }
#endif

    return QImage();
}

QImage CameraMotionSensor::getVisualization() const
{
    QMutexLocker locker(&m_mutex);

#ifdef HAVE_OPENCV
    if (m_currentFrame && !m_currentFrame->empty()) {
        cv::Mat visualization = m_currentFrame->clone();

        // Draw motion center
        if (m_motionMagnitude > 0.01) {
            cv::Point center(static_cast<int>(m_motionCenter.x()),
                           static_cast<int>(m_motionCenter.y()));
            cv::circle(visualization, center, 10, cv::Scalar(0, 0, 255), 2);
        }

        // Draw ROI if set
        if (!m_roi.isEmpty()) {
            cv::rectangle(visualization,
                         cv::Point(m_roi.left(), m_roi.top()),
                         cv::Point(m_roi.right(), m_roi.bottom()),
                         cv::Scalar(0, 255, 0), 2);
        }

        // Draw motion level indicator
        cv::Scalar color;
        switch (m_motionLevel) {
            case MotionLevel::STILL: color = cv::Scalar(0, 255, 0); break;
            case MotionLevel::MINOR: color = cv::Scalar(0, 255, 255); break;
            case MotionLevel::MODERATE: color = cv::Scalar(0, 165, 255); break;
            case MotionLevel::MAJOR: color = cv::Scalar(0, 0, 255); break;
        }
        cv::rectangle(visualization, cv::Point(10, 10), cv::Point(30, 30), color, -1);

        return matToQImage(visualization);
    }
#endif

    return QImage();
}

// ============================================================================
// Recording
// ============================================================================

bool CameraMotionSensor::startRecording(const QString& filename)
{
    if (!m_recordingConsent) {
        emit cameraError("Recording requires explicit user consent");
        return false;
    }

    if (!m_initialized) {
        emit cameraError("Cannot record: camera not initialized");
        return false;
    }

    m_recordingFilename = filename;
    m_recording = true;

    emit recordingStarted(filename);
    qDebug() << "CameraMotionSensor: Recording started to" << filename;
    return true;
}

void CameraMotionSensor::stopRecording()
{
    if (!m_recording) return;

    m_recording = false;
    emit recordingStopped();
    qDebug() << "CameraMotionSensor: Recording stopped";
}

// ============================================================================
// Privacy Controls
// ============================================================================

void CameraMotionSensor::setPrivacyMode(bool enabled)
{
    m_privacyMode = enabled;
}

void CameraMotionSensor::setPrivacyMask(const QImage& mask)
{
    m_privacyMask = mask;
}

// ============================================================================
// Session Control
// ============================================================================

void CameraMotionSensor::startSession()
{
    QMutexLocker locker(&m_mutex);
    m_sessionActive = true;
    resetViolations();
    qDebug() << "CameraMotionSensor: Session started";
}

void CameraMotionSensor::endSession()
{
    QMutexLocker locker(&m_mutex);
    m_sessionActive = false;
    qDebug() << "CameraMotionSensor: Session ended";
}

void CameraMotionSensor::resetSession()
{
    QMutexLocker locker(&m_mutex);
    resetViolations();
    m_motionMagnitude = 0.0;
    m_stillnessScore = 100.0;
    m_motionLevel = MotionLevel::STILL;
}


// ============================================================================
// Timer Callbacks
// ============================================================================

void CameraMotionSensor::onCaptureTimer()
{
    if (!m_initialized) return;

    if (captureFrame()) {
        processFrame();
    }
}

void CameraMotionSensor::onCalibrationTimer()
{
    if (!m_initialized) return;

#ifdef HAVE_OPENCV
    if (captureFrame()) {
        m_calibrationFramesCaptured++;

        // Accumulate background model
        if (m_calibrationFramesCaptured == 1) {
            m_currentFrame->convertTo(*m_backgroundModel, CV_32F);
        } else {
            cv::Mat temp;
            m_currentFrame->convertTo(temp, CV_32F);
            cv::accumulateWeighted(temp, *m_backgroundModel, 0.1);
        }

        int progress = (m_calibrationFramesCaptured * 100) / m_calibrationFramesNeeded;
        emit calibrationProgress(progress);

        if (m_calibrationFramesCaptured >= m_calibrationFramesNeeded) {
            m_calibrationTimer->stop();
            m_calibrated = true;
            emit calibrationComplete(true);
            qDebug() << "CameraMotionSensor: Calibration complete";
        }
    }
#else
    m_calibrationFramesCaptured++;
    int progress = (m_calibrationFramesCaptured * 100) / m_calibrationFramesNeeded;
    emit calibrationProgress(progress);

    if (m_calibrationFramesCaptured >= m_calibrationFramesNeeded) {
        m_calibrationTimer->stop();
        m_calibrated = true;
        emit calibrationComplete(true);
    }
#endif
}

// ============================================================================
// Frame Capture and Processing
// ============================================================================

bool CameraMotionSensor::captureFrame()
{
#ifdef HAVE_OPENCV
    if (!m_capture || !m_capture->isOpened()) {
        return false;
    }

    // Store previous frame
    if (!m_currentFrame->empty()) {
        *m_previousFrame = m_currentFrame->clone();
    }

    // Capture new frame
    if (!m_capture->read(*m_currentFrame)) {
        emit cameraError("Failed to capture frame");
        return false;
    }

    // Apply ROI if set
    if (!m_roi.isEmpty() && m_roi.width() > 0 && m_roi.height() > 0) {
        cv::Rect cvRoi(m_roi.x(), m_roi.y(), m_roi.width(), m_roi.height());
        *m_currentFrame = (*m_currentFrame)(cvRoi).clone();
    }

    emit frameReady(matToQImage(*m_currentFrame));
    return true;
#else
    // Simulation mode
    if (m_cameraType == CameraType::SIMULATED) {
        // Generate simulated motion
        double elapsed = m_simulationTimer.elapsed() / 1000.0;
        m_simulatedMotion = 0.1 * sin(elapsed * 0.5);  // Gentle oscillation
        return true;
    }
    return false;
#endif
}

void CameraMotionSensor::processFrame()
{
    QMutexLocker locker(&m_mutex);

#ifdef HAVE_OPENCV
    if (m_previousFrame->empty() || m_currentFrame->empty()) {
        return;
    }

    switch (m_detectionMethod) {
        case DetectionMethod::FRAME_DIFFERENCE:
            detectMotionFrameDiff();
            break;
        case DetectionMethod::OPTICAL_FLOW:
            detectMotionOpticalFlow();
            break;
        case DetectionMethod::BACKGROUND_SUBTRACT:
            detectMotionBackgroundSubtract();
            break;
        case DetectionMethod::COMBINED:
            detectMotionFrameDiff();
            // Could combine with other methods
            break;
    }
#else
    // Simulation mode
    m_motionMagnitude = qAbs(m_simulatedMotion);
#endif

    updateMotionLevel();
    checkViolation();

    emit motionDetected(m_motionLevel, m_motionMagnitude);
    emit stillnessChanged(m_motionLevel == MotionLevel::STILL, m_stillnessScore);
}

void CameraMotionSensor::detectMotionFrameDiff()
{
#ifdef HAVE_OPENCV
    cv::Mat gray1, gray2, diff;

    cv::cvtColor(*m_previousFrame, gray1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(*m_currentFrame, gray2, cv::COLOR_BGR2GRAY);

    cv::absdiff(gray1, gray2, diff);
    cv::threshold(diff, *m_motionMask, m_motionThreshold, 255, cv::THRESH_BINARY);

    // Apply morphological operations to reduce noise
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(*m_motionMask, *m_motionMask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(*m_motionMask, *m_motionMask, cv::MORPH_CLOSE, kernel);

    // Calculate motion metrics
    int totalPixels = m_motionMask->rows * m_motionMask->cols;
    int motionPixels = cv::countNonZero(*m_motionMask);
    m_motionAreaPercent = (motionPixels * 100.0) / totalPixels;

    // Find contours and calculate center of motion
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(m_motionMask->clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (!contours.empty()) {
        cv::Moments moments = cv::moments(*m_motionMask);
        if (moments.m00 > 0) {
            m_motionCenter = QPointF(moments.m10 / moments.m00, moments.m01 / moments.m00);
        }
    }

    // Calculate motion magnitude (0-1)
    m_motionMagnitude = qMin(m_motionAreaPercent / 10.0, 1.0);
#endif
}


void CameraMotionSensor::detectMotionOpticalFlow()
{
#ifdef HAVE_OPENCV
    cv::Mat gray1, gray2;
    cv::cvtColor(*m_previousFrame, gray1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(*m_currentFrame, gray2, cv::COLOR_BGR2GRAY);

    // Detect good features to track
    std::vector<cv::Point2f> prevPoints, nextPoints;
    cv::goodFeaturesToTrack(gray1, prevPoints, 100, 0.3, 7);

    if (prevPoints.empty()) {
        m_motionMagnitude = 0.0;
        return;
    }

    // Calculate optical flow
    std::vector<uchar> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(gray1, gray2, prevPoints, nextPoints, status, err);

    // Calculate average motion magnitude
    double totalMotion = 0.0;
    int validPoints = 0;

    for (size_t i = 0; i < prevPoints.size(); i++) {
        if (status[i]) {
            double dx = nextPoints[i].x - prevPoints[i].x;
            double dy = nextPoints[i].y - prevPoints[i].y;
            totalMotion += sqrt(dx * dx + dy * dy);
            validPoints++;
        }
    }

    if (validPoints > 0) {
        double avgMotion = totalMotion / validPoints;
        m_motionMagnitude = qMin(avgMotion / 20.0, 1.0);  // Normalize
    } else {
        m_motionMagnitude = 0.0;
    }
#endif
}

void CameraMotionSensor::detectMotionBackgroundSubtract()
{
#ifdef HAVE_OPENCV
    if (!m_calibrated || m_backgroundModel->empty()) {
        detectMotionFrameDiff();  // Fallback
        return;
    }

    cv::Mat currentFloat, diff;
    m_currentFrame->convertTo(currentFloat, CV_32F);

    cv::absdiff(currentFloat, *m_backgroundModel, diff);

    cv::Mat diffGray;
    cv::cvtColor(diff, diffGray, cv::COLOR_BGR2GRAY);
    diffGray.convertTo(diffGray, CV_8U);

    cv::threshold(diffGray, *m_motionMask, m_motionThreshold, 255, cv::THRESH_BINARY);

    int totalPixels = m_motionMask->rows * m_motionMask->cols;
    int motionPixels = cv::countNonZero(*m_motionMask);
    m_motionAreaPercent = (motionPixels * 100.0) / totalPixels;
    m_motionMagnitude = qMin(m_motionAreaPercent / 10.0, 1.0);
#endif
}

void CameraMotionSensor::updateMotionLevel()
{
    // Calculate stillness score (inverse of motion)
    m_stillnessScore = qMax(0.0, 100.0 - (m_motionMagnitude * 100.0));

    // Determine motion level based on thresholds
    if (m_motionAreaPercent < m_thresholdStill) {
        m_motionLevel = MotionLevel::STILL;
    } else if (m_motionAreaPercent < m_thresholdMinor) {
        m_motionLevel = MotionLevel::MINOR;
    } else if (m_motionAreaPercent < m_thresholdModerate) {
        m_motionLevel = MotionLevel::MODERATE;
    } else {
        m_motionLevel = MotionLevel::MAJOR;
    }
}

void CameraMotionSensor::checkViolation()
{
    if (!m_sessionActive) return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Check for violation (debounced)
    if (m_motionLevel >= MotionLevel::MODERATE) {
        if (now - m_lastViolationTime > m_violationDebounceMs) {
            m_violationCount++;
            m_lastViolationTime = now;
            emit violationDetected(m_motionLevel, m_motionMagnitude);
        }
    } else if (m_motionLevel == MotionLevel::MINOR) {
        m_warningCount++;
        emit warningIssued("Minor movement detected");
    }
}

void CameraMotionSensor::applyPrivacyMask(QImage& frame)
{
    if (!m_privacyMode || m_privacyMask.isNull()) return;

    // Apply blur or mask to sensitive areas
    // This is a placeholder - actual implementation would use the privacy mask
    Q_UNUSED(frame)
}

QImage CameraMotionSensor::matToQImage(const cv::Mat& mat)
{
#ifdef HAVE_OPENCV
    if (mat.empty()) {
        return QImage();
    }

    switch (mat.type()) {
        case CV_8UC1:
            return QImage(mat.data, mat.cols, mat.rows,
                         static_cast<int>(mat.step), QImage::Format_Grayscale8).copy();
        case CV_8UC3:
            return QImage(mat.data, mat.cols, mat.rows,
                         static_cast<int>(mat.step), QImage::Format_BGR888).copy();
        case CV_8UC4:
            return QImage(mat.data, mat.cols, mat.rows,
                         static_cast<int>(mat.step), QImage::Format_ARGB32).copy();
        default:
            return QImage();
    }
#else
    Q_UNUSED(mat)
    return QImage();
#endif
}