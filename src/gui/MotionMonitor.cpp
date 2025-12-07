#include "MotionMonitor.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/MotionSensor.h"
#include <QDebug>
#include <QGroupBox>
#include <QGridLayout>

MotionMonitor::MotionMonitor(HardwareManager* hardware, QWidget *parent)
    : QWidget(parent)
    , m_hardware(hardware)
    , m_motionSensor(hardware ? hardware->getMotionSensor() : nullptr)
    , m_mainLayout(nullptr)
    , m_chart(nullptr)
    , m_chartView(nullptr)
    , m_stillnessSeries(nullptr)
    , m_timeAxis(nullptr)
    , m_stillnessAxis(nullptr)
    , m_chartTimeRangeSeconds(DEFAULT_CHART_TIME_RANGE)
    , m_updatesPaused(false)
    , m_sessionActive(false)
    , m_currentAccelMagnitude(0.0)
    , m_currentGyroMagnitude(0.0)
    , m_currentStillnessScore(100.0)
    , m_currentMotionLevel(0)
    , m_violationCount(0)
    , m_warningCount(0)
    , m_chartUpdateTimer(new QTimer(this))
{
    setupUI();
    
    // Connect to motion sensor signals
    if (m_motionSensor) {
        connect(m_motionSensor, &MotionSensor::motionDetected,
                this, [this](MotionSensor::MotionLevel level, double magnitude) {
            onMotionDetected(static_cast<int>(level), magnitude);
        });
        connect(m_motionSensor, &MotionSensor::stillnessChanged,
                this, &MotionMonitor::onStillnessChanged);
        connect(m_motionSensor, &MotionSensor::violationDetected,
                this, [this](MotionSensor::MotionLevel level, double intensity) {
            onViolationDetected(static_cast<int>(level), intensity);
        });
        connect(m_motionSensor, &MotionSensor::warningIssued,
                this, &MotionMonitor::onWarningIssued);
        connect(m_motionSensor, &MotionSensor::calibrationComplete,
                this, &MotionMonitor::onCalibrationComplete);
        connect(m_motionSensor, &MotionSensor::calibrationProgress,
                this, &MotionMonitor::onCalibrationProgress);
    }
    
    // Start chart update timer
    connect(m_chartUpdateTimer, &QTimer::timeout, this, &MotionMonitor::updateChart);
    m_chartUpdateTimer->start(CHART_UPDATE_INTERVAL);
    m_sessionTimer.start();
}

MotionMonitor::~MotionMonitor()
{
    m_chartUpdateTimer->stop();
}

void MotionMonitor::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    setupMotionDisplays();
    setupStillnessDisplay();
    setupChart();
    setupViolationCounters();
    setupControls();
    
    setLayout(m_mainLayout);
}

void MotionMonitor::setupMotionDisplays()
{
    m_motionFrame = new QFrame(this);
    m_motionFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    
    QGridLayout* layout = new QGridLayout(m_motionFrame);
    
    // Acceleration display
    m_accelLabel = new QLabel("Acceleration:", m_motionFrame);
    m_accelLabel->setStyleSheet("font-weight: bold;");
    m_accelValueLabel = new QLabel("0.000 g", m_motionFrame);
    m_accelValueLabel->setStyleSheet("font-size: 18px; color: #2196F3;");
    m_accelBar = new QProgressBar(m_motionFrame);
    m_accelBar->setRange(0, 100);
    m_accelBar->setValue(0);
    m_accelBar->setTextVisible(false);
    m_accelBar->setStyleSheet("QProgressBar::chunk { background-color: #2196F3; }");
    
    // Gyroscope display
    m_gyroLabel = new QLabel("Rotation:", m_motionFrame);
    m_gyroLabel->setStyleSheet("font-weight: bold;");
    m_gyroValueLabel = new QLabel("0.0 °/s", m_motionFrame);
    m_gyroValueLabel->setStyleSheet("font-size: 18px; color: #9C27B0;");
    m_gyroBar = new QProgressBar(m_motionFrame);
    m_gyroBar->setRange(0, 100);
    m_gyroBar->setValue(0);
    m_gyroBar->setTextVisible(false);
    m_gyroBar->setStyleSheet("QProgressBar::chunk { background-color: #9C27B0; }");
    
    // Motion level indicator
    m_motionLevelLabel = new QLabel("Level:", m_motionFrame);
    m_motionLevelLabel->setStyleSheet("font-weight: bold;");
    m_motionLevelIndicator = new QLabel("STILL", m_motionFrame);
    m_motionLevelIndicator->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: #4CAF50; "
        "padding: 5px 15px; border-radius: 5px; background-color: #E8F5E9;");
    m_motionLevelIndicator->setAlignment(Qt::AlignCenter);
    
    layout->addWidget(m_accelLabel, 0, 0);
    layout->addWidget(m_accelValueLabel, 0, 1);
    layout->addWidget(m_accelBar, 0, 2);
    layout->addWidget(m_gyroLabel, 1, 0);
    layout->addWidget(m_gyroValueLabel, 1, 1);
    layout->addWidget(m_gyroBar, 1, 2);
    layout->addWidget(m_motionLevelLabel, 0, 3, 2, 1);
    layout->addWidget(m_motionLevelIndicator, 0, 4, 2, 1);
    
    layout->setColumnStretch(2, 1);
    
    m_mainLayout->addWidget(m_motionFrame);
}

