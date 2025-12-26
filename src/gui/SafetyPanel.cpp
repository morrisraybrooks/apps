#include "SafetyPanel.h"
#include "components/TouchButton.h"
#include "components/StatusIndicator.h"
#include "styles/ModernMedicalStyle.h"
#include "../VacuumController.h"
#include <QDebug>
#include <QDateTime>
#include <QScrollArea>

// Constants
const double SafetyPanel::PRESSURE_LIMIT = 100.0;
const double SafetyPanel::WARNING_THRESHOLD = 80.0;
const double SafetyPanel::ANTI_DETACHMENT_THRESHOLD = 50.0;

SafetyPanel::SafetyPanel(VacuumController* controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_mainLayout(new QVBoxLayout(this))
    , m_statusIndicators(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_currentAVL(0.0)
    , m_currentTank(0.0)
    , m_emergencyStop(false)
    , m_systemHealthy(true)
{
    setupUI();
    connectSignals();
    
    // Start status update timer
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &SafetyPanel::updateStatusIndicators);
    m_updateTimer->start();
}

SafetyPanel::~SafetyPanel()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void SafetyPanel::setupUI()
{
    // Create scroll area for better space utilization with compact scaling
    QScrollArea* scrollArea = new QScrollArea(this);
    QWidget* scrollContent = new QWidget;
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);

    scrollLayout->setSpacing(ModernMedicalStyle::Spacing::getMedium());
    scrollLayout->setContentsMargins(
        ModernMedicalStyle::Spacing::getMedium(),
        ModernMedicalStyle::Spacing::getMedium(),
        ModernMedicalStyle::Spacing::getMedium(),
        ModernMedicalStyle::Spacing::getMedium()
    );

    // Configure scroll area
    scrollArea->setWidget(scrollContent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    // Add scroll area to main layout
    m_mainLayout->addWidget(scrollArea);

    // Use scroll layout for content instead of main layout
    m_contentLayout = scrollLayout;
    
    setupEmergencyControls();
    setupStatusMonitoring();
    setupPressureLimits();
    setupSystemDiagnostics();
    
    // Add alert group at the bottom
    m_alertGroup = new QGroupBox("System Alerts");
    m_alertGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle(ModernMedicalStyle::Colors::MEDICAL_RED));
    
    QVBoxLayout* alertLayout = new QVBoxLayout(m_alertGroup);
    
    m_alertLabel = new QLabel("No active alerts");
    m_alertLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; padding: 10px;");
    m_alertLabel->setWordWrap(true);
    
    m_clearAlertsButton = new TouchButton("Clear Alerts");
    m_clearAlertsButton->setButtonType(TouchButton::Warning);
    m_clearAlertsButton->setMinimumSize(150, 50);
    m_clearAlertsButton->setEnabled(false);
    
    connect(m_clearAlertsButton, &TouchButton::clicked, this, &SafetyPanel::clearAlerts);
    
    alertLayout->addWidget(m_alertLabel);
    alertLayout->addWidget(m_clearAlertsButton);

    m_contentLayout->addWidget(m_alertGroup);
    m_contentLayout->addStretch();
}

void SafetyPanel::setupEmergencyControls()
{
    m_emergencyGroup = new QGroupBox("Emergency Controls");
    m_emergencyGroup->setStyleSheet("QGroupBox { font-size: 18pt; font-weight: bold; color: #f44336; }");
    
    QVBoxLayout* emergencyLayout = new QVBoxLayout(m_emergencyGroup);
    
    // Emergency stop button
    m_emergencyStopButton = new TouchButton("EMERGENCY STOP");
    m_emergencyStopButton->setButtonType(TouchButton::Emergency);
    m_emergencyStopButton->setMinimumSize(200, 100);
    m_emergencyStopButton->setPulseEffect(true);
    
    // Reset emergency stop button
    m_resetEmergencyButton = new TouchButton("Reset Emergency Stop");
    m_resetEmergencyButton->setButtonType(TouchButton::Warning);
    m_resetEmergencyButton->setMinimumSize(180, 60);
    m_resetEmergencyButton->setEnabled(false);
    
    // Emergency status label
    m_emergencyStatusLabel = new QLabel("System Normal");
    m_emergencyStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #4CAF50; padding: 10px;");
    m_emergencyStatusLabel->setAlignment(Qt::AlignCenter);
    
    // Layout emergency controls
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_emergencyStopButton);
    buttonLayout->addWidget(m_resetEmergencyButton);
    buttonLayout->addStretch();
    
    emergencyLayout->addLayout(buttonLayout);
    emergencyLayout->addWidget(m_emergencyStatusLabel);
    
    m_contentLayout->addWidget(m_emergencyGroup);
}

