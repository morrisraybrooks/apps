#include "ArousalMonitor.h"
#include "../VacuumController.h"
#include "../control/OrgasmControlAlgorithm.h"
#include "styles/ModernMedicalStyle.h"
#include <QDateTime>
#include <QDebug>

ArousalMonitor::ArousalMonitor(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_algorithm(nullptr)
    , m_mainLayout(new QVBoxLayout(this))
    , m_edgeThreshold(0.70)
    , m_orgasmThreshold(0.85)
    , m_recoveryThreshold(0.45)
    , m_milkingZoneLower(0.75)
    , m_milkingZoneUpper(0.90)
    , m_chartTimeRangeSeconds(DEFAULT_CHART_TIME_RANGE)
    , m_showGrid(true)
    , m_showThresholdZones(true)
    , m_updatesPaused(false)
    , m_currentArousal(0.0)
    , m_currentState(0)
{
    if (m_controller) {
        m_algorithm = m_controller->getOrgasmControlAlgorithm();
    }
    
    setupUI();
    
    // Connect to algorithm signals
    if (m_algorithm) {
        connect(m_algorithm, &OrgasmControlAlgorithm::arousalLevelChanged,
                this, &ArousalMonitor::onArousalLevelChanged);
        connect(m_algorithm, &OrgasmControlAlgorithm::stateChanged,
                this, [this](OrgasmControlAlgorithm::ControlState state) {
            onStateChanged(static_cast<int>(state));
        });
        
        // Initialize thresholds from algorithm
        m_edgeThreshold = m_algorithm->edgeThreshold();
        m_orgasmThreshold = m_algorithm->orgasmThreshold();
        m_recoveryThreshold = m_algorithm->recoveryThreshold();
        
        // Connect threshold change signals
        connect(m_algorithm, &OrgasmControlAlgorithm::edgeThresholdChanged,
                this, [this](double threshold) {
            m_edgeThreshold = threshold;
            updateThresholdZones();
        });
        connect(m_algorithm, &OrgasmControlAlgorithm::orgasmThresholdChanged,
                this, [this](double threshold) {
            m_orgasmThreshold = threshold;
            updateThresholdZones();
        });
        connect(m_algorithm, &OrgasmControlAlgorithm::recoveryThresholdChanged,
                this, [this](double threshold) {
            m_recoveryThreshold = threshold;
            updateThresholdZones();
        });
    }
    
    // Setup chart update timer
    m_chartUpdateTimer = new QTimer(this);
    connect(m_chartUpdateTimer, &QTimer::timeout, this, &ArousalMonitor::updateChart);
    m_chartUpdateTimer->start(CHART_UPDATE_INTERVAL);
}

ArousalMonitor::~ArousalMonitor()
{
    if (m_chartUpdateTimer) {
        m_chartUpdateTimer->stop();
    }
}

void ArousalMonitor::setupUI()
{
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    setupArousalDisplay();
    setupThresholdIndicators();
    setupChart();
    setupStateIndicator();
}

void ArousalMonitor::setupArousalDisplay()
{
    m_displayFrame = new QFrame(this);
    m_displayFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_displayFrame->setStyleSheet(ModernMedicalStyle::getFrameStyle());
    
    QVBoxLayout* displayLayout = new QVBoxLayout(m_displayFrame);
    
    // Title
    QLabel* titleLabel = new QLabel(tr("Arousal Level"), m_displayFrame);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #E91E63;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Main arousal value display
    m_arousalValueLabel = new QLabel("0.00", m_displayFrame);
    m_arousalValueLabel->setStyleSheet("font-size: 72pt; font-weight: bold; color: #E91E63;");
    m_arousalValueLabel->setAlignment(Qt::AlignCenter);
    
    m_arousalPercentLabel = new QLabel("0%", m_displayFrame);
    m_arousalPercentLabel->setStyleSheet("font-size: 24pt; color: #666;");
    m_arousalPercentLabel->setAlignment(Qt::AlignCenter);
    
    // Progress bar with gradient
    m_arousalProgressBar = new QProgressBar(m_displayFrame);
    m_arousalProgressBar->setRange(0, 100);
    m_arousalProgressBar->setValue(0);
    m_arousalProgressBar->setTextVisible(false);
    m_arousalProgressBar->setMinimumHeight(40);
    m_arousalProgressBar->setStyleSheet(
        "QProgressBar {"
        "  border: 2px solid #ccc;"
        "  border-radius: 10px;"
        "  background-color: #f0f0f0;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #4CAF50, stop:0.5 #FFC107, stop:0.7 #FF9800, stop:1 #F44336);"
        "  border-radius: 8px;"
        "}"
    );
    
    displayLayout->addWidget(titleLabel);
    displayLayout->addWidget(m_arousalValueLabel);
    displayLayout->addWidget(m_arousalPercentLabel);
    displayLayout->addWidget(m_arousalProgressBar);
    
    m_mainLayout->addWidget(m_displayFrame);
}

void ArousalMonitor::setupThresholdIndicators()
{
    m_thresholdFrame = new QFrame(this);
    m_thresholdFrame->setFrameStyle(QFrame::StyledPanel);
    
    QHBoxLayout* thresholdLayout = new QHBoxLayout(m_thresholdFrame);
    
    // Recovery threshold
    QVBoxLayout* recoveryLayout = new QVBoxLayout();
    m_recoveryThresholdLabel = new QLabel(QString("Recovery\n%1").arg(m_recoveryThreshold, 0, 'f', 2));
    m_recoveryThresholdLabel->setAlignment(Qt::AlignCenter);
    m_recoveryThresholdLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
    recoveryLayout->addWidget(m_recoveryThresholdLabel);

    // Edge threshold
    QVBoxLayout* edgeLayout = new QVBoxLayout();
    m_edgeThresholdLabel = new QLabel(QString("Edge\n%1").arg(m_edgeThreshold, 0, 'f', 2));
    m_edgeThresholdLabel->setAlignment(Qt::AlignCenter);
    m_edgeThresholdLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
    edgeLayout->addWidget(m_edgeThresholdLabel);

    // Orgasm threshold
    QVBoxLayout* orgasmLayout = new QVBoxLayout();
    m_orgasmThresholdLabel = new QLabel(QString("Orgasm\n%1").arg(m_orgasmThreshold, 0, 'f', 2));
    m_orgasmThresholdLabel->setAlignment(Qt::AlignCenter);
    m_orgasmThresholdLabel->setStyleSheet("color: #F44336; font-weight: bold;");
    orgasmLayout->addWidget(m_orgasmThresholdLabel);

    thresholdLayout->addLayout(recoveryLayout);
    thresholdLayout->addLayout(edgeLayout);
    thresholdLayout->addLayout(orgasmLayout);

    m_mainLayout->addWidget(m_thresholdFrame);
}

void ArousalMonitor::setupChart()
{
    m_chartFrame = new QFrame(this);
    m_chartFrame->setFrameStyle(QFrame::StyledPanel);
    m_chartFrame->setMinimumHeight(200);

    QVBoxLayout* chartLayout = new QVBoxLayout(m_chartFrame);

    // Create chart
    m_chart = new QChart();
    m_chart->setTitle("Arousal History");
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->legend()->hide();

    // Create arousal series
    m_arousalSeries = new QLineSeries();
    m_arousalSeries->setName("Arousal");
    m_arousalSeries->setColor(QColor("#E91E63"));
    QPen pen = m_arousalSeries->pen();
    pen.setWidth(3);
    m_arousalSeries->setPen(pen);

    // Create axes
    m_timeAxis = new QValueAxis();
    m_timeAxis->setTitleText("Time (s)");
    m_timeAxis->setRange(0, m_chartTimeRangeSeconds);
    m_timeAxis->setLabelFormat("%d");

    m_arousalAxis = new QValueAxis();
    m_arousalAxis->setTitleText("Arousal");
    m_arousalAxis->setRange(0.0, 1.0);
    m_arousalAxis->setLabelFormat("%.2f");
    m_arousalAxis->setTickCount(6);

    m_chart->addSeries(m_arousalSeries);
    m_chart->addAxis(m_timeAxis, Qt::AlignBottom);
    m_chart->addAxis(m_arousalAxis, Qt::AlignLeft);
    m_arousalSeries->attachAxis(m_timeAxis);
    m_arousalSeries->attachAxis(m_arousalAxis);

    // Create chart view
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    chartLayout->addWidget(m_chartView);
    m_mainLayout->addWidget(m_chartFrame, 1);  // Give chart stretch priority
}

void ArousalMonitor::setupStateIndicator()
{
    QFrame* stateFrame = new QFrame(this);
    stateFrame->setFrameStyle(QFrame::StyledPanel);

    QHBoxLayout* stateLayout = new QHBoxLayout(stateFrame);

    QLabel* stateTitle = new QLabel(tr("State:"), stateFrame);
    stateTitle->setStyleSheet("font-weight: bold;");

    m_stateLabel = new QLabel(tr("IDLE"), stateFrame);
    m_stateLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #666; padding: 5px 15px; background: #f0f0f0; border-radius: 5px;");

    QLabel* modeTitle = new QLabel(tr("Mode:"), stateFrame);
    modeTitle->setStyleSheet("font-weight: bold;");

    m_modeLabel = new QLabel(tr("MANUAL"), stateFrame);
    m_modeLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #2196F3; padding: 5px 15px; background: #E3F2FD; border-radius: 5px;");

    stateLayout->addWidget(stateTitle);
    stateLayout->addWidget(m_stateLabel);
    stateLayout->addStretch();
    stateLayout->addWidget(modeTitle);
    stateLayout->addWidget(m_modeLabel);

    m_mainLayout->addWidget(stateFrame);
}

void ArousalMonitor::updateArousalLevel(double arousalLevel)
{
    if (m_updatesPaused) return;

    m_currentArousal = qBound(0.0, arousalLevel, 1.0);
    updateArousalDisplay(m_currentArousal);
    addDataPoint(m_currentArousal);

    // Emit signals based on thresholds
    if (m_currentArousal >= m_orgasmThreshold) {
        emit orgasmDetected(m_currentArousal);
    } else if (m_currentArousal >= m_edgeThreshold) {
        emit edgeApproaching(m_currentArousal);
    } else if (m_currentArousal <= m_recoveryThreshold) {
        emit recoveryComplete(m_currentArousal);
    }
}

void ArousalMonitor::updateControlState(int state)
{
    m_currentState = state;
    m_stateLabel->setText(stateToString(state));
    m_stateLabel->setStyleSheet(QString("font-size: 16pt; font-weight: bold; color: white; padding: 5px 15px; background: %1; border-radius: 5px;").arg(stateToColor(state).name()));
}

void ArousalMonitor::updateArousalDisplay(double arousalLevel)
{
    m_arousalValueLabel->setText(QString::number(arousalLevel, 'f', 2));
    m_arousalPercentLabel->setText(QString("%1%").arg(static_cast<int>(arousalLevel * 100)));
    m_arousalProgressBar->setValue(static_cast<int>(arousalLevel * 100));

    // Update color based on level
    QString color;
    if (arousalLevel >= m_orgasmThreshold) {
        color = "#F44336";  // Red - orgasm zone
    } else if (arousalLevel >= m_edgeThreshold) {
        color = "#FF9800";  // Orange - edge zone
    } else if (arousalLevel >= m_recoveryThreshold) {
        color = "#FFC107";  // Yellow - building
    } else {
        color = "#4CAF50";  // Green - recovery/low
    }

    m_arousalValueLabel->setStyleSheet(QString("font-size: 72pt; font-weight: bold; color: %1;").arg(color));
}

void ArousalMonitor::updateThresholdZones()
{
    m_recoveryThresholdLabel->setText(QString("Recovery\n%1").arg(m_recoveryThreshold, 0, 'f', 2));
    m_edgeThresholdLabel->setText(QString("Edge\n%1").arg(m_edgeThreshold, 0, 'f', 2));
    m_orgasmThresholdLabel->setText(QString("Orgasm\n%1").arg(m_orgasmThreshold, 0, 'f', 2));
}

void ArousalMonitor::addDataPoint(double arousalLevel)
{
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    m_arousalData.enqueue(qMakePair(timestamp, arousalLevel));

    // Remove old data points
    while (m_arousalData.size() > MAX_DATA_POINTS) {
        m_arousalData.dequeue();
    }
}

void ArousalMonitor::updateChart()
{
    if (m_updatesPaused || m_arousalData.isEmpty()) return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 startTime = now - (m_chartTimeRangeSeconds * 1000);

    m_arousalSeries->clear();

    for (const auto& point : m_arousalData) {
        if (point.first >= startTime) {
            double timeSeconds = (point.first - startTime) / 1000.0;
            m_arousalSeries->append(timeSeconds, point.second);
        }
    }
}

void ArousalMonitor::onArousalLevelChanged(double level)
{
    updateArousalLevel(level);
}

void ArousalMonitor::onStateChanged(int state)
{
    updateControlState(state);
}

void ArousalMonitor::setChartTimeRange(int seconds)
{
    m_chartTimeRangeSeconds = seconds;
    m_timeAxis->setRange(0, seconds);
}

void ArousalMonitor::setShowGrid(bool show)
{
    m_showGrid = show;
    m_timeAxis->setGridLineVisible(show);
    m_arousalAxis->setGridLineVisible(show);
}

void ArousalMonitor::setShowThresholdZones(bool show)
{
    m_showThresholdZones = show;
    // Could add/remove zone series here
}

void ArousalMonitor::resetChart()
{
    m_arousalData.clear();
    m_arousalSeries->clear();
}

void ArousalMonitor::pauseUpdates(bool pause)
{
    m_updatesPaused = pause;
}

QString ArousalMonitor::stateToString(int state) const
{
    switch (state) {
        case 0: return tr("IDLE");
        case 1: return tr("BUILDING");
        case 2: return tr("EDGING");
        case 3: return tr("BACKING OFF");
        case 4: return tr("RECOVERY");
        case 5: return tr("ORGASM");
        case 6: return tr("POST-ORGASM");
        case 7: return tr("MILKING");
        case 8: return tr("DANGER ZONE");
        case 9: return tr("EMERGENCY");
        default: return tr("UNKNOWN");
    }
}

QColor ArousalMonitor::stateToColor(int state) const
{
    switch (state) {
        case 0: return QColor("#9E9E9E");  // Grey - IDLE
        case 1: return QColor("#4CAF50");  // Green - BUILDING
        case 2: return QColor("#FF9800");  // Orange - EDGING
        case 3: return QColor("#2196F3");  // Blue - BACKING OFF
        case 4: return QColor("#00BCD4");  // Cyan - RECOVERY
        case 5: return QColor("#E91E63");  // Pink - ORGASM
        case 6: return QColor("#9C27B0");  // Purple - POST-ORGASM
        case 7: return QColor("#795548");  // Brown - MILKING
        case 8: return QColor("#F44336");  // Red - DANGER ZONE
        case 9: return QColor("#D32F2F");  // Dark Red - EMERGENCY
        default: return QColor("#666666");
    }
}

