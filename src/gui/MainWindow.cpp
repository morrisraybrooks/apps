#include "MainWindow.h"
#include "PressureMonitor.h"
#include "PatternSelector.h"
#include "SafetyPanel.h"
#include "SettingsDialog.h"
#include "../VacuumController.h"

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QFont>
#include <QSizePolicy>
#include <QDebug>

MainWindow::MainWindow(VacuumController* controller, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_centralWidget(nullptr)
    , m_stackedWidget(nullptr)
    , m_statusUpdateTimer(new QTimer(this))
    , m_systemRunning(false)
    , m_systemPaused(false)
    , m_emergencyStop(false)
{
    if (!m_controller) {
        qCritical() << "VacuumController not provided to MainWindow";
        return;
    }
    
    // Set window properties for 50-inch display
    setWindowTitle("Vacuum Controller - Medical Device Interface");
    setMinimumSize(1920, 1080);  // Full HD minimum
    
    // Setup UI
    setupUI();
    connectSignals();
    applyLargeDisplayStyles();
    
    // Start status updates
    m_statusUpdateTimer->setInterval(1000);  // 1 second updates
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &MainWindow::updateStatusDisplay);
    m_statusUpdateTimer->start();
    
    // Show main panel by default
    showMainPanel();
    
    qDebug() << "MainWindow initialized for 50-inch display";
}

