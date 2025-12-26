#include "MainWindow.h"
#include "PressureMonitor.h"
#include "ArousalMonitor.h"
#include "PatternSelector.h"
#include "SafetyPanel.h"
#include "SettingsPanel.h"
#include "SystemDiagnosticsPanel.h"
#include "CustomPatternEditor.h"
#include "ExecutionModeSelector.h"
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
#include <QScrollArea>
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
    
    // Set window properties for 50-inch medical display
    setWindowTitle("Vacuum Controller - Professional Medical Device Interface");

    // Configure for large display with window decorations
    setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    // Set reasonable size constraints for large display
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        qDebug() << "Screen size available:" << screenGeometry.size();

        // Set window to use most of the screen but leave room for window decorations
        int windowWidth = screenGeometry.width() - 100;  // Leave some margin
        int windowHeight = screenGeometry.height() - 100; // Leave room for title bar and taskbar

        setMinimumSize(1200, 800);  // Reasonable minimum
        resize(windowWidth, windowHeight);

        // Center the window
        move((screenGeometry.width() - windowWidth) / 2,
             (screenGeometry.height() - windowHeight) / 2);
    }

    // Start maximized but with window decorations visible
    showMaximized();

    // Ensure proper sizing policy for large displays
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

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
        showSettingsPanel();
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
    // Handle window resize events for full-screen medical display
    QSize newSize = event->size();
    qDebug() << "Window resized to:" << newSize.width() << "x" << newSize.height();

    // For 50-inch displays, we want to use the full screen
    // No artificial size constraints - let it use the full display

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
        updateNavigationHighlight(m_mainPanelButton);
    }
}

void MainWindow::showSafetyPanel()
{
    if (m_stackedWidget && m_safetyPanelWidget) {
        m_stackedWidget->setCurrentWidget(m_safetyPanelWidget.get());
        updateNavigationHighlight(m_safetyPanelButton);
    }
}

void MainWindow::showSettingsPanel()
{
    if (m_settingsPanelWidget) {
        m_stackedWidget->setCurrentWidget(m_settingsPanelWidget.get());
        updateNavigationHighlight(m_settingsButton);
    }
}

void MainWindow::showDiagnosticsPanel()
{
    if (m_stackedWidget && m_diagnosticsPanelWidget) {
        m_stackedWidget->setCurrentWidget(m_diagnosticsPanelWidget.get());
        updateNavigationHighlight(m_diagnosticsButton);
    }
}

void MainWindow::showPatternEditor()
{
    if (m_stackedWidget && m_customPatternEditor) {
        m_stackedWidget->setCurrentWidget(m_customPatternEditor.get());

        // Clear all navigation button highlights (pattern editor is not in main nav)
        updateNavigationHighlight(nullptr);

        // Show the editor for creating a new pattern
        m_customPatternEditor->createNewPattern();
        m_customPatternEditor->showEditor();
    }
}

void MainWindow::showPatternEditor(const QString& patternName)
{
    if (m_stackedWidget && m_customPatternEditor) {
        m_stackedWidget->setCurrentWidget(m_customPatternEditor.get());

        // Clear all navigation button highlights (pattern editor is not in main nav)
        updateNavigationHighlight(nullptr);

        // Show the editor with the specified pattern for editing
        if (!patternName.isEmpty()) {
            m_customPatternEditor->loadPattern(patternName);
        } else {
            m_customPatternEditor->createNewPattern();
        }
        m_customPatternEditor->showEditor();
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
    if (!m_controller) return;

    if (m_emergencyStop) {
        // Currently in emergency stop - handle reset
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
    } else {
        // Normal operation - trigger emergency stop
        m_controller->emergencyStop();
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

    // Update shutdown button text based on emergency state
    if (m_emergencyStop) {
        m_shutdownButton->setText("RESET EMERGENCY");
        m_shutdownButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("success"));
    } else {
        m_shutdownButton->setText("EMERGENCY STOP");
        m_shutdownButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("danger"));
    }
}

