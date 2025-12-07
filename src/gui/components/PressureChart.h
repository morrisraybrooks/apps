#ifndef PRESSURECHART_H
#define PRESSURECHART_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QQueue>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLegend>
#include <QDateTime>

QT_CHARTS_USE_NAMESPACE

/**
 * @brief Real-time pressure chart widget for vacuum controller
 * 
 * This widget provides comprehensive pressure visualization:
 * - Real-time plotting of AVL and Tank pressure
 * - Configurable time ranges (1min, 5min, 15min, 1hr)
 * - Threshold lines for warning and critical levels
 * - Zoom and pan capabilities
 * - Data export functionality
 * - Touch-optimized controls for 50-inch displays
 */
class PressureChart : public QWidget
{
    Q_OBJECT

public:
    struct PressureDataPoint {
        qint64 timestamp;
        double avlPressure;
        double tankPressure;
        
        PressureDataPoint() : timestamp(0), avlPressure(0.0), tankPressure(0.0) {}
        PressureDataPoint(qint64 ts, double avl, double tank) 
            : timestamp(ts), avlPressure(avl), tankPressure(tank) {}
    };

    enum TimeRange {
        RANGE_1MIN = 60,
        RANGE_5MIN = 300,
        RANGE_15MIN = 900,
        RANGE_1HOUR = 3600
    };

    explicit PressureChart(QWidget *parent = nullptr);
    ~PressureChart();

    // Data management
    void addDataPoint(double avlPressure, double tankPressure);
    void clearData();
    void setMaxDataPoints(int maxPoints);
    
    // Display configuration
    void setTimeRange(TimeRange range);
    TimeRange getTimeRange() const { return m_timeRange; }
    
    void setPressureRange(double minPressure, double maxPressure);
    void setAutoScale(bool enabled);
    
    // Threshold lines
    void setWarningThreshold(double threshold);
    void setCriticalThreshold(double threshold);
    void setAntiDetachmentThreshold(double threshold);
    void setShowThresholds(bool show);
    
    // Chart appearance
    void setShowGrid(bool show);
    void setShowLegend(bool show);
    void setLineWidth(int width);
    void setAVLColor(const QColor& color);
    void setTankColor(const QColor& color);
    
    // Data access
    QList<PressureDataPoint> getData(int maxPoints = -1) const;
    int getDataPointCount() const { return m_dataQueue.size(); }
    
    // Export functionality
    bool exportToCSV(const QString& filePath) const;
    bool exportChart(const QString& filePath) const;

public Q_SLOTS:
    void pauseUpdates(bool pause);
    void resetZoom();
    void zoomIn();
    void zoomOut();

Q_SIGNALS:
    void dataPointAdded(const PressureDataPoint& point);
    void thresholdViolation(const QString& type, double value);
    void chartClicked(const QPointF& point);

private Q_SLOTS:
    void updateChart();
    void onTimeRangeChanged();
    void onResetZoomClicked();
    void onPauseClicked();
    void onExportClicked();

private:
    void setupUI();
    void setupChart();
    void setupControls();
    void updateTimeAxis();
    void updatePressureAxis();
    void addThresholdLines();
    void removeOldData();
    void connectSignals();
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_controlLayout;
    
    // Chart components
    QtCharts::QChart* m_chart;
    QtCharts::QChartView* m_chartView;
    QtCharts::QLineSeries* m_avlSeries;
    QtCharts::QLineSeries* m_tankSeries;
    QtCharts::QLineSeries* m_warningLine;
    QtCharts::QLineSeries* m_criticalLine;
    QtCharts::QLineSeries* m_antiDetachmentLine;
    QtCharts::QDateTimeAxis* m_timeAxis;
    QtCharts::QValueAxis* m_pressureAxis;
    
    // Controls
    QComboBox* m_timeRangeCombo;
    QPushButton* m_pauseButton;
    QPushButton* m_resetZoomButton;
    QPushButton* m_exportButton;
    QLabel* m_statusLabel;
    
    // Data storage
    QQueue<PressureDataPoint> m_dataQueue;
    int m_maxDataPoints;
    
    // Configuration
    TimeRange m_timeRange;
    double m_minPressure;
    double m_maxPressure;
    bool m_autoScale;
    bool m_showThresholds;
    bool m_showGrid;
    bool m_showLegend;
    bool m_updatesPaused;
    
    // Thresholds
    double m_warningThreshold;
    double m_criticalThreshold;
    double m_antiDetachmentThreshold;
    
    // Appearance
    QColor m_avlColor;
    QColor m_tankColor;
    int m_lineWidth;
    
    // Update timer
    QTimer* m_updateTimer;
    
    // Constants
    static const int DEFAULT_MAX_DATA_POINTS = 3600;  // 1 hour at 1Hz
    static const int UPDATE_INTERVAL_MS = 1000;       // 1 second updates
    static const double DEFAULT_MIN_PRESSURE;         // 0.0 mmHg
    static const double DEFAULT_MAX_PRESSURE;         // 75.0 mmHg
    static const double DEFAULT_WARNING_THRESHOLD;    // 60.0 mmHg
    static const double DEFAULT_CRITICAL_THRESHOLD;   // 70.0 mmHg
    static const double DEFAULT_ANTI_DETACHMENT;      // 50.0 mmHg
};

#endif // PRESSURECHART_H
