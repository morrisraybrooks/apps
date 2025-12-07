#include "CameraMonitor.h"
#include "../hardware/CameraMotionSensor.h"
#include "../hardware/HardwareManager.h"
#include <QDebug>
#include <QDateTime>
#include <QPainter>

CameraMonitor::CameraMonitor(HardwareManager* hardware, QWidget *parent)
    : QWidget(parent)
    , m_hardware(hardware)
    , m_cameraSensor(nullptr)
    , m_mainLayout(nullptr)
    , m_cameraFrame(nullptr)
    , m_cameraFeedLabel(nullptr)
    , m_cameraStatusLabel(nullptr)
    , m_motionFrame(nullptr)
    , m_motionLevelLabel(nullptr)
    , m_motionLevelIndicator(nullptr)
    , m_stillnessLabel(nullptr)
    , m_stillnessBar(nullptr)
    , m_motionAreaLabel(nullptr)
    , m_motionAreaBar(nullptr)
    , m_violationCountLabel(nullptr)
    , m_warningCountLabel(nullptr)
    , m_controlFrame(nullptr)
    , m_startCameraButton(nullptr)
    , m_stopCameraButton(nullptr)
    , m_sensitivityCombo(nullptr)
    , m_calibrateButton(nullptr)
    , m_calibrationProgress(nullptr)
    , m_privacyFrame(nullptr)
    , m_privacyModeCheckbox(nullptr)
    , m_privacyStatusLabel(nullptr)
    , m_recordingFrame(nullptr)
    , m_recordingConsentCheckbox(nullptr)
    , m_startRecordingButton(nullptr)
    , m_stopRecordingButton(nullptr)
    , m_recordingStatusLabel(nullptr)
    , m_recordingDurationLabel(nullptr)
    , m_cameraActive(false)
    , m_privacyMode(false)
    , m_recordingConsent(false)
    , m_recording(false)
    , m_showMotionOverlay(true)
    , m_currentMotionLevel(0)
    , m_currentStillnessScore(100.0)
    , m_currentMotionArea(0.0)
    , m_violationCount(0)
    , m_warningCount(0)
    , m_displayUpdateTimer(new QTimer(this))
{
    setupUI();

    connect(m_displayUpdateTimer, &QTimer::timeout, this, &CameraMonitor::updateDisplay);
}

CameraMonitor::~CameraMonitor()
{
    if (m_recording) {
        stopRecording();
    }
    if (m_cameraActive) {
        stopCamera();
    }
}

// ============================================================================
// Setup UI
// ============================================================================

void CameraMonitor::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // Title
    QLabel* titleLabel = new QLabel("Camera Motion Monitor", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2196F3;");
    titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(titleLabel);

    setupCameraDisplay();
    setupMotionIndicators();
    setupControls();
    setupPrivacyControls();
    setupRecordingControls();

    setLayout(m_mainLayout);
}

void CameraMonitor::setupCameraDisplay()
{
    m_cameraFrame = new QFrame(this);
    m_cameraFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_cameraFrame->setStyleSheet("background-color: #1a1a1a; border-radius: 5px;");

    QVBoxLayout* cameraLayout = new QVBoxLayout(m_cameraFrame);

    // Camera feed display
    m_cameraFeedLabel = new QLabel(this);
    m_cameraFeedLabel->setFixedSize(CAMERA_DISPLAY_WIDTH, CAMERA_DISPLAY_HEIGHT);
    m_cameraFeedLabel->setAlignment(Qt::AlignCenter);
    m_cameraFeedLabel->setStyleSheet("background-color: #000; border: 1px solid #333;");
    m_cameraFeedLabel->setText("Camera Off");
    cameraLayout->addWidget(m_cameraFeedLabel, 0, Qt::AlignCenter);

    // Camera status
    m_cameraStatusLabel = new QLabel("Status: Disconnected", this);
    m_cameraStatusLabel->setStyleSheet("color: #888;");
    m_cameraStatusLabel->setAlignment(Qt::AlignCenter);
    cameraLayout->addWidget(m_cameraStatusLabel);

    m_mainLayout->addWidget(m_cameraFrame);
}