MainWindow::~MainWindow()
{
    if (m_statusUpdateTimer) {
        m_statusUpdateTimer->stop();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Confirm shutdown for safety
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Confirm Shutdown",
        "Are you sure you want to shut down the vacuum controller?\n\n"
        "This will stop all operations and shut down the system safely.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Ensure system is stopped safely
        if (m_controller && m_systemRunning) {
            m_controller->stopPattern();
        }
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle emergency stop with Escape key
    if (event->key() == Qt::Key_Escape) {
        onEmergencyStopClicked();
        return;
    }
    
    // Handle F-keys for quick navigation
    switch (event->key()) {
    case Qt::Key_F1:
        showMainPanel();
        break;
    case Qt::Key_F2:
        showSafetyPanel();
        break;
    case Qt::Key_F3:
        showSettingsDialog();
        break;
    case Qt::Key_F4:
        showDiagnosticsPanel();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Handle touch events for large display
    if (event->type() == QEvent::TouchBegin || 
        event->type() == QEvent::TouchUpdate || 
        event->type() == QEvent::TouchEnd) {
        // Touch events are automatically converted to mouse events by Qt
        return false;
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onSystemStateChanged(VacuumController::SystemState state)
{
    VacuumController::SystemState systemState = state;
    
    switch (systemState) {
    case VacuumController::STOPPED:
        m_systemRunning = false;
        m_systemPaused = false;
        m_emergencyStop = false;
        break;
    case VacuumController::RUNNING:
        m_systemRunning = true;
        m_systemPaused = false;
        m_emergencyStop = false;
        break;
    case VacuumController::PAUSED:
        m_systemRunning = true;
        m_systemPaused = true;
        m_emergencyStop = false;
        break;
    case VacuumController::EMERGENCY_STOP:
        m_systemRunning = false;
        m_systemPaused = false;
        m_emergencyStop = true;
        break;
    case VacuumController::ERROR:
        m_systemRunning = false;
        m_systemPaused = false;
        break;
    }
    
    updateControlButtons();
    updateStatusDisplay();
}

void MainWindow::onPressureUpdated(double avlPressure, double tankPressure)
{
    // Update pressure display
    if (m_pressureMonitor) {
        m_pressureMonitor->updatePressures(avlPressure, tankPressure);
    }
    
    // Update status bar
    m_pressureStatusLabel->setText(
        QString("AVL: %1 mmHg | Tank: %2 mmHg")
        .arg(avlPressure, 0, 'f', 1)
        .arg(tankPressure, 0, 'f', 1)
    );
}

void MainWindow::onEmergencyStopTriggered()
{
    m_emergencyStop = true;
    updateControlButtons();
    updateStatusDisplay();
    
    // Show emergency stop message
    QMessageBox::critical(this, "EMERGENCY STOP", 
                         "EMERGENCY STOP ACTIVATED\n\n"
                         "All operations have been stopped immediately.\n"
                         "Check system status before attempting to reset.");
}

void MainWindow::onSystemError(const QString& error)
{
    qCritical() << "System error:" << error;
    
    // Show error message
    QMessageBox::critical(this, "System Error", 
                         QString("System Error Detected:\n\n%1\n\n"
                                "Please check the system and resolve the issue.").arg(error));
    
    updateStatusDisplay();
}

void MainWindow::onAntiDetachmentActivated()
{
    // Show anti-detachment notification
    if (m_safetyPanelWidget) {
        m_safetyPanelWidget->showAntiDetachmentAlert();
    }
    
    // Update status
    m_systemStatusLabel->setText("ANTI-DETACHMENT ACTIVE");
    m_systemStatusLabel->setStyleSheet("background-color: #FFA500; color: white; font-weight: bold;");
}

void MainWindow::showMainPanel()
{
    if (m_stackedWidget) {
        m_stackedWidget->setCurrentWidget(m_mainPanel);
        
        // Update navigation buttons
        m_mainPanelButton->setStyleSheet("background-color: #2196F3; color: white;");
        m_safetyPanelButton->setStyleSheet("");
        m_settingsButton->setStyleSheet("");
        m_diagnosticsButton->setStyleSheet("");
    }
}

void MainWindow::showSafetyPanel()
{
    if (m_stackedWidget) {
        m_stackedWidget->setCurrentWidget(m_safetyPanel);
        
        // Update navigation buttons
        m_mainPanelButton->setStyleSheet("");
        m_safetyPanelButton->setStyleSheet("background-color: #2196F3; color: white;");
        m_settingsButton->setStyleSheet("");
        m_diagnosticsButton->setStyleSheet("");
    }
}

void MainWindow::showSettingsDialog()
{
    if (m_settingsDialog) {
        m_settingsDialog->exec();
    }
}

void MainWindow::showDiagnosticsPanel()
{
    if (m_stackedWidget) {
        m_stackedWidget->setCurrentWidget(m_diagnosticsPanel);
        
        // Update navigation buttons
        m_mainPanelButton->setStyleSheet("");
        m_safetyPanelButton->setStyleSheet("");
        m_settingsButton->setStyleSheet("");
        m_diagnosticsButton->setStyleSheet("background-color: #2196F3; color: white;");
    }
}

void MainWindow::onStartStopClicked()
{
    if (!m_controller) return;
    
    if (m_systemRunning) {
        m_controller->stopPattern();
    } else {
        // Get selected pattern from pattern selector
        QString selectedPattern = m_patternSelector ? m_patternSelector->getSelectedPattern() : "Slow Pulse";
        m_controller->startPattern(selectedPattern);
    }
}

void MainWindow::onPauseResumeClicked()
{
    if (!m_controller) return;
    
    if (m_systemPaused) {
        m_controller->resumePattern();
    } else if (m_systemRunning) {
        m_controller->pausePattern();
    }
}

void MainWindow::onEmergencyStopClicked()
{
    if (m_controller) {
        m_controller->emergencyStop();
    }
}

void MainWindow::onResetEmergencyStopClicked()
{
    if (m_controller && m_emergencyStop) {
        // Confirm reset
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Reset Emergency Stop",
            "Are you sure you want to reset the emergency stop?\n\n"
            "Ensure all safety conditions have been resolved.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            m_controller->resetEmergencyStop();
        }
    }
}

void MainWindow::updateStatusDisplay()
{
    // Update time display
    m_timeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    
    // Update system status
    QString statusText;
    QString statusStyle;
    
    if (m_emergencyStop) {
        statusText = "EMERGENCY STOP";
        statusStyle = "background-color: #f44336; color: white; font-weight: bold;";
    } else if (m_systemRunning && !m_systemPaused) {
        statusText = "RUNNING";
        statusStyle = "background-color: #4CAF50; color: white; font-weight: bold;";
    } else if (m_systemPaused) {
        statusText = "PAUSED";
        statusStyle = "background-color: #FF9800; color: white; font-weight: bold;";
    } else {
        statusText = "STOPPED";
        statusStyle = "background-color: #9E9E9E; color: white; font-weight: bold;";
    }
    
    m_systemStatusLabel->setText(statusText);
    m_systemStatusLabel->setStyleSheet(statusStyle);
}

void MainWindow::updateControlButtons()
{
    if (!m_startStopButton || !m_pauseResumeButton) return;
    
    // Update start/stop button
    if (m_systemRunning) {
        m_startStopButton->setText("STOP");
        m_startStopButton->setStyleSheet("background-color: #f44336; border: 2px solid #da190b;");
    } else {
        m_startStopButton->setText("START");
        m_startStopButton->setStyleSheet("background-color: #4CAF50; border: 2px solid #45a049;");
    }
    
    // Update pause/resume button
    if (m_systemPaused) {
        m_pauseResumeButton->setText("RESUME");
        m_pauseResumeButton->setEnabled(true);
    } else if (m_systemRunning) {
        m_pauseResumeButton->setText("PAUSE");
        m_pauseResumeButton->setEnabled(true);
    } else {
        m_pauseResumeButton->setText("PAUSE");
        m_pauseResumeButton->setEnabled(false);
    }
    
    // Disable controls during emergency stop
    bool controlsEnabled = !m_emergencyStop;
    m_startStopButton->setEnabled(controlsEnabled);
    m_pauseResumeButton->setEnabled(controlsEnabled && m_systemRunning);
    
    // Update emergency stop reset button
    m_resetEmergencyButton->setEnabled(m_emergencyStop);
}

void MainWindow::setupUI()
{
    // Create central widget
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);

    // Create main layout
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // Setup navigation bar
    setupNavigationBar();

    // Create stacked widget for main content
    m_stackedWidget = new QStackedWidget;
    m_mainLayout->addWidget(m_stackedWidget, 1);  // Takes most space

    // Setup main panels
    setupMainPanel();

    // Setup emergency controls
    setupEmergencyControls();

    // Setup status bar
    setupStatusBar();

    // Create specialized components
    m_pressureMonitor = std::make_unique<PressureMonitor>(m_controller);
    m_patternSelector = std::make_unique<PatternSelector>(m_controller);
    m_safetyPanelWidget = std::make_unique<SafetyPanel>(m_controller);
    m_settingsDialog = std::make_unique<SettingsDialog>(m_controller, this);
}

void MainWindow::setupMainPanel()
{
    // Create main control panel
    m_mainPanel = new QWidget;
    QHBoxLayout* mainPanelLayout = new QHBoxLayout(m_mainPanel);
    mainPanelLayout->setSpacing(20);

    // Left side - Pattern selection and controls
    QVBoxLayout* leftLayout = new QVBoxLayout;

    // Pattern selection area
    QFrame* patternFrame = new QFrame;
    patternFrame->setFrameStyle(QFrame::Box);
    patternFrame->setMinimumHeight(400);
    QVBoxLayout* patternLayout = new QVBoxLayout(patternFrame);

    QLabel* patternLabel = new QLabel("PATTERN SELECTION");
    patternLabel->setAlignment(Qt::AlignCenter);
    patternLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #2196F3;");
    patternLayout->addWidget(patternLabel);

    // Pattern selector will be added here
    patternLayout->addStretch();

    leftLayout->addWidget(patternFrame);

    // Control buttons
    QHBoxLayout* controlLayout = new QHBoxLayout;

    m_startStopButton = new QPushButton("START");
    m_startStopButton->setMinimumSize(LARGE_BUTTON_WIDTH, LARGE_BUTTON_HEIGHT);
    m_startStopButton->setStyleSheet("font-size: 18pt; font-weight: bold;");

    m_pauseResumeButton = new QPushButton("PAUSE");
    m_pauseResumeButton->setMinimumSize(LARGE_BUTTON_WIDTH, LARGE_BUTTON_HEIGHT);
    m_pauseResumeButton->setStyleSheet("font-size: 18pt; font-weight: bold;");
    m_pauseResumeButton->setEnabled(false);

    controlLayout->addWidget(m_startStopButton);
    controlLayout->addWidget(m_pauseResumeButton);
    controlLayout->addStretch();

    leftLayout->addLayout(controlLayout);

    // Right side - Pressure monitoring
    QVBoxLayout* rightLayout = new QVBoxLayout;

    QFrame* pressureFrame = new QFrame;
    pressureFrame->setFrameStyle(QFrame::Box);
    pressureFrame->setMinimumHeight(600);
    QVBoxLayout* pressureLayout = new QVBoxLayout(pressureFrame);

    QLabel* pressureLabel = new QLabel("PRESSURE MONITORING");
    pressureLabel->setAlignment(Qt::AlignCenter);
    pressureLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #2196F3;");
    pressureLayout->addWidget(pressureLabel);

    // Pressure monitor will be added here
    pressureLayout->addStretch();

    rightLayout->addWidget(pressureFrame);

    // Add layouts to main panel
    mainPanelLayout->addLayout(leftLayout, 1);
    mainPanelLayout->addLayout(rightLayout, 1);

    // Add panels to stacked widget
    m_stackedWidget->addWidget(m_mainPanel);

    // Create other panels (simplified for now)
    m_safetyPanel = new QWidget;
    QLabel* safetyLabel = new QLabel("SAFETY PANEL - Under Construction");
    safetyLabel->setAlignment(Qt::AlignCenter);
    safetyLabel->setStyleSheet("font-size: 24pt; color: #666;");
    QVBoxLayout* safetyLayout = new QVBoxLayout(m_safetyPanel);
    safetyLayout->addWidget(safetyLabel);
    m_stackedWidget->addWidget(m_safetyPanel);

    m_diagnosticsPanel = new QWidget;
    QLabel* diagLabel = new QLabel("DIAGNOSTICS PANEL - Under Construction");
    diagLabel->setAlignment(Qt::AlignCenter);
    diagLabel->setStyleSheet("font-size: 24pt; color: #666;");
    QVBoxLayout* diagLayout = new QVBoxLayout(m_diagnosticsPanel);
    diagLayout->addWidget(diagLabel);
    m_stackedWidget->addWidget(m_diagnosticsPanel);
}

void MainWindow::setupNavigationBar()
{
    m_navigationBar = new QFrame;
    m_navigationBar->setFrameStyle(QFrame::Box);
    m_navigationBar->setFixedHeight(NAVIGATION_HEIGHT);
    m_navigationBar->setStyleSheet("background-color: #f0f0f0; border: 2px solid #ddd;");

    m_navLayout = new QHBoxLayout(m_navigationBar);
    m_navLayout->setSpacing(10);

    // Navigation buttons
    m_mainPanelButton = new QPushButton("MAIN CONTROL");
    m_mainPanelButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);

    m_safetyPanelButton = new QPushButton("SAFETY");
    m_safetyPanelButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);

    m_settingsButton = new QPushButton("SETTINGS");
    m_settingsButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);

    m_diagnosticsButton = new QPushButton("DIAGNOSTICS");
    m_diagnosticsButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);

    m_navLayout->addWidget(m_mainPanelButton);
    m_navLayout->addWidget(m_safetyPanelButton);
    m_navLayout->addWidget(m_settingsButton);
    m_navLayout->addWidget(m_diagnosticsButton);
    m_navLayout->addStretch();

    m_mainLayout->addWidget(m_navigationBar);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new QFrame;
    m_statusBar->setFrameStyle(QFrame::Box);
    m_statusBar->setFixedHeight(STATUS_BAR_HEIGHT);
    m_statusBar->setStyleSheet("background-color: #f8f8f8; border: 2px solid #ddd;");

    m_statusLayout = new QHBoxLayout(m_statusBar);
    m_statusLayout->setSpacing(20);

    // System status
    m_systemStatusLabel = new QLabel("STOPPED");
    m_systemStatusLabel->setMinimumSize(200, 50);
    m_systemStatusLabel->setAlignment(Qt::AlignCenter);
    m_systemStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; padding: 10px; border-radius: 5px;");

    // Pressure status
    m_pressureStatusLabel = new QLabel("AVL: -- mmHg | Tank: -- mmHg");
    m_pressureStatusLabel->setStyleSheet("font-size: 14pt; color: #333;");

    // Time display
    m_timeLabel = new QLabel;
    m_timeLabel->setStyleSheet("font-size: 14pt; color: #333;");

    m_statusLayout->addWidget(m_systemStatusLabel);
    m_statusLayout->addWidget(m_pressureStatusLabel);
    m_statusLayout->addStretch();
    m_statusLayout->addWidget(m_timeLabel);

    m_mainLayout->addWidget(m_statusBar);
}

