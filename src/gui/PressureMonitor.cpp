#include "PressureMonitor.h"
#include "components/PressureGauge.h"
#include "components/PressureChart.h"
#include "components/StatusIndicator.h"
#include "../VacuumController.h"
#include <QDebug>
#include <QDateTime>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

// Constants
const double PressureMonitor::DEFAULT_MAX_PRESSURE = 100.0;
const double PressureMonitor::DEFAULT_WARNING_THRESHOLD = 80.0;
const double PressureMonitor::DEFAULT_ANTI_DETACHMENT = 50.0;

PressureMonitor::PressureMonitor(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_maxPressure(DEFAULT_MAX_PRESSURE)
    , m_warningThreshold(DEFAULT_WARNING_THRESHOLD)
    , m_antiDetachmentThreshold(DEFAULT_ANTI_DETACHMENT)
    , m_chartTimeRangeSeconds(DEFAULT_CHART_TIME_RANGE)
    , m_showGrid(true)
    , m_showAlarms(true)
    , m_updatesPaused(false)
    , m_currentAVL(0.0)
    , m_currentTank(0.0)
    , m_chartUpdateTimer(new QTimer(this))
    , m_avlSeries(nullptr)
    , m_tankSeries(nullptr)
    , m_timeAxis(nullptr)
    , m_pressureAxis(nullptr)
{
    setupUI();
    setupPressureDisplays();
    setupChart();
    setupAlarmIndicators();
    
    // Connect timer for chart updates
    m_chartUpdateTimer->setInterval(CHART_UPDATE_INTERVAL);
    connect(m_chartUpdateTimer, &QTimer::timeout, this, &PressureMonitor::updateChart);
    m_chartUpdateTimer->start();
    
    // Connect to controller if available
    if (m_controller) {
        connect(m_controller, &VacuumController::pressureUpdated,
                this, &PressureMonitor::updatePressures);
    }
}

PressureMonitor::~PressureMonitor()
{
    if (m_chartUpdateTimer) {
        m_chartUpdateTimer->stop();
    }
}

void PressureMonitor::updatePressures(double avlPressure, double tankPressure)
{
    if (m_updatesPaused) return;
    
    m_currentAVL = avlPressure;
    m_currentTank = tankPressure;
    
    // Update pressure displays
    updatePressureDisplay(m_avlValueLabel, m_avlProgressBar, avlPressure, m_maxPressure);
    updatePressureDisplay(m_tankValueLabel, m_tankProgressBar, tankPressure, m_maxPressure);
    
    // Update status labels
    updateAlarmStates();
    
    // Add data point for chart
    addDataPoint(avlPressure, tankPressure);
}

void PressureMonitor::setMaxPressure(double maxPressure)
{
    if (maxPressure > 0) {
        m_maxPressure = maxPressure;
        
        // Update progress bar ranges
        if (m_avlProgressBar) {
            m_avlProgressBar->setRange(0, static_cast<int>(maxPressure));
        }
        if (m_tankProgressBar) {
            m_tankProgressBar->setRange(0, static_cast<int>(maxPressure));
        }
        
        // Update chart axis
        if (m_pressureAxis) {
            m_pressureAxis->setRange(0, maxPressure);
        }
    }
}

void PressureMonitor::setWarningThreshold(double warningThreshold)
{
    if (warningThreshold > 0 && warningThreshold < m_maxPressure) {
        m_warningThreshold = warningThreshold;
        updateAlarmStates();
    }
}

void PressureMonitor::setAntiDetachmentThreshold(double threshold)
{
    if (threshold > 0 && threshold < m_maxPressure) {
        m_antiDetachmentThreshold = threshold;
        updateAlarmStates();
    }
}

void PressureMonitor::setChartTimeRange(int seconds)
{
    if (seconds > 0) {
        m_chartTimeRangeSeconds = seconds;
        
        // Update time axis
        if (m_timeAxis) {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            m_timeAxis->setRange(currentTime - (seconds * 1000), currentTime);
        }
    }
}

void PressureMonitor::resetChart()
{
    m_avlData.clear();
    m_tankData.clear();
    
    if (m_avlSeries) {
        m_avlSeries->clear();
    }
    if (m_tankSeries) {
        m_tankSeries->clear();
    }
}

void PressureMonitor::pauseUpdates(bool pause)
{
    m_updatesPaused = pause;
    
    if (pause) {
        m_chartUpdateTimer->stop();
    } else {
        m_chartUpdateTimer->start();
    }
}

void PressureMonitor::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
}

