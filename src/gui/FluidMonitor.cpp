#include "FluidMonitor.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/FluidSensor.h"
#include <QDebug>
#include <QGroupBox>
#include <QGridLayout>

FluidMonitor::FluidMonitor(HardwareManager* hardware, QWidget *parent)
    : QWidget(parent)
    , m_hardware(hardware)
    , m_fluidSensor(hardware ? hardware->getFluidSensor() : nullptr)
    , m_mainLayout(nullptr)
    , m_chart(nullptr)
    , m_chartView(nullptr)
    , m_volumeSeries(nullptr)
    , m_flowSeries(nullptr)
    , m_timeAxis(nullptr)
    , m_volumeAxis(nullptr)
    , m_reservoirCapacity(DEFAULT_CAPACITY)
    , m_overflowWarningMl(DEFAULT_WARNING)
    , m_chartTimeRangeSeconds(DEFAULT_CHART_TIME_RANGE)
    , m_showOrgasmMarkers(true)
    , m_updatesPaused(false)
    , m_currentVolumeMl(0.0)
    , m_cumulativeVolumeMl(0.0)
    , m_flowRateMlPerMin(0.0)
    , m_lubricationMl(0.0)
    , m_orgasmicMl(0.0)
    , m_orgasmCount(0)
    , m_chartUpdateTimer(new QTimer(this))
{
    setupUI();
    
    // Connect to fluid sensor signals
    if (m_fluidSensor) {
        connect(m_fluidSensor, &FluidSensor::volumeUpdated,
                this, &FluidMonitor::onVolumeUpdated);
        connect(m_fluidSensor, &FluidSensor::flowRateUpdated,
                this, &FluidMonitor::onFlowRateUpdated);
        connect(m_fluidSensor, &FluidSensor::orgasmicBurstDetected,
                this, &FluidMonitor::onOrgasmBurstDetected);
        connect(m_fluidSensor, &FluidSensor::overflowWarning,
                this, &FluidMonitor::onOverflowWarning);
    }
    
    // Start chart update timer
    connect(m_chartUpdateTimer, &QTimer::timeout, this, &FluidMonitor::updateChart);
    m_chartUpdateTimer->start(CHART_UPDATE_INTERVAL);
    m_sessionTimer.start();
}

FluidMonitor::~FluidMonitor()
{
    m_chartUpdateTimer->stop();
}

void FluidMonitor::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    setupVolumeDisplays();
    setupChart();
    setupStatistics();
    setupControls();
    
    setLayout(m_mainLayout);
}

void FluidMonitor::setupVolumeDisplays()
{
    m_displayFrame = new QFrame(this);
    m_displayFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    
    QGridLayout* layout = new QGridLayout(m_displayFrame);
    
    // Current volume
    QLabel* currentLabel = new QLabel("Current:", m_displayFrame);
    currentLabel->setStyleSheet("font-weight: bold;");
    m_currentVolumeLabel = new QLabel("0.0 mL", m_displayFrame);
    m_currentVolumeLabel->setStyleSheet("font-size: 24px; color: #2196F3;");
    
    // Cumulative volume
    QLabel* cumulativeLabel = new QLabel("Session Total:", m_displayFrame);
    cumulativeLabel->setStyleSheet("font-weight: bold;");
    m_cumulativeVolumeLabel = new QLabel("0.0 mL", m_displayFrame);
    m_cumulativeVolumeLabel->setStyleSheet("font-size: 24px; color: #4CAF50;");
    
    // Flow rate
    QLabel* flowLabel = new QLabel("Flow Rate:", m_displayFrame);
    flowLabel->setStyleSheet("font-weight: bold;");
    m_flowRateLabel = new QLabel("0.0 mL/min", m_displayFrame);
    m_flowRateLabel->setStyleSheet("font-size: 18px; color: #FF9800;");
    
    // Reservoir level
    QLabel* reservoirLabel = new QLabel("Reservoir:", m_displayFrame);
    reservoirLabel->setStyleSheet("font-weight: bold;");
    m_reservoirBar = new QProgressBar(m_displayFrame);
    m_reservoirBar->setRange(0, 100);
    m_reservoirBar->setValue(0);
    m_reservoirBar->setTextVisible(true);
    m_reservoirBar->setFormat("%v%");
    m_reservoirStatusLabel = new QLabel("Empty", m_displayFrame);
    
    layout->addWidget(currentLabel, 0, 0);
    layout->addWidget(m_currentVolumeLabel, 0, 1);
    layout->addWidget(cumulativeLabel, 0, 2);
    layout->addWidget(m_cumulativeVolumeLabel, 0, 3);
    layout->addWidget(flowLabel, 1, 0);
    layout->addWidget(m_flowRateLabel, 1, 1);
    layout->addWidget(reservoirLabel, 1, 2);
    layout->addWidget(m_reservoirBar, 1, 3);
    layout->addWidget(m_reservoirStatusLabel, 1, 4);
    
    m_mainLayout->addWidget(m_displayFrame);
}