void MainWindow::updateNavigationHighlight(QPushButton* activeButton)
{
    // Define navigation buttons and their styles
    QList<QPushButton*> navButtons = {
        m_mainPanelButton, m_safetyPanelButton, m_settingsButton, m_diagnosticsButton
    };

    const QString activeStyle = "background-color: #2196F3; color: white;";
    const QString inactiveStyle = "";

    for (QPushButton* button : navButtons) {
        if (button) {
            button->setStyleSheet(button == activeButton ? activeStyle : inactiveStyle);
        }
    }
}

void MainWindow::setupUI()
{
    // Create central widget
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);

    // Create main layout with generous spacing for 50-inch display
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(ModernMedicalStyle::Spacing::getXLarge());
    m_mainLayout->setContentsMargins(
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getLarge(),
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getLarge()
    );

    // Create specialized components FIRST before setting up layouts
    m_pressureMonitor = std::make_unique<PressureMonitor>(m_controller);
    m_patternSelector = std::make_unique<PatternSelector>(m_controller, this);
    m_safetyPanelWidget = std::make_unique<SafetyPanel>(m_controller);
    m_settingsPanelWidget = std::make_unique<SettingsPanel>(m_controller, this);
    m_diagnosticsPanelWidget = std::make_unique<SystemDiagnosticsPanel>(m_controller);
    m_customPatternEditor = std::make_unique<CustomPatternEditor>(m_controller, this);
    m_executionModeSelector = std::make_unique<ExecutionModeSelector>(m_controller, this);

    // Setup navigation bar
    setupNavigationBar();

    // Create stacked widget for main content with scroll support
    m_stackedWidget = new QStackedWidget;

    // Wrap stacked widget in scroll area for better space utilization
    QScrollArea* mainScrollArea = new QScrollArea;
    mainScrollArea->setWidget(m_stackedWidget);
    mainScrollArea->setWidgetResizable(true);
    mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainScrollArea->setFrameStyle(QFrame::NoFrame);

    m_mainLayout->addWidget(mainScrollArea, 1);  // Takes most space

    // Setup main panels (now that components exist)
    setupMainPanel();

    // Emergency controls are now only in the navigation bar

    // Setup status bar
    setupStatusBar();
}

