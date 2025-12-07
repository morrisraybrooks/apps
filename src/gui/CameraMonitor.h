#ifndef CAMERAMONITOR_H
#define CAMERAMONITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QImage>
#include <QPixmap>
#include <QElapsedTimer>

// Forward declarations
class CameraMotionSensor;
class HardwareManager;

/**
 * @brief Camera-based motion monitoring widget with privacy controls
 * 
 * This widget provides camera-based motion monitoring including:
 * - Live camera feed display with optional privacy blur
 * - Motion detection visualization overlay
 * - Stillness score and motion level indicators
 * - Dual-camera support (patient monitor + cup area)
 * - Recording controls with explicit consent
 * - Privacy mode toggle
 */
class CameraMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit CameraMonitor(HardwareManager* hardware, QWidget *parent = nullptr);
    ~CameraMonitor();

    // Camera control
    void setCamera(CameraMotionSensor* camera);
    void setCameraIndex(int index);
    
    // Configuration
    void setShowMotionOverlay(bool show);
    void setPrivacyMode(bool enabled);
    void setRecordingConsent(bool consent);
    bool hasRecordingConsent() const { return m_recordingConsent; }

public Q_SLOTS:
    void startCamera();
    void stopCamera();
    void startRecording();
    void stopRecording();
    void togglePrivacyMode();
    void onSensitivityChanged(int index);
    void startCalibration();

Q_SIGNALS:
    void sensitivityChanged(int presetIndex);
    void calibrationRequested();
    void recordingConsentChanged(bool consent);
    void privacyModeChanged(bool enabled);

private Q_SLOTS:
    void onFrameReady(const QImage& frame);
    void onMotionDetected(int level, double magnitude);
    void onStillnessChanged(bool isStill, double stillnessScore);
    void onViolationDetected(int level, double intensity);
    void onCalibrationComplete(bool success);
    void onCalibrationProgress(int percent);
    void onRecordingStarted(const QString& filename);
    void onRecordingStopped();
    void updateDisplay();

private:
    void setupUI();
    void setupCameraDisplay();
    void setupMotionIndicators();
    void setupControls();
    void setupPrivacyControls();
    void setupRecordingControls();
    void updateMotionLevelDisplay();
    void applyPrivacyBlur(QImage& frame);
    QString motionLevelToString(int level);
    QString motionLevelToColor(int level);
    
    // Hardware interface
    HardwareManager* m_hardware;
    CameraMotionSensor* m_cameraSensor;
    
    // UI components - Main layout
    QVBoxLayout* m_mainLayout;
    
    // Camera display
    QFrame* m_cameraFrame;
    QLabel* m_cameraFeedLabel;
    QLabel* m_cameraStatusLabel;
    
    // Motion indicators
    QFrame* m_motionFrame;
    QLabel* m_motionLevelLabel;
    QLabel* m_motionLevelIndicator;
    QLabel* m_stillnessLabel;
    QProgressBar* m_stillnessBar;
    QLabel* m_motionAreaLabel;
    QProgressBar* m_motionAreaBar;
    
    // Violation counters
    QLabel* m_violationCountLabel;
    QLabel* m_warningCountLabel;
    
    // Camera controls
    QFrame* m_controlFrame;
    QPushButton* m_startCameraButton;
    QPushButton* m_stopCameraButton;
    QComboBox* m_sensitivityCombo;
    QPushButton* m_calibrateButton;
    QProgressBar* m_calibrationProgress;
    
    // Privacy controls
    QFrame* m_privacyFrame;
    QCheckBox* m_privacyModeCheckbox;
    QLabel* m_privacyStatusLabel;
    
    // Recording controls
    QFrame* m_recordingFrame;
    QCheckBox* m_recordingConsentCheckbox;
    QPushButton* m_startRecordingButton;
    QPushButton* m_stopRecordingButton;
    QLabel* m_recordingStatusLabel;
    QLabel* m_recordingDurationLabel;
    
    // State
    bool m_cameraActive;
    bool m_privacyMode;
    bool m_recordingConsent;
    bool m_recording;
    bool m_showMotionOverlay;
    int m_currentMotionLevel;
    double m_currentStillnessScore;
    double m_currentMotionArea;
    int m_violationCount;
    int m_warningCount;
    
    // Timers
    QTimer* m_displayUpdateTimer;
    QElapsedTimer m_recordingTimer;
    
    // Constants
    static const int DISPLAY_UPDATE_INTERVAL = 33;  // ~30 fps
    static const int CAMERA_DISPLAY_WIDTH = 640;
    static const int CAMERA_DISPLAY_HEIGHT = 480;
};

#endif // CAMERAMONITOR_H

