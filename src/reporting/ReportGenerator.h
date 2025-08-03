#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QObject>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QTextDocument>
#include <QPrinter>
#include <QChart>

/**
 * @brief Professional report generation system
 * 
 * This system provides:
 * - Professional PDF report generation
 * - Customizable report templates
 * - Chart and graph integration
 * - Statistical analysis inclusion
 * - Compliance reporting for medical devices
 * - Multi-language support
 * - Corporate branding integration
 * - Automated report distribution
 */
class ReportGenerator : public QObject
{
    Q_OBJECT

public:
    enum ReportTemplate {
        STANDARD_TEMPLATE,      // Standard report layout
        MEDICAL_TEMPLATE,       // Medical device compliance template
        TECHNICAL_TEMPLATE,     // Technical analysis template
        EXECUTIVE_TEMPLATE,     // Executive summary template
        MAINTENANCE_TEMPLATE,   // Maintenance and service template
        CUSTOM_TEMPLATE         // User-defined template
    };

    struct ReportSection {
        QString title;
        QString content;
        QStringList chartPaths;
        QJsonObject data;
        bool includeInTOC;
        int pageBreakBefore;
        
        ReportSection() : includeInTOC(true), pageBreakBefore(0) {}
        ReportSection(const QString& t, const QString& c, bool toc = true)
            : title(t), content(c), includeInTOC(toc), pageBreakBefore(0) {}
    };

    struct ReportConfiguration {
        ReportTemplate templateType;
        QString title;
        QString subtitle;
        QString author;
        QString organization;
        QString logoPath;
        QDateTime reportDate;
        QString language;
        bool includeTableOfContents;
        bool includeExecutiveSummary;
        bool includeCharts;
        bool includeRawData;
        QJsonObject customSettings;
        
        ReportConfiguration() : templateType(STANDARD_TEMPLATE), 
                               reportDate(QDateTime::currentDateTime()),
                               language("en"), includeTableOfContents(true),
                               includeExecutiveSummary(true), includeCharts(true),
                               includeRawData(false) {}
    };

    explicit ReportGenerator(QObject *parent = nullptr);
    ~ReportGenerator();

    // Report generation
    bool generateReport(const QString& outputPath, const ReportConfiguration& config, 
                       const QList<ReportSection>& sections);
    bool generateSessionReport(const QString& outputPath, const QJsonObject& sessionData);
    bool generateSafetyReport(const QString& outputPath, const QJsonObject& safetyData);
    bool generateMaintenanceReport(const QString& outputPath, const QJsonObject& maintenanceData);
    bool generateComplianceReport(const QString& outputPath, const QJsonObject& complianceData);
    
    // Template management
    void setReportTemplate(ReportTemplate templateType);
    bool loadCustomTemplate(const QString& templatePath);
    bool saveCustomTemplate(const QString& templatePath, const ReportConfiguration& config);
    QStringList getAvailableTemplates() const;
    
    // Content generation
    QString generateExecutiveSummary(const QJsonObject& data);
    QString generateStatisticalSummary(const QJsonObject& statistics);
    QString generateSafetyAnalysis(const QJsonArray& safetyEvents);
    QString generatePatternAnalysis(const QJsonArray& patternData);
    QString generateRecommendations(const QJsonObject& analysisData);
    
    // Chart integration
    bool addChartToReport(const QString& chartPath, const QString& caption, const QString& description = QString());
    bool generatePressureChart(const QString& outputPath, const QJsonArray& pressureData, const QString& title);
    bool generatePatternChart(const QString& outputPath, const QJsonArray& patternData, const QString& title);
    bool generateStatisticsChart(const QString& outputPath, const QJsonObject& statistics, const QString& title);
    
    // Formatting and styling
    void setReportStyle(const QString& cssStyleSheet);
    void setHeaderFooter(const QString& header, const QString& footer);
    void setPageMargins(int left, int top, int right, int bottom);
    void setFontFamily(const QString& fontFamily);
    void setFontSize(int fontSize);
    