void MotionMonitor::setupStillnessDisplay()
{
    m_stillnessFrame = new QFrame(this);
    m_stillnessFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    
    QHBoxLayout* layout = new QHBoxLayout(m_stillnessFrame);
    
    m_stillnessLabel = new QLabel("Stillness Score:", m_stillnessFrame);
    m_stillnessLabel->setStyleSheet("font-weight: bold;");
    
    m_stillnessValueLabel = new QLabel("100%", m_stillnessFrame);
    m_stillnessValueLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #4CAF50;");
    
    m_stillnessBar = new QProgressBar(m_stillnessFrame);
    m_stillnessBar->setRange(0, 100);
    m_stillnessBar->setValue(100);
    m_stillnessBar->setTextVisible(false);
    m_stillnessBar->setMinimumHeight(30);
    m_stillnessBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    
    m_stillnessStatusLabel = new QLabel("Perfect", m_stillnessFrame);
    m_stillnessStatusLabel->setStyleSheet("font-size: 14px; color: #4CAF50;");
    
    layout->addWidget(m_stillnessLabel);
    layout->addWidget(m_stillnessValueLabel);
    layout->addWidget(m_stillnessBar, 1);
    layout->addWidget(m_stillnessStatusLabel);
    
    m_mainLayout->addWidget(m_stillnessFrame);
}

void MotionMonitor::setupChart()
{
    m_chartFrame = new QFrame(this);
    m_chartFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    QVBoxLayout* layout = new QVBoxLayout(m_chartFrame);

    // Create chart
    m_chart = new QChart();
    m_chart->setTitle("Stillness Over Time");
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->legend()->setVisible(false);

    // Stillness series
    m_stillnessSeries = new QLineSeries();
    m_stillnessSeries->setName("Stillness %");
    m_stillnessSeries->setColor(QColor("#4CAF50"));
    m_chart->addSeries(m_stillnessSeries);

    // Time axis
    m_timeAxis = new QValueAxis();
    m_timeAxis->setTitleText("Time (sec)");
    m_timeAxis->setRange(0, m_chartTimeRangeSeconds);
    m_chart->addAxis(m_timeAxis, Qt::AlignBottom);
    m_stillnessSeries->attachAxis(m_timeAxis);

    // Stillness axis
    m_stillnessAxis = new QValueAxis();
    m_stillnessAxis->setTitleText("Stillness %");
    m_stillnessAxis->setRange(0, 100);
    m_chart->addAxis(m_stillnessAxis, Qt::AlignLeft);
    m_stillnessSeries->attachAxis(m_stillnessAxis);

    // Chart view
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(150);

    layout->addWidget(m_chartView);
    m_mainLayout->addWidget(m_chartFrame);
}

