#include "PressureChart.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>
#include <cmath>

QT_CHARTS_USE_NAMESPACE

// Constants
const double PressureChart::DEFAULT_MIN_PRESSURE = 0.0;
const double PressureChart::DEFAULT_MAX_PRESSURE = 100.0;
const double PressureChart::DEFAULT_WARNING_THRESHOLD = 80.0;
const double PressureChart::DEFAULT_CRITICAL_THRESHOLD = 95.0;
const double PressureChart::DEFAULT_ANTI_DETACHMENT = 50.0;

PressureChart::PressureChart(QWidget *parent)
    : QWidget(parent)
    , m_chart(nullptr)
    , m_chartView(nullptr)
    , m_avlSeries(nullptr)
    , m_tankSeries(nullptr)
    , m_warningLine(nullptr)
    , m_criticalLine(nullptr)
    , m_antiDetachmentLine(nullptr)
    , m_timeAxis(nullptr)
    , m_pressureAxis(nullptr)
    , m_maxDataPoints(DEFAULT_MAX_DATA_POINTS)
    , m_timeRange(RANGE_5MIN)
    , m_minPressure(DEFAULT_MIN_PRESSURE)
    , m_maxPressure(DEFAULT_MAX_PRESSURE)
    , m_autoScale(true)
    , m_showThresholds(true)
    , m_showGrid(true)
    , m_showLegend(true)
    , m_updatesPaused(false)
    , m_warningThreshold(DEFAULT_WARNING_THRESHOLD)
    , m_criticalThreshold(DEFAULT_CRITICAL_THRESHOLD)
    , m_antiDetachmentThreshold(DEFAULT_ANTI_DETACHMENT)
    , m_avlColor(QColor(33, 150, 243))      // Blue
    , m_tankColor(QColor(76, 175, 80))      // Green
    , m_lineWidth(2)
    , m_updateTimer(new QTimer(this))
{
    setupUI();
    setupChart();
    setupControls();
    connectSignals();
    
    // Start update timer
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &PressureChart::updateChart);
    m_updateTimer->start();
}

PressureChart::~PressureChart()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void PressureChart::addDataPoint(double avlPressure, double tankPressure)
{
    if (m_updatesPaused) return;
    
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    PressureDataPoint point(timestamp, avlPressure, tankPressure);
    
    m_dataQueue.enqueue(point);
    
    // Remove old data
    removeOldData();
    
    // Check for threshold violations
    if (avlPressure > m_criticalThreshold || tankPressure > m_criticalThreshold) {
        emit thresholdViolation("Critical", qMax(avlPressure, tankPressure));
    } else if (avlPressure > m_warningThreshold || tankPressure > m_warningThreshold) {
        emit thresholdViolation("Warning", qMax(avlPressure, tankPressure));
    }
    
    if (avlPressure < m_antiDetachmentThreshold) {
        emit thresholdViolation("Anti-detachment", avlPressure);
    }
    
    emit dataPointAdded(point);
}

void PressureChart::clearData()
{
    m_dataQueue.clear();
    if (m_avlSeries) m_avlSeries->clear();
    if (m_tankSeries) m_tankSeries->clear();
    updateChart();
}

void PressureChart::setTimeRange(TimeRange range)
{
    m_timeRange = range;
    removeOldData();
    updateTimeAxis();
    updateChart();
}

void PressureChart::setPressureRange(double minPressure, double maxPressure)
{
    if (minPressure < maxPressure) {
        m_minPressure = minPressure;
        m_maxPressure = maxPressure;
        m_autoScale = false;
        updatePressureAxis();
    }
}

void PressureChart::setAutoScale(bool enabled)
{
    m_autoScale = enabled;
    if (enabled) {
        updatePressureAxis();
    }
}

void PressureChart::setWarningThreshold(double threshold)
{
    m_warningThreshold = threshold;
    addThresholdLines();
}