void SafetyPanel::setupStatusMonitoring()
{
    m_statusGroup = new QGroupBox("System Status");
    m_statusGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    QVBoxLayout* statusLayout = new QVBoxLayout(m_statusGroup);
    
    // Create multi-status indicator
    m_statusIndicators = new MultiStatusIndicator();
    m_statusIndicators->setColumns(2);
    
    // Add status indicators
    m_statusIndicators->addStatus("hardware", "Hardware", StatusIndicator::OK);
    m_statusIndicators->addStatus("sensors", "Sensors", StatusIndicator::OK);
    m_statusIndicators->addStatus("actuators", "Actuators", StatusIndicator::OK);
    m_statusIndicators->addStatus("safety", "Safety System", StatusIndicator::OK);
    m_statusIndicators->addStatus("anti_detachment", "Anti-detachment", StatusIndicator::OK);
    m_statusIndicators->addStatus("pressure", "Pressure Limits", StatusIndicator::OK);
    
    connect(m_statusIndicators, &MultiStatusIndicator::statusClicked,
            this, [this](const QString& name, StatusIndicator::StatusLevel status) {
                qDebug() << "Status clicked:" << name << "Level:" << status;
            });
    
    statusLayout->addWidget(m_statusIndicators);
    m_contentLayout->addWidget(m_statusGroup);
}

void SafetyPanel::setupPressureLimits()
{
    m_pressureGroup = new QGroupBox("Pressure Monitoring");
    m_pressureGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    QGridLayout* pressureLayout = new QGridLayout(m_pressureGroup);
    pressureLayout->setSpacing(10);
    
    // AVL Pressure
    QLabel* avlLabel = new QLabel("AVL Pressure:");
    avlLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_avlPressureLabel = new QLabel("0.0 mmHg");
    m_avlPressureLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    m_avlPressureBar = new QProgressBar();
    m_avlPressureBar->setRange(0, static_cast<int>(PRESSURE_LIMIT));
    m_avlPressureBar->setValue(0);
    m_avlPressureBar->setMinimumHeight(25);
    
    // Tank Pressure
    QLabel* tankLabel = new QLabel("Tank Pressure:");
    tankLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    
    m_tankPressureLabel = new QLabel("0.0 mmHg");
    m_tankPressureLabel->setStyleSheet("font-size: 14pt; color: #333;");
    
    m_tankPressureBar = new QProgressBar();
    m_tankPressureBar->setRange(0, static_cast<int>(PRESSURE_LIMIT));
    m_tankPressureBar->setValue(0);
    m_tankPressureBar->setMinimumHeight(25);
    
    // Pressure limits info
    m_pressureLimitLabel = new QLabel(QString("Pressure Limit: %1 mmHg").arg(PRESSURE_LIMIT));
    m_pressureLimitLabel->setStyleSheet("font-size: 12pt; color: #666;");
    
    m_antiDetachmentLabel = new QLabel(QString("Anti-detachment Threshold: %1 mmHg").arg(ANTI_DETACHMENT_THRESHOLD));
    m_antiDetachmentLabel->setStyleSheet("font-size: 12pt; color: #666;");
    
    // Layout pressure monitoring
    pressureLayout->addWidget(avlLabel, 0, 0);
    pressureLayout->addWidget(m_avlPressureLabel, 0, 1);
    pressureLayout->addWidget(m_avlPressureBar, 0, 2);
    
    pressureLayout->addWidget(tankLabel, 1, 0);
    pressureLayout->addWidget(m_tankPressureLabel, 1, 1);
    pressureLayout->addWidget(m_tankPressureBar, 1, 2);
    
    pressureLayout->addWidget(m_pressureLimitLabel, 2, 0, 1, 3);
    pressureLayout->addWidget(m_antiDetachmentLabel, 3, 0, 1, 3);
    
    m_contentLayout->addWidget(m_pressureGroup);
}

