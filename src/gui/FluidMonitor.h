#ifndef FLUIDMONITOR_H
#define FLUIDMONITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QTimer>
#include <QElapsedTimer>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QQueue>
#include <QPushButton>

QT_CHARTS_USE_NAMESPACE

// Forward declarations
class FluidSensor;
class HardwareManager;

/**
 * @brief Real-time fluid collection monitoring widget
 * 
 * This widget provides comprehensive fluid monitoring including:
 * - Real-time volume display (current and cumulative)
 * - Flow rate display (mL/min)
 * - Reservoir fill level indicator
 * - Historical volume chart
 * - Orgasm burst event markers
 * - Session statistics (lubrication vs orgasmic fluid)
 * - Overflow warnings
 */
class FluidMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit FluidMonitor(HardwareManager* hardware, QWidget *parent = nullptr);
    ~FluidMonitor();

    // Volume updates
    void updateVolume(double currentMl, double cumulativeMl);
    void updateFlowRate(double mlPerMin);
    
    // Configuration
    void setReservoirCapacity(double capacityMl);
    void setOverflowWarning(double warningMl);
    
    // Display options
    void setChartTimeRange(int seconds);
    void setShowOrgasmMarkers(bool show);

public Q_SLOTS:
    void resetSession();
    void pauseUpdates(bool pause);
    void tareReservoir();
    void calibrateSensor();

Q_SIGNALS:
    void overflowWarning(double volumeMl);
    void sessionReset();

private Q_SLOTS:
    void updateChart();
    void onVolumeUpdated(double currentMl, double cumulativeMl);
    void onFlowRateUpdated(double mlPerMin, double mlPerSec);
    void onOrgasmBurstDetected(double volumeMl, double peakRate, int orgasmNum);
    void onOverflowWarning(double volumeMl, double capacityMl);

private:
    void setupUI();
    void setupVolumeDisplays();
    void setupChart();
    void setupStatistics();
    void setupControls();
    void updateVolumeDisplay();
    void updateFlowDisplay();
    void updateReservoirLevel();
    void addDataPoint(double volumeMl);
    void addOrgasmMarker(int orgasmNumber, double volumeMl);
    
    // Hardware interface
    HardwareManager* m_hardware;
    FluidSensor* m_fluidSensor;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Volume displays
    QFrame* m_displayFrame;
    QLabel* m_currentVolumeLabel;
    QLabel* m_cumulativeVolumeLabel;
    QLabel* m_flowRateLabel;
    QProgressBar* m_reservoirBar;
    QLabel* m_reservoirStatusLabel;
    
    // Chart components
    QFrame* m_chartFrame;
    QChartView* m_chartView;
    QChart* m_chart;
    QLineSeries* m_volumeSeries;
    QLineSeries* m_flowSeries;
    QValueAxis* m_timeAxis;
    QValueAxis* m_volumeAxis;
    
    // Statistics display
    QFrame* m_statsFrame;
    QLabel* m_lubricationLabel;
    QLabel* m_orgasmicLabel;
    QLabel* m_orgasmCountLabel;
    QLabel* m_avgPerOrgasmLabel;
    
    // Control buttons
    QFrame* m_controlFrame;
    QPushButton* m_tareButton;
    QPushButton* m_calibrateButton;
    QPushButton* m_resetButton;
    
    // Data storage
    QQueue<QPair<qint64, double>> m_volumeData;  // timestamp, volume
    QQueue<QPair<qint64, double>> m_flowData;    // timestamp, flow rate
    QVector<QPair<qint64, int>> m_orgasmMarkers; // timestamp, orgasm number
    
    // Configuration
    double m_reservoirCapacity;
    double m_overflowWarningMl;
    int m_chartTimeRangeSeconds;
    bool m_showOrgasmMarkers;
    bool m_updatesPaused;
    
    // Current values
    double m_currentVolumeMl;
    double m_cumulativeVolumeMl;
    double m_flowRateMlPerMin;
    double m_lubricationMl;
    double m_orgasmicMl;
    int m_orgasmCount;
    
    // Update timer
    QTimer* m_chartUpdateTimer;
    QElapsedTimer m_sessionTimer;
    
    // Constants
    static const int DEFAULT_CHART_TIME_RANGE = 600;  // 10 minutes
    static const int CHART_UPDATE_INTERVAL = 1000;    // 1 second
    static const int MAX_DATA_POINTS = 1200;          // 20 minutes at 1/sec
    static constexpr double DEFAULT_CAPACITY = 150.0; // mL
    static constexpr double DEFAULT_WARNING = 120.0;  // mL
};

#endif // FLUIDMONITOR_H