void PressureChart::setCriticalThreshold(double threshold)
{
    m_criticalThreshold = threshold;
    addThresholdLines();
}

void PressureChart::setAntiDetachmentThreshold(double threshold)
{
    m_antiDetachmentThreshold = threshold;
    addThresholdLines();
}

void PressureChart::setShowThresholds(bool show)
{
    m_showThresholds = show;
    addThresholdLines();
}

void PressureChart::pauseUpdates(bool pause)
{
    m_updatesPaused = pause;
    m_pauseButton->setText(pause ? "Resume" : "Pause");
    m_statusLabel->setText(pause ? "Paused" : "Recording");
}

void PressureChart::resetZoom()
{
    if (m_chartView) {
        m_chartView->chart()->zoomReset();
        updateTimeAxis();
        updatePressureAxis();
    }
}

void PressureChart::zoomIn()
{
    if (m_chartView) {
        m_chartView->chart()->zoom(1.5);
    }
}

void PressureChart::zoomOut()
{
    if (m_chartView) {
        m_chartView->chart()->zoom(0.75);
    }
}

void PressureChart::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
}

void PressureChart::setupChart()
{
    // Create chart
    m_chart = new QtCharts::QChart();
    m_chart->setTitle("Pressure Monitoring");
    m_chart->setTitleFont(QFont("Arial", 16, QFont::Bold));
    m_chart->setBackgroundBrush(QBrush(QColor(250, 250, 250)));
    
    // Create series
    m_avlSeries = new QtCharts::QLineSeries();
    m_avlSeries->setName("AVL Pressure");
    m_avlSeries->setColor(m_avlColor);
    m_avlSeries->setPen(QPen(m_avlColor, m_lineWidth));
    
    m_tankSeries = new QtCharts::QLineSeries();
    m_tankSeries->setName("Tank Pressure");
    m_tankSeries->setColor(m_tankColor);
    m_tankSeries->setPen(QPen(m_tankColor, m_lineWidth));
    
    // Add series to chart
    m_chart->addSeries(m_avlSeries);
    m_chart->addSeries(m_tankSeries);
    
    // Create axes
    m_timeAxis = new QtCharts::QDateTimeAxis();
    m_timeAxis->setFormat("hh:mm:ss");
    m_timeAxis->setTitleText("Time");
    m_timeAxis->setTitleFont(QFont("Arial", 12, QFont::Bold));
    m_chart->addAxis(m_timeAxis, Qt::AlignBottom);
    
    m_pressureAxis = new QtCharts::QValueAxis();
    m_pressureAxis->setTitleText("Pressure (mmHg)");
    m_pressureAxis->setTitleFont(QFont("Arial", 12, QFont::Bold));
    m_pressureAxis->setLabelFormat("%.1f");
    m_chart->addAxis(m_pressureAxis, Qt::AlignLeft);
    
    // Attach series to axes
    m_avlSeries->attachAxis(m_timeAxis);
    m_avlSeries->attachAxis(m_pressureAxis);
    m_tankSeries->attachAxis(m_timeAxis);
    m_tankSeries->attachAxis(m_pressureAxis);
    
    // Configure legend
    m_chart->legend()->setVisible(m_showLegend);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_chart->legend()->setFont(QFont("Arial", 10));
    
    // Create chart view
    m_chartView = new QtCharts::QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(400);
    
    // Add threshold lines
    addThresholdLines();
    
    // Update axes
    updateTimeAxis();
    updatePressureAxis();
    
    m_mainLayout->addWidget(m_chartView, 1);
}

