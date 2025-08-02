#ifndef CALIBRATIONINTERFACE_H
#define CALIBRATIONINTERFACE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QJsonObject>
#include "../calibration/CalibrationManager.h"

// Forward declarations
class TouchButton;
class VacuumController;

/**
 * @brief Comprehensive calibration interface for the vacuum controller system
 * 
 * This widget provides a complete calibration interface that integrates with
 * the CalibrationManager to provide sensor and actuator calibration capabilities.
 * It's designed for the 800x480 Pi touchscreen with touch-optimized controls.
 */
class CalibrationInterface : public QWidget
{
    Q_OBJECT

public:
    explicit CalibrationInterface(VacuumController* controller, QWidget *parent = nullptr);
    ~CalibrationInterface();

    // Interface control
    void refreshCalibrationStatus();
    void showCalibrationResults(const QString& component);
    void resetInterface();

public slots:
    void startSensorCalibration();
    void startActuatorCalibration();
    void startSystemCalibration();
    void cancelCurrentCalibration();
    void saveCalibrationSettings();
    void loadCalibrationSettings();

signals:
    void calibrationStarted(const QString& component);
    void calibrationCompleted(const QString& component, bool successful);
    void calibrationCancelled();
    void settingsChanged();

private slots:
    // CalibrationManager signals
    void onCalibrationStarted(const QString& component, CalibrationManager::CalibrationType type);
    void onCalibrationProgress(int percentage, const QString& status);
    void onCalibrationPointAdded(const CalibrationManager::CalibrationPoint& point);
    void onCalibrationCompleted(const CalibrationManager::CalibrationResult& result);
    void onCalibrationFailed(const QString& component, const QString& error);
    void onCalibrationDataSaved(const QString& component);
    void onCalibrationValidated(const QString& component, bool valid);
    
    // UI interaction slots
    void onComponentSelectionChanged();
    void onCalibrationTypeChanged();
    void onStartCalibrationClicked();
    void onCancelCalibrationClicked();
    void onValidateCalibrationClicked();
    void onExportCalibrationClicked();
    void onImportCalibrationClicked();
    void onResetCalibrationClicked();
    void onSettingsChanged();
    void onStatusUpdateTimer();

private:
    void setupUI();
    void setupControlPanel();
    void setupStatusPanel();
    void setupProgressPanel();
    void setupResultsPanel();
    void setupSettingsPanel();
    void connectSignals();
    
    void updateComponentList();
    void updateCalibrationStatus();
    void updateProgressDisplay();
    void updateResultsTable();
    void updateSettingsPanel();
    
    void addCalibrationPointToTable(const CalibrationManager::CalibrationPoint& point);
    void clearResultsTable();
    void showCalibrationError(const QString& error);
    void showCalibrationSuccess(const QString& message);
    
    void applyTouchOptimizedStyles();
    void enableCalibrationControls(bool enabled);
    
    // Export/Import functionality
    bool exportCalibrationData(const QString& filePath);
    bool importCalibrationData(const QString& filePath);
    
    // Controller and manager interfaces
    VacuumController* m_controller;
    CalibrationManager* m_calibrationManager;
    
    // Main layout
    QVBoxLayout* m_mainLayout;
    
    // Control Panel
    QGroupBox* m_controlGroup;
    QComboBox* m_componentCombo;
    QComboBox* m_calibrationTypeCombo;
    TouchButton* m_startButton;
    TouchButton* m_cancelButton;
    TouchButton* m_validateButton;
    
    // Status Panel
    QGroupBox* m_statusGroup;
    QTableWidget* m_statusTable;
    TouchButton* m_refreshStatusButton;
    QLabel* m_lastUpdateLabel;
    
    // Progress Panel
    QGroupBox* m_progressGroup;
    QProgressBar* m_calibrationProgress;
    QLabel* m_progressStatusLabel;
    QLabel* m_currentStepLabel;
    QLabel* m_elapsedTimeLabel;
    QTimer* m_statusUpdateTimer;
    QDateTime m_calibrationStartTime;
    
    // Results Panel
    QGroupBox* m_resultsGroup;
    QTableWidget* m_resultsTable;
    QTextEdit* m_calibrationLog;
    TouchButton* m_exportResultsButton;
    TouchButton* m_clearLogButton;
    
    // Settings Panel
    QGroupBox* m_settingsGroup;
    QSpinBox* m_minPointsSpin;
    QDoubleSpinBox* m_maxErrorSpin;
    QSpinBox* m_calibrationTimeoutSpin;
    QCheckBox* m_autoSaveCheck;
    QCheckBox* m_autoValidateCheck;
    TouchButton* m_saveSettingsButton;
    TouchButton* m_resetSettingsButton;
    
    // Import/Export Panel
    QGroupBox* m_importExportGroup;
    TouchButton* m_exportButton;
    TouchButton* m_importButton;
    TouchButton* m_backupButton;
    TouchButton* m_restoreButton;
    
    // Current state
    QString m_currentComponent;
    CalibrationManager::CalibrationType m_currentType;
    bool m_calibrationInProgress;
    int m_currentProgress;
    QString m_currentStatus;
    
    // Configuration
    QJsonObject m_calibrationSettings;
    QString m_settingsFilePath;
    
    // Constants for touch optimization
    static const int BUTTON_MIN_HEIGHT = 50;
    static const int BUTTON_MIN_WIDTH = 120;
    static const int FONT_SIZE_NORMAL = 14;
    static const int FONT_SIZE_LARGE = 16;
    static const int SPACING_NORMAL = 10;
    static const int SPACING_LARGE = 15;
};

#endif // CALIBRATIONINTERFACE_H
