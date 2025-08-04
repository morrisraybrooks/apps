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
class SettingsPanel;
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
    void showSettingsPanel();
    void showDiagnosticsPanel();
    
    // Control actions
    void onStartStopClicked();
    void onPauseResumeClicked();
    void onEmergencyStopClicked();
    
    // UI updates
    void updateStatusDisplay();
    void updateControlButtons();

private:
    void setupUI();
    void setupMainPanel();
    void setupNavigationBar();
    void setupStatusBar();
    void connectSignals();
    void applyLargeDisplayStyles();

    // Modern dashboard card setup methods
    void setupPatternSelectionCard(QGridLayout* layout);
    void setupPressureMonitoringCard(QGridLayout* layout);
    void setupControlPanelCard(QGridLayout* layout);
    void setupStatusCard(QGridLayout* layout);
    QFrame* createDashboardCard(const QString& title, QWidget* content);
    void updateDashboardCardSizes(const QSize& windowSize);
    
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
    
    // Emergency control is now only the shutdown button in navigation
    
    // Main panels
    QWidget* m_mainPanel;
    
    // Specialized UI components
    std::unique_ptr<PressureMonitor> m_pressureMonitor;
    std::unique_ptr<PatternSelector> m_patternSelector;
    std::unique_ptr<SafetyPanel> m_safetyPanelWidget;
    std::unique_ptr<SettingsPanel> m_settingsPanelWidget;
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
    
    // UI constants for 50-inch medical display
    
};

#endif // MAINWINDOW_H