void PressureChart::setupControls()
{
    m_controlLayout = new QHBoxLayout();
    
    // Time range selector
    QLabel* rangeLabel = new QLabel("Time Range:");
    rangeLabel->setFont(QFont("Arial", 12, QFont::Bold));
    
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->addItem("1 Minute", RANGE_1MIN);
    m_timeRangeCombo->addItem("5 Minutes", RANGE_5MIN);
    m_timeRangeCombo->addItem("15 Minutes", RANGE_15MIN);
    m_timeRangeCombo->addItem("1 Hour", RANGE_1HOUR);
    m_timeRangeCombo->setCurrentIndex(1); // 5 minutes default
    m_timeRangeCombo->setMinimumHeight(40);
    
    // Control buttons
    m_pauseButton = new QPushButton("Pause");
    m_pauseButton->setMinimumSize(100, 40);
    m_pauseButton->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_resetZoomButton = new QPushButton("Reset Zoom");
    m_resetZoomButton->setMinimumSize(120, 40);
    m_resetZoomButton->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_exportButton = new QPushButton("Export");
    m_exportButton->setMinimumSize(100, 40);
    m_exportButton->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    // Status label
    m_statusLabel = new QLabel("Recording");
    m_statusLabel->setFont(QFont("Arial", 12, QFont::Bold));
    m_statusLabel->setStyleSheet("color: #4CAF50;");
    
    // Layout controls
    m_controlLayout->addWidget(rangeLabel);
    m_controlLayout->addWidget(m_timeRangeCombo);
    m_controlLayout->addSpacing(20);
    m_controlLayout->addWidget(m_pauseButton);
    m_controlLayout->addWidget(m_resetZoomButton);
    m_controlLayout->addWidget(m_exportButton);
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_statusLabel);
    
    m_mainLayout->addLayout(m_controlLayout);
}

void PressureChart::connectSignals()
{
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PressureChart::onTimeRangeChanged);
    connect(m_pauseButton, &QPushButton::clicked, this, &PressureChart::onPauseClicked);
    connect(m_resetZoomButton, &QPushButton::clicked, this, &PressureChart::onResetZoomClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &PressureChart::onExportClicked);
}

void PressureChart::updateChart()
{
    if (m_updatesPaused || m_dataQueue.isEmpty()) return;
    
    // Clear existing data
    m_avlSeries->clear();
    m_tankSeries->clear();
    
    // Add data points
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 cutoffTime = currentTime - (m_timeRange * 1000);
    
    for (const auto& point : m_dataQueue) {
        if (point.timestamp >= cutoffTime) {
            m_avlSeries->append(point.timestamp, point.avlPressure);
            m_tankSeries->append(point.timestamp, point.tankPressure);
        }
    }
    
    // Update axes if auto-scaling
    if (m_autoScale) {
        updatePressureAxis();
    }
    updateTimeAxis();
}

void PressureChart::updateTimeAxis()
{
    if (!m_timeAxis) return;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 startTime = currentTime - (m_timeRange * 1000);
    
    m_timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(startTime),
                        QDateTime::fromMSecsSinceEpoch(currentTime));
}

void PressureChart::updatePressureAxis()
{
    if (!m_pressureAxis) return;
    
    if (m_autoScale && !m_dataQueue.isEmpty()) {
        double minVal = std::numeric_limits<double>::max();
        double maxVal = std::numeric_limits<double>::lowest();
        
        for (const auto& point : m_dataQueue) {
            minVal = qMin(minVal, qMin(point.avlPressure, point.tankPressure));
            maxVal = qMax(maxVal, qMax(point.avlPressure, point.tankPressure));
        }
        
        // Add 10% padding
        double range = maxVal - minVal;
        double padding = range * 0.1;
        
        m_pressureAxis->setRange(qMax(0.0, minVal - padding), maxVal + padding);
    } else {
        m_pressureAxis->setRange(m_minPressure, m_maxPressure);
    }
}