void MotionMonitor::setupViolationCounters()
{
    m_countersFrame = new QFrame(this);
    m_countersFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    QHBoxLayout* layout = new QHBoxLayout(m_countersFrame);

    // Violation count
    QGroupBox* violationGroup = new QGroupBox("Violations", m_countersFrame);
    QVBoxLayout* violationLayout = new QVBoxLayout(violationGroup);
    m_violationValueLabel = new QLabel("0", violationGroup);
    m_violationValueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #F44336;");
    m_violationValueLabel->setAlignment(Qt::AlignCenter);
    violationLayout->addWidget(m_violationValueLabel);

    // Warning count
    QGroupBox* warningGroup = new QGroupBox("Warnings", m_countersFrame);
    QVBoxLayout* warningLayout = new QVBoxLayout(warningGroup);
    m_warningValueLabel = new QLabel("0", warningGroup);
    m_warningValueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #FF9800;");
    m_warningValueLabel->setAlignment(Qt::AlignCenter);
    warningLayout->addWidget(m_warningValueLabel);

    // Still duration
    QGroupBox* durationGroup = new QGroupBox("Still Duration", m_countersFrame);
    QVBoxLayout* durationLayout = new QVBoxLayout(durationGroup);
    m_stillDurationValueLabel = new QLabel("0:00", durationGroup);
    m_stillDurationValueLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    m_stillDurationValueLabel->setAlignment(Qt::AlignCenter);
    durationLayout->addWidget(m_stillDurationValueLabel);

    layout->addWidget(violationGroup);
    layout->addWidget(warningGroup);
    layout->addWidget(durationGroup);

    m_mainLayout->addWidget(m_countersFrame);
}

void MotionMonitor::setupControls()
{
    m_controlFrame = new QFrame(this);
    QHBoxLayout* layout = new QHBoxLayout(m_controlFrame);

    // Sensitivity selector
    QLabel* sensitivityLabel = new QLabel("Sensitivity:", m_controlFrame);
    m_sensitivityCombo = new QComboBox(m_controlFrame);
    m_sensitivityCombo->addItem("Lenient", 0);
    m_sensitivityCombo->addItem("Normal", 1);
    m_sensitivityCombo->addItem("Strict", 2);
    m_sensitivityCombo->addItem("Extreme", 3);
    m_sensitivityCombo->setCurrentIndex(1);  // Default to Normal
    connect(m_sensitivityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MotionMonitor::onSensitivityChanged);

    // Calibration button and progress
    m_calibrateButton = new QPushButton("Calibrate", m_controlFrame);
    m_calibrateButton->setToolTip("Calibrate sensor for current position");
    connect(m_calibrateButton, &QPushButton::clicked, this, &MotionMonitor::startCalibration);

    m_calibrationProgress = new QProgressBar(m_controlFrame);
    m_calibrationProgress->setRange(0, 100);
    m_calibrationProgress->setValue(0);
    m_calibrationProgress->setTextVisible(true);
    m_calibrationProgress->setMaximumWidth(100);
    m_calibrationProgress->setVisible(false);

    m_calibrationStatusLabel = new QLabel("", m_controlFrame);
    m_calibrationStatusLabel->setStyleSheet("font-size: 12px;");

    // Reset button
    m_resetButton = new QPushButton("Reset Session", m_controlFrame);
    m_resetButton->setToolTip("Reset all session counters");
    connect(m_resetButton, &QPushButton::clicked, this, &MotionMonitor::resetSession);

    layout->addWidget(sensitivityLabel);
    layout->addWidget(m_sensitivityCombo);
    layout->addSpacing(20);
    layout->addWidget(m_calibrateButton);
    layout->addWidget(m_calibrationProgress);
    layout->addWidget(m_calibrationStatusLabel);
    layout->addStretch();
    layout->addWidget(m_resetButton);

    m_mainLayout->addWidget(m_controlFrame);
}

// ============================================================================
// Update Methods
// ============================================================================

void MotionMonitor::updateMotion(double accelMagnitude, double gyroMagnitude)
{
    m_currentAccelMagnitude = accelMagnitude;
    m_currentGyroMagnitude = gyroMagnitude;
    updateAccelDisplay(accelMagnitude);
    updateGyroDisplay(gyroMagnitude);
}