void FluidMonitor::setupChart()
{
    m_chartFrame = new QFrame(this);
    m_chartFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    
    QVBoxLayout* layout = new QVBoxLayout(m_chartFrame);
    
    // Create chart
    m_chart = new QChart();
    m_chart->setTitle("Fluid Volume Over Time");
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->legend()->setVisible(true);
    
    // Volume series
    m_volumeSeries = new QLineSeries();
    m_volumeSeries->setName("Volume (mL)");
    m_volumeSeries->setColor(QColor("#2196F3"));
    m_chart->addSeries(m_volumeSeries);
    
    // Flow rate series (secondary)
    m_flowSeries = new QLineSeries();
    m_flowSeries->setName("Flow Rate (mL/min)");
    m_flowSeries->setColor(QColor("#FF9800"));
    m_chart->addSeries(m_flowSeries);
    
    // Time axis
    m_timeAxis = new QValueAxis();
    m_timeAxis->setTitleText("Time (min)");
    m_timeAxis->setRange(0, m_chartTimeRangeSeconds / 60.0);
    m_chart->addAxis(m_timeAxis, Qt::AlignBottom);
    m_volumeSeries->attachAxis(m_timeAxis);
    m_flowSeries->attachAxis(m_timeAxis);
    
    // Volume axis
    m_volumeAxis = new QValueAxis();
    m_volumeAxis->setTitleText("Volume (mL)");
    m_volumeAxis->setRange(0, m_reservoirCapacity);
    m_chart->addAxis(m_volumeAxis, Qt::AlignLeft);
    m_volumeSeries->attachAxis(m_volumeAxis);
    m_flowSeries->attachAxis(m_volumeAxis);

    // Chart view
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(200);

    layout->addWidget(m_chartView);
    m_mainLayout->addWidget(m_chartFrame);
}

void FluidMonitor::setupStatistics()
{
    m_statsFrame = new QFrame(this);
    m_statsFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    QHBoxLayout* layout = new QHBoxLayout(m_statsFrame);

    // Lubrication volume
    QGroupBox* lubGroup = new QGroupBox("Lubrication", m_statsFrame);
    QVBoxLayout* lubLayout = new QVBoxLayout(lubGroup);
    m_lubricationLabel = new QLabel("0.0 mL", lubGroup);
    m_lubricationLabel->setStyleSheet("font-size: 18px; color: #9C27B0;");
    lubLayout->addWidget(m_lubricationLabel);

    // Orgasmic fluid volume
    QGroupBox* orgGroup = new QGroupBox("Orgasmic Fluid", m_statsFrame);
    QVBoxLayout* orgLayout = new QVBoxLayout(orgGroup);
    m_orgasmicLabel = new QLabel("0.0 mL", orgGroup);
    m_orgasmicLabel->setStyleSheet("font-size: 18px; color: #E91E63;");
    orgLayout->addWidget(m_orgasmicLabel);

    // Orgasm count
    QGroupBox* countGroup = new QGroupBox("Orgasms", m_statsFrame);
    QVBoxLayout* countLayout = new QVBoxLayout(countGroup);
    m_orgasmCountLabel = new QLabel("0", countGroup);
    m_orgasmCountLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #F44336;");
    countLayout->addWidget(m_orgasmCountLabel);

    // Average per orgasm
    QGroupBox* avgGroup = new QGroupBox("Avg/Orgasm", m_statsFrame);
    QVBoxLayout* avgLayout = new QVBoxLayout(avgGroup);
    m_avgPerOrgasmLabel = new QLabel("-- mL", avgGroup);
    m_avgPerOrgasmLabel->setStyleSheet("font-size: 16px;");
    avgLayout->addWidget(m_avgPerOrgasmLabel);

    layout->addWidget(lubGroup);
    layout->addWidget(orgGroup);
    layout->addWidget(countGroup);
    layout->addWidget(avgGroup);

    m_mainLayout->addWidget(m_statsFrame);
}