void MainWindow::setupEmergencyControls()
{
    m_emergencyFrame = new QFrame;
    m_emergencyFrame->setFrameStyle(QFrame::Box);
    m_emergencyFrame->setStyleSheet("background-color: #ffebee; border: 3px solid #f44336;");

    QHBoxLayout* emergencyLayout = new QHBoxLayout(m_emergencyFrame);
    emergencyLayout->setSpacing(20);

    QLabel* emergencyLabel = new QLabel("EMERGENCY CONTROLS");
    emergencyLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #f44336;");

    m_emergencyStopButton = new QPushButton("EMERGENCY STOP");
    m_emergencyStopButton->setMinimumSize(EMERGENCY_BUTTON_SIZE, EMERGENCY_BUTTON_SIZE);
    m_emergencyStopButton->setStyleSheet(
        "background-color: #f44336; color: white; font-size: 16pt; font-weight: bold; "
        "border: 3px solid #da190b; border-radius: 10px;"
    );

    m_resetEmergencyButton = new QPushButton("RESET EMERGENCY");
    m_resetEmergencyButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_resetEmergencyButton->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_resetEmergencyButton->setEnabled(false);

    emergencyLayout->addWidget(emergencyLabel);
    emergencyLayout->addStretch();
    emergencyLayout->addWidget(m_emergencyStopButton);
    emergencyLayout->addWidget(m_resetEmergencyButton);

    m_mainLayout->addWidget(m_emergencyFrame);
}

