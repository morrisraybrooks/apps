#include "DataExporter.h"
#include "../VacuumController.h"
#include "../logging/DataLogger.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QJsonDocument>
#include <QXmlStreamWriter>
#include <QStandardPaths>
#include <QMetaEnum>
#include <QApplication>

// Constants
const QString DataExporter::DEFAULT_EXPORT_PATH = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VacuumController/Exports";
const QStringList DataExporter::CSV_HEADERS_PRESSURE = {"Timestamp", "AVL_Pressure", "Tank_Pressure", "Target_Pressure", "Pattern_Name"};
const QStringList DataExporter::CSV_HEADERS_PATTERN = {"Timestamp", "Pattern_Name", "Step", "Action", "Pressure", "Duration"};
const QStringList DataExporter::CSV_HEADERS_SAFETY = {"Timestamp", "Event_Type", "Severity", "Component", "Message", "Data"};

DataExporter::DataExporter(VacuumController* controller, DataLogger* logger, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_logger(logger)
    , m_exportInProgress(false)
    , m_progressDialog(nullptr)
    , m_scheduledReportTimer(new QTimer(this))
    , m_defaultExportPath(DEFAULT_EXPORT_PATH)
    , m_defaultFormat(CSV_FORMAT)
    , m_includeChartsDefault(true)
    , m_includeStatisticsDefault(true)
    , m_maxDataPointsPerExport(DEFAULT_MAX_DATA_POINTS)
{
    // Create export directory if it doesn't exist
    QDir exportDir(m_defaultExportPath);
    if (!exportDir.exists()) {
        exportDir.mkpath(".");
    }
    
    // Setup scheduled reporting timer
    m_scheduledReportTimer->setInterval(SCHEDULED_REPORT_CHECK_INTERVAL);
    connect(m_scheduledReportTimer, &QTimer::timeout, this, &DataExporter::onScheduledReportTimer);
    m_scheduledReportTimer->start();
    
    qDebug() << "DataExporter initialized with export path:" << m_defaultExportPath;
}

DataExporter::~DataExporter()
{
    if (m_progressDialog) {
        m_progressDialog->deleteLater();
    }
}

bool DataExporter::exportData(const ExportOptions& options)
{
    if (m_exportInProgress) {
        qWarning() << "Export already in progress";
        return false;
    }
    
    m_exportInProgress = true;
    m_currentExportPath = options.outputPath;
    
    emit exportStarted(QString("Exporting %1 data").arg(QString(QMetaEnum::fromType<ReportType>().valueToKey(options.reportType))));
    
    bool success = false;
    
    try {
        // Generate report data based on type
        ReportData reportData;
        
        switch (options.reportType) {
        case SESSION_SUMMARY:
            reportData = generateSessionReport(options.startTime, options.endTime);
            break;
        case SAFETY_COMPLIANCE:
            reportData = generateSafetyReport(options.startTime, options.endTime);
            break;
        case PERFORMANCE_ANALYSIS:
            reportData = generatePerformanceReport(options.startTime, options.endTime);
            break;
        case PATTERN_USAGE:
            reportData = generatePatternUsageReport(options.startTime, options.endTime);
            break;
        case PRESSURE_HISTORY:
            reportData.title = "Pressure History Report";
            reportData.dataPoints = collectPressureData(options.startTime, options.endTime);
            break;
        case ERROR_ANALYSIS:
            reportData.title = "Error Analysis Report";
            reportData.dataPoints = collectErrorData(options.startTime, options.endTime);
            break;
        default:
            reportData = generateSessionReport(options.startTime, options.endTime);
            break;
        }
        
        // Apply filters if specified
        // if (!options.dataFilters.isEmpty()) {
        //     reportData.dataPoints = applyDataFilters(reportData.dataPoints, options.dataFilters);
        // }
        
        // Limit data points if necessary
        if (reportData.dataPoints.size() > m_maxDataPointsPerExport) {
            qWarning() << "Data points exceed limit, truncating from" << reportData.dataPoints.size() << "to" << m_maxDataPointsPerExport;
            QJsonArray truncatedData;
            for (int i = 0; i < m_maxDataPointsPerExport; ++i) {
                truncatedData.append(reportData.dataPoints[i]);
            }
            reportData.dataPoints = truncatedData;
        }
        
        // Export in the specified format
        switch (options.format) {
        case CSV_FORMAT:
            success = exportToCSV(options.outputPath, reportData);
            break;
        case JSON_FORMAT:
            success = exportToJSON(options.outputPath, reportData);
            break;
        case XML_FORMAT:
            success = exportToXML(options.outputPath, reportData);
            break;
        case PDF_FORMAT:
            success = exportToPDF(options.outputPath, reportData);
            break;
        case EXCEL_FORMAT:
            success = exportToExcel(options.outputPath, reportData);
            break;
        default:
            success = exportToCSV(options.outputPath, reportData);
            break;
        }
        
    } catch (const std::exception& e) {
        emit exportError(QString("Export failed: %1").arg(e.what()));
        success = false;
    }
    
    m_exportInProgress = false;
    emit exportCompleted(options.outputPath, success);
    
    return success;
}

bool DataExporter::exportSessionSummary(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime)
{
    ExportOptions options;
    options.format = CSV_FORMAT;
    options.reportType = SESSION_SUMMARY;
    options.startTime = startTime;
    options.endTime = endTime;
    options.outputPath = filePath;
    
    return exportData(options);
}

bool DataExporter::exportPressureData(const QString& filePath, const QDateTime& startTime, const QDateTime& endTime)
{
    ExportOptions options;
    options.format = CSV_FORMAT;
    options.reportType = PRESSURE_HISTORY;
    options.startTime = startTime;
    options.endTime = endTime;
    options.outputPath = filePath;
    
    return exportData(options);
}

DataExporter::ReportData DataExporter::generateSessionReport(const QDateTime& startTime, const QDateTime& endTime)
{
    ReportData report;
    report.title = "Session Summary Report";
    report.description = QString("Session data from %1 to %2").arg(startTime.toString(), endTime.toString());
    report.startTime = startTime;
    report.endTime = endTime;
    
    // Collect various data types
    QJsonArray pressureData = collectPressureData(startTime, endTime);
    QJsonArray patternData = collectPatternData(startTime, endTime);
    QJsonArray safetyData = collectSafetyData(startTime, endTime);
    
    // Combine all data
    QJsonArray combinedData;
    for (const auto& item : pressureData) {
        QJsonObject obj = item.toObject();
        obj["data_type"] = "pressure";
        combinedData.append(obj);
    }
    
    for (const auto& item : patternData) {
        QJsonObject obj = item.toObject();
        obj["data_type"] = "pattern";
        combinedData.append(obj);
    }
    
    for (const auto& item : safetyData) {
        QJsonObject obj = item.toObject();
        obj["data_type"] = "safety";
        combinedData.append(obj);
    }
    
    report.dataPoints = combinedData;
    
    // Generate statistics
    // report.statistics = calculateSessionStatistics(pressureData, patternData, safetyData);
    
    return report;
}

DataExporter::ReportData DataExporter::generateSafetyReport(const QDateTime& startTime, const QDateTime& endTime)
{
    ReportData report;
    report.title = "Safety Compliance Report";
    report.description = QString("Safety events and compliance data from %1 to %2").arg(startTime.toString(), endTime.toString());
    report.startTime = startTime;
    report.endTime = endTime;
    
    report.dataPoints = collectSafetyData(startTime, endTime);
    report.statistics = calculateSafetyStatistics(report.dataPoints);
    
    return report;
}

QJsonArray DataExporter::collectPressureData(const QDateTime& startTime, const QDateTime& endTime)
{
    QJsonArray data;
    
    if (!m_logger) {
        qWarning() << "Data logger not available";
        return data;
    }
    
    // Get pressure log entries from the logger
    auto logEntries = m_logger->getLogEntries(startTime, endTime, DataLogger::PRESSURE_DATA);
    
    for (const auto& entry : logEntries) {
        QJsonObject dataPoint;
        dataPoint["timestamp"] = entry.timestamp;
        dataPoint["avl_pressure"] = entry.data.value("avl_pressure").toDouble();
        dataPoint["tank_pressure"] = entry.data.value("tank_pressure").toDouble();
        dataPoint["target_pressure"] = entry.data.value("target_pressure").toDouble();
        dataPoint["pattern_name"] = entry.data.value("pattern_name").toString();
        
        data.append(dataPoint);
    }
    
    return data;
}

QJsonArray DataExporter::collectPatternData(const QDateTime& startTime, const QDateTime& endTime)
{
    QJsonArray data;
    
    if (!m_logger) {
        qWarning() << "Data logger not available";
        return data;
    }
    
    // Get pattern log entries from the logger
    auto logEntries = m_logger->getLogEntries(startTime, endTime, DataLogger::PATTERN_EXECUTION);
    
    for (const auto& entry : logEntries) {
        QJsonObject dataPoint;
        dataPoint["timestamp"] = entry.timestamp;
        dataPoint["pattern_name"] = entry.data.value("pattern_name").toString();
        dataPoint["step"] = entry.data.value("step").toInt();
        dataPoint["action"] = entry.data.value("action").toString();
        dataPoint["pressure"] = entry.data.value("pressure").toDouble();
        dataPoint["duration"] = entry.data.value("duration").toInt();
        
        data.append(dataPoint);
    }
    
    return data;
}

QJsonArray DataExporter::collectSafetyData(const QDateTime& startTime, const QDateTime& endTime)
{
    QJsonArray data;
    
    if (!m_logger) {
        qWarning() << "Data logger not available";
        return data;
    }
    
    // Get safety log entries from the logger
    auto logEntries = m_logger->getLogEntries(startTime, endTime, DataLogger::SAFETY_EVENTS);
    
    for (const auto& entry : logEntries) {
        QJsonObject dataPoint;
        dataPoint["timestamp"] = entry.timestamp;
        dataPoint["event_type"] = entry.event;
        dataPoint["severity"] = entry.data.value("severity").toString();
        dataPoint["component"] = entry.component;
        dataPoint["message"] = entry.data.value("message").toString();
        dataPoint["data"] = entry.data;
        
        data.append(dataPoint);
    }
    
    return data;
}

bool DataExporter::exportToCSV(const QString& filePath, const ReportData& data)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportError(QString("Cannot open file for writing: %1").arg(filePath));
        return false;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // Write header based on data type
    if (!data.dataPoints.isEmpty()) {
        QJsonObject firstItem = data.dataPoints[0].toObject();
        QString dataType = firstItem.value("data_type").toString();
        
        if (dataType == "pressure") {
            stream << CSV_HEADERS_PRESSURE.join(",") << "\n";
        } else if (dataType == "pattern") {
            stream << CSV_HEADERS_PATTERN.join(",") << "\n";
        } else if (dataType == "safety") {
            stream << CSV_HEADERS_SAFETY.join(",") << "\n";
        } else {
            // Generic header
            QStringList keys = firstItem.keys();
            keys.removeAll("data_type");
            stream << keys.join(",") << "\n";
        }
    }
    
    // Write data rows
    for (const auto& item : data.dataPoints) {
        QJsonObject obj = item.toObject();
        QStringList values;
        
        QString dataType = obj.value("data_type").toString();
        if (dataType == "pressure") {
            values << QDateTime::fromMSecsSinceEpoch(obj.value("timestamp").toVariant().toLongLong()).toString(Qt::ISODate)
                   << QString::number(obj.value("avl_pressure").toDouble(), 'f', 2)
                   << QString::number(obj.value("tank_pressure").toDouble(), 'f', 2)
                   << QString::number(obj.value("target_pressure").toDouble(), 'f', 2)
                   << obj.value("pattern_name").toString();
        } else if (dataType == "pattern") {
            values << QDateTime::fromMSecsSinceEpoch(obj.value("timestamp").toVariant().toLongLong()).toString(Qt::ISODate)
                   << obj.value("pattern_name").toString()
                   << QString::number(obj.value("step").toInt())
                   << obj.value("action").toString()
                   << QString::number(obj.value("pressure").toDouble(), 'f', 2)
                   << QString::number(obj.value("duration").toInt());
        } else if (dataType == "safety") {
            values << QDateTime::fromMSecsSinceEpoch(obj.value("timestamp").toVariant().toLongLong()).toString(Qt::ISODate)
                   << obj.value("event_type").toString()
                   << obj.value("severity").toString()
                   << obj.value("component").toString()
                   << obj.value("message").toString()
                   << QJsonDocument(obj.value("data").toObject()).toJson(QJsonDocument::Compact);
        } else {
            // Generic export
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if (it.key() != "data_type") {
                    values << it.value().toVariant().toString();
                }
            }
        }
        
        // Escape CSV values
        for (QString& value : values) {
            if (value.contains(",") || value.contains("\"") || value.contains("\n")) {
                value = "\"" + value.replace("\"", "\"\"") + "\"";
            }
        }
        
        stream << values.join(",") << "\n";
    }
    
    file.close();
    qDebug() << "Exported" << data.dataPoints.size() << "data points to CSV:" << filePath;
    return true;
}