void PressureChart::addThresholdLines()
{
    if (!m_chart || !m_showThresholds) return;
    
    // Remove existing threshold lines
    if (m_warningLine) {
        m_chart->removeSeries(m_warningLine);
        delete m_warningLine;
        m_warningLine = nullptr;
    }
    if (m_criticalLine) {
        m_chart->removeSeries(m_criticalLine);
        delete m_criticalLine;
        m_criticalLine = nullptr;
    }
    if (m_antiDetachmentLine) {
        m_chart->removeSeries(m_antiDetachmentLine);
        delete m_antiDetachmentLine;
        m_antiDetachmentLine = nullptr;
    }
    
    if (!m_showThresholds) return;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 startTime = currentTime - (m_timeRange * 1000);
    
    // Warning threshold line
    m_warningLine = new QtCharts::QLineSeries();
    m_warningLine->setName("Warning");
    m_warningLine->setPen(QPen(QColor(255, 152, 0), 2, Qt::DashLine));
    m_warningLine->append(startTime, m_warningThreshold);
    m_warningLine->append(currentTime, m_warningThreshold);
    m_chart->addSeries(m_warningLine);
    m_warningLine->attachAxis(m_timeAxis);
    m_warningLine->attachAxis(m_pressureAxis);
    
    // Critical threshold line
    m_criticalLine = new QtCharts::QLineSeries();
    m_criticalLine->setName("Critical");
    m_criticalLine->setPen(QPen(QColor(244, 67, 54), 2, Qt::DashLine));
    m_criticalLine->append(startTime, m_criticalThreshold);
    m_criticalLine->append(currentTime, m_criticalThreshold);
    m_chart->addSeries(m_criticalLine);
    m_criticalLine->attachAxis(m_timeAxis);
    m_criticalLine->attachAxis(m_pressureAxis);
    
    // Anti-detachment threshold line
    m_antiDetachmentLine = new QtCharts::QLineSeries();
    m_antiDetachmentLine->setName("Anti-detachment");
    m_antiDetachmentLine->setPen(QPen(QColor(156, 39, 176), 2, Qt::DotLine));
    m_antiDetachmentLine->append(startTime, m_antiDetachmentThreshold);
    m_antiDetachmentLine->append(currentTime, m_antiDetachmentThreshold);
    m_chart->addSeries(m_antiDetachmentLine);
    m_antiDetachmentLine->attachAxis(m_timeAxis);
    m_antiDetachmentLine->attachAxis(m_pressureAxis);
}

void PressureChart::removeOldData()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 cutoffTime = currentTime - (m_timeRange * 1000);
    
    while (!m_dataQueue.isEmpty() && m_dataQueue.first().timestamp < cutoffTime) {
        m_dataQueue.dequeue();
    }
    
    // Also limit by max data points
    while (m_dataQueue.size() > m_maxDataPoints) {
        m_dataQueue.dequeue();
    }
}

void PressureChart::onTimeRangeChanged()
{
    TimeRange newRange = static_cast<TimeRange>(m_timeRangeCombo->currentData().toInt());
    setTimeRange(newRange);
}

void PressureChart::onResetZoomClicked()
{
    resetZoom();
}

void PressureChart::onPauseClicked()
{
    pauseUpdates(!m_updatesPaused);
}

void PressureChart::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                   "Export Pressure Data", 
                                                   QString("pressure_data_%1.csv")
                                                   .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                   "CSV Files (*.csv)");
    
    if (!fileName.isEmpty()) {
        if (exportToCSV(fileName)) {
            QMessageBox::information(this, "Export Complete", 
                                   QString("Data exported successfully to:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Export Failed", "Failed to export data to file.");
        }
    }
}

bool PressureChart::exportToCSV(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    
    // Write header
    out << "Timestamp,DateTime,AVL_Pressure_mmHg,Tank_Pressure_mmHg\n";
    
    // Write data
    for (const auto& point : m_dataQueue) {
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(point.timestamp);
        out << point.timestamp << ","
            << dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") << ","
            << QString::number(point.avlPressure, 'f', 2) << ","
            << QString::number(point.tankPressure, 'f', 2) << "\n";
    }
    
    return true;
}
