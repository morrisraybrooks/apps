#ifndef AROUSALMONITOR_H
#define AROUSALMONITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAreaSeries>
#include <QQueue>

QT_CHARTS_USE_NAMESPACE

// Forward declarations
class VacuumController;
class OrgasmControlAlgorithm;

/**
 * @brief Real-time arousal level monitoring widget
 * 
 * This widget provides comprehensive arousal monitoring including:
 * - Real-time arousal level display (0.0-1.0)
 * - Historical arousal chart with threshold zones
 * - Edge, orgasm, and recovery threshold visualization
 * - Control state indicator
 * - Milking zone visualization
 */
class ArousalMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit ArousalMonitor(VacuumController* controller, QWidget *parent = nullptr);
    ~ArousalMonitor();

    // Configuration
    void setChartTimeRange(int seconds);
    void setShowGrid(bool show);
    void setShowThresholdZones(bool show);

public Q_SLOTS:
    void updateArousalLevel(double arousalLevel);
    void updateControlState(int state);
    void resetChart();
    void pauseUpdates(bool pause);

Q_SIGNALS:
    void edgeApproaching(double arousalLevel);
    void orgasmDetected(double arousalLevel);
    void recoveryComplete(double arousalLevel);

private Q_SLOTS:
    void updateChart();
    void onArousalLevelChanged(double level);
    void onStateChanged(int state);

private:
    void setupUI();
    void setupArousalDisplay();
    void setupChart();
    void setupStateIndicator();
    void setupThresholdIndicators();
    void updateArousalDisplay(double arousalLevel);
    void updateThresholdZones();
    void addDataPoint(double arousalLevel);
    QString stateToString(int state) const;
    QColor stateToColor(int state) const;
    
    // Controller interface
    VacuumController* m_controller;
    OrgasmControlAlgorithm* m_algorithm;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Arousal display
    QFrame* m_displayFrame;
    QLabel* m_arousalValueLabel;
    QLabel* m_arousalPercentLabel;
    QProgressBar* m_arousalProgressBar;
    QLabel* m_stateLabel;
    QLabel* m_modeLabel;
    
    // Threshold indicators
    QFrame* m_thresholdFrame;
    QLabel* m_edgeThresholdLabel;
    QLabel* m_orgasmThresholdLabel;
    QLabel* m_recoveryThresholdLabel;
    QProgressBar* m_edgeIndicator;
    QProgressBar* m_orgasmIndicator;
    
    // Chart components
    QFrame* m_chartFrame;
    QChart* m_chart;
    QChartView* m_chartView;
    QLineSeries* m_arousalSeries;
    QAreaSeries* m_edgeZoneSeries;
    QAreaSeries* m_orgasmZoneSeries;
    QAreaSeries* m_milkingZoneSeries;
    QValueAxis* m_timeAxis;
    QValueAxis* m_arousalAxis;
    
    // Data storage
    QQueue<QPair<qint64, double>> m_arousalData;  // timestamp, arousal
    
    // Configuration
    double m_edgeThreshold;
    double m_orgasmThreshold;
    double m_recoveryThreshold;
    double m_milkingZoneLower;
    double m_milkingZoneUpper;
    int m_chartTimeRangeSeconds;
    bool m_showGrid;
    bool m_showThresholdZones;
    bool m_updatesPaused;
    
    // Current values
    double m_currentArousal;
    int m_currentState;
    
    // Update timer
    QTimer* m_chartUpdateTimer;
    
    // Constants
    static const int DEFAULT_CHART_TIME_RANGE = 300;  // 5 minutes
    static const int CHART_UPDATE_INTERVAL = 100;     // 100ms for smooth updates
    static const int MAX_DATA_POINTS = 3000;          // Maximum data points to keep
};

#endif // AROUSALMONITOR_H