void CameraMonitor::setupMotionIndicators()
{
    m_motionFrame = new QFrame(this);
    m_motionFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_motionFrame->setStyleSheet("background-color: #2d2d2d; border-radius: 5px; padding: 5px;");

    QHBoxLayout* motionLayout = new QHBoxLayout(m_motionFrame);

    // Motion level indicator
    QVBoxLayout* levelLayout = new QVBoxLayout();
    m_motionLevelLabel = new QLabel("Motion Level:", this);
    m_motionLevelLabel->setStyleSheet("color: #aaa;");
    levelLayout->addWidget(m_motionLevelLabel);

    m_motionLevelIndicator = new QLabel("STILL", this);
    m_motionLevelIndicator->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #4CAF50; "
        "background-color: #1a1a1a; padding: 5px 15px; border-radius: 3px;");
    m_motionLevelIndicator->setAlignment(Qt::AlignCenter);
    levelLayout->addWidget(m_motionLevelIndicator);
    motionLayout->addLayout(levelLayout);

    // Stillness score
    QVBoxLayout* stillnessLayout = new QVBoxLayout();
    m_stillnessLabel = new QLabel("Stillness: 100%", this);
    m_stillnessLabel->setStyleSheet("color: #aaa;");
    stillnessLayout->addWidget(m_stillnessLabel);

    m_stillnessBar = new QProgressBar(this);
    m_stillnessBar->setRange(0, 100);
    m_stillnessBar->setValue(100);
    m_stillnessBar->setTextVisible(false);
    m_stillnessBar->setStyleSheet(
        "QProgressBar { background-color: #1a1a1a; border-radius: 3px; height: 20px; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }");
    stillnessLayout->addWidget(m_stillnessBar);
    motionLayout->addLayout(stillnessLayout);

    // Motion area
    QVBoxLayout* areaLayout = new QVBoxLayout();
    m_motionAreaLabel = new QLabel("Motion Area: 0%", this);
    m_motionAreaLabel->setStyleSheet("color: #aaa;");
    areaLayout->addWidget(m_motionAreaLabel);

    m_motionAreaBar = new QProgressBar(this);
    m_motionAreaBar->setRange(0, 100);
    m_motionAreaBar->setValue(0);
    m_motionAreaBar->setTextVisible(false);
    m_motionAreaBar->setStyleSheet(
        "QProgressBar { background-color: #1a1a1a; border-radius: 3px; height: 20px; }"
        "QProgressBar::chunk { background-color: #FF9800; border-radius: 3px; }");
    areaLayout->addWidget(m_motionAreaBar);
    motionLayout->addLayout(areaLayout);

    m_mainLayout->addWidget(m_motionFrame);
}