    // Localization
    void setLanguage(const QString& languageCode);
    QString getLocalizedString(const QString& key) const;
    void loadTranslations(const QString& translationPath);

public Q_SLOTS:
    void generateScheduledReport();
    void previewReport();

Q_SIGNALS:
    void reportGenerationStarted(const QString& reportPath);
    void reportGenerationProgress(int percentage);
    void reportGenerationCompleted(const QString& reportPath, bool success);
    void reportGenerationError(const QString& error);

private:
    void initializeGenerator();
    void setupTemplates();
    void loadDefaultTranslations();
    
    // Document building
    QTextDocument* createReportDocument(const ReportConfiguration& config, const QList<ReportSection>& sections);
    void addTitlePage(QTextDocument* document, const ReportConfiguration& config);
    void addTableOfContents(QTextDocument* document, const QList<ReportSection>& sections);
    void addExecutiveSummary(QTextDocument* document, const QJsonObject& data);
    void addSection(QTextDocument* document, const ReportSection& section);
    void addChart(QTextDocument* document, const QString& chartPath, const QString& caption);
    void addFooter(QTextDocument* document, const ReportConfiguration& config);
    
    // Content formatting
    QString formatSectionContent(const QString& content, const QJsonObject& data);
    QString formatStatistics(const QJsonObject& statistics);
    QString formatDataTable(const QJsonArray& data, const QStringList& headers);
    QString formatTimestamp(qint64 timestamp, const QString& format = "yyyy-MM-dd hh:mm:ss");
    QString formatDuration(qint64 milliseconds);
    QString formatPressure(double pressure, int decimals = 1);
    
    // Chart generation
    QChart* createPressureChart(const QJsonArray& data, const QString& title);
    QChart* createPatternChart(const QJsonArray& data, const QString& title);
    QChart* createStatisticsChart(const QJsonObject& statistics, const QString& title);
    bool saveChartToFile(QChart* chart, const QString& filePath, const QSize& size = QSize(800, 600));
    
    // Template processing
    QString processTemplate(const QString& templateContent, const QJsonObject& data);
    QString applyTemplateStyle(const QString& content, ReportTemplate templateType);
    QJsonObject getTemplateConfiguration(ReportTemplate templateType);
    
    // PDF generation
    bool generatePDF(QTextDocument* document, const QString& outputPath, const ReportConfiguration& config);
    void configurePrinter(QPrinter* printer, const ReportConfiguration& config);
    
    // Validation
    bool validateReportConfiguration(const ReportConfiguration& config);
    bool validateSectionData(const ReportSection& section);
    
    // Current configuration
    ReportConfiguration m_currentConfig;
    ReportTemplate m_currentTemplate;
    QString m_customTemplatePath;
    
    // Styling
    QString m_cssStyleSheet;
    QString m_headerTemplate;
    QString m_footerTemplate;
    QMargins m_pageMargins;
    QString m_fontFamily;
    int m_fontSize;
    
    // Localization
    QString m_currentLanguage;
    QMap<QString, QString> m_translations;
    
    // Template storage
    QMap<ReportTemplate, QString> m_templatePaths;
    QMap<ReportTemplate, QJsonObject> m_templateConfigs;
    
    // Chart storage
    QStringList m_generatedCharts;
    QString m_chartOutputDirectory;
    
    // Constants
    static const QString DEFAULT_FONT_FAMILY;
    static const int DEFAULT_FONT_SIZE = 12;
    static const QMargins DEFAULT_PAGE_MARGINS;
    static const QString DEFAULT_CSS_STYLE;
    static const QString DEFAULT_HEADER_TEMPLATE;
    static const QString DEFAULT_FOOTER_TEMPLATE;
    static const QSize DEFAULT_CHART_SIZE;
    
    // Template constants
    static const QString STANDARD_TEMPLATE_CSS;
    static const QString MEDICAL_TEMPLATE_CSS;
    static const QString TECHNICAL_TEMPLATE_CSS;
    static const QString EXECUTIVE_TEMPLATE_CSS;
    static const QString MAINTENANCE_TEMPLATE_CSS;
};

#endif // REPORTGENERATOR_H