void MainWindow::setupMainPanel()
{
    // Create main control panel with modern dashboard design
    m_mainPanel = new QWidget;

    // Use a grid layout for modern dashboard appearance
    QGridLayout* dashboardLayout = new QGridLayout(m_mainPanel);
    dashboardLayout->setSpacing(ModernMedicalStyle::Spacing::getXXLarge());
    dashboardLayout->setContentsMargins(
        ModernMedicalStyle::Spacing::getXXLarge(),
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getXXLarge(),
        ModernMedicalStyle::Spacing::getXLarge()
    );

    // Create large dashboard cards
    setupPatternSelectionCard(dashboardLayout);
    setupPressureMonitoringCard(dashboardLayout);
    setupArousalMonitoringCard(dashboardLayout);
    setupControlPanelCard(dashboardLayout);
    setupStatusCard(dashboardLayout);

    // Add Execution Mode Selector card
    if (m_executionModeSelector) {
        QFrame* modeCard = createDashboardCard("EXECUTION MODE", m_executionModeSelector.get());
        dashboardLayout->addWidget(modeCard, 0, 2, 2, 1);  // Right side, span 2 rows
    }

    // Set column and row stretch factors to make the layout expand properly
    dashboardLayout->setColumnStretch(0, 2);  // Left column (patterns) gets more space
    dashboardLayout->setColumnStretch(1, 1);  // Middle column gets less space
    dashboardLayout->setColumnStretch(2, 2);  // Right column (mode selector) gets more space
    dashboardLayout->setRowStretch(0, 2);     // Top row gets more space
    dashboardLayout->setRowStretch(1, 2);     // Middle row gets more space
    dashboardLayout->setRowStretch(2, 1);     // Bottom row gets less space

    // Add the main panel to stacked widget
    m_stackedWidget->addWidget(m_mainPanel);

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

    if (m_settingsPanelWidget) {
        m_stackedWidget->addWidget(m_settingsPanelWidget.get());
    } else {
        QWidget* settingsPanel = new QWidget;
        QLabel* settingsLabel = new QLabel("SETTINGS PANEL - Error");
        settingsLabel->setAlignment(Qt::AlignCenter);
        settingsLabel->setStyleSheet("font-size: 24pt; color: #f44336;");
        QVBoxLayout* settingsLayout = new QVBoxLayout(settingsPanel);
        settingsLayout->addWidget(settingsLabel);
        m_stackedWidget->addWidget(settingsPanel);
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

    // Add custom pattern editor
    if (m_customPatternEditor) {
        m_stackedWidget->addWidget(m_customPatternEditor.get());
    }
}

void MainWindow::setupNavigationBar()
{
    m_navigationBar = new QFrame;
    m_navigationBar->setFixedHeight(ModernMedicalStyle::scaleValue(150));

    // Modern navigation bar styling
    QString navStyle = QString(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %1, stop:1 %2);"
        "    border: %3 solid %4;"
        "    border-radius: %5;"
        "    %6"
        "}"
    ).arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(3))
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_DARK.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getLargeRadius()))
     .arg(ModernMedicalStyle::Elevation::getLevel4());

    m_navigationBar->setStyleSheet(navStyle);

    m_navLayout = new QHBoxLayout(m_navigationBar);
    m_navLayout->setSpacing(ModernMedicalStyle::Spacing::getXLarge());
    m_navLayout->setContentsMargins(
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getMedium(),
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getMedium()
    );

    // Large navigation buttons for 50-inch display
    m_mainPanelButton = new QPushButton("MAIN CONTROL");
    m_mainPanelButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(300),
        ModernMedicalStyle::scaleValue(120)
    );
    m_mainPanelButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("primary"));

    m_safetyPanelButton = new QPushButton("SAFETY PANEL");
    m_safetyPanelButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(300),
        ModernMedicalStyle::scaleValue(120)
    );
    m_safetyPanelButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("warning"));

    m_settingsButton = new QPushButton("SETTINGS");
    m_settingsButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(300),
        ModernMedicalStyle::scaleValue(120)
    );
    m_settingsButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("secondary"));

    m_diagnosticsButton = new QPushButton("DIAGNOSTICS");
    m_diagnosticsButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(300),
        ModernMedicalStyle::scaleValue(120)
    );
    m_diagnosticsButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("secondary"));

    m_shutdownButton = new QPushButton("EMERGENCY STOP");
    m_shutdownButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(350),
        ModernMedicalStyle::scaleValue(120)
    );
    m_shutdownButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("danger"));

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
    m_statusBar->setFixedHeight(ModernMedicalStyle::scaleValue(120));

    // Modern status bar styling
    QString statusStyle = QString(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %1, stop:1 %2);"
        "    border: %3 solid %4;"
        "    border-radius: %5;"
        "    %6"
        "}"
    ).arg(ModernMedicalStyle::Colors::BACKGROUND_MEDIUM.name())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_DARK.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::BORDER_MEDIUM.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMediumRadius()))
     .arg(ModernMedicalStyle::Elevation::getLevel2());

    m_statusBar->setStyleSheet(statusStyle);

    m_statusLayout = new QHBoxLayout(m_statusBar);
    m_statusLayout->setSpacing(ModernMedicalStyle::Spacing::getXXLarge());
    m_statusLayout->setContentsMargins(
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getMedium(),
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getMedium()
    );

    // Large system status display
    m_systemStatusLabel = new QLabel("SYSTEM: READY");
    m_systemStatusLabel->setMinimumSize(
        ModernMedicalStyle::scaleValue(300),
        ModernMedicalStyle::scaleValue(80)
    );
    m_systemStatusLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-family: %1;"
        "    font-size: %2pt;"
        "    font-weight: %3;"
        "    color: %4;"
        "    background-color: %5;"
        "    border: %6 solid %7;"
        "    border-radius: %8;"
        "    padding: %9;"
        "}"
    ).arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
     .arg(ModernMedicalStyle::Typography::getTitle())
     .arg(ModernMedicalStyle::Typography::WEIGHT_BOLD)
     .arg(ModernMedicalStyle::Colors::TEXT_ON_PRIMARY.name())
     .arg(ModernMedicalStyle::Colors::MEDICAL_GREEN.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::MEDICAL_GREEN.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMediumRadius()))
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMedium())));
    m_systemStatusLabel->setAlignment(Qt::AlignCenter);

    // Large pressure status display
    m_pressureStatusLabel = new QLabel("AVL: -- mmHg | Tank: -- mmHg");
    m_pressureStatusLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-family: %1;"
        "    font-size: %2pt;"
        "    font-weight: %3;"
        "    color: %4;"
        "    background: transparent;"
        "    border: none;"
        "}"
    ).arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
     .arg(ModernMedicalStyle::Typography::getSubtitle())
     .arg(ModernMedicalStyle::Typography::WEIGHT_MEDIUM)
     .arg(ModernMedicalStyle::Colors::TEXT_PRIMARY.name()));
    m_pressureStatusLabel->setAlignment(Qt::AlignCenter);

    // Large time display
    m_timeLabel = new QLabel;
    m_timeLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-family: %1;"
        "    font-size: %2pt;"
        "    font-weight: %3;"
        "    color: %4;"
        "    background: transparent;"
        "    border: none;"
        "}"
    ).arg(ModernMedicalStyle::Typography::MONOSPACE_FONT)
     .arg(ModernMedicalStyle::Typography::getSubtitle())
     .arg(ModernMedicalStyle::Typography::WEIGHT_MEDIUM)
     .arg(ModernMedicalStyle::Colors::TEXT_SECONDARY.name()));
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setMinimumWidth(ModernMedicalStyle::scaleValue(200));

    // Add widgets with proper spacing
    m_statusLayout->addWidget(m_systemStatusLabel);
    m_statusLayout->addWidget(m_pressureStatusLabel, 1); // Give it more space
    m_statusLayout->addStretch();
    m_statusLayout->addWidget(m_timeLabel);

    m_mainLayout->addWidget(m_statusBar);
}

