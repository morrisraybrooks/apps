#include "MainWindow.h"
#include "PressureMonitor.h"
#include "PatternSelector.h"
#include "SafetyPanel.h"
#include "SettingsDialog.h"
#include "SystemDiagnosticsPanel.h"
#include "styles/ModernMedicalStyle.h"
#include "../VacuumController.h"

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QWindowStateChangeEvent>
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
    
    // Set window properties with standard title bar
    setWindowTitle("Vacuum Controller - Medical Device Interface");

    // Force normal window behavior with decorations
    setWindowFlags(Qt::Widget);  // Reset to default
    setWindowFlags(Qt::Window);  // Set as top-level window

    // Set size properties
    setMinimumSize(800, 600);   // Reasonable minimum size
    resize(1200, 800);          // Default size - user can resize

    // Ensure window is resizable and has decorations
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Show the window to ensure it gets proper decorations
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    
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

void MainWindow::changeEvent(QEvent *event)
{
    // Handle window state changes (minimize, maximize, etc.)
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *stateEvent = static_cast<QWindowStateChangeEvent*>(event);

        if (windowState() & Qt::WindowMinimized) {
            qDebug() << "Window minimized";
            // Optionally pause operations when minimized
        } else if (windowState() & Qt::WindowMaximized) {
            qDebug() << "Window maximized";
        } else if (windowState() == Qt::WindowNoState) {
            qDebug() << "Window restored to normal state";
        }
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    // Handle window resize events
    QSize newSize = event->size();
    qDebug() << "Window resized to:" << newSize.width() << "x" << newSize.height();

    // Ensure minimum size constraints
    if (newSize.width() < 800 || newSize.height() < 600) {
        resize(qMax(800, newSize.width()), qMax(600, newSize.height()));
    }

    QMainWindow::resizeEvent(event);
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
    if (m_stackedWidget && m_safetyPanelWidget) {
        m_stackedWidget->setCurrentWidget(m_safetyPanelWidget.get());
        
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
    if (m_stackedWidget && m_diagnosticsPanelWidget) {
        m_stackedWidget->setCurrentWidget(m_diagnosticsPanelWidget.get());
        
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
        PatternSelector::PatternInfo selectedPatternInfo = m_patternSelector ? m_patternSelector->getSelectedPatternInfo() : PatternSelector::PatternInfo();
        if (!selectedPatternInfo.name.isEmpty()) {
            m_controller->startPattern(selectedPatternInfo.name, selectedPatternInfo.parameters);
        } else {
            QMessageBox::warning(this, "Pattern Selection Error", "No vacuum pattern selected. Please select a pattern to start.");
        }
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

    // Create specialized components FIRST before setting up layouts
    m_pressureMonitor = std::make_unique<PressureMonitor>(m_controller);
    m_patternSelector = std::make_unique<PatternSelector>(m_controller, this);
    m_safetyPanelWidget = std::make_unique<SafetyPanel>(m_controller);
    m_settingsDialog = std::make_unique<SettingsDialog>(m_controller, this);
    m_diagnosticsPanelWidget = std::make_unique<SystemDiagnosticsPanel>(m_controller);

    // Setup navigation bar
    setupNavigationBar();

    // Create stacked widget for main content
    m_stackedWidget = new QStackedWidget;
    m_mainLayout->addWidget(m_stackedWidget, 1);  // Takes most space

    // Setup main panels (now that components exist)
    setupMainPanel();

    // Setup emergency controls
    setupEmergencyControls();

    // Setup status bar
    setupStatusBar();
}

void MainWindow::setupMainPanel()
{
    // Create main control panel
    m_mainPanel = new QWidget;
    QHBoxLayout* mainPanelLayout = new QHBoxLayout(m_mainPanel);
    mainPanelLayout->setSpacing(20);
    mainPanelLayout->setContentsMargins(10, 10, 10, 10);

    // Left side - Pattern selection and controls
    QVBoxLayout* leftLayout = new QVBoxLayout;

    // Pattern selection area - make it the main focus
    QFrame* patternFrame = new QFrame;
    patternFrame->setFrameStyle(QFrame::Box);
    patternFrame->setMinimumHeight(600); // Increased height
    patternFrame->setStyleSheet("QFrame { border: 2px solid #2196F3; border-radius: 10px; background-color: #f8f9fa; }");
    QVBoxLayout* patternLayout = new QVBoxLayout(patternFrame);
    patternLayout->setSpacing(10);
    patternLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* patternLabel = new QLabel("VACUUM CYCLE SELECTION");
    patternLabel->setAlignment(Qt::AlignCenter);
    patternLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #2196F3; margin: 10px;");
    patternLayout->addWidget(patternLabel);

    // Add pattern selector - properly integrate it
    if (m_patternSelector) {
        // Don't reparent, just add to layout
        patternLayout->addWidget(m_patternSelector.get(), 1); // Give it more stretch factor
    }

    leftLayout->addWidget(patternFrame, 2); // Give pattern frame more space

    // Control buttons
    QHBoxLayout* controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(15);

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
    pressureFrame->setStyleSheet("QFrame { border: 2px solid #2196F3; border-radius: 10px; background-color: #f8f9fa; }");
    QVBoxLayout* pressureLayout = new QVBoxLayout(pressureFrame);
    pressureLayout->setSpacing(10);
    pressureLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* pressureLabel = new QLabel("PRESSURE MONITORING");
    pressureLabel->setAlignment(Qt::AlignCenter);
    pressureLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #2196F3;");
    pressureLayout->addWidget(pressureLabel);

    // Add pressure monitor
    if (m_pressureMonitor) {
        pressureLayout->addWidget(m_pressureMonitor.get());
    }
    pressureLayout->addStretch();

    rightLayout->addWidget(pressureFrame);

    // Add layouts to main panel
    mainPanelLayout->addLayout(leftLayout, 1);
    mainPanelLayout->addLayout(rightLayout, 1);

    // Add panels to stacked widget
    m_stackedWidget->addWidget(m_mainPanel);

    // Create other panels
    if (m_safetyPanelWidget) {
        m_stackedWidget->addWidget(m_safetyPanelWidget.get());
    } else {
        QWidget* safetyPanel = new QWidget;
        QLabel* safetyLabel = new QLabel("SAFETY PANEL - Error");
        safetyLabel->setAlignment(Qt::AlignCenter);
        safetyLabel->setStyleSheet("font-size: 24pt; color: #f44336;");
        QVBoxLayout* safetyLayout = new QVBoxLayout(safetyPanel);
        safetyLayout->addWidget(safetyLabel);
        m_stackedWidget->addWidget(safetyPanel);
    }

    if (m_diagnosticsPanelWidget) {
        m_stackedWidget->addWidget(m_diagnosticsPanelWidget.get());
    } else {
        QWidget* diagnosticsPanel = new QWidget;
        QLabel* diagLabel = new QLabel("DIAGNOSTICS PANEL - Error");
        diagLabel->setAlignment(Qt::AlignCenter);
        diagLabel->setStyleSheet("font-size: 24pt; color: #f44336;");
        QVBoxLayout* diagLayout = new QVBoxLayout(diagnosticsPanel);
        diagLayout->addWidget(diagLabel);
        m_stackedWidget->addWidget(diagnosticsPanel);
    }
}

void MainWindow::setupNavigationBar()
{
    m_navigationBar = new QFrame;
    m_navigationBar->setFrameStyle(QFrame::Box);
    m_navigationBar->setFixedHeight(NAVIGATION_HEIGHT);
    m_navigationBar->setStyleSheet("background-color: #f0f0f0; border: 2px solid #ddd; border-radius: 5px;");

    m_navLayout = new QHBoxLayout(m_navigationBar);
    m_navLayout->setSpacing(15);
    m_navLayout->setContentsMargins(10, 5, 10, 5);

    // Navigation buttons with consistent styling
    m_mainPanelButton = new QPushButton("MAIN CONTROL");
    m_mainPanelButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_mainPanelButton->setStyleSheet("QPushButton { font-size: 14pt; font-weight: bold; border-radius: 5px; }");

    m_safetyPanelButton = new QPushButton("SAFETY");
    m_safetyPanelButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_safetyPanelButton->setStyleSheet("QPushButton { font-size: 14pt; font-weight: bold; border-radius: 5px; }");

    m_settingsButton = new QPushButton("SETTINGS");
    m_settingsButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_settingsButton->setStyleSheet("QPushButton { font-size: 14pt; font-weight: bold; border-radius: 5px; }");

    m_diagnosticsButton = new QPushButton("DIAGNOSTICS");
    m_diagnosticsButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_diagnosticsButton->setStyleSheet("QPushButton { font-size: 14pt; font-weight: bold; border-radius: 5px; }");

    m_shutdownButton = new QPushButton("SHUTDOWN");
    m_shutdownButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_shutdownButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-size: 14pt; font-weight: bold; border-radius: 5px; }");

    // Add buttons with proper alignment
    m_navLayout->addWidget(m_mainPanelButton);
    m_navLayout->addWidget(m_safetyPanelButton);
    m_navLayout->addWidget(m_settingsButton);
    m_navLayout->addWidget(m_diagnosticsButton);
    m_navLayout->addStretch(); // Push shutdown button to the right
    m_navLayout->addWidget(m_shutdownButton);

    m_mainLayout->addWidget(m_navigationBar);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new QFrame;
    m_statusBar->setFrameStyle(QFrame::Box);
    m_statusBar->setFixedHeight(STATUS_BAR_HEIGHT);
    m_statusBar->setStyleSheet("background-color: #f8f8f8; border: 2px solid #ddd; border-radius: 5px;");

    m_statusLayout = new QHBoxLayout(m_statusBar);
    m_statusLayout->setSpacing(20);
    m_statusLayout->setContentsMargins(15, 5, 15, 5);

    // System status with proper alignment
    m_systemStatusLabel = new QLabel("STOPPED");
    m_systemStatusLabel->setMinimumSize(200, 50);
    m_systemStatusLabel->setAlignment(Qt::AlignCenter);
    m_systemStatusLabel->setStyleSheet("font-size: 16pt; font-weight: bold; padding: 10px; border-radius: 5px; background-color: #9E9E9E; color: white;");

    // Pressure status with consistent styling
    m_pressureStatusLabel = new QLabel("AVL: -- mmHg | Tank: -- mmHg");
    m_pressureStatusLabel->setStyleSheet("font-size: 14pt; color: #333; font-weight: bold;");
    m_pressureStatusLabel->setAlignment(Qt::AlignCenter);

    // Time display with consistent styling
    m_timeLabel = new QLabel;
    m_timeLabel->setStyleSheet("font-size: 14pt; color: #333; font-weight: bold;");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setMinimumWidth(120);

    // Add widgets with proper spacing
    m_statusLayout->addWidget(m_systemStatusLabel);
    m_statusLayout->addWidget(m_pressureStatusLabel, 1); // Give it more space
    m_statusLayout->addStretch();
    m_statusLayout->addWidget(m_timeLabel);

    m_mainLayout->addWidget(m_statusBar);
}

void MainWindow::setupEmergencyControls()
{
    m_emergencyFrame = new QFrame;
    m_emergencyFrame->setFrameStyle(QFrame::Box);
    m_emergencyFrame->setStyleSheet("background-color: #ffebee; border: 3px solid #f44336; border-radius: 10px;");
    m_emergencyFrame->setFixedHeight(100); // Fixed height for consistent layout

    QHBoxLayout* emergencyLayout = new QHBoxLayout(m_emergencyFrame);
    emergencyLayout->setSpacing(20);
    emergencyLayout->setContentsMargins(15, 10, 15, 10);

    QLabel* emergencyLabel = new QLabel("EMERGENCY CONTROLS");
    emergencyLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #f44336;");
    emergencyLabel->setAlignment(Qt::AlignVCenter);

    m_emergencyStopButton = new QPushButton("EMERGENCY\nSTOP");
    m_emergencyStopButton->setMinimumSize(EMERGENCY_BUTTON_SIZE, EMERGENCY_BUTTON_SIZE);
    m_emergencyStopButton->setMaximumSize(EMERGENCY_BUTTON_SIZE, EMERGENCY_BUTTON_SIZE);
    m_emergencyStopButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #f44336; color: white; font-size: 14pt; font-weight: bold;"
        "  border: 3px solid #da190b; border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #da190b;"
        "}"
    );

    m_resetEmergencyButton = new QPushButton("RESET\nEMERGENCY");
    m_resetEmergencyButton->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_resetEmergencyButton->setStyleSheet(
        "QPushButton {"
        "  font-size: 12pt; font-weight: bold; border-radius: 5px;"
        "  background-color: #4CAF50; color: white;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc; color: #666666;"
        "}"
    );
    m_resetEmergencyButton->setEnabled(false);

    // Add widgets with proper alignment
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
    connect(m_shutdownButton, &QPushButton::clicked, this, &MainWindow::close);

    // Connect control buttons
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(m_pauseResumeButton, &QPushButton::clicked, this, &MainWindow::onPauseResumeClicked);
    connect(m_emergencyStopButton, &QPushButton::clicked, this, &MainWindow::onEmergencyStopClicked);
    connect(m_resetEmergencyButton, &QPushButton::clicked, this, &MainWindow::onResetEmergencyStopClicked);
}