void SafetyPanel::setupSystemDiagnostics()
{
    m_diagnosticsGroup = new QGroupBox("System Diagnostics");
    m_diagnosticsGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle());
    
    QVBoxLayout* diagnosticsLayout = new QVBoxLayout(m_diagnosticsGroup);
    
    // Diagnostic buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_safetyTestButton = new TouchButton("Run Safety Test");
    m_safetyTestButton->setButtonType(TouchButton::Primary);
    m_safetyTestButton->setMinimumSize(150, 50);
    
    m_systemDiagnosticsButton = new TouchButton("System Diagnostics");
    m_systemDiagnosticsButton->setButtonType(TouchButton::Normal);
    m_systemDiagnosticsButton->setMinimumSize(150, 50);
    
    buttonLayout->addWidget(m_safetyTestButton);
    buttonLayout->addWidget(m_systemDiagnosticsButton);
    buttonLayout->addStretch();
    
    // Status labels
    m_lastTestLabel = new QLabel("Last safety test: Never");
    m_lastTestLabel->setStyleSheet("font-size: 12pt; color: #666;");
    
    m_systemHealthLabel = new QLabel("System Health: Good");
    m_systemHealthLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #4CAF50;");
    
    diagnosticsLayout->addLayout(buttonLayout);
    diagnosticsLayout->addWidget(m_lastTestLabel);
    diagnosticsLayout->addWidget(m_systemHealthLabel);
    
    m_contentLayout->addWidget(m_diagnosticsGroup);
}

void SafetyPanel::connectSignals()
{
    // Connect emergency buttons
    connect(m_emergencyStopButton, &TouchButton::clicked, this, &SafetyPanel::onEmergencyStopClicked);
    connect(m_resetEmergencyButton, &TouchButton::clicked, this, &SafetyPanel::onResetEmergencyStopClicked);
    
    // Connect diagnostic buttons
    connect(m_safetyTestButton, &TouchButton::clicked, this, &SafetyPanel::onSafetyTestClicked);
    connect(m_systemDiagnosticsButton, &TouchButton::clicked, this, &SafetyPanel::onSystemDiagnosticsClicked);
    
    // Connect to controller if available
    if (m_controller) {
        connect(m_controller, &VacuumController::pressureUpdated,
                this, [this](double avl, double tank) {
                    m_currentAVL = avl;
                    m_currentTank = tank;
                });
        
        connect(m_controller, &VacuumController::emergencyStopTriggered,
                this, &SafetyPanel::onEmergencyStopTriggered);
        
        connect(m_controller, &VacuumController::systemStateChanged,
                this, &SafetyPanel::onSafetyStateChanged);
    }
}

void SafetyPanel::showAntiDetachmentAlert()
{
    m_alertLabel->setText("ANTI-DETACHMENT ACTIVATED: Cup detachment detected. Vacuum increased automatically.");
    m_alertLabel->setStyleSheet("font-size: 14pt; color: #FF9800; font-weight: bold; padding: 10px; background-color: #FFF3E0;");
    m_clearAlertsButton->setEnabled(true);
    
    // Update status indicator
    if (m_statusIndicators) {
        m_statusIndicators->updateStatus("anti_detachment", StatusIndicator::WARNING, "Active");
    }
}

void SafetyPanel::showOverpressureAlert(double pressure)
{
    m_alertLabel->setText(QString("OVERPRESSURE ALERT: Pressure exceeded safe limits (%1 mmHg). System stopped.").arg(pressure, 0, 'f', 1));
    m_alertLabel->setStyleSheet("font-size: 14pt; color: #f44336; font-weight: bold; padding: 10px; background-color: #FFEBEE;");
    m_clearAlertsButton->setEnabled(true);
    
    // Update status indicator
    if (m_statusIndicators) {
        m_statusIndicators->updateStatus("pressure", StatusIndicator::CRITICAL, "Overpressure");
    }
}

void SafetyPanel::showSensorErrorAlert(const QString& sensor)
{
    m_alertLabel->setText(QString("SENSOR ERROR: %1 sensor malfunction detected. Check connections.").arg(sensor));
    m_alertLabel->setStyleSheet("font-size: 14pt; color: #f44336; font-weight: bold; padding: 10px; background-color: #FFEBEE;");
    m_clearAlertsButton->setEnabled(true);
    
    // Update status indicator
    if (m_statusIndicators) {
        m_statusIndicators->updateStatus("sensors", StatusIndicator::ERROR, "Malfunction");
    }
}

void SafetyPanel::clearAlerts()
{
    m_alertLabel->setText("No active alerts");
    m_alertLabel->setStyleSheet("font-size: 14pt; color: #4CAF50; padding: 10px;");
    m_clearAlertsButton->setEnabled(false);
    
    // Reset status indicators to OK (if system is actually OK)
    if (m_statusIndicators && m_systemHealthy) {
        m_statusIndicators->updateStatus("anti_detachment", StatusIndicator::OK, "Normal");
        m_statusIndicators->updateStatus("pressure", StatusIndicator::OK, "Normal");
        m_statusIndicators->updateStatus("sensors", StatusIndicator::OK, "Normal");
    }
}

void SafetyPanel::updateSafetyStatus()
{
    updateStatusIndicators();
}