void MainWindow::connectSignals()
{
    if (!m_controller) return;

    // Connect controller signals
    connect(m_controller, &VacuumController::systemStateChanged,
            this, &MainWindow::onSystemStateChanged);
    connect(m_controller, &VacuumController::pressureUpdated,
            this, &MainWindow::onPressureUpdated);
    connect(m_controller, &VacuumController::emergencyStopTriggered,
            this, &MainWindow::onEmergencyStopTriggered);
    connect(m_controller, &VacuumController::systemError,
            this, &MainWindow::onSystemError);
    connect(m_controller, &VacuumController::antiDetachmentActivated,
            this, &MainWindow::onAntiDetachmentActivated);

    // Connect navigation buttons
    connect(m_mainPanelButton, &QPushButton::clicked, this, &MainWindow::showMainPanel);
    connect(m_safetyPanelButton, &QPushButton::clicked, this, &MainWindow::showSafetyPanel);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
    connect(m_diagnosticsButton, &QPushButton::clicked, this, &MainWindow::showDiagnosticsPanel);

    // Connect control buttons
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(m_pauseResumeButton, &QPushButton::clicked, this, &MainWindow::onPauseResumeClicked);
    connect(m_emergencyStopButton, &QPushButton::clicked, this, &MainWindow::onEmergencyStopClicked);
    connect(m_resetEmergencyButton, &QPushButton::clicked, this, &MainWindow::onResetEmergencyStopClicked);
}

void MainWindow::applyLargeDisplayStyles()
{
    // Set application-wide font for large display
    QFont appFont = QApplication::font();
    appFont.setPointSize(FONT_SIZE_NORMAL);
    QApplication::setFont(appFont);

    // Apply touch-friendly styles
    setStyleSheet(
        "QPushButton { "
        "  min-height: 60px; "
        "  min-width: 120px; "
        "  font-size: 16pt; "
        "  padding: 10px; "
        "  border-radius: 8px; "
        "} "
        "QLabel { "
        "  font-size: 14pt; "
        "} "
        "QFrame { "
        "  border-radius: 5px; "
        "}"
    );
}
