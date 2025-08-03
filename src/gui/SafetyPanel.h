#ifndef SAFETYPANEL_H
#define SAFETYPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <memory>

// Forward declarations
class VacuumController;
class TouchButton;
class StatusIndicator;
class MultiStatusIndicator;

/**
 * @brief Safety control and monitoring panel
 * 
 * This panel provides comprehensive safety monitoring and control:
 * - Emergency stop controls
 * - System status indicators
 * - Pressure limit monitoring
 * - Anti-detachment status
 * - Safety system diagnostics
 * - Manual safety overrides
 */
class SafetyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SafetyPanel(VacuumController* controller, QWidget *parent = nullptr);
    ~SafetyPanel();

    // Safety alerts
    void showAntiDetachmentAlert();
    void showOverpressureAlert(double pressure);
    void showSensorErrorAlert(const QString& sensor);
    void clearAlerts();

public Q_SLOTS:
    void updateSafetyStatus();
    void onEmergencyStopTriggered();
    void onSafetyStateChanged(int state);

Q_SIGNALS:
    void emergencyStopRequested();
    void resetEmergencyStopRequested();
    void safetyTestRequested();

private Q_SLOTS:
    void onEmergencyStopClicked();
    void onResetEmergencyStopClicked();
    void onSafetyTestClicked();
    void onSystemDiagnosticsClicked();
    void updateStatusIndicators();

private:
    void setupUI();
    void setupEmergencyControls();
    void setupStatusMonitoring();
    void setupPressureLimits();
    void setupSystemDiagnostics();
    void connectSignals();
    
    // Controller interface
    VacuumController* m_controller;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Emergency controls
    QGroupBox* m_emergencyGroup;
    TouchButton* m_emergencyStopButton;
    TouchButton* m_resetEmergencyButton;
    QLabel* m_emergencyStatusLabel;
    
    // Status monitoring
    QGroupBox* m_statusGroup;
    MultiStatusIndicator* m_statusIndicators;
    
    // Pressure limits
    QGroupBox* m_pressureGroup;
    QLabel* m_avlPressureLabel;
    QLabel* m_tankPressureLabel;
    QProgressBar* m_avlPressureBar;
    QProgressBar* m_tankPressureBar;
    QLabel* m_pressureLimitLabel;
    QLabel* m_antiDetachmentLabel;
    
    // System diagnostics
    QGroupBox* m_diagnosticsGroup;
    TouchButton* m_safetyTestButton;
    TouchButton* m_systemDiagnosticsButton;
    QLabel* m_lastTestLabel;
    QLabel* m_systemHealthLabel;
    
    // Alert display
    QGroupBox* m_alertGroup;
    QLabel* m_alertLabel;
    TouchButton* m_clearAlertsButton;
    
    // Update timer
    QTimer* m_updateTimer;
    
    // Current values
    double m_currentAVL;
    double m_currentTank;
    bool m_emergencyStop;
    bool m_systemHealthy;
    
    // Constants
    static const int UPDATE_INTERVAL_MS = 500;  // 2Hz updates
    static const double PRESSURE_LIMIT;         // 100.0 mmHg
    static const double WARNING_THRESHOLD;      // 80.0 mmHg
    static const double ANTI_DETACHMENT_THRESHOLD; // 50.0 mmHg
};

#endif // SAFETYPANEL_H
