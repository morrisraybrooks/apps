#include "CrashHandler.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <unistd.h>
#include <sys/types.h>
#include <execinfo.h>

// Static member initialization
CrashHandler* CrashHandler::s_instance = nullptr;
bool CrashHandler::s_signalHandlersInstalled = false;
QString CrashHandler::s_crashReportPath;

CrashHandler::CrashHandler(QObject *parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_heartbeatInterval(DEFAULT_HEARTBEAT_INTERVAL)
    , m_lastHeartbeat(0)
    , m_autoRestart(true)
    , m_maxRestartAttempts(DEFAULT_MAX_RESTART_ATTEMPTS)
    , m_restartDelay(DEFAULT_RESTART_DELAY)
    , m_currentRestartAttempts(0)
    , m_crashDetected(false)
    , m_safeShutdownInProgress(false)
    , m_shutdownTimer(new QTimer(this))
{
    s_instance = this;
    
    // Setup file paths
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    
    m_heartbeatFilePath = appDataPath + "/heartbeat.txt";
    m_crashReportPath = appDataPath + "/crash_reports";
    m_systemStatePath = appDataPath + "/system_state.json";
    s_crashReportPath = m_crashReportPath;
    
    // Create crash reports directory
    QDir().mkpath(m_crashReportPath);
    
    // Setup timers
    m_heartbeatTimer->setInterval(m_heartbeatInterval);
    m_shutdownTimer->setSingleShot(true);
    m_shutdownTimer->setInterval(SAFE_SHUTDOWN_TIMEOUT);
    
    connect(m_heartbeatTimer, &QTimer::timeout, this, &CrashHandler::onHeartbeatTimer);
    connect(m_shutdownTimer, &QTimer::timeout, this, &CrashHandler::forceSafeShutdown);
    
    // Connect to application quit signal
    connect(QApplication::instance(), &QApplication::aboutToQuit, 
            this, &CrashHandler::onApplicationAboutToQuit);
    
    initializeCrashHandler();
    
    qDebug() << "CrashHandler initialized";
}

CrashHandler::~CrashHandler()
{
    stopHeartbeat();
    uninstallSignalHandlers();
    s_instance = nullptr;
}

void CrashHandler::installSignalHandlers()
{
    if (s_signalHandlersInstalled) return;
    
    // Install signal handlers for common crash signals
    signal(SIGSEGV, signalHandler);  // Segmentation fault
    signal(SIGABRT, signalHandler);  // Abort signal
    signal(SIGFPE, signalHandler);   // Floating point exception
    signal(SIGILL, signalHandler);   // Illegal instruction
    signal(SIGTERM, signalHandler);  // Termination request
    signal(SIGINT, signalHandler);   // Interrupt signal (Ctrl+C)
    
    s_signalHandlersInstalled = true;
    
    qDebug() << "Signal handlers installed";
}

void CrashHandler::uninstallSignalHandlers()
{
    if (!s_signalHandlersInstalled) return;
    
    // Restore default signal handlers
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    
    s_signalHandlersInstalled = false;
    
    qDebug() << "Signal handlers uninstalled";
}

void CrashHandler::startHeartbeat()
{
    setupHeartbeatFile();
    m_heartbeatTimer->start();
    updateHeartbeat();
    
    qDebug() << "Heartbeat started";
}

void CrashHandler::stopHeartbeat()
{
    m_heartbeatTimer->stop();
    
    // Remove heartbeat file
    QFile::remove(m_heartbeatFilePath);
    
    qDebug() << "Heartbeat stopped";
}

void CrashHandler::updateHeartbeat()
{
    m_lastHeartbeat = QDateTime::currentMSecsSinceEpoch();
    writeHeartbeat();
}

bool CrashHandler::detectPreviousCrash()
{
    // Check if heartbeat file exists and is stale
    QFile heartbeatFile(m_heartbeatFilePath);
    if (!heartbeatFile.exists()) {
        return false; // No previous session
    }
    
    if (heartbeatFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&heartbeatFile);
        QString timestampStr = stream.readLine();
        heartbeatFile.close();
        
        bool ok;
        qint64 lastHeartbeat = timestampStr.toLongLong(&ok);
        
        if (ok) {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            qint64 timeDiff = currentTime - lastHeartbeat;
            
            if (timeDiff > HEARTBEAT_STALE_THRESHOLD) {
                m_crashDetected = true;
                m_lastCrashInfo = QString("Previous session crashed. Last heartbeat: %1 ms ago")
                                 .arg(timeDiff);
                
                // Create crash report for the detected crash
                createCrashReport("Heartbeat timeout", m_lastCrashInfo);
                
                qWarning() << "Previous crash detected:" << m_lastCrashInfo;
                emit crashDetected(m_lastCrashInfo);
                
                return true;
            }
        }
    }
    
    return false;
}