bool DataExporter::exportToJSON(const QString& filePath, const ReportData& data)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportError(QString("Cannot open file for writing: %1").arg(filePath));
        return false;
    }
    
    QJsonObject root;
    root["title"] = data.title;
    root["description"] = data.description;
    root["generated_time"] = data.generatedTime.toString(Qt::ISODate);
    root["start_time"] = data.startTime.toString(Qt::ISODate);
    root["end_time"] = data.endTime.toString(Qt::ISODate);
    root["metadata"] = data.metadata;
    root["statistics"] = data.statistics;
    root["data_points"] = data.dataPoints;
    
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
    
    qDebug() << "Exported" << data.dataPoints.size() << "data points to JSON:" << filePath;
    return true;
}

bool DataExporter::exportToXML(const QString& filePath, const ReportData& data)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportError(QString("Cannot open file for writing: %1").arg(filePath));
        return false;
    }
    
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    
    xml.writeStartElement("report");
    xml.writeAttribute("title", data.title);
    xml.writeAttribute("generated", data.generatedTime.toString(Qt::ISODate));
    
    xml.writeTextElement("description", data.description);
    xml.writeTextElement("start_time", data.startTime.toString(Qt::ISODate));
    xml.writeTextElement("end_time", data.endTime.toString(Qt::ISODate));
    
    // Write statistics
    xml.writeStartElement("statistics");
    for (auto it = data.statistics.begin(); it != data.statistics.end(); ++it) {
        xml.writeTextElement(it.key(), it.value().toVariant().toString());
    }
    xml.writeEndElement(); // statistics
    
    // Write data points
    xml.writeStartElement("data_points");
    xml.writeAttribute("count", QString::number(data.dataPoints.size()));
    
    for (const auto& item : data.dataPoints) {
        QJsonObject obj = item.toObject();
        xml.writeStartElement("data_point");
        
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            xml.writeTextElement(it.key(), it.value().toVariant().toString());
        }
        
        xml.writeEndElement(); // data_point
    }
    
    xml.writeEndElement(); // data_points
    xml.writeEndElement(); // report
    xml.writeEndDocument();
    
    file.close();
    qDebug() << "Exported" << data.dataPoints.size() << "data points to XML:" << filePath;
    return true;
}

