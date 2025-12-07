#ifndef PRESSUREMONITOR_H
#define PRESSUREMONITOR_H

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
#include <QQueue>

QT_CHARTS_USE_NAMESPACE

// Forward declarations
class VacuumController;
class PressureChart;

/**
 * @brief Real-time pressure monitoring widget
 * 
 * This widget provides comprehensive pressure monitoring including:
 * - Real-time pressure displays for AVL and Tank
 * - Historical pressure charts
 * - Pressure limit indicators
 * - Anti-detachment threshold visualization
 * - Alarm indicators for overpressure conditions
 */
class PressureMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit PressureMonitor(VacuumController* controller, QWidget *parent = nullptr);
    ~PressureMonitor();

    // Pressure updates
    void updatePressures(double avlPressure, double tankPressure);
    
    // Configuration
    void setMaxPressure(double maxPressure);
    void setWarningThreshold(double warningThreshold);
    void setAntiDetachmentThreshold(double threshold);
    
    // Display options
    void setChartTimeRange(int seconds);
    void setShowGrid(bool show);
    void setShowAlarms(bool show);

public Q_SLOTS:
    void resetChart();
    void pauseUpdates(bool pause);

Q_SIGNALS:
    void pressureAlarm(const QString& message);
    void antiDetachmentTriggered();

private Q_SLOTS:
    void updateChart();

private:
    void setupUI();
    void setupPressureDisplays();
    void setupChart();
    void setupAlarmIndicators();
    void updatePressureDisplay(QLabel* valueLabel, QProgressBar* progressBar, 
                              double pressure, double maxPressure);
    void updateAlarmStates();
    void addDataPoint(double avlPressure, double tankPressure);
    
    // Controller interface
    VacuumController* m_controller;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Pressure displays
    QFrame* m_displayFrame;
    QLabel* m_avlValueLabel;
    QLabel* m_tankValueLabel;
    QProgressBar* m_avlProgressBar;
    QProgressBar* m_tankProgressBar;
    QLabel* m_avlStatusLabel;
    QLabel* m_tankStatusLabel;
    
    // Chart components
    QFrame* m_chartFrame;
    PressureChart* m_chart;

    // Chart series and axes
    class QLineSeries* m_avlSeries;
    class QLineSeries* m_tankSeries;
    class QValueAxis* m_timeAxis;
    class QValueAxis* m_pressureAxis;
    
    // Alarm indicators
    QFrame* m_alarmFrame;
    QLabel* m_overpressureAlarm;
    QLabel* m_antiDetachmentAlarm;
    QLabel* m_sensorErrorAlarm;
    
    // Data storage
    QQueue<QPair<qint64, double>> m_avlData;  // timestamp, pressure
    QQueue<QPair<qint64, double>> m_tankData;
    
    // Configuration
    double m_maxPressure;
    double m_warningThreshold;
    double m_antiDetachmentThreshold;
    int m_chartTimeRangeSeconds;
    bool m_showGrid;
    bool m_showAlarms;
    bool m_updatesPaused;
    
    // Current values
    double m_currentAVL;
    double m_currentTank;
    
    // Update timer
    QTimer* m_chartUpdateTimer;
    
    // Constants
    static const int DEFAULT_CHART_TIME_RANGE = 300;  // 5 minutes
    static const int CHART_UPDATE_INTERVAL = 1000;    // 1 second
    static const int MAX_DATA_POINTS = 1000;          // Maximum data points to keep
    static const double DEFAULT_MAX_PRESSURE;         // 75.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD;    // 60.0 mmHg
    static const double DEFAULT_ANTI_DETACHMENT;      // 50.0 mmHg
};

#endif // PRESSUREMONITOR_H