QString CrashHandler::getLastCrashReport()
{
    // Find the most recent crash report
    QDir crashDir(m_crashReportPath);
    QStringList filters;
    filters << "crash_*.txt";
    
    QFileInfoList crashFiles = crashDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    if (!crashFiles.isEmpty()) {
        QFile crashFile(crashFiles.first().absoluteFilePath());
        if (crashFile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&crashFile);
            return stream.readAll();
        }
    }
    
    return QString();
}

void CrashHandler::clearCrashData()
{
    // Remove all crash reports
    QDir crashDir(m_crashReportPath);
    QStringList filters;
    filters << "crash_*.txt";
    
    QFileInfoList crashFiles = crashDir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo& fileInfo : crashFiles) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
    
    // Clear crash detection flag
    m_crashDetected = false;
    m_lastCrashInfo.clear();
    
    qDebug() << "Crash data cleared";
}

void CrashHandler::setAutoRestart(bool enabled)
{
    m_autoRestart = enabled;
}

void CrashHandler::setMaxRestartAttempts(int maxAttempts)
{
    m_maxRestartAttempts = qMax(0, maxAttempts);
}

void CrashHandler::setRestartDelay(int delaySeconds)
{
    m_restartDelay = qMax(1, delaySeconds);
}

void CrashHandler::requestSafeShutdown()
{
    if (m_safeShutdownInProgress) return;
    
    m_safeShutdownInProgress = true;
    
    qDebug() << "Safe shutdown requested";
    
    // Save system state before shutdown
    saveSystemState();
    
    // Start shutdown timer as fallback
    m_shutdownTimer->start();
    
    emit safeShutdownRequested();
}

void CrashHandler::forceSafeShutdown()
{
    qWarning() << "Forcing safe shutdown due to timeout";
    
    // Perform emergency shutdown
    performEmergencyShutdown();
    
    // Exit application
    QApplication::quit();
}

void CrashHandler::saveSystemState()
{
    try {
        QJsonObject state;
        state["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        state["application_version"] = QApplication::applicationVersion();
        state["crash_detected"] = m_crashDetected;
        state["restart_attempts"] = m_currentRestartAttempts;
        state["safe_shutdown"] = m_safeShutdownInProgress;
        
        // Add system information
        state["process_id"] = QApplication::applicationPid();
        state["thread_count"] = QThread::idealThreadCount();
        
        QJsonDocument doc(state);
        
        QFile stateFile(m_systemStatePath);
        if (stateFile.open(QIODevice::WriteOnly)) {
            stateFile.write(doc.toJson());
            stateFile.close();
            
            qDebug() << "System state saved";
        } else {
            qWarning() << "Failed to save system state";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception while saving system state:" << e.what();
    }
}

void CrashHandler::restoreSystemState()
{
    QFile stateFile(m_systemStatePath);
    if (!stateFile.exists()) {
        qDebug() << "No previous system state found";
        return;
    }
    
    if (stateFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(stateFile.readAll());
        stateFile.close();
        
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject state = doc.object();
            
            // Check if previous session had a crash
            if (state["crash_detected"].toBool()) {
                m_crashDetected = true;
                m_lastCrashInfo = "Previous session had detected crashes";
            }
            
            // Get restart attempt count
            m_currentRestartAttempts = state["restart_attempts"].toInt();
            
            qDebug() << "System state restored";
            emit systemStateRestored();
        }
    }
}

// Slot implementations
void CrashHandler::onApplicationAboutToQuit()
{
    qDebug() << "Application about to quit - performing cleanup";
    
    // Save system state
    saveSystemState();
    
    // Stop heartbeat
    stopHeartbeat();
}

void CrashHandler::onHeartbeatTimer()
{
    updateHeartbeat();
}

void CrashHandler::checkForCrash()
{
    if (isHeartbeatStale()) {
        m_crashDetected = true;
        m_lastCrashInfo = "Heartbeat timeout detected";
        
        createCrashReport("Heartbeat timeout", m_lastCrashInfo);
        emit crashDetected(m_lastCrashInfo);
    }
}

// Static signal handler
void CrashHandler::signalHandler(int signal)
{
    // Generate crash report immediately
    generateCrashReport(signal);

    // Perform emergency shutdown
    performEmergencyShutdown();

    // Restore default handler and re-raise signal
    ::signal(signal, SIG_DFL);
    raise(signal);
}

void CrashHandler::generateCrashReport(int signal)
{
    QString signalName;
    QString signalDescription;

    switch (signal) {
    case SIGSEGV:
        signalName = "SIGSEGV";
        signalDescription = "Segmentation fault";
        break;
    case SIGABRT:
        signalName = "SIGABRT";
        signalDescription = "Abort signal";
        break;
    case SIGFPE:
        signalName = "SIGFPE";
        signalDescription = "Floating point exception";
        break;
    case SIGILL:
        signalName = "SIGILL";
        signalDescription = "Illegal instruction";
        break;
    case SIGTERM:
        signalName = "SIGTERM";
        signalDescription = "Termination request";
        break;
    case SIGINT:
        signalName = "SIGINT";
        signalDescription = "Interrupt signal";
        break;
    default:
        signalName = QString("SIGNAL_%1").arg(signal);
        signalDescription = "Unknown signal";
        break;
    }

    // Create crash report file
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString crashFileName = QString("crash_%1_%2.txt").arg(timestamp).arg(signalName);
    QString crashFilePath = s_crashReportPath + "/" + crashFileName;

    QFile crashFile(crashFilePath);
    if (crashFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&crashFile);

        stream << "=== CRASH REPORT ===\n";
        stream << "Timestamp: " << QDateTime::currentDateTime().toString() << "\n";
        stream << "Signal: " << signalName << " (" << signal << ")\n";
        stream << "Description: " << signalDescription << "\n";
        stream << "Process ID: " << getpid() << "\n";
        stream << "Application: " << QApplication::applicationName() << "\n";
        stream << "Version: " << QApplication::applicationVersion() << "\n";
        stream << "\n";

        // Get stack trace if available
        void* array[10];
        size_t size = backtrace(array, 10);
        char** strings = backtrace_symbols(array, size);

        if (strings) {
            stream << "Stack Trace:\n";
            for (size_t i = 0; i < size; ++i) {
                stream << "  " << strings[i] << "\n";
            }
            free(strings);
        }

        stream << "\n=== END CRASH REPORT ===\n";
        crashFile.close();
    }
}

