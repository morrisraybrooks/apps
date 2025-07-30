#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include <QObject>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QProgressDialog>

// Forward declarations
class VacuumController;
class DataLogger;

/**
 * @brief Comprehensive data export and reporting system
 * 
 * This system provides:
 * - Multi-format data export (CSV, JSON, XML, PDF)
 * - Customizable report generation
 * - Session summary reports
 * - Safety compliance reports
 * - Performance analysis reports
 * - Pattern usage statistics
 * - Automated report scheduling
 * - Data filtering and aggregation
 */
class DataExporter : public QObject
{
    Q_OBJECT

public:
    enum ExportFormat {
        CSV_FORMAT,         // Comma-separated values
        JSON_FORMAT,        // JSON format
        XML_FORMAT,         // XML format
        PDF_FORMAT,         // PDF report
        EXCEL_FORMAT        // Excel spreadsheet
    };
    Q_ENUM(ExportFormat)

    enum ReportType {
        SESSION_SUMMARY,    // Summary of a session
        SAFETY_COMPLIANCE,  // Safety events and compliance
        PERFORMANCE_ANALYSIS, // System performance metrics
        PATTERN_USAGE,      // Pattern usage statistics
        PRESSURE_HISTORY,   // Pressure data over time
        ERROR_ANALYSIS,     // Error and warning analysis
        CALIBRATION_REPORT, // Calibration history and status
        MAINTENANCE_REPORT  // Maintenance and system health
    };
    Q_ENUM(ReportType)

    struct ExportOptions {
        ExportFormat format;
        ReportType reportType;
        QDateTime startTime;
        QDateTime endTime;
        QString outputPath;
        bool includeCharts;
        bool includeStatistics;
        bool includeRawData;
        QStringList dataFilters;
        QJsonObject customParameters;
        
        ExportOptions() : format(CSV_FORMAT), reportType(SESSION_SUMMARY), 
                         includeCharts(true), includeStatistics(true), includeRawData(false) {}
    };

    struct ReportData {
        QString title;
        QString description;
        QDateTime generatedTime;
        QDateTime startTime;
        QDateTime endTime;
        QJsonObject metadata;
        QJsonObject statistics;
        QJsonArray dataPoints;
        QStringList chartPaths;
        
        ReportData() : generatedTime(QDateTime::currentDateTime()) {}
    };

    explicit DataExporter(VacuumController* controller, DataLogger* logger, QObject *parent = nullptr);
    ~DataExporter();

    // Export methods
    bool exportData(const ExportOptions& options);
    bool exportSessionSummary(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime);
    bool exportPressureData(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime);
    bool exportPatternUsage(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime);
    bool exportSafetyEvents(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime);
    
    // Report generation
    ReportData generateSessionReport(const QDateTime& startTime, const QDateTime& endTime);
    ReportData generateSafetyReport(const QDateTime& startTime, const QDateTime& endTime);
    ReportData generatePerformanceReport(const QDateTime& startTime, const QDateTime& endTime);
    ReportData generatePatternUsageReport(const QDateTime& startTime, const QDateTime& endTime);
    
    // Format-specific exports
    bool exportToCSV(const QString& filePath, const ReportData& data);
    bool exportToJSON(const QString& filePath, const ReportData& data);
    bool exportToXML(const QString& filePath, const ReportData& data);
    bool exportToPDF(const QString& filePath, const ReportData& data);
    bool exportToExcel(const QString& filePath, const ReportData& data);
    
    // Chart generation
    bool generatePressureChart(const QString& filePath, const QJsonArray& pressureData);
    bool generatePatternChart(const QString& filePath, const QJsonArray& patternData);
    bool generateStatisticsChart(const QString& filePath, const QJsonObject& statistics);
    
    // Automated reporting
    void scheduleReport(ReportType type, const QString& outputPath, int intervalHours);
    void cancelScheduledReport(ReportType type);
    QStringList getScheduledReports() const;
    
    // Data aggregation
    QJsonObject aggregatePressureData(const QJsonArray& rawData, int intervalMinutes = 1);
    QJsonObject aggregatePatternData(const QJsonArray& rawData);
    QJsonObject aggregateSafetyData(const QJsonArray& rawData);
    QJsonObject calculateStatistics(const QJsonArray& data, const QStringList& fields);

public slots:
    void exportCurrentSession();
    void exportLastHour();
    void exportLastDay();
    void exportLastWeek();
    void generateScheduledReports();

signals:
    void exportStarted(const QString& description);
    void exportProgress(int percentage);
    void exportCompleted(const QString& filePath, bool success);
    void exportError(const QString& error);
    void reportGenerated(ReportType type, const QString& filePath);

private slots:
    void onScheduledReportTimer();

private:
    void initializeExporter();
    void setupScheduledReports();
    
    // Data collection
    QJsonArray collectPressureData(const QDateTime& startTime, const QDateTime& endTime);
    QJsonArray collectPatternData(const QDateTime& startTime, const QDateTime& endTime);
    QJsonArray collectSafetyData(const QDateTime& startTime, const QDateTime& endTime);
    QJsonArray collectErrorData(const QDateTime& startTime, const QDateTime& endTime);
    QJsonArray collectCalibrationData(const QDateTime& startTime, const QDateTime& endTime);
    
    // Report building
    void buildSessionSummary(ReportData& report, const QDateTime& startTime, const QDateTime& endTime);
    void buildSafetyCompliance(ReportData& report, const QDateTime& startTime, const QDateTime& endTime);
    void buildPerformanceAnalysis(ReportData& report, const QDateTime& startTime, const QDateTime& endTime);
    void buildPatternUsageAnalysis(ReportData& report, const QDateTime& startTime, const QDateTime& endTime);
    
    // Statistics calculation
    QJsonObject calculatePressureStatistics(const QJsonArray& data);
    QJsonObject calculatePatternStatistics(const QJsonArray& data);
    QJsonObject calculateSafetyStatistics(const QJsonArray& data);
    QJsonObject calculateSystemStatistics();
    
    // Formatting helpers
    QString formatDuration(qint64 milliseconds);
    QString formatPressure(double pressure);
    QString formatTimestamp(qint64 timestamp);
    QString formatFileSize(qint64 bytes);
    
    // File operations
    bool ensureDirectoryExists(const QString& filePath);
    QString generateUniqueFileName(const QString& basePath, const QString& extension);
    bool validateExportPath(const QString& filePath);
    
    // Controller and logger interfaces
    VacuumController* m_controller;
    DataLogger* m_logger;
    
    // Export state
    bool m_exportInProgress;
    QString m_currentExportPath;
    QProgressDialog* m_progressDialog;
    
    // Scheduled reporting
    QMap<ReportType, QString> m_scheduledReports;
    QMap<ReportType, int> m_reportIntervals;
    QTimer* m_scheduledReportTimer;
    
    // Configuration
    QString m_defaultExportPath;
    ExportFormat m_defaultFormat;
    bool m_includeChartsDefault;
    bool m_includeStatisticsDefault;
    int m_maxDataPointsPerExport;
    
    // Constants
    static const QString DEFAULT_EXPORT_PATH;
    static const int DEFAULT_MAX_DATA_POINTS = 100000;
    static const int SCHEDULED_REPORT_CHECK_INTERVAL = 3600000; // 1 hour
    static const QStringList CSV_HEADERS_PRESSURE;
    static const QStringList CSV_HEADERS_PATTERN;
    static const QStringList CSV_HEADERS_SAFETY;
};

#endif // DATAEXPORTER_H
