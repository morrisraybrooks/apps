#ifndef CAMERAMOTIONSENSOR_H
#define CAMERAMOTIONSENSOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QImage>
#include <QRect>
#include <memory>

// Forward declarations for OpenCV types
namespace cv {
    class VideoCapture;
    class Mat;
}

/**
 * @brief Camera-based Motion Detection for stillness monitoring
 *
 * Uses computer vision (OpenCV) to detect body movement via camera input.
 * Supports dual-camera setup:
 * - Patient safety monitor: Wide-angle view for overall body movement
 * - Cup area monitor: Close-up view for detecting pelvic/hip movement
 *
 * Motion Detection Methods:
 * - Frame differencing: Detects pixel changes between frames
 * - Optical flow: Tracks movement vectors for direction/magnitude
 * - Background subtraction: Isolates moving objects from static background
 *
 * Hardware Support:
 * - USB webcams (V4L2 on Linux)
 * - Raspberry Pi Camera Module (via V4L2)
 * - IP cameras (RTSP streams)
 */
class CameraMotionSensor : public QObject
{
    Q_OBJECT

public:
    // Camera types
    enum class CameraType {
        USB_WEBCAM,           // Standard USB webcam
        RASPBERRY_PI_CAM,     // Raspberry Pi Camera Module
        IP_CAMERA,            // Network camera (RTSP)
        SIMULATED             // For testing without hardware
    };
    Q_ENUM(CameraType)

    // Camera roles in dual-camera setup
    enum class CameraRole {
        PATIENT_MONITOR,      // Wide-angle safety/body monitor
        CUP_AREA_MONITOR,     // Close-up pelvic/hip area
        SINGLE_CAMERA         // Single camera mode
    };
    Q_ENUM(CameraRole)

    // Motion detection algorithms
    enum class DetectionMethod {
        FRAME_DIFFERENCE,     // Simple frame-to-frame difference
        OPTICAL_FLOW,         // Lucas-Kanade optical flow
        BACKGROUND_SUBTRACT,  // MOG2 background subtraction
        COMBINED              // Fusion of multiple methods
    };
    Q_ENUM(DetectionMethod)

    // Motion level (compatible with MotionSensor)
    enum class MotionLevel {
        STILL,                // No significant motion
        MINOR,                // Slight movement (breathing)
        MODERATE,             // Noticeable movement
        MAJOR                 // Significant movement
    };
    Q_ENUM(MotionLevel)

    // Sensitivity presets
    enum class SensitivityPreset {
        LENIENT,              // Allows more movement
        NORMAL,               // Standard sensitivity
        STRICT,               // Minimal movement allowed
        EXTREME               // Almost no movement tolerance
    };
    Q_ENUM(SensitivityPreset)

    explicit CameraMotionSensor(CameraType type = CameraType::USB_WEBCAM,
                                 CameraRole role = CameraRole::SINGLE_CAMERA,
                                 QObject* parent = nullptr);
    ~CameraMotionSensor();

    // Initialization
    bool initialize(int deviceIndex = 0);
    bool initializeFromUrl(const QString& url);  // For IP cameras
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Camera configuration
    void setResolution(int width, int height);
    void setFrameRate(int fps);
    void setRegionOfInterest(const QRect& roi);  // Focus on specific area
    QRect regionOfInterest() const { return m_roi; }

    // Motion detection configuration
    void setDetectionMethod(DetectionMethod method);
    void setSensitivity(SensitivityPreset preset);
    void setCustomThresholds(double motionThreshold, double areaThreshold);
    void setMinMotionArea(int pixels);  // Minimum contour area to count

    // Calibration
    bool calibrateBackground(int durationMs = 3000);
    bool isCalibrated() const { return m_calibrated; }
    void resetCalibration();

    // Motion readings
    double getMotionMagnitude() const;      // 0-1 motion metric
    MotionLevel getMotionLevel() const;     // Current motion category
    double getStillnessScore() const;       // 0-100% stillness quality
    QPointF getMotionCenter() const;        // Center of detected motion
    double getMotionArea() const;           // Percentage of frame with motion