// Emergency controls removed - now only in navigation bar

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
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::showSettingsPanel);
    connect(m_diagnosticsButton, &QPushButton::clicked, this, &MainWindow::showDiagnosticsPanel);
    connect(m_shutdownButton, &QPushButton::clicked, this, &MainWindow::close);

    // Connect control buttons
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(m_pauseResumeButton, &QPushButton::clicked, this, &MainWindow::onPauseResumeClicked);

    // Connect navigation emergency shutdown button
    connect(m_shutdownButton, &QPushButton::clicked, this, &MainWindow::onEmergencyStopClicked);

    // Connect custom pattern editor signals
    if (m_customPatternEditor) {
        connect(m_customPatternEditor.get(), &CustomPatternEditor::backToPatternSelector,
                this, &MainWindow::showMainPanel);
        connect(m_customPatternEditor.get(), &CustomPatternEditor::editorClosed,
                this, &MainWindow::showMainPanel);

        // Connect pattern creation/modification signals to pattern selector
        if (m_patternSelector) {
            connect(m_customPatternEditor.get(), &CustomPatternEditor::patternCreated,
                    m_patternSelector.get(), &PatternSelector::onPatternCreated);
            connect(m_customPatternEditor.get(), &CustomPatternEditor::patternModified,
                    m_patternSelector.get(), &PatternSelector::onPatternModified);
        }
    }

    // Connect pattern selector signals
    if (m_patternSelector) {
        connect(m_patternSelector.get(), &PatternSelector::patternEditorRequested,
                this, QOverload<const QString&>::of(&MainWindow::showPatternEditor));
    }
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
    ).arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scaleValue(150))
     .arg(ModernMedicalStyle::Elevation::getLevel3());

    // Set enhanced styling for main window components
    mainWindowStyle += QString(
        "/* Status Bar Styling */"
        "QFrame#statusBar {"
        "    background-color: %1;"
        "    border-top: %2 solid %3;"
        "    min-height: %4;"
        "    %5"
        "}"
        "/* Emergency Controls Styling */"
        "QFrame#emergencyFrame {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
        "                fx:0.3, fy:0.3, stop:0 %6, stop:1 %7);"
        "    border: %8 solid %9;"
        "    border-radius: %10;"
        "    %11"
        "}"
        "/* Enhanced button styling for navigation */"
        "QPushButton#navButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %12, stop:1 %13);"
        "    border: %14 solid %15;"
        "    border-radius: %16;"
        "    color: %17;"
        "    font-size: %18pt;"
        "    font-weight: %19;"
        "    min-height: %20;"
        "    min-width: %21;"
        "    padding: %22 %23;"
        "    %24"
        "}"
        "QPushButton#navButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %25, stop:1 %26);"
        "}"
        "QPushButton#navButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %27, stop:1 %28);"
        "}"
        "QPushButton#navButton:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %29, stop:1 %30);"
        "    border-color: %31;"
        "}"
    ).arg(ModernMedicalStyle::Colors::BACKGROUND_MEDIUM.name())
     .arg(ModernMedicalStyle::scalePixelValue(1))
     .arg(ModernMedicalStyle::Colors::BORDER_LIGHT.name())
     .arg(ModernMedicalStyle::scaleValue(120))
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