QJsonObject DataExporter::calculateSessionStatistics(const QJsonArray& pressureData, const QJsonArray& patternData, const QJsonArray& safetyData)
{
    QJsonObject stats;
    
    stats["pressure_data_points"] = pressureData.size();
    stats["pattern_data_points"] = patternData.size();
    stats["safety_events"] = safetyData.size();
    
    // Calculate pressure statistics
    if (!pressureData.isEmpty()) {
        double minPressure = 1000.0, maxPressure = 0.0, avgPressure = 0.0;
        int validPoints = 0;
        
        for (const auto& item : pressureData) {
            QJsonObject obj = item.toObject();
            double pressure = obj.value("avl_pressure").toDouble();
            
            if (pressure >= 0.0) {
                minPressure = qMin(minPressure, pressure);
                maxPressure = qMax(maxPressure, pressure);
                avgPressure += pressure;
                validPoints++;
            }
        }
        
        if (validPoints > 0) {
            avgPressure /= validPoints;
            stats["min_pressure"] = minPressure;
            stats["max_pressure"] = maxPressure;
            stats["avg_pressure"] = avgPressure;
        }
    }
    
    return stats;
}

void DataExporter::onScheduledReportTimer()
{
    // Check for scheduled reports that need to be generated
    // This is a placeholder for scheduled reporting functionality
    qDebug() << "Checking for scheduled reports...";
}