    // Frame access
    QImage getCurrentFrame() const;         // Current camera frame
    QImage getMotionMask() const;           // Binary motion mask
    QImage getVisualization() const;        // Frame with motion overlay

    // Violation tracking (compatible with MotionSensor)
    int getViolationCount() const { return m_violationCount; }
    int getWarningCount() const { return m_warningCount; }
    void resetViolations();

    // Recording
    bool startRecording(const QString& filename);
    void stopRecording();
    bool isRecording() const { return m_recording; }
    void setRecordingConsent(bool consent) { m_recordingConsent = consent; }
    bool hasRecordingConsent() const { return m_recordingConsent; }

    // Privacy controls
    void setPrivacyMode(bool enabled);      // Blur/mask sensitive areas
    bool isPrivacyModeEnabled() const { return m_privacyMode; }
    void setPrivacyMask(const QImage& mask);  // Custom privacy mask

    // Session control
    void startSession();
    void endSession();
    void resetSession();

Q_SIGNALS:
    void motionDetected(MotionLevel level, double magnitude);
    void stillnessChanged(bool isStill, double stillnessScore);
    void violationDetected(MotionLevel level, double intensity);

private Q_SLOTS:
    void onCaptureTimer();
    void onCalibrationTimer();

private:
    bool captureFrame();
    void processFrame();
    void detectMotionFrameDiff();
    void detectMotionOpticalFlow();
    void detectMotionBackgroundSubtract();
    void updateMotionLevel();
    void checkViolation();
    void applyPrivacyMask(QImage& frame);
    QImage matToQImage(const cv::Mat& mat);

    // Camera configuration
    CameraType m_cameraType;
    CameraRole m_cameraRole;
    bool m_initialized;
    bool m_calibrated;
    bool m_sessionActive;

    // OpenCV capture
    std::unique_ptr<cv::VideoCapture> m_capture;
    std::unique_ptr<cv::Mat> m_currentFrame;
    std::unique_ptr<cv::Mat> m_previousFrame;
    std::unique_ptr<cv::Mat> m_motionMask;
    std::unique_ptr<cv::Mat> m_backgroundModel;

    // Frame settings
    int m_frameWidth;
    int m_frameHeight;
    int m_frameRate;
    QRect m_roi;

    // Detection settings
    DetectionMethod m_detectionMethod;
    double m_motionThreshold;      // Pixel difference threshold
    double m_areaThreshold;        // Minimum motion area percentage
    int m_minMotionArea;           // Minimum contour area in pixels

    // Motion analysis
    double m_motionMagnitude;      // 0-1 combined metric
    MotionLevel m_motionLevel;
    double m_stillnessScore;       // 0-100%
    QPointF m_motionCenter;
    double m_motionAreaPercent;

    // Thresholds (based on sensitivity)
    double m_thresholdStill;
    double m_thresholdMinor;
    double m_thresholdModerate;

    // Violation tracking
    int m_violationCount;
    int m_warningCount;
    qint64 m_lastViolationTime;
    int m_violationDebounceMs;

    // Recording
    bool m_recording;
    bool m_recordingConsent;
    QString m_recordingFilename;
    std::unique_ptr<cv::VideoCapture> m_videoWriter;  // Actually VideoWriter

    // Privacy
    bool m_privacyMode;
    QImage m_privacyMask;

    // Calibration
    QTimer* m_calibrationTimer;
    int m_calibrationFramesNeeded;
    int m_calibrationFramesCaptured;

    // Timers
    QTimer* m_captureTimer;

    // Thread safety
    mutable QMutex m_mutex;

    // Simulation
    double m_simulatedMotion;
    QElapsedTimer m_simulationTimer;

    // Constants
    static constexpr int DEFAULT_FRAME_WIDTH = 640;
    static constexpr int DEFAULT_FRAME_HEIGHT = 480;
    static constexpr int DEFAULT_FRAME_RATE = 30;
    static constexpr int CALIBRATION_FRAMES = 30;
    static constexpr double DEFAULT_MOTION_THRESHOLD = 25.0;
    static constexpr double DEFAULT_AREA_THRESHOLD = 0.5;  // 0.5% of frame
};

#endif // CAMERAMOTIONSENSOR_H