void CrashHandler::performEmergencyShutdown()
{
    // Perform minimal cleanup operations
    // This must be signal-safe and minimal

    // Remove heartbeat file to indicate clean shutdown attempt
    unlink(s_instance ? s_instance->m_heartbeatFilePath.toLocal8Bit().constData() : "");

    // Sync file systems
    sync();
}

// Private methods
void CrashHandler::initializeCrashHandler()
{
    // Install signal handlers
    installSignalHandlers();

    // Check for previous crash
    detectPreviousCrash();

    // Restore system state if available
    restoreSystemState();

    qDebug() << "CrashHandler initialization complete";
}

void CrashHandler::setupHeartbeatFile()
{
    // Create heartbeat file
    QFile heartbeatFile(m_heartbeatFilePath);
    if (heartbeatFile.open(QIODevice::WriteOnly)) {
        heartbeatFile.close();
    }
}

void CrashHandler::writeHeartbeat()
{
    QFile heartbeatFile(m_heartbeatFilePath);
    if (heartbeatFile.open(QIODevice::WriteOnly)) {
        QTextStream stream(&heartbeatFile);
        stream << m_lastHeartbeat << "\n";
        stream << QApplication::applicationPid() << "\n";
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        heartbeatFile.close();
    }
}

bool CrashHandler::isHeartbeatStale()
{
    QFile heartbeatFile(m_heartbeatFilePath);
    if (!heartbeatFile.exists()) {
        return false;
    }

    if (heartbeatFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&heartbeatFile);
        QString timestampStr = stream.readLine();
        heartbeatFile.close();

        bool ok;
        qint64 lastHeartbeat = timestampStr.toLongLong(&ok);

        if (ok) {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            return (currentTime - lastHeartbeat) > HEARTBEAT_STALE_THRESHOLD;
        }
    }

    return false;
}

void CrashHandler::createCrashReport(const QString& reason, const QString& details)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString crashFileName = QString("crash_%1.txt").arg(timestamp);
    QString crashFilePath = m_crashReportPath + "/" + crashFileName;

    QFile crashFile(crashFilePath);
    if (crashFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&crashFile);

        stream << "=== CRASH REPORT ===\n";
        stream << "Timestamp: " << QDateTime::currentDateTime().toString() << "\n";
        stream << "Reason: " << reason << "\n";
        stream << "Details: " << details << "\n";
        stream << "Process ID: " << QApplication::applicationPid() << "\n";
        stream << "Application: " << QApplication::applicationName() << "\n";
        stream << "Version: " << QApplication::applicationVersion() << "\n";
        stream << "Restart Attempts: " << m_currentRestartAttempts << "\n";
        stream << "\n=== END CRASH REPORT ===\n";

        crashFile.close();

        qDebug() << "Crash report created:" << crashFilePath;
    }
}

void CrashHandler::attemptRestart()
{
    if (!m_autoRestart) {
        qDebug() << "Auto-restart disabled";
        return;
    }

    if (m_currentRestartAttempts >= m_maxRestartAttempts) {
        qWarning() << "Maximum restart attempts reached:" << m_currentRestartAttempts;
        return;
    }

    m_currentRestartAttempts++;

    qDebug() << "Attempting restart" << m_currentRestartAttempts << "of" << m_maxRestartAttempts;

    // Save current restart attempt count
    saveSystemState();

    // Wait for restart delay
    QThread::sleep(m_restartDelay);

    // Start new process
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    arguments.removeFirst(); // Remove program name

    if (QProcess::startDetached(program, arguments)) {
        qDebug() << "Restart successful";
        QApplication::quit();
    } else {
        qCritical() << "Failed to restart application";
    }
}