void PressureMonitor::setupPressureDisplays()
{
    // Create display frame
    m_displayFrame = new QFrame;
    m_displayFrame->setFrameStyle(QFrame::Box);
    m_displayFrame->setStyleSheet("QFrame { border: 2px solid #ddd; border-radius: 5px; background-color: white; }");
    
    QGridLayout* displayLayout = new QGridLayout(m_displayFrame);
    displayLayout->setSpacing(15);
    displayLayout->setContentsMargins(10, 10, 10, 10);
    
    // AVL Pressure Display
    QLabel* avlLabel = new QLabel("AVL Pressure");
    avlLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #2196F3;");
    avlLabel->setAlignment(Qt::AlignCenter);
    
    m_avlValueLabel = new QLabel("0.0 mmHg");
    m_avlValueLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #333;");
    m_avlValueLabel->setAlignment(Qt::AlignCenter);
    
    m_avlProgressBar = new QProgressBar;
    m_avlProgressBar->setRange(0, static_cast<int>(m_maxPressure));
    m_avlProgressBar->setValue(0);
    m_avlProgressBar->setMinimumHeight(30);
    m_avlProgressBar->setStyleSheet(
        "QProgressBar { border: 2px solid #ddd; border-radius: 5px; text-align: center; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }"
    );
    
    m_avlStatusLabel = new QLabel("Normal");
    m_avlStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    m_avlStatusLabel->setAlignment(Qt::AlignCenter);
    
    // Tank Pressure Display
    QLabel* tankLabel = new QLabel("Tank Pressure");
    tankLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #2196F3;");
    tankLabel->setAlignment(Qt::AlignCenter);
    
    m_tankValueLabel = new QLabel("0.0 mmHg");
    m_tankValueLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #333;");
    m_tankValueLabel->setAlignment(Qt::AlignCenter);
    
    m_tankProgressBar = new QProgressBar;
    m_tankProgressBar->setRange(0, static_cast<int>(m_maxPressure));
    m_tankProgressBar->setValue(0);
    m_tankProgressBar->setMinimumHeight(30);
    m_tankProgressBar->setStyleSheet(
        "QProgressBar { border: 2px solid #ddd; border-radius: 5px; text-align: center; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }"
    );
    
    m_tankStatusLabel = new QLabel("Normal");
    m_tankStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    m_tankStatusLabel->setAlignment(Qt::AlignCenter);
    
    // Layout displays
    displayLayout->addWidget(avlLabel, 0, 0);
    displayLayout->addWidget(m_avlValueLabel, 1, 0);
    displayLayout->addWidget(m_avlProgressBar, 2, 0);
    displayLayout->addWidget(m_avlStatusLabel, 3, 0);
    
    displayLayout->addWidget(tankLabel, 0, 1);
    displayLayout->addWidget(m_tankValueLabel, 1, 1);
    displayLayout->addWidget(m_tankProgressBar, 2, 1);
    displayLayout->addWidget(m_tankStatusLabel, 3, 1);
    
    m_mainLayout->addWidget(m_displayFrame);
}

void PressureMonitor::setupChart()
{
    // Create chart frame
    m_chartFrame = new QFrame;
    m_chartFrame->setFrameStyle(QFrame::Box);
    m_chartFrame->setStyleSheet("QFrame { border: 2px solid #ddd; border-radius: 5px; }");

    QVBoxLayout* chartLayout = new QVBoxLayout(m_chartFrame);

    // Chart title
    QLabel* chartTitle = new QLabel("Pressure History");
    chartTitle->setStyleSheet("font-size: 16pt; font-weight: bold; color: #2196F3;");
    chartTitle->setAlignment(Qt::AlignCenter);
    chartLayout->addWidget(chartTitle);

    // Create real-time pressure chart
    m_chart = new PressureChart();
    m_chart->setWarningThreshold(m_warningThreshold);
    m_chart->setCriticalThreshold(m_maxPressure * 0.95);
    m_chart->setAntiDetachmentThreshold(m_antiDetachmentThreshold);
    m_chart->setTimeRange(PressureChart::RANGE_5MIN);

    // Connect chart signals
    connect(m_chart, &PressureChart::thresholdViolation,
            this, &PressureMonitor::pressureAlarm);

    chartLayout->addWidget(m_chart);
    m_mainLayout->addWidget(m_chartFrame);
}