// Modern Dashboard Card Implementation
QFrame* MainWindow::createDashboardCard(const QString& title, QWidget* content)
{
    QFrame* card = new QFrame;

    // Use percentage-based sizing instead of fixed sizes
    // Cards will automatically resize with the window
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    card->setProperty("isDashboardCard", true);  // Mark as dashboard card for resizing

    // Modern card styling
    QString cardStyle = QString(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %1, stop:1 %2);"
        "    border: %3 solid %4;"
        "    border-radius: %5;"
        "    %6"
        "}"
    ).arg(ModernMedicalStyle::Colors::BACKGROUND_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::BACKGROUND_MEDIUM.name())
     .arg(ModernMedicalStyle::scalePixelValue(3))
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getLargeRadius()))
     .arg(ModernMedicalStyle::Elevation::getLevel3());

    card->setStyleSheet(cardStyle);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(ModernMedicalStyle::Spacing::getLarge());
    cardLayout->setContentsMargins(
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getLarge(),
        ModernMedicalStyle::Spacing::getXLarge(),
        ModernMedicalStyle::Spacing::getLarge()
    );

    // Card title
    QLabel* titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-family: %1;"
        "    font-size: %2pt;"
        "    font-weight: %3;"
        "    color: %4;"
        "    background: transparent;"
        "    border: none;"
        "    padding: %5;"
        "}"
    ).arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
     .arg(ModernMedicalStyle::Typography::getDisplaySubtitle())
     .arg(ModernMedicalStyle::Typography::WEIGHT_BOLD)
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMedium())));

    cardLayout->addWidget(titleLabel);

    // Card content
    if (content) {
        cardLayout->addWidget(content, 1);
    }

    return card;
}

void MainWindow::setupPatternSelectionCard(QGridLayout* layout)
{
    if (m_patternSelector) {
        QFrame* card = createDashboardCard("VACUUM CYCLE SELECTION", m_patternSelector.get());
        layout->addWidget(card, 0, 0, 2, 1);  // Span 2 rows, 1 column (left side)
    }
}

