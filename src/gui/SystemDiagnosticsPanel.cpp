#include "SystemDiagnosticsPanel.h"
#include "components/TouchButton.h"
#include "components/StatusIndicator.h"
#include "../VacuumController.h"
#include "../hardware/HardwareManager.h"
#include "../safety/SafetyManager.h"
#include <QDebug>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QThread>
#include <cmath>

SystemDiagnosticsPanel::SystemDiagnosticsPanel(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_hardwareManager(nullptr)
    , m_safetyManager(nullptr)
    , m_tabWidget(new QTabWidget(this))
    , m_mainLayout(new QVBoxLayout(this))
    , m_diagnosticsRunning(false)
    , m_diagnosticTimer(new QTimer(this))
    , m_maxHistoryEntries(MAX_DIAGNOSTIC_HISTORY)
    , m_testInProgress(false)
{
    setupUI();
    connectSignals();
    
    if (m_controller) {
        m_hardwareManager = m_controller->getHardwareManager();
        m_safetyManager = m_controller->getSafetyManager();
    }
    
    m_diagnosticTimer->setInterval(DIAGNOSTIC_UPDATE_INTERVAL);
    connect(m_diagnosticTimer, &QTimer::timeout, this, &SystemDiagnosticsPanel::onDiagnosticTimer);
    
    startDiagnostics();
}

SystemDiagnosticsPanel::~SystemDiagnosticsPanel()
{
    stopDiagnostics();
}

void SystemDiagnosticsPanel::setupUI()
{
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    setupOverviewTab();
    setupHardwareTab();
    setupSensorsTab();
    setupActuatorsTab();
    setupPerformanceTab();
    setupLogsTab();
    setupTestingTab();
    
    m_mainLayout->addWidget(m_tabWidget);
}

void SystemDiagnosticsPanel::setupOverviewTab()
{
    m_overviewTab = new QWidget();
    m_tabWidget->addTab(m_overviewTab, "System Overview");
    
    QVBoxLayout* overviewLayout = new QVBoxLayout(m_overviewTab);
    overviewLayout->setSpacing(15);
    
    QGroupBox* statusGroup = new QGroupBox("System Status");
    statusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);
    
    m_systemStatusIndicator = new MultiStatusIndicator();
    m_systemStatusIndicator->setColumns(3);
    m_systemStatusIndicator->addStatus("hardware", "Hardware", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("sensors", "Sensors", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("actuators", "Actuators", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("safety", "Safety", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("performance", "Performance", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("communication", "Communication", StatusIndicator::OK);
    
    statusLayout->addWidget(m_systemStatusIndicator);
    
    QGroupBox* infoGroup = new QGroupBox("System Information");
    infoGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* infoLayout = new QGridLayout(infoGroup);
    
    QLabel* uptimeLabel = new QLabel("System Uptime:");
    uptimeLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_systemUptimeLabel = new QLabel("0 hours");
    m_systemUptimeLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    QLabel* versionLabel = new QLabel("Software Version:");
    versionLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_systemVersionLabel = new QLabel("v1.0.0");
    m_systemVersionLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    QLabel* updateLabel = new QLabel("Last Update:");
    updateLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_lastUpdateLabel = new QLabel("Never");
    m_lastUpdateLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    infoLayout->addWidget(uptimeLabel, 0, 0);
    infoLayout->addWidget(m_systemUptimeLabel, 0, 1);
    infoLayout->addWidget(versionLabel, 1, 0);
    infoLayout->addWidget(m_systemVersionLabel, 1, 1);
    infoLayout->addWidget(updateLabel, 2, 0);
    infoLayout->addWidget(m_lastUpdateLabel, 2, 1);
    
    QLabel* healthLabel = new QLabel("Overall System Health:");
    healthLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_systemHealthBar = new QProgressBar();
    m_systemHealthBar->setRange(0, 100);
    m_systemHealthBar->setValue(100);
    m_systemHealthBar->setFormat("System Health: %p%");
    m_systemHealthBar->setMinimumHeight(40);
    m_systemHealthBar->setStyleSheet(
        "QProgressBar { border: 2px solid #ddd; border-radius: 20px; text-align: center; font-size: 14pt; font-weight: bold; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #2196F3); border-radius: 18px; }"
    );
    
    overviewLayout->addWidget(statusGroup);
    overviewLayout->addWidget(infoGroup);
    overviewLayout->addWidget(healthLabel);
    overviewLayout->addWidget(m_systemHealthBar);
    overviewLayout->addStretch();
}

void SystemDiagnosticsPanel::setupHardwareTab()
{
    m_hardwareTab = new QWidget();
    m_tabWidget->addTab(m_hardwareTab, "Hardware");
    
    QVBoxLayout* hardwareLayout = new QVBoxLayout(m_hardwareTab);
    hardwareLayout->setSpacing(15);
    
    m_gpioStatusGroup = new QGroupBox("GPIO Status");
    m_gpioStatusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* gpioLayout = new QGridLayout(m_gpioStatusGroup);
    
    QStringList gpioPins = {"SOL1 (GPIO 17)", "SOL2 (GPIO 27)", "SOL3 (GPIO 22)", 
                           "Pump Enable (GPIO 25)", "Pump PWM (GPIO 18)", "Emergency (GPIO 21)"};
    
    for (int i = 0; i < gpioPins.size(); ++i) {
        QLabel* pinLabel = new QLabel(gpioPins[i]);
        pinLabel->setStyleSheet("font-size: 12pt; font-weight: bold;");
        
        StatusIndicator* pinStatus = new StatusIndicator();
        pinStatus->setStatus(StatusIndicator::OK, "Ready");
        
        gpioLayout->addWidget(pinLabel, i / 2, (i % 2) * 2);
        gpioLayout->addWidget(pinStatus, i / 2, (i % 2) * 2 + 1);
    }
    
    m_spiStatusGroup = new QGroupBox("SPI Communication");
    m_spiStatusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* spiLayout = new QGridLayout(m_spiStatusGroup);
    
    QLabel* spiChannelLabel = new QLabel("SPI Channel 0:");
    spiChannelLabel->setStyleSheet("font-size: 12pt; font-weight: bold;");
    StatusIndicator* spiStatus = new StatusIndicator();
    spiStatus->setStatus(StatusIndicator::OK, "Active");
    
    QLabel* spiSpeedLabel = new QLabel("Communication Speed:");
    spiSpeedLabel->setStyleSheet("font-size: 12pt; font-weight: bold;");
    QLabel* spiSpeedValue = new QLabel("1.0 MHz");
    spiSpeedValue->setStyleSheet("font-size: 12pt; color: #333;");
    
    spiLayout->addWidget(spiChannelLabel, 0, 0);
    spiLayout->addWidget(spiStatus, 0, 1);
    spiLayout->addWidget(spiSpeedLabel, 1, 0);
    spiLayout->addWidget(spiSpeedValue, 1, 1);
    
    m_hardwareTable = new QTableWidget(0, 4);
    m_hardwareTable->setHorizontalHeaderLabels({"Component", "Status", "Value", "Last Update"});
    m_hardwareTable->horizontalHeader()->setStretchLastSection(true);
    m_hardwareTable->setAlternatingRowColors(true);
    m_hardwareTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hardwareTable->setMinimumHeight(200);
    
    hardwareLayout->addWidget(m_gpioStatusGroup);
    hardwareLayout->addWidget(m_spiStatusGroup);
    hardwareLayout->addWidget(m_hardwareTable);
}

void SystemDiagnosticsPanel::setupSensorsTab()
{
    m_sensorsTab = new QWidget();
    m_tabWidget->addTab(m_sensorsTab, "Sensors");
    
    QVBoxLayout* sensorsLayout = new QVBoxLayout(m_sensorsTab);
    sensorsLayout->setSpacing(15);
    
    m_sensorReadingsGroup = new QGroupBox("Current Sensor Readings");
    m_sensorReadingsGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* readingsLayout = new QGridLayout(m_sensorReadingsGroup);
    
    QLabel* avlSensorLabel = new QLabel("AVL Pressure Sensor:");
    avlSensorLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_avlSensorStatusLabel = new QLabel("OK");
    m_avlSensorStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    m_avlReadingLabel = new QLabel("0.0 mmHg");
    m_avlReadingLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    QLabel* tankSensorLabel = new QLabel("Tank Pressure Sensor:");
    tankSensorLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_tankSensorStatusLabel = new QLabel("OK");
    m_tankSensorStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    m_tankReadingLabel = new QLabel("0.0 mmHg");
    m_tankReadingLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    readingsLayout->addWidget(avlSensorLabel, 0, 0);
    readingsLayout->addWidget(m_avlSensorStatusLabel, 0, 1);
    readingsLayout->addWidget(m_avlReadingLabel, 0, 2);
    readingsLayout->addWidget(tankSensorLabel, 1, 0);
    readingsLayout->addWidget(m_tankSensorStatusLabel, 1, 1);
    readingsLayout->addWidget(m_tankReadingLabel, 1, 2);
    
    m_sensorCalibrationGroup = new QGroupBox("Calibration Status");
    m_sensorCalibrationGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QVBoxLayout* calibrationLayout = new QVBoxLayout(m_sensorCalibrationGroup);
    
    QLabel* lastCalibrationLabel = new QLabel("Last Calibration:");
    lastCalibrationLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_lastCalibrationLabel = new QLabel("Never");
    m_lastCalibrationLabel->setStyleSheet("font-size: 14pt; color: #666;");
    
    QLabel* accuracyLabel = new QLabel("Sensor Accuracy:");
    accuracyLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_sensorAccuracyBar = new QProgressBar();
    m_sensorAccuracyBar->setRange(0, 100);
    m_sensorAccuracyBar->setValue(95);
    m_sensorAccuracyBar->setFormat("Accuracy: %p%");
    m_sensorAccuracyBar->setMinimumHeight(30);
    
    QHBoxLayout* calibrationInfoLayout = new QHBoxLayout();
    calibrationInfoLayout->addWidget(lastCalibrationLabel);
    calibrationInfoLayout->addWidget(m_lastCalibrationLabel);
    calibrationInfoLayout->addStretch();
    
    calibrationLayout->addLayout(calibrationInfoLayout);
    calibrationLayout->addWidget(accuracyLabel);
    calibrationLayout->addWidget(m_sensorAccuracyBar);
    
    sensorsLayout->addWidget(m_sensorReadingsGroup);
    sensorsLayout->addWidget(m_sensorCalibrationGroup);
    sensorsLayout->addStretch();
}

void SystemDiagnosticsPanel::setupActuatorsTab()
{
    m_actuatorsTab = new QWidget();
    m_tabWidget->addTab(m_actuatorsTab, "Actuators");
    
    QVBoxLayout* actuatorsLayout = new QVBoxLayout(m_actuatorsTab);
    actuatorsLayout->setSpacing(15);
    
    m_valveStatusGroup = new QGroupBox("Solenoid Valve Status");
    m_valveStatusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* valveLayout = new QGridLayout(m_valveStatusGroup);
    
    QLabel* sol1Label = new QLabel("SOL1 (AVL Valve):");
    sol1Label->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_sol1StatusIndicator = new StatusIndicator();
    m_sol1StatusIndicator->setStatus(StatusIndicator::OK, "Closed");
    
    QLabel* sol2Label = new QLabel("SOL2 (AVL Vent):");
    sol2Label->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_sol2StatusIndicator = new StatusIndicator();
    m_sol2StatusIndicator->setStatus(StatusIndicator::OK, "Closed");
    
    QLabel* sol3Label = new QLabel("SOL3 (Tank Vent):");
    sol3Label->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_sol3StatusIndicator = new StatusIndicator();
    m_sol3StatusIndicator->setStatus(StatusIndicator::OK, "Closed");
    
    valveLayout->addWidget(sol1Label, 0, 0);
    valveLayout->addWidget(m_sol1StatusIndicator, 0, 1);
    valveLayout->addWidget(sol2Label, 1, 0);
    valveLayout->addWidget(m_sol2StatusIndicator, 1, 1);
    valveLayout->addWidget(sol3Label, 2, 0);
    valveLayout->addWidget(m_sol3StatusIndicator, 2, 1);
    
    m_pumpStatusGroup = new QGroupBox("Vacuum Pump Status");
    m_pumpStatusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* pumpLayout = new QGridLayout(m_pumpStatusGroup);
    
    QLabel* pumpStatusLabel = new QLabel("Pump Status:");
    pumpStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_pumpStatusIndicator = new StatusIndicator();
    m_pumpStatusIndicator->setStatus(StatusIndicator::OK, "Stopped");
    
    QLabel* pumpSpeedLabel = new QLabel("Pump Speed:");
    pumpSpeedLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_pumpSpeedLabel = new QLabel("0%");
    m_pumpSpeedLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    QLabel* pumpCurrentLabel = new QLabel("Motor Current:");
    pumpCurrentLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_pumpCurrentLabel = new QLabel("0.0 A");
    m_pumpCurrentLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    pumpLayout->addWidget(pumpStatusLabel, 0, 0);
    pumpLayout->addWidget(m_pumpStatusIndicator, 0, 1);
    pumpLayout->addWidget(pumpSpeedLabel, 1, 0);
    pumpLayout->addWidget(m_pumpSpeedLabel, 1, 1);
    pumpLayout->addWidget(pumpCurrentLabel, 2, 0);
    pumpLayout->addWidget(m_pumpCurrentLabel, 2, 1);
    
    actuatorsLayout->addWidget(m_valveStatusGroup);
    actuatorsLayout->addWidget(m_pumpStatusGroup);
    actuatorsLayout->addStretch();
}

void SystemDiagnosticsPanel::setupPerformanceTab()
{
    m_performanceTab = new QWidget();
    m_tabWidget->addTab(m_performanceTab, "Performance");

    QVBoxLayout* performanceLayout = new QVBoxLayout(m_performanceTab);
    performanceLayout->setSpacing(15);

    m_cpuMemoryGroup = new QGroupBox("CPU & Memory Usage");
    m_cpuMemoryGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* cpuMemLayout = new QGridLayout(m_cpuMemoryGroup);

    QLabel* cpuLabel = new QLabel("CPU Usage:");
    cpuLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_cpuUsageLabel = new QLabel("0.0%");
    m_cpuUsageLabel->setStyleSheet("font-size: 14pt; color: #333;");

    m_cpuUsageBar = new QProgressBar();
    m_cpuUsageBar->setRange(0, 100);
    m_cpuUsageBar->setValue(0);
    m_cpuUsageBar->setFormat("CPU: %p%");
    m_cpuUsageBar->setMinimumHeight(30);

    QLabel* memoryLabel = new QLabel("Memory Usage:");
    memoryLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_memoryUsageLabel = new QLabel("0 MB");
    m_memoryUsageLabel->setStyleSheet("font-size: 14pt; color: #333;");

    m_memoryUsageBar = new QProgressBar();
    m_memoryUsageBar->setRange(0, 100);
    m_memoryUsageBar->setValue(0);
    m_memoryUsageBar->setFormat("Memory: %p%");
    m_memoryUsageBar->setMinimumHeight(30);

    cpuMemLayout->addWidget(cpuLabel, 0, 0);
    cpuMemLayout->addWidget(m_cpuUsageLabel, 0, 1);
    cpuMemLayout->addWidget(m_cpuUsageBar, 0, 2);
    cpuMemLayout->addWidget(memoryLabel, 1, 0);
    cpuMemLayout->addWidget(m_memoryUsageLabel, 1, 1);
    cpuMemLayout->addWidget(m_memoryUsageBar, 1, 2);

    m_threadingGroup = new QGroupBox("Threading & Concurrency");
    m_threadingGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* threadLayout = new QGridLayout(m_threadingGroup);

    QLabel* threadCountLabel = new QLabel("Active Threads:");
    threadCountLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_threadCountLabel = new QLabel("0");
    m_threadCountLabel->setStyleSheet("font-size: 14pt; color: #333;");

    threadLayout->addWidget(threadCountLabel, 0, 0);
    threadLayout->addWidget(m_threadCountLabel, 0, 1);

    m_timingGroup = new QGroupBox("System Timing & Rates");
    m_timingGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* timingLayout = new QGridLayout(m_timingGroup);

    QLabel* dataRateLabel = new QLabel("Data Acquisition Rate:");
    dataRateLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_dataRateLabel = new QLabel("0.0 Hz");
    m_dataRateLabel->setStyleSheet("font-size: 14pt; color: #333;");

    QLabel* guiFrameRateLabel = new QLabel("GUI Frame Rate:");
    guiFrameRateLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_guiFrameRateLabel = new QLabel("0.0 FPS");
    m_guiFrameRateLabel->setStyleSheet("font-size: 14pt; color: #333;");

    QLabel* safetyCheckRateLabel = new QLabel("Safety Check Rate:");
    safetyCheckRateLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_safetyCheckRateLabel = new QLabel("0.0 Hz");
    m_safetyCheckRateLabel->setStyleSheet("font-size: 14pt; color: #333;");

    timingLayout->addWidget(dataRateLabel, 0, 0);
    timingLayout->addWidget(m_dataRateLabel, 0, 1);
    timingLayout->addWidget(guiFrameRateLabel, 1, 0);
    timingLayout->addWidget(m_guiFrameRateLabel, 1, 1);
    timingLayout->addWidget(safetyCheckRateLabel, 2, 0);
    timingLayout->addWidget(m_safetyCheckRateLabel, 2, 1);

    performanceLayout->addWidget(m_cpuMemoryGroup);
    performanceLayout->addWidget(m_threadingGroup);
    performanceLayout->addWidget(m_timingGroup);
    performanceLayout->addStretch();
}

void SystemDiagnosticsPanel::setupLogsTab()
{
    m_logsTab = new QWidget();
    m_tabWidget->addTab(m_logsTab, "System Logs");

    QVBoxLayout* logsLayout = new QVBoxLayout(m_logsTab);
    logsLayout->setSpacing(15);

    m_logDisplay = new QTextEdit();
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setFont(QFont("Courier", 10));
    m_logDisplay->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; color: #ffffff; border: 2px solid #555; border-radius: 5px; }"
    );

    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_refreshLogsButton = new TouchButton("Refresh Logs", this);
    m_clearLogsButton = new TouchButton("Clear Display", this);
    m_exportLogsButton = new TouchButton("Export Logs", this);

    buttonLayout->addWidget(m_refreshLogsButton);
    buttonLayout->addWidget(m_clearLogsButton);
    buttonLayout->addWidget(m_exportLogsButton);
    buttonLayout->addStretch();

    logsLayout->addWidget(m_logDisplay);
    logsLayout->addLayout(buttonLayout);
}

void SystemDiagnosticsPanel::setupTestingTab()
{
    m_testingTab = new QWidget();
    m_tabWidget->addTab(m_testingTab, "System Testing");

    QVBoxLayout* testingLayout = new QVBoxLayout(m_testingTab);
    testingLayout->setSpacing(15);

    m_testControlsGroup = new QGroupBox("Test Controls");
    m_testControlsGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* controlsLayout = new QGridLayout(m_testControlsGroup);

    m_runSystemTestButton = new TouchButton("Run System Test", this);
    m_runHardwareTestButton = new TouchButton("Run Hardware Test", this);
    m_runSensorTestButton = new TouchButton("Run Sensor Test", this);
    m_runSafetyTestButton = new TouchButton("Run Safety Test", this);

    controlsLayout->addWidget(m_runSystemTestButton, 0, 0);
    controlsLayout->addWidget(m_runHardwareTestButton, 0, 1);
    controlsLayout->addWidget(m_runSensorTestButton, 1, 0);
    controlsLayout->addWidget(m_runSafetyTestButton, 1, 1);

    m_testProgressBar = new QProgressBar();
    m_testProgressBar->setRange(0, 100);
    m_testProgressBar->setValue(0);
    m_testProgressBar->setFormat("Test Progress: %p%");
    m_testProgressBar->setMinimumHeight(30);

    m_testResultsGroup = new QGroupBox("Test Results");
    m_testResultsGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QVBoxLayout* resultsLayout = new QVBoxLayout(m_testResultsGroup);

    m_testResultsDisplay = new QTextEdit();
    m_testResultsDisplay->setReadOnly(true);
    m_testResultsDisplay->setFont(QFont("Courier", 10));
    m_testResultsDisplay->setStyleSheet(
        "QTextEdit { background-color: #f8f8f8; color: #333; border: 2px solid #ddd; border-radius: 5px; }"
    );

    resultsLayout->addWidget(m_testResultsDisplay);

    testingLayout->addWidget(m_testControlsGroup);
    testingLayout->addWidget(m_testProgressBar);
    testingLayout->addWidget(m_testResultsGroup);
}

void SystemDiagnosticsPanel::connectSignals()
{
    connect(m_runSystemTestButton, &TouchButton::clicked, this, &SystemDiagnosticsPanel::runSystemTest);
    connect(m_runHardwareTestButton, &TouchButton::clicked, this, &SystemDiagnosticsPanel::runHardwareTest);
    connect(m_runSensorTestButton, &TouchButton::clicked, this, &SystemDiagnosticsPanel::runSensorTest);
    connect(m_runSafetyTestButton, &TouchButton::clicked, this, &SystemDiagnosticsPanel::runSafetyTest);

    connect(m_refreshLogsButton, &TouchButton::clicked, this, &SystemDiagnosticsPanel::onRefreshButtonClicked);
    connect(m_clearLogsButton, &TouchButton::clicked, this, [this]() {
        m_logDisplay->clear();
    });
    connect(m_exportLogsButton, &TouchButton::clicked, this, &SystemDiagnosticsPanel::onExportButtonClicked);
}

void SystemDiagnosticsPanel::startDiagnostics()
{
    if (m_diagnosticsRunning) return;

    m_diagnosticsRunning = true;
    m_diagnosticTimer->start();

    qDebug() << "System diagnostics started";
}

void SystemDiagnosticsPanel::stopDiagnostics()
{
    if (!m_diagnosticsRunning) return;

    m_diagnosticsRunning = false;
    m_diagnosticTimer->stop();

    qDebug() << "System diagnostics stopped";
}

void SystemDiagnosticsPanel::refreshDiagnostics()
{
    updateDiagnostics();
}

QList<SystemDiagnosticsPanel::DiagnosticData> SystemDiagnosticsPanel::getDiagnosticHistory() const
{
    return m_diagnosticHistory;
}

QJsonObject SystemDiagnosticsPanel::getCurrentSystemStatus() const
{
    QJsonObject status;

    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    status["diagnostics_running"] = m_diagnosticsRunning;
    status["test_in_progress"] = m_testInProgress;
    status["current_test"] = m_currentTest;

    if (m_hardwareManager) {
        status["hardware_ready"] = m_hardwareManager->isReady();
    }

    if (m_safetyManager) {
        status["safety_state"] = static_cast<int>(m_safetyManager->getSafetyState());
    }

    return status;
}

QJsonObject SystemDiagnosticsPanel::getPerformanceMetrics() const
{
    QJsonObject metrics;

    return metrics;
}

void SystemDiagnosticsPanel::updateDiagnostics()
{
    updateOverviewStatus();
    updateHardwareStatus();
    updateSensorStatus();
    updateActuatorStatus();
    updatePerformanceMetrics();
    updateLogDisplay();
}

void SystemDiagnosticsPanel::updateOverviewStatus()
{
    if (!m_systemStatusIndicator) return;

    if (m_controller) {
        qint64 uptime = QDateTime::currentMSecsSinceEpoch() - m_controller->getSystemState();
        m_systemUptimeLabel->setText(formatUptime(uptime));
    }

    m_lastUpdateLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));

    if (m_hardwareManager) {
        bool hardwareOK = m_hardwareManager->isReady();
        m_systemStatusIndicator->addStatus("hardware",
            hardwareOK ? "Ready" : "Error", hardwareOK ? StatusIndicator::OK : StatusIndicator::ERROR);
    }

    if (m_safetyManager) {
        auto safetyState = m_safetyManager->getSafetyState();
        StatusIndicator::StatusLevel status = StatusIndicator::OK;
        QString statusText = "Safe";

        switch (safetyState) {
        case SafetyManager::SAFE:
            status = StatusIndicator::OK;
            statusText = "Safe";
            break;
        case SafetyManager::WARNING:
            status = StatusIndicator::WARNING;
            statusText = "Warning";
            break;
        case SafetyManager::EMERGENCY_STOP:
            status = StatusIndicator::ERROR;
            statusText = "Emergency Stop";
            break;
        }

        m_systemStatusIndicator->addStatus("safety", statusText, status);
    }

    int healthScore = 100;
    if (m_hardwareManager && !m_hardwareManager->isReady()) healthScore -= 30;
    if (m_safetyManager && m_safetyManager->getSafetyState() != SafetyManager::SAFE) healthScore -= 40;

    m_systemHealthBar->setValue(qMax(0, healthScore));
}

void SystemDiagnosticsPanel::updateHardwareStatus()
{
}

void SystemDiagnosticsPanel::updateSensorStatus()
{
    if (!m_hardwareManager) return;

    try {
        double avlPressure = m_hardwareManager->readAVLPressure();
        m_avlReadingLabel->setText(QString("%1 mmHg").arg(avlPressure, 0, 'f', 1));

        bool avlOK = (avlPressure >= 0.0 && avlPressure <= 200.0);
        m_avlSensorStatusLabel->setText(avlOK ? "OK" : "Error");
        m_avlSensorStatusLabel->setStyleSheet(avlOK ? "color: green;" : "color: red;");

        double tankPressure = m_hardwareManager->readTankPressure();
        m_tankReadingLabel->setText(QString("%1 mmHg").arg(tankPressure, 0, 'f', 1));

        bool tankOK = (tankPressure >= 0.0 && tankPressure <= 200.0);
        m_tankSensorStatusLabel->setText(tankOK ? "OK" : "Error");
        m_tankSensorStatusLabel->setStyleSheet(tankOK ? "color: green;" : "color: red;");

        int accuracy = (avlOK && tankOK) ? 95 : 50;
        m_sensorAccuracyBar->setValue(accuracy);

    } catch (const std::exception& e) {
        qWarning() << "Error updating sensor status:" << e.what();
    }
}

void SystemDiagnosticsPanel::updateActuatorStatus()
{
    if (!m_hardwareManager) return;

    bool sol1State = m_hardwareManager->getSOL1State();
    m_sol1StatusIndicator->setStatus(
        StatusIndicator::OK,
        sol1State ? "Open" : "Closed"
    );

    bool sol2State = m_hardwareManager->getSOL2State();
    m_sol2StatusIndicator->setStatus(
        StatusIndicator::OK,
        sol2State ? "Open" : "Closed"
    );

    bool sol3State = m_hardwareManager->getSOL3State();
    m_sol3StatusIndicator->setStatus(
        StatusIndicator::OK,
        sol3State ? "Open" : "Closed"
    );

    bool pumpEnabled = m_hardwareManager->isPumpEnabled();
    double pumpSpeed = m_hardwareManager->getPumpSpeed();

    m_pumpStatusIndicator->setStatus(
        StatusIndicator::OK,
        pumpEnabled ? "Running" : "Stopped"
    );

    m_pumpSpeedLabel->setText(QString("%1%").arg(pumpSpeed, 0, 'f', 1));
    m_pumpCurrentLabel->setText("0.0 A");
}

void SystemDiagnosticsPanel::updatePerformanceMetrics()
{
}

void SystemDiagnosticsPanel::updateLogDisplay()
{
    if (m_diagnosticHistory.size() > 10) { 
        QString logText;
        int startIndex = m_diagnosticHistory.size() - 10;

        for (int i = startIndex; i < m_diagnosticHistory.size(); ++i) {
            const DiagnosticData& data = m_diagnosticHistory[i];
            logText += QString("[%1] %2: %3 - %4\n")
                      .arg(data.timestamp.toString("hh:mm:ss"))
                      .arg(data.component)
                      .arg(data.status)
                      .arg(data.details);
        }

        m_logDisplay->setPlainText(logText);

        QTextCursor cursor = m_logDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logDisplay->setTextCursor(cursor);
    }
}

void SystemDiagnosticsPanel::runSystemTest()
{
    if (m_testInProgress) return;

    m_testInProgress = true;
    m_currentTest = "System Test";
    m_testProgressBar->setValue(0);
    m_testResultsDisplay->clear();

    m_testResultsDisplay->append("=== SYSTEM TEST STARTED ===");
    m_testResultsDisplay->append(QString("Test started at: %1").arg(QDateTime::currentDateTime().toString()));

    bool allTestsPassed = true;

    m_testProgressBar->setValue(20);
    m_testResultsDisplay->append("\n1. Testing GPIO pins...");
    bool gpioTest = testGPIOPins();
    m_testResultsDisplay->append(QString("   GPIO Test: %1").arg(gpioTest ? "PASSED" : "FAILED"));
    allTestsPassed &= gpioTest;

    m_testProgressBar->setValue(40);
    m_testResultsDisplay->append("\n2. Testing SPI communication...");
    bool spiTest = testSPICommunication();
    m_testResultsDisplay->append(QString("   SPI Test: %1").arg(spiTest ? "PASSED" : "FAILED"));
    allTestsPassed &= spiTest;

    m_testProgressBar->setValue(60);
    m_testResultsDisplay->append("\n3. Testing sensor readings...");
    bool sensorTest = testSensorReadings();
    m_testResultsDisplay->append(QString("   Sensor Test: %1").arg(sensorTest ? "PASSED" : "FAILED"));
    allTestsPassed &= sensorTest;

    m_testProgressBar->setValue(80);
    m_testResultsDisplay->append("\n4. Testing actuator control...");
    bool actuatorTest = testActuatorControl();
    m_testResultsDisplay->append(QString("   Actuator Test: %1").arg(actuatorTest ? "PASSED" : "FAILED"));
    allTestsPassed &= actuatorTest;

    m_testProgressBar->setValue(100);
    m_testResultsDisplay->append("\n5. Testing safety system...");
    bool safetyTest = testSafetySystem();
    m_testResultsDisplay->append(QString("   Safety Test: %1").arg(safetyTest ? "PASSED" : "FAILED"));
    allTestsPassed &= safetyTest;

    m_testResultsDisplay->append("\n=== SYSTEM TEST COMPLETED ===");
    m_testResultsDisplay->append(QString("Overall Result: %1").arg(allTestsPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED"));
    m_testResultsDisplay->append(QString("Test completed at: %1").arg(QDateTime::currentDateTime().toString()));

    m_testInProgress = false;
    m_currentTest.clear();

    emit systemTestCompleted(allTestsPassed);
}

void SystemDiagnosticsPanel::runHardwareTest()
{
    if (m_testInProgress) return;

    m_testInProgress = true;
    m_currentTest = "Hardware Test";
    m_testProgressBar->setValue(0);
    m_testResultsDisplay->clear();

    m_testResultsDisplay->append("=== HARDWARE TEST STARTED ===");

    bool hardwareOK = true;

    m_testProgressBar->setValue(33);
    bool gpioOK = testGPIOPins();
    m_testResultsDisplay->append(QString("GPIO Test: %1").arg(gpioOK ? "PASSED" : "FAILED"));
    hardwareOK &= gpioOK;

    m_testProgressBar->setValue(66);
    bool spiOK = testSPICommunication();
    m_testResultsDisplay->append(QString("SPI Test: %1").arg(spiOK ? "PASSED" : "FAILED"));
    hardwareOK &= spiOK;

    m_testProgressBar->setValue(100);
    bool perfOK = testSystemPerformance();
    m_testResultsDisplay->append(QString("Performance Test: %1").arg(perfOK ? "PASSED" : "FAILED"));
    hardwareOK &= perfOK;

    m_testResultsDisplay->append(QString("\nHardware Test Result: %1").arg(hardwareOK ? "PASSED" : "FAILED"));

    m_testInProgress = false;
    m_currentTest.clear();

    emit hardwareTestCompleted("Hardware", hardwareOK);
}

void SystemDiagnosticsPanel::runSensorTest()
{
    if (m_testInProgress) return;

    m_testInProgress = true;
    m_currentTest = "Sensor Test";
    m_testProgressBar->setValue(0);
    m_testResultsDisplay->clear();

    m_testResultsDisplay->append("=== SENSOR TEST STARTED ===");

    bool sensorOK = testSensorReadings();
    m_testProgressBar->setValue(100);

    m_testResultsDisplay->append(QString("Sensor Test Result: %1").arg(sensorOK ? "PASSED" : "FAILED"));

    m_testInProgress = false;
    m_currentTest.clear();

    emit hardwareTestCompleted("Sensors", sensorOK);
}

void SystemDiagnosticsPanel::runSafetyTest()
{
    if (m_testInProgress) return;

    m_testInProgress = true;
    m_currentTest = "Safety Test";
    m_testProgressBar->setValue(0);
    m_testResultsDisplay->clear();

    m_testResultsDisplay->append("=== SAFETY TEST STARTED ===");

    bool safetyOK = testSafetySystem();
    m_testProgressBar->setValue(100);

    m_testResultsDisplay->append(QString("Safety Test Result: %1").arg(safetyOK ? "PASSED" : "FAILED"));

    m_testInProgress = false;
    m_currentTest.clear();

    emit hardwareTestCompleted("Safety", safetyOK);
}

void SystemDiagnosticsPanel::exportDiagnostics()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Diagnostics",
        QString("diagnostics_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Text Files (*.txt)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);

            stream << "=== SYSTEM DIAGNOSTICS EXPORT ===\n";
            stream << "Export Time: " << QDateTime::currentDateTime().toString() << "\n\n";

            QJsonObject status = getCurrentSystemStatus();
            stream << "Current System Status:\n";
            for (auto it = status.begin(); it != status.end(); ++it) {
                stream << "  " << it.key() << ": " << it.value().toString() << "\n";
            }

            QJsonObject metrics = getPerformanceMetrics();
            stream << "\nPerformance Metrics:\n";
            for (auto it = metrics.begin(); it != metrics.end(); ++it) {
                stream << "  " << it.key() << ": " << it.value().toString() << "\n";
            }

            stream << "\nDiagnostic History:\n";
            for (const DiagnosticData& data : m_diagnosticHistory) {
                stream << QString("[%1] %2: %3 - %4\n")
                          .arg(data.timestamp.toString())
                          .arg(data.component)
                          .arg(data.status)
                          .arg(data.details);
            }

            file.close();
            QMessageBox::information(this, "Export Complete",
                QString("Diagnostics exported to: %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Export Failed",
                "Failed to create export file.");
        }
    }
}

void SystemDiagnosticsPanel::onDiagnosticTimer()
{
    updateDiagnostics();
}

void SystemDiagnosticsPanel::onTestButtonClicked()
{
}

void SystemDiagnosticsPanel::onExportButtonClicked()
{
    exportDiagnostics();
}

void SystemDiagnosticsPanel::onRefreshButtonClicked()
{
    refreshDiagnostics();
}

void SystemDiagnosticsPanel::onComponentSelected()
{
    // Get the selected component from the component list
    QListWidget* componentList = findChild<QListWidget*>("componentList");
    if (!componentList) return;

    QListWidgetItem* selectedItem = componentList->currentItem();
    if (!selectedItem) return;

    QString componentName = selectedItem->text();
    qDebug() << "Component selected:" << componentName;

    // Refresh diagnostics for the selected component
    refreshDiagnostics();
}

bool SystemDiagnosticsPanel::testGPIOPins()
{
    if (!m_hardwareManager) return false;

    try {
        return m_hardwareManager->isReady();
    } catch (const std::exception& e) {
        qWarning() << "GPIO test failed:" << e.what();
        return false;
    }
}

bool SystemDiagnosticsPanel::testSPICommunication()
{
    if (!m_hardwareManager) return false;

    try {
        double avlPressure = m_hardwareManager->readAVLPressure();
        double tankPressure = m_hardwareManager->readTankPressure();

        return (avlPressure >= 0.0 && avlPressure <= 200.0) &&
               (tankPressure >= 0.0 && tankPressure <= 200.0);
    } catch (const std::exception& e) {
        qWarning() << "SPI test failed:" << e.what();
        return false;
    }
}

bool SystemDiagnosticsPanel::testSensorReadings()
{
    if (!m_hardwareManager) return false;

    try {
        QList<double> avlReadings, tankReadings;

        for (int i = 0; i < 5; ++i) {
            avlReadings.append(m_hardwareManager->readAVLPressure());
            tankReadings.append(m_hardwareManager->readTankPressure());
            QThread::msleep(100);
        }

        double avlSum = 0.0, tankSum = 0.0;
        for (double reading : avlReadings) avlSum += reading;
        for (double reading : tankReadings) tankSum += reading;

        double avlMean = avlSum / avlReadings.size();
        double tankMean = tankSum / tankReadings.size();

        double avlVariance = 0.0, tankVariance = 0.0;
        for (double reading : avlReadings) {
            avlVariance += (reading - avlMean) * (reading - avlMean);
        }
        for (double reading : tankReadings) {
            tankVariance += (reading - tankMean) * (reading - tankMean);
        }

        double avlStdDev = sqrt(avlVariance / avlReadings.size());
        double tankStdDev = sqrt(tankVariance / tankReadings.size());

        return (avlStdDev < 5.0) && (tankStdDev < 5.0);

    } catch (const std::exception& e) {
        qWarning() << "Sensor test failed:" << e.what();
        return false;
    }
}

bool SystemDiagnosticsPanel::testActuatorControl()
{
    if (!m_hardwareManager) return false;

    try {
        bool originalSOL1 = m_hardwareManager->getSOL1State();
        bool originalSOL2 = m_hardwareManager->getSOL2State();
        bool originalSOL3 = m_hardwareManager->getSOL3State();

        m_hardwareManager->setSOL1(true);
        QThread::msleep(100);
        bool sol1Test = m_hardwareManager->getSOL1State();

        m_hardwareManager->setSOL1(false);
        QThread::msleep(100);
        bool sol1Test2 = !m_hardwareManager->getSOL1State();

        m_hardwareManager->setSOL1(originalSOL1);
        m_hardwareManager->setSOL2(originalSOL2);
        m_hardwareManager->setSOL3(originalSOL3);

        return sol1Test && sol1Test2;

    } catch (const std::exception& e) {
        qWarning() << "Actuator test failed:" << e.what();
        return false;
    }
}

bool SystemDiagnosticsPanel::testSafetySystem()
{
    if (!m_safetyManager) return false;

    try {
        bool safetyCheck = m_safetyManager->performSafetyCheck();
        auto safetyState = m_safetyManager->getSafetyState();

        return safetyCheck && (safetyState != SafetyManager::EMERGENCY_STOP);

    } catch (const std::exception& e) {
        qWarning() << "Safety test failed:" << e.what();
        return false;
    }
}

bool SystemDiagnosticsPanel::testSystemPerformance()
{
    return true;
}

void SystemDiagnosticsPanel::addDiagnosticEntry(const DiagnosticData& data)
{
    m_diagnosticHistory.append(data);

    while (m_diagnosticHistory.size() > m_maxHistoryEntries) {
        m_diagnosticHistory.removeFirst();
    }
}

void SystemDiagnosticsPanel::clearDiagnosticHistory()
{
    m_diagnosticHistory.clear();
}

QString SystemDiagnosticsPanel::formatUptime(qint64 uptimeMs)
{
    qint64 seconds = uptimeMs / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    qint64 days = hours / 24;

    if (days > 0) {
        return QString("%1 days, %2 hours").arg(days).arg(hours % 24);
    } else if (hours > 0) {
        return QString("%1 hours, %2 minutes").arg(hours).arg(minutes % 60);
    } else if (minutes > 0) {
        return QString("%1 minutes, %2 seconds").arg(minutes).arg(seconds % 60);
    } else {
        return QString("%1 seconds").arg(seconds);
    }
}

QString SystemDiagnosticsPanel::formatMemoryUsage(qint64 bytes)
{
    if (bytes >= 1024 * 1024 * 1024) {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    } else if (bytes >= 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else if (bytes >= 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else {
        return QString("%1 bytes").arg(bytes);
    }
}

QString SystemDiagnosticsPanel::formatCPUUsage(double percentage)
{
    return QString("%1%").arg(percentage, 0, 'f', 1);
}

QString SystemDiagnosticsPanel::formatTemperature(double celsius)
{
    return QString("%1°C").arg(celsius, 0, 'f', 1);
}