void MotionMonitor::updateStillness(double stillnessScore)
{
    m_currentStillnessScore = stillnessScore;
    m_stillnessValueLabel->setText(QString("%1%").arg(stillnessScore, 0, 'f', 0));
    m_stillnessBar->setValue(static_cast<int>(stillnessScore));

    // Update color based on stillness level
    QString color;
    QString status;
    if (stillnessScore >= 90) {
        color = "#4CAF50";  // Green
        status = "Perfect";
    } else if (stillnessScore >= 70) {
        color = "#8BC34A";  // Light green
        status = "Good";
    } else if (stillnessScore >= 50) {
        color = "#FF9800";  // Orange
        status = "Warning";
    } else {
        color = "#F44336";  // Red
        status = "Moving!";
    }

    m_stillnessValueLabel->setStyleSheet(QString("font-size: 36px; font-weight: bold; color: %1;").arg(color));
    m_stillnessBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
    m_stillnessStatusLabel->setText(status);
    m_stillnessStatusLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(color));
}

void MotionMonitor::updateAccelDisplay(double magnitude)
{
    m_accelValueLabel->setText(QString("%1 g").arg(magnitude, 0, 'f', 3));
    int percent = static_cast<int>((magnitude / MAX_ACCEL_DISPLAY) * 100);
    m_accelBar->setValue(qMin(percent, 100));

    // Color based on magnitude
    QString color = magnitude < 0.1 ? "#4CAF50" : magnitude < 0.3 ? "#FF9800" : "#F44336";
    m_accelBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
}

void MotionMonitor::updateGyroDisplay(double magnitude)
{
    m_gyroValueLabel->setText(QString("%1 °/s").arg(magnitude, 0, 'f', 1));
    int percent = static_cast<int>((magnitude / MAX_GYRO_DISPLAY) * 100);
    m_gyroBar->setValue(qMin(percent, 100));

    // Color based on magnitude
    QString color = magnitude < 10 ? "#4CAF50" : magnitude < 30 ? "#FF9800" : "#F44336";
    m_gyroBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
}

void MotionMonitor::updateMotionLevelDisplay()
{
    QString levelText = motionLevelToString(m_currentMotionLevel);
    QString color = motionLevelToColor(m_currentMotionLevel);

    m_motionLevelIndicator->setText(levelText);

    QString bgColor = m_currentMotionLevel == 0 ? "#E8F5E9" :
                      m_currentMotionLevel == 1 ? "#FFF3E0" :
                      m_currentMotionLevel == 2 ? "#FFEBEE" : "#F44336";

    m_motionLevelIndicator->setStyleSheet(
        QString("font-size: 24px; font-weight: bold; color: %1; "
                "padding: 5px 15px; border-radius: 5px; background-color: %2;")
        .arg(color, bgColor));
}

void MotionMonitor::updateChart()
{
    if (m_updatesPaused) return;

    qint64 elapsed = m_sessionTimer.elapsed();
    double timeSeconds = elapsed / 1000.0;

    // Add current data point
    addDataPoint(m_currentStillnessScore);

    // Update time axis range (scrolling window)
    if (timeSeconds > m_chartTimeRangeSeconds) {
        m_timeAxis->setRange(timeSeconds - m_chartTimeRangeSeconds, timeSeconds);
    }

    // Update still duration display
    if (m_motionSensor && m_motionSensor->isCurrentlyStill()) {
        qint64 stillMs = m_motionSensor->getStillDurationMs();
        int minutes = stillMs / 60000;
        int seconds = (stillMs % 60000) / 1000;
        m_stillDurationValueLabel->setText(QString("%1:%2")
            .arg(minutes).arg(seconds, 2, 10, QChar('0')));
    }
}

void MotionMonitor::addDataPoint(double stillness)
{
    qint64 elapsed = m_sessionTimer.elapsed();
    double timeSeconds = elapsed / 1000.0;

    m_stillnessSeries->append(timeSeconds, stillness);

    // Limit data points
    while (m_stillnessSeries->count() > MAX_DATA_POINTS) {
        m_stillnessSeries->remove(0);
    }
}

QString MotionMonitor::motionLevelToString(int level)
{
    switch (level) {
        case 0: return "STILL";
        case 1: return "MINOR";
        case 2: return "MODERATE";
        case 3: return "MAJOR";
        default: return "UNKNOWN";
    }
}

QString MotionMonitor::motionLevelToColor(int level)
{
    switch (level) {
        case 0: return "#4CAF50";  // Green
        case 1: return "#FF9800";  // Orange
        case 2: return "#F44336";  // Red
        case 3: return "#B71C1C";  // Dark red
        default: return "#9E9E9E"; // Gray
    }
}