void MainWindow::setupPressureMonitoringCard(QGridLayout* layout)
{
    if (m_pressureMonitor) {
        QFrame* card = createDashboardCard("REAL-TIME PRESSURE MONITORING", m_pressureMonitor.get());
        layout->addWidget(card, 0, 1, 1, 1);  // Top right
    }
}

void MainWindow::setupArousalMonitoringCard(QGridLayout* layout)
{
    // Create arousal monitor if not already created
    if (!m_arousalMonitor) {
        m_arousalMonitor = std::make_unique<ArousalMonitor>(m_controller, nullptr);
    }

    if (m_arousalMonitor) {
        QFrame* card = createDashboardCard("AROUSAL LEVEL MONITORING", m_arousalMonitor.get());
        layout->addWidget(card, 0, 2, 1, 1);  // Top right of pressure
    }
}

void MainWindow::setupControlPanelCard(QGridLayout* layout)
{
    // Create control panel content
    QWidget* controlContent = new QWidget;
    QVBoxLayout* controlLayout = new QVBoxLayout(controlContent);
    controlLayout->setSpacing(ModernMedicalStyle::Spacing::getXLarge());

    // Large control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(ModernMedicalStyle::Spacing::getLarge());

    m_startStopButton = new QPushButton("START SYSTEM");
    m_startStopButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(450),
        ModernMedicalStyle::scaleValue(180)
    );
    m_startStopButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("success"));

    m_pauseResumeButton = new QPushButton("PAUSE");
    m_pauseResumeButton->setMinimumSize(
        ModernMedicalStyle::scaleValue(450),
        ModernMedicalStyle::scaleValue(180)
    );
    m_pauseResumeButton->setStyleSheet(ModernMedicalStyle::getButtonStyle("warning"));
    m_pauseResumeButton->setEnabled(false);

    buttonLayout->addWidget(m_startStopButton);
    buttonLayout->addWidget(m_pauseResumeButton);
    buttonLayout->addStretch();

    controlLayout->addLayout(buttonLayout);
    controlLayout->addStretch();

    QFrame* card = createDashboardCard("SYSTEM CONTROL", controlContent);
    layout->addWidget(card, 1, 1, 1, 1);  // Bottom right
}

void MainWindow::setupStatusCard(QGridLayout* layout)
{
    // Create status content
    QWidget* statusContent = new QWidget;
    QVBoxLayout* statusLayout = new QVBoxLayout(statusContent);
    statusLayout->setSpacing(ModernMedicalStyle::Spacing::getLarge());

    // System status display
    QLabel* systemStatus = new QLabel("SYSTEM STATUS: READY");
    systemStatus->setAlignment(Qt::AlignCenter);
    systemStatus->setStyleSheet(QString(
        "QLabel {"
        "    font-family: %1;"
        "    font-size: %2pt;"
        "    font-weight: %3;"
        "    color: %4;"
        "    background-color: %5;"
        "    border: %6 solid %7;"
        "    border-radius: %8;"
        "    padding: %9;"
        "}"
    ).arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
     .arg(ModernMedicalStyle::Typography::getTitle())
     .arg(ModernMedicalStyle::Typography::WEIGHT_BOLD)
     .arg(ModernMedicalStyle::Colors::TEXT_ON_PRIMARY.name())
     .arg(ModernMedicalStyle::Colors::MEDICAL_GREEN.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::MEDICAL_GREEN.name())
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getMediumRadius()))
     .arg(ModernMedicalStyle::scalePixelValue(ModernMedicalStyle::Spacing::getLarge())));

    statusLayout->addWidget(systemStatus);
    statusLayout->addStretch();

    QFrame* card = createDashboardCard("SYSTEM STATUS", statusContent);
    layout->addWidget(card, 2, 0, 1, 2);  // Bottom spanning both columns
}