// Missing slot implementations
void DataExporter::exportCurrentSession()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime sessionStart = now.addSecs(-3600); // Last hour as current session

    ExportOptions options;
    options.format = CSV_FORMAT;
    options.reportType = SESSION_SUMMARY;
    options.startTime = sessionStart;
    options.endTime = now;
    options.outputPath = "current_session_export.csv";

    exportData(options);
}

void DataExporter::exportLastHour()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime hourAgo = now.addSecs(-3600);

    ExportOptions options;
    options.format = CSV_FORMAT;
    options.reportType = SESSION_SUMMARY;
    options.startTime = hourAgo;
    options.endTime = now;
    options.outputPath = "last_hour_export.csv";

    exportData(options);
}

void DataExporter::exportLastDay()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime dayAgo = now.addDays(-1);

    ExportOptions options;
    options.format = CSV_FORMAT;
    options.reportType = SESSION_SUMMARY;
    options.startTime = dayAgo;
    options.endTime = now;
    options.outputPath = "last_day_export.csv";

    exportData(options);
}

void DataExporter::exportLastWeek()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime weekAgo = now.addDays(-7);

    ExportOptions options;
    options.format = CSV_FORMAT;
    options.reportType = SESSION_SUMMARY;
    options.startTime = weekAgo;
    options.endTime = now;
    options.outputPath = "last_week_export.csv";

    exportData(options);
}

