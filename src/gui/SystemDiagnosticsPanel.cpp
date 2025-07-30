#include "SystemDiagnosticsPanel.h"
#include "components/TouchButton.h"
#include "components/StatusIndicator.h"
#include "components/MultiStatusIndicator.h"
#include "../VacuumController.h"
#include "../hardware/HardwareManager.h"
#include "../safety/SafetyManager.h"
#include "../performance/PerformanceMonitor.h"
#include <QDebug>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

SystemDiagnosticsPanel::SystemDiagnosticsPanel(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_hardwareManager(nullptr)
    , m_safetyManager(nullptr)
    , m_performanceMonitor(nullptr)
    , m_tabWidget(new QTabWidget(this))
    , m_mainLayout(new QVBoxLayout(this))
    , m_diagnosticsRunning(false)
    , m_diagnosticTimer(new QTimer(this))
    , m_maxHistoryEntries(MAX_DIAGNOSTIC_HISTORY)
    , m_testInProgress(false)
{
    setupUI();
    connectSignals();
    
    // Get component references
    if (m_controller) {
        m_hardwareManager = m_controller->getHardwareManager();
        m_safetyManager = m_controller->getSafetyManager();
        m_performanceMonitor = m_controller->getPerformanceMonitor();
    }
    
    // Setup diagnostic timer
    m_diagnosticTimer->setInterval(DIAGNOSTIC_UPDATE_INTERVAL);
    connect(m_diagnosticTimer, &QTimer::timeout, this, &SystemDiagnosticsPanel::onDiagnosticTimer);
    
    // Start diagnostics automatically
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
    
    // System Status Group
    QGroupBox* statusGroup = new QGroupBox("System Status");
    statusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);
    
    // Multi-status indicator
    m_systemStatusIndicator = new MultiStatusIndicator();
    m_systemStatusIndicator->setColumns(3);
    m_systemStatusIndicator->addStatus("hardware", "Hardware", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("sensors", "Sensors", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("actuators", "Actuators", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("safety", "Safety", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("performance", "Performance", StatusIndicator::OK);
    m_systemStatusIndicator->addStatus("communication", "Communication", StatusIndicator::OK);
    
    statusLayout->addWidget(m_systemStatusIndicator);
    
    // System Information Group
    QGroupBox* infoGroup = new QGroupBox("System Information");
    infoGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* infoLayout = new QGridLayout(infoGroup);
    
    // System uptime
    QLabel* uptimeLabel = new QLabel("System Uptime:");
    uptimeLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_systemUptimeLabel = new QLabel("0 hours");
    m_systemUptimeLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    // System version
    QLabel* versionLabel = new QLabel("Software Version:");
    versionLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_systemVersionLabel = new QLabel("v1.0.0");
    m_systemVersionLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    // Last update
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
    
    // System Health Bar
    QLabel* healthLabel = new QLabel("Overall System Health:");
    healthLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_systemHealthBar = new QProgressBar();
    m_systemHealthBar->setRange(0, 100);
    m_systemHealthBar->setValue(100);
    m_systemHealthBar->setFormat("System Health: %p%");
    m_systemHealthBar->setMinimumHeight(40);
    m_systemHealthBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #ddd;"
        "    border-radius: 20px;"
        "    text-align: center;"
        "    font-size: 14pt;"
        "    font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #2196F3);"
        "    border-radius: 18px;"
        "}"
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
    
    // GPIO Status Group
    m_gpioStatusGroup = new QGroupBox("GPIO Status");
    m_gpioStatusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* gpioLayout = new QGridLayout(m_gpioStatusGroup);
    
    // GPIO pin status labels
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
    
    // SPI Status Group
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
    
    // Hardware Table
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
    
    // Sensor Readings Group
    m_sensorReadingsGroup = new QGroupBox("Current Sensor Readings");
    m_sensorReadingsGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* readingsLayout = new QGridLayout(m_sensorReadingsGroup);
    
    // AVL Sensor
    QLabel* avlSensorLabel = new QLabel("AVL Pressure Sensor:");
    avlSensorLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_avlSensorStatusLabel = new QLabel("OK");
    m_avlSensorStatusLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; font-weight: bold;");
    m_avlReadingLabel = new QLabel("0.0 mmHg");
    m_avlReadingLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    // Tank Sensor
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
    
    // Sensor Calibration Group
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
    
    // Valve Status Group
    m_valveStatusGroup = new QGroupBox("Solenoid Valve Status");
    m_valveStatusGroup->setStyleSheet("QGroupBox { font-size: 16pt; font-weight: bold; color: #2196F3; }");
    QGridLayout* valveLayout = new QGridLayout(m_valveStatusGroup);
    
    // SOL1 (AVL Valve)
    QLabel* sol1Label = new QLabel("SOL1 (AVL Valve):");
    sol1Label->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_sol1StatusIndicator = new StatusIndicator();
    m_sol1StatusIndicator->setStatus(StatusIndicator::OK, "Closed");
    
    // SOL2 (AVL Vent)
    QLabel* sol2Label = new QLabel("SOL2 (AVL Vent):");
    sol2Label->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_sol2StatusIndicator = new StatusIndicator();
    m_sol2StatusIndicator->setStatus(StatusIndicator::OK, "Closed");
    
    // SOL3 (Tank Vent)
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
    
    // Pump Status Group
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