void CameraMonitor::setupControls()
{
    m_controlFrame = new QFrame(this);
    m_controlFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_controlFrame->setStyleSheet("background-color: #2d2d2d; border-radius: 5px; padding: 5px;");

    QHBoxLayout* controlLayout = new QHBoxLayout(m_controlFrame);

    // Start/Stop camera buttons
    m_startCameraButton = new QPushButton("Start Camera", this);
    m_startCameraButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #45a049; }");
    connect(m_startCameraButton, &QPushButton::clicked, this, &CameraMonitor::startCamera);
    controlLayout->addWidget(m_startCameraButton);

    m_stopCameraButton = new QPushButton("Stop Camera", this);
    m_stopCameraButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #da190b; }");
    m_stopCameraButton->setEnabled(false);
    connect(m_stopCameraButton, &QPushButton::clicked, this, &CameraMonitor::stopCamera);
    controlLayout->addWidget(m_stopCameraButton);

    controlLayout->addSpacing(20);

    // Sensitivity selector
    QLabel* sensitivityLabel = new QLabel("Sensitivity:", this);
    sensitivityLabel->setStyleSheet("color: #aaa;");
    controlLayout->addWidget(sensitivityLabel);

    m_sensitivityCombo = new QComboBox(this);
    m_sensitivityCombo->addItems({"Lenient", "Normal", "Strict", "Extreme"});
    m_sensitivityCombo->setCurrentIndex(1);  // Normal
    m_sensitivityCombo->setStyleSheet(
        "QComboBox { background-color: #3d3d3d; color: white; padding: 5px; border-radius: 3px; }");
    connect(m_sensitivityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CameraMonitor::onSensitivityChanged);
    controlLayout->addWidget(m_sensitivityCombo);

    controlLayout->addSpacing(20);

    // Calibration
    m_calibrateButton = new QPushButton("Calibrate", this);
    m_calibrateButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #1976D2; }");
    connect(m_calibrateButton, &QPushButton::clicked, this, &CameraMonitor::startCalibration);
    controlLayout->addWidget(m_calibrateButton);

    m_calibrationProgress = new QProgressBar(this);
    m_calibrationProgress->setRange(0, 100);
    m_calibrationProgress->setValue(0);
    m_calibrationProgress->setFixedWidth(100);
    m_calibrationProgress->setVisible(false);
    controlLayout->addWidget(m_calibrationProgress);

    controlLayout->addStretch();

    m_mainLayout->addWidget(m_controlFrame);
}

void CameraMonitor::setupPrivacyControls()
{
    m_privacyFrame = new QFrame(this);
    m_privacyFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_privacyFrame->setStyleSheet("background-color: #2d2d2d; border-radius: 5px; padding: 5px;");

    QHBoxLayout* privacyLayout = new QHBoxLayout(m_privacyFrame);

    QLabel* privacyTitle = new QLabel("Privacy:", this);
    privacyTitle->setStyleSheet("color: #aaa; font-weight: bold;");
    privacyLayout->addWidget(privacyTitle);

    m_privacyModeCheckbox = new QCheckBox("Enable Privacy Mode (blur sensitive areas)", this);
    m_privacyModeCheckbox->setStyleSheet("color: #ddd;");
    connect(m_privacyModeCheckbox, &QCheckBox::toggled, this, &CameraMonitor::togglePrivacyMode);
    privacyLayout->addWidget(m_privacyModeCheckbox);

    m_privacyStatusLabel = new QLabel("", this);
    m_privacyStatusLabel->setStyleSheet("color: #4CAF50;");
    privacyLayout->addWidget(m_privacyStatusLabel);

    privacyLayout->addStretch();

    m_mainLayout->addWidget(m_privacyFrame);
}