void FluidMonitor::setupControls()
{
    m_controlFrame = new QFrame(this);
    QHBoxLayout* layout = new QHBoxLayout(m_controlFrame);

    m_tareButton = new QPushButton("Tare", m_controlFrame);
    m_tareButton->setToolTip("Zero the sensor with empty reservoir");
    connect(m_tareButton, &QPushButton::clicked, this, &FluidMonitor::tareReservoir);

    m_calibrateButton = new QPushButton("Calibrate", m_controlFrame);
    m_calibrateButton->setToolTip("Calibrate with known weight");
    connect(m_calibrateButton, &QPushButton::clicked, this, &FluidMonitor::calibrateSensor);

    m_resetButton = new QPushButton("Reset Session", m_controlFrame);
    m_resetButton->setToolTip("Reset all session statistics");
    connect(m_resetButton, &QPushButton::clicked, this, &FluidMonitor::resetSession);

    layout->addWidget(m_tareButton);
    layout->addWidget(m_calibrateButton);
    layout->addStretch();
    layout->addWidget(m_resetButton);

    m_mainLayout->addWidget(m_controlFrame);
}

// ============================================================================
// Update Methods
// ============================================================================

void FluidMonitor::updateVolume(double currentMl, double cumulativeMl)
{
    m_currentVolumeMl = currentMl;
    m_cumulativeVolumeMl = cumulativeMl;
    updateVolumeDisplay();
    updateReservoirLevel();
}

void FluidMonitor::updateFlowRate(double mlPerMin)
{
    m_flowRateMlPerMin = mlPerMin;
    updateFlowDisplay();
}

void FluidMonitor::updateVolumeDisplay()
{
    m_currentVolumeLabel->setText(QString("%1 mL").arg(m_currentVolumeMl, 0, 'f', 1));
    m_cumulativeVolumeLabel->setText(QString("%1 mL").arg(m_cumulativeVolumeMl, 0, 'f', 1));
}

void FluidMonitor::updateFlowDisplay()
{
    m_flowRateLabel->setText(QString("%1 mL/min").arg(m_flowRateMlPerMin, 0, 'f', 2));

    // Color based on flow rate
    if (m_flowRateMlPerMin > 5.0) {
        m_flowRateLabel->setStyleSheet("font-size: 18px; color: #F44336;");  // High = red
    } else if (m_flowRateMlPerMin > 1.0) {
        m_flowRateLabel->setStyleSheet("font-size: 18px; color: #FF9800;");  // Medium = orange
    } else {
        m_flowRateLabel->setStyleSheet("font-size: 18px; color: #4CAF50;");  // Low = green
    }
}

void FluidMonitor::updateReservoirLevel()
{
    double percent = (m_currentVolumeMl / m_reservoirCapacity) * 100.0;
    m_reservoirBar->setValue(static_cast<int>(percent));

    if (percent >= 90) {
        m_reservoirBar->setStyleSheet("QProgressBar::chunk { background-color: #F44336; }");
        m_reservoirStatusLabel->setText("FULL!");
        m_reservoirStatusLabel->setStyleSheet("color: #F44336; font-weight: bold;");
    } else if (percent >= 75) {
        m_reservoirBar->setStyleSheet("QProgressBar::chunk { background-color: #FF9800; }");
        m_reservoirStatusLabel->setText("High");
        m_reservoirStatusLabel->setStyleSheet("color: #FF9800;");
    } else if (percent >= 25) {
        m_reservoirBar->setStyleSheet("QProgressBar::chunk { background-color: #2196F3; }");
        m_reservoirStatusLabel->setText("OK");
        m_reservoirStatusLabel->setStyleSheet("color: #2196F3;");
    } else {
        m_reservoirBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
        m_reservoirStatusLabel->setText("Low");
        m_reservoirStatusLabel->setStyleSheet("color: #4CAF50;");
    }
}

void FluidMonitor::updateChart()
{
    if (m_updatesPaused) return;

    qint64 elapsed = m_sessionTimer.elapsed();
    double timeMinutes = elapsed / 60000.0;

    // Add current data point
    addDataPoint(m_currentVolumeMl);

    // Update time axis range (scrolling window)
    double rangeMinutes = m_chartTimeRangeSeconds / 60.0;
    if (timeMinutes > rangeMinutes) {
        m_timeAxis->setRange(timeMinutes - rangeMinutes, timeMinutes);
    }
}