// ============================================================================
// Slot Handlers
// ============================================================================

void MotionMonitor::onMotionDetected(int level, double magnitude)
{
    m_currentMotionLevel = level;

    // Get accel/gyro from sensor
    if (m_motionSensor) {
        QVector3D accel = m_motionSensor->getAcceleration();
        QVector3D gyro = m_motionSensor->getGyroscope();
        updateMotion(accel.length(), gyro.length());
    }

    updateMotionLevelDisplay();
}

void MotionMonitor::onStillnessChanged(bool isStill, double stillnessScore)
{
    updateStillness(stillnessScore);

    if (isStill) {
        m_stillDurationValueLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    } else {
        m_stillDurationValueLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #FF9800;");
    }
}

void MotionMonitor::onViolationDetected(int level, double intensity)
{
    Q_UNUSED(level)
    Q_UNUSED(intensity)

    m_violationCount++;
    m_violationValueLabel->setText(QString::number(m_violationCount));

    // Flash the violation counter
    m_violationValueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #FFFFFF; background-color: #F44336;");
    QTimer::singleShot(200, this, [this]() {
        m_violationValueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #F44336;");
    });
}

void MotionMonitor::onWarningIssued(const QString& message)
{
    Q_UNUSED(message)

    m_warningCount++;
    m_warningValueLabel->setText(QString::number(m_warningCount));

    // Flash the warning counter
    m_warningValueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #FFFFFF; background-color: #FF9800;");
    QTimer::singleShot(200, this, [this]() {
        m_warningValueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #FF9800;");
    });
}

void MotionMonitor::onCalibrationComplete(bool success)
{
    m_calibrationProgress->setVisible(false);
    m_calibrateButton->setEnabled(true);

    if (success) {
        m_calibrationStatusLabel->setText("Calibrated ✓");
        m_calibrationStatusLabel->setStyleSheet("font-size: 12px; color: #4CAF50;");
    } else {
        m_calibrationStatusLabel->setText("Failed ✗");
        m_calibrationStatusLabel->setStyleSheet("font-size: 12px; color: #F44336;");
    }
}

void MotionMonitor::onCalibrationProgress(int percent)
{
    m_calibrationProgress->setValue(percent);
}

// ============================================================================
// Control Methods
// ============================================================================

void MotionMonitor::resetSession()
{
    m_violationCount = 0;
    m_warningCount = 0;
    m_currentStillnessScore = 100.0;
    m_currentMotionLevel = 0;

    m_stillnessSeries->clear();
    m_sessionTimer.restart();
    m_timeAxis->setRange(0, m_chartTimeRangeSeconds);

    m_violationValueLabel->setText("0");
    m_warningValueLabel->setText("0");
    m_stillDurationValueLabel->setText("0:00");

    updateStillness(100.0);
    updateMotionLevelDisplay();

    if (m_motionSensor) {
        m_motionSensor->resetSession();
    }

    emit sessionReset();
}

void MotionMonitor::pauseUpdates(bool pause)
{
    m_updatesPaused = pause;
}

void MotionMonitor::startCalibration()
{
    if (!m_motionSensor) {
        m_calibrationStatusLabel->setText("No sensor!");
        m_calibrationStatusLabel->setStyleSheet("font-size: 12px; color: #F44336;");
        return;
    }

    m_calibrateButton->setEnabled(false);
    m_calibrationProgress->setVisible(true);
    m_calibrationProgress->setValue(0);
    m_calibrationStatusLabel->setText("Hold still...");
    m_calibrationStatusLabel->setStyleSheet("font-size: 12px; color: #2196F3;");

    m_motionSensor->calibrate(3000);  // 3 second calibration
    emit calibrationRequested();
}

void MotionMonitor::onSensitivityChanged(int index)
{
    if (m_motionSensor) {
        m_motionSensor->setSensitivity(static_cast<MotionSensor::SensitivityPreset>(index));
    }
    emit sensitivityChanged(index);
}

void MotionMonitor::setChartTimeRange(int seconds)
{
    m_chartTimeRangeSeconds = seconds;
    m_timeAxis->setRange(0, seconds);
}

void MotionMonitor::setSessionActive(bool active)
{
    m_sessionActive = active;
    if (active) {
        resetSession();
    }
}