void CameraMonitor::setupRecordingControls()
{
    m_recordingFrame = new QFrame(this);
    m_recordingFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_recordingFrame->setStyleSheet("background-color: #2d2d2d; border-radius: 5px; padding: 5px;");

    QHBoxLayout* recordingLayout = new QHBoxLayout(m_recordingFrame);

    QLabel* recordingTitle = new QLabel("Recording:", this);
    recordingTitle->setStyleSheet("color: #aaa; font-weight: bold;");
    recordingLayout->addWidget(recordingTitle);

    m_recordingConsentCheckbox = new QCheckBox("I consent to recording this session", this);
    m_recordingConsentCheckbox->setStyleSheet("color: #ddd;");
    connect(m_recordingConsentCheckbox, &QCheckBox::toggled, [this](bool checked) {
        m_recordingConsent = checked;
        m_startRecordingButton->setEnabled(checked && m_cameraActive);
        emit recordingConsentChanged(checked);
    });
    recordingLayout->addWidget(m_recordingConsentCheckbox);

    m_startRecordingButton = new QPushButton("Start Recording", this);
    m_startRecordingButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #da190b; }");
    m_startRecordingButton->setEnabled(false);
    connect(m_startRecordingButton, &QPushButton::clicked, this, &CameraMonitor::startRecording);
    recordingLayout->addWidget(m_startRecordingButton);

    m_stopRecordingButton = new QPushButton("Stop Recording", this);
    m_stopRecordingButton->setStyleSheet(
        "QPushButton { background-color: #666; color: white; padding: 8px 16px; border-radius: 4px; }");
    m_stopRecordingButton->setEnabled(false);
    connect(m_stopRecordingButton, &QPushButton::clicked, this, &CameraMonitor::stopRecording);
    recordingLayout->addWidget(m_stopRecordingButton);

    m_recordingStatusLabel = new QLabel("Not Recording", this);
    m_recordingStatusLabel->setStyleSheet("color: #888;");
    recordingLayout->addWidget(m_recordingStatusLabel);

    m_recordingDurationLabel = new QLabel("", this);
    m_recordingDurationLabel->setStyleSheet("color: #f44336;");
    recordingLayout->addWidget(m_recordingDurationLabel);

    recordingLayout->addStretch();

    // Violation counters
    m_violationCountLabel = new QLabel("Violations: 0", this);
    m_violationCountLabel->setStyleSheet("color: #f44336; font-weight: bold;");
    recordingLayout->addWidget(m_violationCountLabel);

    m_warningCountLabel = new QLabel("Warnings: 0", this);
    m_warningCountLabel->setStyleSheet("color: #FF9800;");
    recordingLayout->addWidget(m_warningCountLabel);

    m_mainLayout->addWidget(m_recordingFrame);
}


// ============================================================================
// Camera Control
// ============================================================================

void CameraMonitor::setCamera(CameraMotionSensor* camera)
{
    if (m_cameraSensor) {
        disconnect(m_cameraSensor, nullptr, this, nullptr);
    }

    m_cameraSensor = camera;

    if (m_cameraSensor) {
        connect(m_cameraSensor, &CameraMotionSensor::frameReady,
                this, &CameraMonitor::onFrameReady);
        connect(m_cameraSensor, &CameraMotionSensor::motionDetected,
                this, &CameraMonitor::onMotionDetected);
        connect(m_cameraSensor, &CameraMotionSensor::stillnessChanged,
                this, &CameraMonitor::onStillnessChanged);
        connect(m_cameraSensor, &CameraMotionSensor::violationDetected,
                this, &CameraMonitor::onViolationDetected);
        connect(m_cameraSensor, &CameraMotionSensor::calibrationComplete,
                this, &CameraMonitor::onCalibrationComplete);
        connect(m_cameraSensor, &CameraMotionSensor::calibrationProgress,
                this, &CameraMonitor::onCalibrationProgress);
        connect(m_cameraSensor, &CameraMotionSensor::recordingStarted,
                this, &CameraMonitor::onRecordingStarted);
        connect(m_cameraSensor, &CameraMotionSensor::recordingStopped,
                this, &CameraMonitor::onRecordingStopped);
    }
}

void CameraMonitor::setCameraIndex(int index)
{
    Q_UNUSED(index)
    // Would be used to select between multiple cameras
}

void CameraMonitor::startCamera()
{
    if (!m_cameraSensor) {
        m_cameraStatusLabel->setText("Status: No camera sensor configured");
        return;
    }

    if (m_cameraSensor->initialize(0)) {
        m_cameraActive = true;
        m_startCameraButton->setEnabled(false);
        m_stopCameraButton->setEnabled(true);
        m_startRecordingButton->setEnabled(m_recordingConsent);
        m_cameraStatusLabel->setText("Status: Connected");
        m_displayUpdateTimer->start(DISPLAY_UPDATE_INTERVAL);
    } else {
        m_cameraStatusLabel->setText("Status: Failed to initialize");
    }
}