void FluidMonitor::addDataPoint(double volumeMl)
{
    qint64 elapsed = m_sessionTimer.elapsed();
    double timeMinutes = elapsed / 60000.0;

    m_volumeSeries->append(timeMinutes, volumeMl);
    m_flowSeries->append(timeMinutes, m_flowRateMlPerMin);

    // Limit data points
    while (m_volumeSeries->count() > MAX_DATA_POINTS) {
        m_volumeSeries->remove(0);
        m_flowSeries->remove(0);
    }

    // Auto-scale volume axis if needed
    if (volumeMl > m_volumeAxis->max() * 0.9) {
        m_volumeAxis->setMax(volumeMl * 1.2);
    }
}

void FluidMonitor::addOrgasmMarker(int orgasmNumber, double volumeMl)
{
    if (!m_showOrgasmMarkers) return;

    qint64 elapsed = m_sessionTimer.elapsed();
    m_orgasmMarkers.append({elapsed, orgasmNumber});

    // Update statistics
    m_orgasmCount = orgasmNumber;
    m_orgasmCountLabel->setText(QString::number(m_orgasmCount));

    if (m_orgasmCount > 0) {
        double avg = m_orgasmicMl / m_orgasmCount;
        m_avgPerOrgasmLabel->setText(QString("%1 mL").arg(avg, 0, 'f', 1));
    }
}

// ============================================================================
// Slot Handlers
// ============================================================================

void FluidMonitor::onVolumeUpdated(double currentMl, double cumulativeMl)
{
    updateVolume(currentMl, cumulativeMl);
}

void FluidMonitor::onFlowRateUpdated(double mlPerMin, double /*mlPerSec*/)
{
    updateFlowRate(mlPerMin);
}

void FluidMonitor::onOrgasmBurstDetected(double volumeMl, double /*peakRate*/, int orgasmNum)
{
    m_orgasmicMl += volumeMl;
    m_orgasmicLabel->setText(QString("%1 mL").arg(m_orgasmicMl, 0, 'f', 1));
    addOrgasmMarker(orgasmNum, volumeMl);
}

void FluidMonitor::onOverflowWarning(double volumeMl, double /*capacityMl*/)
{
    emit overflowWarning(volumeMl);

    // Flash warning
    m_reservoirStatusLabel->setText("⚠ OVERFLOW!");
    m_reservoirStatusLabel->setStyleSheet("color: #F44336; font-weight: bold; font-size: 14px;");
}

// ============================================================================
// Control Methods
// ============================================================================

void FluidMonitor::resetSession()
{
    m_currentVolumeMl = 0.0;
    m_cumulativeVolumeMl = 0.0;
    m_flowRateMlPerMin = 0.0;
    m_lubricationMl = 0.0;
    m_orgasmicMl = 0.0;
    m_orgasmCount = 0;

    m_volumeSeries->clear();
    m_flowSeries->clear();
    m_orgasmMarkers.clear();

    m_sessionTimer.restart();
    m_timeAxis->setRange(0, m_chartTimeRangeSeconds / 60.0);

    updateVolumeDisplay();
    updateFlowDisplay();
    updateReservoirLevel();

    m_lubricationLabel->setText("0.0 mL");
    m_orgasmicLabel->setText("0.0 mL");
    m_orgasmCountLabel->setText("0");
    m_avgPerOrgasmLabel->setText("-- mL");

    if (m_fluidSensor) {
        m_fluidSensor->resetSession();
    }

    emit sessionReset();
}

void FluidMonitor::pauseUpdates(bool pause)
{
    m_updatesPaused = pause;
}

void FluidMonitor::tareReservoir()
{
    if (m_fluidSensor) {
        m_fluidSensor->tare();
    }
}

void FluidMonitor::calibrateSensor()
{
    // TODO: Show calibration dialog
    // For now, calibrate with 100g known weight
    if (m_fluidSensor) {
        m_fluidSensor->calibrate(100.0);
    }
}

void FluidMonitor::setReservoirCapacity(double capacityMl)
{
    m_reservoirCapacity = capacityMl;
    m_volumeAxis->setMax(capacityMl);
}

void FluidMonitor::setOverflowWarning(double warningMl)
{
    m_overflowWarningMl = warningMl;
    if (m_fluidSensor) {
        m_fluidSensor->setOverflowWarning(warningMl);
    }
}

void FluidMonitor::setChartTimeRange(int seconds)
{
    m_chartTimeRangeSeconds = seconds;
    m_timeAxis->setRange(0, seconds / 60.0);
}

void FluidMonitor::setShowOrgasmMarkers(bool show)
{
    m_showOrgasmMarkers = show;
}