void DataExporter::generateScheduledReports()
{
    // Placeholder for scheduled report generation
    qDebug() << "Generating scheduled reports...";
    emit reportGenerated(SESSION_SUMMARY, "scheduled_report.csv");
}

// Missing method implementations
QJsonObject DataExporter::calculateSafetyStatistics(const QJsonArray& data)
{
    QJsonObject stats;
    stats["total_events"] = data.size();
    stats["critical_events"] = 0;
    stats["warning_events"] = 0;

    for (const auto& value : data) {
        QJsonObject event = value.toObject();
        QString severity = event["severity"].toString();
        if (severity == "critical") {
            stats["critical_events"] = stats["critical_events"].toInt() + 1;
        } else if (severity == "warning") {
            stats["warning_events"] = stats["warning_events"].toInt() + 1;
        }
    }

    return stats;
}

DataExporter::ReportData DataExporter::generatePerformanceReport(const QDateTime& startTime, const QDateTime& endTime)
{
    ReportData report;
    report.title = "Performance Analysis Report";
    report.description = QString("System performance analysis from %1 to %2")
                        .arg(startTime.toString())
                        .arg(endTime.toString());
    report.startTime = startTime;
    report.endTime = endTime;

    // Add basic performance statistics
    QJsonObject perfStats;
    perfStats["uptime_hours"] = startTime.secsTo(endTime) / 3600.0;
    perfStats["avg_pressure"] = 0.0; // Placeholder
    perfStats["pattern_executions"] = 0; // Placeholder

    report.statistics = perfStats;
    return report;
}

QJsonArray DataExporter::collectErrorData(const QDateTime& startTime, const QDateTime& endTime)
{
    QJsonArray errorData;

    if (!m_logger) {
        qWarning() << "Data logger not available";
        return errorData;
    }

    // Get error log entries from the logger
    auto logEntries = m_logger->getLogEntries(startTime, endTime, DataLogger::ERROR_EVENTS);

    for (const auto& entry : logEntries) {
        QJsonObject errorPoint;
        errorPoint["timestamp"] = entry.timestamp;
        errorPoint["error_code"] = entry.data.value("error_code").toString();
        errorPoint["error_message"] = entry.data.value("error_message").toString();
        errorPoint["severity"] = entry.data.value("severity").toString();
        errorData.append(errorPoint);
    }

    return errorData;
}

bool DataExporter::exportToExcel(const QString& filePath, const ReportData& data)
{
    Q_UNUSED(filePath);
    Q_UNUSED(data);

    // Excel export not implemented yet
    qWarning() << "Excel export not implemented";
    return false;
}

bool DataExporter::exportToPDF(const QString& filePath, const ReportData& data)
{
    Q_UNUSED(filePath);
    Q_UNUSED(data);

    // PDF export not implemented yet
    qWarning() << "PDF export not implemented";
    return false;
}

DataExporter::ReportData DataExporter::generatePatternUsageReport(const QDateTime& startTime, const QDateTime& endTime)
{
    ReportData report;
    report.title = "Pattern Usage Report";
    report.description = QString("Pattern usage analysis from %1 to %2")
                        .arg(startTime.toString())
                        .arg(endTime.toString());
    report.startTime = startTime;
    report.endTime = endTime;

    // Collect pattern data
    QJsonArray patternData = collectPatternData(startTime, endTime);
    report.dataPoints = patternData;

    // Add basic pattern statistics
    QJsonObject patternStats;
    patternStats["total_patterns"] = patternData.size();
    patternStats["unique_patterns"] = 0; // Placeholder
    patternStats["avg_duration"] = 0.0; // Placeholder

    report.statistics = patternStats;
    return report;
}
