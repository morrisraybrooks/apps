#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QImage>
#include <memory>
#include "CameraMotionSensor.h"

/**
 * @brief Manages dual-camera setup for motion detection and orgasm detection
 *
 * Supports two cameras:
 * - Body Camera: Wide-angle view monitoring overall body movement for stillness games
 * - Cup Camera: Close-up view of cup/genital area for detecting orgasm-related movements
 *
 * Features:
 * - Unified interface for both cameras
 * - Combined motion analysis with weighted contribution
 * - Privacy controls with consent management
 * - Orgasm detection via visual pattern recognition
 */
class CameraManager : public QObject
{
    Q_OBJECT

public:
    // Camera configuration
    struct CameraConfig {
        int deviceIndex = -1;           // -1 = auto-detect
        QString deviceUrl;              // For IP cameras
        CameraMotionSensor::CameraType type = CameraMotionSensor::CameraType::USB_WEBCAM;
        int width = 640;
        int height = 480;
        int fps = 30;
        QRect roi;                      // Region of interest
        double motionWeight = 1.0;      // Weight in combined motion calculation
    };

    // Orgasm detection state
    enum class OrgasmState {
        NONE,               // No orgasm indicators
        PRE_ORGASM,         // Buildup indicators detected
        ORGASM_ONSET,       // Likely orgasm beginning
        ORGASM_PEAK,        // Peak orgasm indicators
        POST_ORGASM         // Recovery phase
    };
    Q_ENUM(OrgasmState)

    explicit CameraManager(QObject* parent = nullptr);
    ~CameraManager();

    // Initialization
    bool initialize();
    bool initializeBodyCamera(const CameraConfig& config);
    bool initializeCupCamera(const CameraConfig& config);
    void shutdown();
    bool isReady() const;

    // Camera access
    CameraMotionSensor* bodyCamera() const { return m_bodyCamera.get(); }
    CameraMotionSensor* cupCamera() const { return m_cupCamera.get(); }
    bool hasBodyCamera() const { return m_bodyCamera && m_bodyCamera->isReady(); }
    bool hasCupCamera() const { return m_cupCamera && m_cupCamera->isReady(); }

    // Calibration
    bool calibrateBothCameras(int durationMs = 3000);
    bool isCalibrated() const;

    // Motion analysis (combined from both cameras)
    double getCombinedMotionMagnitude() const;
    CameraMotionSensor::MotionLevel getCombinedMotionLevel() const;
    double getCombinedStillnessScore() const;

    // Orgasm detection
    OrgasmState getOrgasmState() const { return m_orgasmState; }
    double getOrgasmConfidence() const { return m_orgasmConfidence; }
    void enableOrgasmDetection(bool enable);
    bool isOrgasmDetectionEnabled() const { return m_orgasmDetectionEnabled; }

    // Sensitivity configuration
    void setSensitivity(CameraMotionSensor::SensitivityPreset preset);
    void setBodyCameraWeight(double weight);  // 0-1
    void setCupCameraWeight(double weight);   // 0-1

    // Privacy and consent
    void setRecordingConsent(bool consent);
    bool hasRecordingConsent() const { return m_recordingConsent; }
    void setPrivacyMode(bool enabled);
    bool isPrivacyModeEnabled() const { return m_privacyMode; }

    // Session control
    void startSession();
    void endSession();
    void resetSession();

    // Violation tracking (combined)
    int getTotalViolationCount() const;
    int getTotalWarningCount() const;
    void resetViolations();

    // Frame access
    QImage getBodyCameraFrame() const;
    QImage getCupCameraFrame() const;
    QImage getBodyCameraVisualization() const;
    QImage getCupCameraVisualization() const;

Q_SIGNALS:
    void cameraInitialized(CameraMotionSensor::CameraRole role, bool success);
    void calibrationProgress(int percent);
    void calibrationComplete(bool success);
    void combinedMotionDetected(CameraMotionSensor::MotionLevel level, double magnitude);
    void combinedStillnessChanged(bool isStill, double score);
    void violationDetected(CameraMotionSensor::CameraRole source, 
                          CameraMotionSensor::MotionLevel level, double intensity);
    void warningIssued(CameraMotionSensor::CameraRole source, const QString& message);
    void orgasmStateChanged(OrgasmState state, double confidence);
    void orgasmDetected(double intensity, qint64 durationMs);
    void cameraError(CameraMotionSensor::CameraRole role, const QString& error);
    void frameReady(CameraMotionSensor::CameraRole role, const QImage& frame);

private Q_SLOTS:
    void onBodyCameraMotion(CameraMotionSensor::MotionLevel level, double magnitude);
    void onCupCameraMotion(CameraMotionSensor::MotionLevel level, double magnitude);
    void onBodyCameraViolation(CameraMotionSensor::MotionLevel level, double intensity);
    void onCupCameraViolation(CameraMotionSensor::MotionLevel level, double intensity);
    void onOrgasmAnalysisTick();

private:
    void updateCombinedMotion();
    void analyzeOrgasmIndicators();
    void connectCameraSignals(CameraMotionSensor* camera, CameraMotionSensor::CameraRole role);

    // Cameras
    std::unique_ptr<CameraMotionSensor> m_bodyCamera;
    std::unique_ptr<CameraMotionSensor> m_cupCamera;
    CameraConfig m_bodyConfig;
    CameraConfig m_cupConfig;

    // Combined motion state
    double m_combinedMotionMagnitude;
    CameraMotionSensor::MotionLevel m_combinedMotionLevel;
    double m_combinedStillnessScore;
    double m_bodyWeight;
    double m_cupWeight;

    // Orgasm detection
    bool m_orgasmDetectionEnabled;
    OrgasmState m_orgasmState;
    double m_orgasmConfidence;
    QTimer* m_orgasmAnalysisTimer;
    QVector<double> m_cupMotionHistory;  // Recent cup area motion for pattern analysis
    qint64 m_orgasmOnsetTime;
    static constexpr int ORGASM_HISTORY_SIZE = 60;  // ~2 seconds at 30fps

    // Privacy and consent
    bool m_recordingConsent;
    bool m_privacyMode;

    // Thread safety
    mutable QMutex m_mutex;
};

#endif // CAMERAMANAGER_H