void PressureMonitor::setupAlarmIndicators()
{
    // Create alarm frame
    m_alarmFrame = new QFrame;
    m_alarmFrame->setFrameStyle(QFrame::Box);
    m_alarmFrame->setStyleSheet("QFrame { border: 2px solid #ddd; border-radius: 5px; background-color: #f8f8f8; }");
    
    QHBoxLayout* alarmLayout = new QHBoxLayout(m_alarmFrame);
    alarmLayout->setSpacing(20);
    
    // Alarm indicators
    m_overpressureAlarm = new QLabel("Overpressure: OK");
    m_overpressureAlarm->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    
    m_antiDetachmentAlarm = new QLabel("Anti-detachment: OK");
    m_antiDetachmentAlarm->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    
    m_sensorErrorAlarm = new QLabel("Sensors: OK");
    m_sensorErrorAlarm->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    
    alarmLayout->addWidget(m_overpressureAlarm);
    alarmLayout->addWidget(m_antiDetachmentAlarm);
    alarmLayout->addWidget(m_sensorErrorAlarm);
    alarmLayout->addStretch();
    
    m_mainLayout->addWidget(m_alarmFrame);
}

void PressureMonitor::updateChart()
{
    // Chart update implementation would go here
    // This would update the QtCharts components with new data
}

void PressureMonitor::updatePressureDisplay(QLabel* valueLabel, QProgressBar* progressBar, 
                                           double pressure, double maxPressure)
{
    if (!valueLabel || !progressBar) return;
    
    // Update value label
    valueLabel->setText(QString("%1 mmHg").arg(pressure, 0, 'f', 1));
    
    // Update progress bar
    progressBar->setValue(static_cast<int>(pressure));
    
    // Update progress bar color based on pressure level
    QString color = "#4CAF50";  // Green (safe)
    if (pressure > m_warningThreshold) {
        color = "#FF9800";  // Orange (warning)
    }
    if (pressure > maxPressure * 0.9) {
        color = "#f44336";  // Red (critical)
    }
    
    progressBar->setStyleSheet(
        QString("QProgressBar { border: 2px solid #ddd; border-radius: 5px; text-align: center; }"
                "QProgressBar::chunk { background-color: %1; border-radius: 3px; }").arg(color)
    );
}

void PressureMonitor::updateAlarmStates()
{
    // Update overpressure alarm
    if (m_currentAVL > m_maxPressure || m_currentTank > m_maxPressure) {
        m_overpressureAlarm->setText("Overpressure: ALARM");
        m_overpressureAlarm->setStyleSheet("font-size: 14pt; color: #f44336; font-weight: bold;");
        emit pressureAlarm("Overpressure detected");
    } else if (m_currentAVL > m_warningThreshold || m_currentTank > m_warningThreshold) {
        m_overpressureAlarm->setText("Overpressure: WARNING");
        m_overpressureAlarm->setStyleSheet("font-size: 14pt; color: #FF9800; font-weight: bold;");
    } else {
        m_overpressureAlarm->setText("Overpressure: OK");
        m_overpressureAlarm->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    }
    
    // Update anti-detachment alarm
    if (m_currentAVL < m_antiDetachmentThreshold) {
        m_antiDetachmentAlarm->setText("Anti-detachment: ACTIVE");
        m_antiDetachmentAlarm->setStyleSheet("font-size: 14pt; color: #FF9800; font-weight: bold;");
        emit antiDetachmentTriggered();
    } else {
        m_antiDetachmentAlarm->setText("Anti-detachment: OK");
        m_antiDetachmentAlarm->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    }
    
    // Update status labels
    if (m_avlStatusLabel) {
        if (m_currentAVL > m_warningThreshold) {
            m_avlStatusLabel->setText("High Pressure");
            m_avlStatusLabel->setStyleSheet("font-size: 14pt; color: #FF9800; font-weight: bold;");
        } else {
            m_avlStatusLabel->setText("Normal");
            m_avlStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
        }
    }
    
    if (m_tankStatusLabel) {
        if (m_currentTank > m_warningThreshold) {
            m_tankStatusLabel->setText("High Pressure");
            m_tankStatusLabel->setStyleSheet("font-size: 14pt; color: #FF9800; font-weight: bold;");
        } else {
            m_tankStatusLabel->setText("Normal");
            m_tankStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
        }
    }
}

void PressureMonitor::addDataPoint(double avlPressure, double tankPressure)
{
    // Add data to chart
    if (m_chart) {
        m_chart->addDataPoint(avlPressure, tankPressure);
    }
}