void SafetyPanel::onEmergencyStopTriggered()
{
    m_emergencyStop = true;
    m_emergencyStatusLabel->setText("EMERGENCY STOP ACTIVE");
    m_emergencyStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #f44336; padding: 10px; background-color: #FFEBEE;");
    
    m_emergencyStopButton->setEnabled(false);
    m_resetEmergencyButton->setEnabled(true);
    
    // Update status indicators
    if (m_statusIndicators) {
        m_statusIndicators->updateStatus("safety", StatusIndicator::CRITICAL, "Emergency Stop");
    }
}

void SafetyPanel::onSafetyStateChanged(int state)
{
    // Update safety status based on system state
    VacuumController::SystemState systemState = static_cast<VacuumController::SystemState>(state);
    
    switch (systemState) {
    case VacuumController::EMERGENCY_STOP:
        onEmergencyStopTriggered();
        break;
    case VacuumController::ERROR:
        m_systemHealthy = false;
        if (m_statusIndicators) {
            m_statusIndicators->updateStatus("safety", StatusIndicator::ERROR, "System Error");
        }
        break;
    default:
        if (m_emergencyStop) {
            // Reset emergency stop state
            m_emergencyStop = false;
            m_emergencyStatusLabel->setText("System Normal");
            m_emergencyStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #4CAF50; padding: 10px;");
            
            m_emergencyStopButton->setEnabled(true);
            m_resetEmergencyButton->setEnabled(false);
        }
        
        m_systemHealthy = true;
        if (m_statusIndicators) {
            m_statusIndicators->updateStatus("safety", StatusIndicator::OK, "Normal");
        }
        break;
    }
}

void SafetyPanel::onEmergencyStopClicked()
{
    emit emergencyStopRequested();
}

void SafetyPanel::onResetEmergencyStopClicked()
{
    emit resetEmergencyStopRequested();
}

void SafetyPanel::onSafetyTestClicked()
{
    emit safetyTestRequested();
    m_lastTestLabel->setText(QString("Last safety test: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
}

void SafetyPanel::onSystemDiagnosticsClicked()
{
    // This would open a detailed diagnostics dialog
    qDebug() << "System diagnostics requested";
}

void SafetyPanel::updateStatusIndicators()
{
    if (!m_statusIndicators) return;
    
    // Update pressure monitoring
    m_avlPressureLabel->setText(QString("%1 mmHg").arg(m_currentAVL, 0, 'f', 1));
    m_tankPressureLabel->setText(QString("%1 mmHg").arg(m_currentTank, 0, 'f', 1));
    
    m_avlPressureBar->setValue(static_cast<int>(m_currentAVL));
    m_tankPressureBar->setValue(static_cast<int>(m_currentTank));
    
    // Update pressure bar colors
    QString avlColor = "#4CAF50";  // Green
    QString tankColor = "#4CAF50"; // Green
    
    if (m_currentAVL > WARNING_THRESHOLD) avlColor = "#FF9800";  // Orange
    if (m_currentAVL > PRESSURE_LIMIT * 0.9) avlColor = "#f44336";  // Red
    
    if (m_currentTank > WARNING_THRESHOLD) tankColor = "#FF9800";  // Orange
    if (m_currentTank > PRESSURE_LIMIT * 0.9) tankColor = "#f44336";  // Red
    
    m_avlPressureBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(avlColor));
    m_tankPressureBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(tankColor));
    
    // Update status indicators based on current conditions
    if (m_controller && m_controller->isSystemReady()) {
        m_statusIndicators->updateStatus("hardware", StatusIndicator::OK, "Ready");
    } else {
        m_statusIndicators->updateStatus("hardware", StatusIndicator::ERROR, "Not Ready");
    }
    
    // Check pressure status
    if (m_currentAVL > PRESSURE_LIMIT || m_currentTank > PRESSURE_LIMIT) {
        m_statusIndicators->updateStatus("pressure", StatusIndicator::CRITICAL, "Overpressure");
    } else if (m_currentAVL > WARNING_THRESHOLD || m_currentTank > WARNING_THRESHOLD) {
        m_statusIndicators->updateStatus("pressure", StatusIndicator::WARNING, "High Pressure");
    } else {
        m_statusIndicators->updateStatus("pressure", StatusIndicator::OK, "Normal");
    }
    
    // Check anti-detachment
    if (m_currentAVL < ANTI_DETACHMENT_THRESHOLD) {
        m_statusIndicators->updateStatus("anti_detachment", StatusIndicator::WARNING, "Risk Detected");
    } else {
        m_statusIndicators->updateStatus("anti_detachment", StatusIndicator::OK, "Normal");
    }
}
