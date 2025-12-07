#ifndef MOTIONMONITOR_H
#define MOTIONMONITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QQueue>
#include <QElapsedTimer>

QT_CHARTS_USE_NAMESPACE

// Forward declarations
class MotionSensor;
class HardwareManager;

/**
 * @brief Real-time motion and stillness monitoring widget
 * 
 * This widget provides comprehensive motion monitoring including:
 * - Real-time motion magnitude (acceleration in g-forces, rotation in °/s)
 * - Current stillness score (0-100%)
 * - Motion level indicator (STILL/MINOR/MODERATE/MAJOR with color coding)
 * - Violation and warning counters for game sessions
 * - Historical stillness quality graph over last 60 seconds
 * - Calibration status and recalibrate button
 * - Sensitivity preset selector
 */
class MotionMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit MotionMonitor(HardwareManager* hardware, QWidget *parent = nullptr);
    ~MotionMonitor();

    // Manual updates (if not using sensor signals)
    void updateMotion(double accelMagnitude, double gyroMagnitude);
    void updateStillness(double stillnessScore);
    
    // Configuration
    void setChartTimeRange(int seconds);
    void setSessionActive(bool active);

public Q_SLOTS:
    void resetSession();
    void pauseUpdates(bool pause);
    void startCalibration();
    void onSensitivityChanged(int index);

Q_SIGNALS:
    void sensitivityChanged(int presetIndex);
    void calibrationRequested();
    void sessionReset();

private Q_SLOTS:
    void updateChart();
    void onMotionDetected(int level, double magnitude);
    void onStillnessChanged(bool isStill, double stillnessScore);
    void onViolationDetected(int level, double intensity);
    void onWarningIssued(const QString& message);
    void onCalibrationComplete(bool success);
    void onCalibrationProgress(int percent);

private:
    void setupUI();
    void setupMotionDisplays();
    void setupStillnessDisplay();
    void setupChart();
    void setupViolationCounters();
    void setupControls();
    void updateMotionLevelDisplay();
    void updateAccelDisplay(double magnitude);
    void updateGyroDisplay(double magnitude);
    void addDataPoint(double stillness);
    QString motionLevelToString(int level);
    QString motionLevelToColor(int level);
    
    // Hardware interface
    HardwareManager* m_hardware;
    MotionSensor* m_motionSensor;
    
    // UI components - Main layout
    QVBoxLayout* m_mainLayout;
    
    // Motion displays
    QFrame* m_motionFrame;
    QLabel* m_accelLabel;
    QLabel* m_accelValueLabel;
    QProgressBar* m_accelBar;
    QLabel* m_gyroLabel;
    QLabel* m_gyroValueLabel;
    QProgressBar* m_gyroBar;
    
    // Motion level indicator
    QFrame* m_levelFrame;
    QLabel* m_motionLevelLabel;
    QLabel* m_motionLevelIndicator;
    
    // Stillness display
    QFrame* m_stillnessFrame;
    QLabel* m_stillnessLabel;
    QLabel* m_stillnessValueLabel;
    QProgressBar* m_stillnessBar;
    QLabel* m_stillnessStatusLabel;
    
    // Chart components
    QFrame* m_chartFrame;
    QChartView* m_chartView;
    QChart* m_chart;
    QLineSeries* m_stillnessSeries;
    QValueAxis* m_timeAxis;
    QValueAxis* m_stillnessAxis;
    
    // Violation counters
    QFrame* m_countersFrame;
    QLabel* m_violationCountLabel;
    QLabel* m_violationValueLabel;
    QLabel* m_warningCountLabel;
    QLabel* m_warningValueLabel;
    QLabel* m_stillDurationLabel;
    QLabel* m_stillDurationValueLabel;
    
    // Control buttons
    QFrame* m_controlFrame;
    QComboBox* m_sensitivityCombo;
    QPushButton* m_calibrateButton;
    QProgressBar* m_calibrationProgress;
    QLabel* m_calibrationStatusLabel;
    QPushButton* m_resetButton;
    
    // Data storage
    QQueue<QPair<qint64, double>> m_stillnessData;  // timestamp, stillness score
    
    // Configuration
    int m_chartTimeRangeSeconds;
    bool m_updatesPaused;
    bool m_sessionActive;
    
    // Current values
    double m_currentAccelMagnitude;
    double m_currentGyroMagnitude;
    double m_currentStillnessScore;
    int m_currentMotionLevel;
    int m_violationCount;
    int m_warningCount;
    
    // Timers
    QTimer* m_chartUpdateTimer;
    QElapsedTimer m_sessionTimer;
    
    // Constants
    static const int DEFAULT_CHART_TIME_RANGE = 60;   // 60 seconds
    static const int CHART_UPDATE_INTERVAL = 100;     // 100ms for smooth updates
    static const int MAX_DATA_POINTS = 600;           // 60 seconds at 10/sec
    static constexpr double MAX_ACCEL_DISPLAY = 1.0;  // 1g max display
    static constexpr double MAX_GYRO_DISPLAY = 100.0; // 100°/s max display
};

#endif // MOTIONMONITOR_H

