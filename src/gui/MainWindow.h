#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <memory>
#include "../VacuumController.h"

// Forward declarations
class PressureMonitor;
class PatternSelector;
class SafetyPanel;
class SettingsDialog;
class SystemDiagnosticsPanel;

/**
 * @brief Main window for the vacuum controller GUI
 * 
 * This class provides the main user interface optimized for a 50-inch display.
 * It implements a tabbed interface with the following sections:
 * - Main Control Panel (patterns, pressure monitoring)
 * - Safety Panel (emergency controls, system status)
 * - Settings (calibration, configuration)
 * - Diagnostics (system health, logs)
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(VacuumController* controller, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void onSystemStateChanged(VacuumController::SystemState state);
    void onPressureUpdated(double avlPressure, double tankPressure);
    void onEmergencyStopTriggered();
    void onSystemError(const QString& error);
    void onAntiDetachmentActivated();
    
    // Navigation
    void showMainPanel();
    void showSafetyPanel();
    void showSettingsDialog();
    void showDiagnosticsPanel();
    
    // Control actions
    void onStartStopClicked();
    void onPauseResumeClicked();
    void onEmergencyStopClicked();
    void onResetEmergencyStopClicked();
    
    // UI updates
    void updateStatusDisplay();
    void updateControlButtons();

private:
    void setupUI();
    void setupMainPanel();
    void setupNavigationBar();
    void setupStatusBar();
    void setupEmergencyControls();
    void connectSignals();
    void applyLargeDisplayStyles();
    
    // Controller interface
    VacuumController* m_controller;
    
    // Main UI components
    QWidget* m_centralWidget;
    QStackedWidget* m_stackedWidget;
    QVBoxLayout* m_mainLayout;
    
    // Navigation
    QFrame* m_navigationBar;
    QHBoxLayout* m_navLayout;
    QPushButton* m_mainPanelButton;
    QPushButton* m_safetyPanelButton;
    QPushButton* m_settingsButton;
    QPushButton* m_diagnosticsButton;
    QPushButton* m_shutdownButton;
    
    // Status bar
    QFrame* m_statusBar;
    QHBoxLayout* m_statusLayout;
    QLabel* m_systemStatusLabel;
    QLabel* m_pressureStatusLabel;
    QLabel* m_timeLabel;
    
    // Emergency controls (always visible)
    QFrame* m_emergencyFrame;
    QPushButton* m_emergencyStopButton;
    QPushButton* m_resetEmergencyButton;
    
    // Main panels
    QWidget* m_mainPanel;
    
    // Specialized UI components
    std::unique_ptr<PressureMonitor> m_pressureMonitor;
    std::unique_ptr<PatternSelector> m_patternSelector;
    std::unique_ptr<SafetyPanel> m_safetyPanelWidget;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    std::unique_ptr<SystemDiagnosticsPanel> m_diagnosticsPanelWidget;
    
    // Control buttons
    QPushButton* m_startStopButton;
    QPushButton* m_pauseResumeButton;
    
    // Status update timer
    QTimer* m_statusUpdateTimer;
    
    // Current state
    bool m_systemRunning;
    bool m_systemPaused;
    bool m_emergencyStop;
    
    // UI constants for big screen display
    static const int BUTTON_HEIGHT = 80;
    static const int BUTTON_WIDTH = 200;
    static const int LARGE_BUTTON_HEIGHT = 120;
    static const int LARGE_BUTTON_WIDTH = 300;
    static const int NAVIGATION_HEIGHT = 100;
    static const int STATUS_BAR_HEIGHT = 80;
    static const int EMERGENCY_BUTTON_SIZE = 150;
    static const int FONT_SIZE_NORMAL = 10;
    static const int FONT_SIZE_LARGE = 20;
    static const int FONT_SIZE_HUGE = 24;
};

#endif // MAINWINDOW_H