void CameraMonitor::stopCamera()
{
    if (m_recording) {
        stopRecording();
    }

    if (m_cameraSensor) {
        m_cameraSensor->shutdown();
    }

    m_cameraActive = false;
    m_startCameraButton->setEnabled(true);
    m_stopCameraButton->setEnabled(false);
    m_startRecordingButton->setEnabled(false);
    m_cameraStatusLabel->setText("Status: Disconnected");
    m_cameraFeedLabel->setText("Camera Off");
    m_displayUpdateTimer->stop();
}

void CameraMonitor::startRecording()
{
    if (!m_recordingConsent) {
        m_recordingStatusLabel->setText("Recording requires consent");
        return;
    }

    if (!m_cameraSensor) return;

    QString filename = QString("session_%1.mp4")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    if (m_cameraSensor->startRecording(filename)) {
        m_recording = true;
        m_recordingTimer.start();
        m_startRecordingButton->setEnabled(false);
        m_stopRecordingButton->setEnabled(true);
        m_recordingStatusLabel->setText("Recording...");
        m_recordingStatusLabel->setStyleSheet("color: #f44336; font-weight: bold;");
    }
}

void CameraMonitor::stopRecording()
{
    if (!m_cameraSensor) return;

    m_cameraSensor->stopRecording();
    m_recording = false;
    m_startRecordingButton->setEnabled(m_recordingConsent && m_cameraActive);
    m_stopRecordingButton->setEnabled(false);
    m_recordingStatusLabel->setText("Not Recording");
    m_recordingStatusLabel->setStyleSheet("color: #888;");
    m_recordingDurationLabel->setText("");
}

void CameraMonitor::togglePrivacyMode()
{
    m_privacyMode = m_privacyModeCheckbox->isChecked();

    if (m_cameraSensor) {
        m_cameraSensor->setPrivacyMode(m_privacyMode);
    }

    m_privacyStatusLabel->setText(m_privacyMode ? "Privacy Mode Active" : "");
    emit privacyModeChanged(m_privacyMode);
}

void CameraMonitor::onSensitivityChanged(int index)
{
    if (m_cameraSensor) {
        m_cameraSensor->setSensitivity(
            static_cast<CameraMotionSensor::SensitivityPreset>(index));
    }
    emit sensitivityChanged(index);
}

void CameraMonitor::startCalibration()
{
    if (!m_cameraSensor) return;

    m_calibrateButton->setEnabled(false);
    m_calibrationProgress->setVisible(true);
    m_calibrationProgress->setValue(0);

    m_cameraSensor->calibrateBackground(3000);
    emit calibrationRequested();
}


// ============================================================================
// Slot Implementations
// ============================================================================

void CameraMonitor::onFrameReady(const QImage& frame)
{
    if (frame.isNull()) return;

    QImage displayFrame = frame;

    // Apply privacy blur if enabled
    if (m_privacyMode) {
        applyPrivacyBlur(displayFrame);
    }

    // Scale to display size
    QPixmap pixmap = QPixmap::fromImage(displayFrame.scaled(
        CAMERA_DISPLAY_WIDTH, CAMERA_DISPLAY_HEIGHT,
        Qt::KeepAspectRatio, Qt::SmoothTransformation));

    m_cameraFeedLabel->setPixmap(pixmap);
}

void CameraMonitor::onMotionDetected(int level, double magnitude)
{
    m_currentMotionLevel = level;
    m_currentMotionArea = magnitude * 100.0;
    updateMotionLevelDisplay();
}