void MainWindow::applyLargeDisplayStyles()
{
    // Apply modern medical device styling to the main window
    ModernMedicalStyle::applyToWidget(this);

    // Set enhanced styling for main window components
    QString mainWindowStyle = QString(
        "MainWindow {"
        "    background-color: %1;"
        "    color: %2;"
        "}"
        "QStackedWidget {"
        "    background-color: %3;"
        "    border: none;"
        "}"
        "/* Navigation Bar Styling */"
        "QFrame#navigationBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %4, stop:1 %5);"
        "    border-bottom: %6 solid %7;"
        "    min-height: %8;"
        "    %9"
        "}"
        "/* Status Bar Styling */"
        "QFrame#statusBar {"
        "    background-color: %10;"
        "    border-top: %11 solid %12;"
        "    min-height: %13;"
        "    %14"
        "}"
        "/* Emergency Controls Styling */"
        "QFrame#emergencyFrame {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
        "                fx:0.3, fy:0.3, stop:0 %15, stop:1 %16);"
        "    border: %17 solid %18;"
        "    border-radius: %19;"
        "    %20"
        "}"
        "/* Enhanced button styling for navigation */"
        "QPushButton#navButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %21, stop:1 %22);"
        "    border: %23 solid %24;"
        "    border-radius: %25;"
        "    color: %26;"
        "    font-size: %27pt;"
        "    font-weight: %28;"
        "    min-height: %29;"
        "    min-width: %30;"
        "    padding: %31 %32;"
        "    %33"
        "}"
        "QPushButton#navButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %34, stop:1 %35);"
        "}"
        "QPushButton#navButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %36, stop:1 %37);"
        "}"
        "QPushButton#navButton:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %38, stop:1 %39);"
        "    border-color: %40;"
        "}"
    ).arg(ModernMedicalStyle::Colors::BACKGROUND_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::TEXT_PRIMARY.name())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(NAVIGATION_HEIGHT))
     .arg(ModernMedicalStyle::Elevation::getLevel3())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_MEDIUM.name())
     .arg(ModernMedicalStyle::scalePixelValue(1))
     .arg(ModernMedicalStyle::Colors::BORDER_LIGHT.name())
     .arg(ModernMedicalStyle::scalePixelValue(STATUS_BAR_HEIGHT))
     .arg(ModernMedicalStyle::Elevation::getLevel1())
     .arg(ModernMedicalStyle::adjustColorForContrast(ModernMedicalStyle::Colors::MEDICAL_RED, 0.2).name())
     .arg(ModernMedicalStyle::Colors::MEDICAL_RED.name())
     .arg(ModernMedicalStyle::scalePixelValue(3))
     .arg(ModernMedicalStyle::Colors::MEDICAL_RED.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getLargeRadius()))
     .arg(ModernMedicalStyle::Elevation::getLevel4())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_MEDIUM.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::BORDER_MEDIUM.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMediumRadius()))
     .arg(ModernMedicalStyle::Colors::TEXT_PRIMARY.name())
     .arg(ModernMedicalStyle::Typography::getSubtitle())
     .arg(ModernMedicalStyle::Typography::WEIGHT_MEDIUM)
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getLargeTouchTarget()))
     .arg(ModernMedicalStyle::scalePixelValue(150))
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMedium()))
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getLarge()))
     .arg(ModernMedicalStyle::Elevation::getLevel2())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_MEDIUM.name())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_DARK.name())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_DARK.name())
     .arg(ModernMedicalStyle::Colors::BORDER_DARK.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name());

    setStyleSheet(mainWindowStyle);
}