void CameraMonitor::onStillnessChanged(bool isStill, double stillnessScore)
{
    Q_UNUSED(isStill)
    m_currentStillnessScore = stillnessScore;

    m_stillnessLabel->setText(QString("Stillness: %1%").arg(static_cast<int>(stillnessScore)));
    m_stillnessBar->setValue(static_cast<int>(stillnessScore));

    // Update color based on stillness
    QString color;
    if (stillnessScore >= 80) {
        color = "#4CAF50";  // Green
    } else if (stillnessScore >= 50) {
        color = "#FF9800";  // Orange
    } else {
        color = "#f44336";  // Red
    }
    m_stillnessBar->setStyleSheet(QString(
        "QProgressBar { background-color: #1a1a1a; border-radius: 3px; height: 20px; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 3px; }").arg(color));
}

void CameraMonitor::onViolationDetected(int level, double intensity)
{
    Q_UNUSED(level)
    Q_UNUSED(intensity)
    m_violationCount++;
    m_violationCountLabel->setText(QString("Violations: %1").arg(m_violationCount));
}

void CameraMonitor::onCalibrationComplete(bool success)
{
    m_calibrateButton->setEnabled(true);
    m_calibrationProgress->setVisible(false);

    if (success) {
        m_cameraStatusLabel->setText("Status: Calibrated");
    } else {
        m_cameraStatusLabel->setText("Status: Calibration failed");
    }
}

void CameraMonitor::onCalibrationProgress(int percent)
{
    m_calibrationProgress->setValue(percent);
}

void CameraMonitor::onRecordingStarted(const QString& filename)
{
    Q_UNUSED(filename)
    m_recordingStatusLabel->setText("Recording...");
}

void CameraMonitor::onRecordingStopped()
{
    m_recordingStatusLabel->setText("Recording saved");
}

void CameraMonitor::updateDisplay()
{
    // Update recording duration
    if (m_recording) {
        qint64 elapsed = m_recordingTimer.elapsed() / 1000;
        int minutes = elapsed / 60;
        int seconds = elapsed % 60;
        m_recordingDurationLabel->setText(QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0')));
    }

    // Update motion area display
    m_motionAreaLabel->setText(QString("Motion Area: %1%").arg(
        static_cast<int>(m_currentMotionArea)));
    m_motionAreaBar->setValue(static_cast<int>(qMin(m_currentMotionArea, 100.0)));
}

void CameraMonitor::updateMotionLevelDisplay()
{
    QString levelText = motionLevelToString(m_currentMotionLevel);
    QString color = motionLevelToColor(m_currentMotionLevel);

    m_motionLevelIndicator->setText(levelText);
    m_motionLevelIndicator->setStyleSheet(QString(
        "font-size: 18px; font-weight: bold; color: %1; "
        "background-color: #1a1a1a; padding: 5px 15px; border-radius: 3px;").arg(color));
}

void CameraMonitor::applyPrivacyBlur(QImage& frame)
{
    // Simple blur effect for privacy
    // In production, would use a more sophisticated approach
    frame = frame.scaled(frame.width() / 8, frame.height() / 8,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    frame = frame.scaled(CAMERA_DISPLAY_WIDTH, CAMERA_DISPLAY_HEIGHT,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QString CameraMonitor::motionLevelToString(int level)
{
    switch (level) {
        case 0: return "STILL";
        case 1: return "MINOR";
        case 2: return "MODERATE";
        case 3: return "MAJOR";
        default: return "UNKNOWN";
    }
}

QString CameraMonitor::motionLevelToColor(int level)
{
    switch (level) {
        case 0: return "#4CAF50";  // Green
        case 1: return "#FFEB3B";  // Yellow
        case 2: return "#FF9800";  // Orange
        case 3: return "#f44336";  // Red
        default: return "#888";
    }
}

void CameraMonitor::setShowMotionOverlay(bool show)
{
    m_showMotionOverlay = show;
}

void CameraMonitor::setPrivacyMode(bool enabled)
{
    m_privacyMode = enabled;
    m_privacyModeCheckbox->setChecked(enabled);
}

void CameraMonitor::setRecordingConsent(bool consent)
{
    m_recordingConsent = consent;
    m_recordingConsentCheckbox->setChecked(consent);
}