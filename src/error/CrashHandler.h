#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <QProcess>
#include <signal.h>

/**
 * @brief Crash detection and recovery system
 * 
 * This system provides:
 * - Signal handling for crashes (SIGSEGV, SIGABRT, etc.)
 * - Automatic crash detection via heartbeat
 * - Safe shutdown procedures
 * - Crash report generation
 * - Automatic restart capabilities
 * - System state preservation
 */
class CrashHandler : public QObject
{
    Q_OBJECT

public:
    explicit CrashHandler(QObject *parent = nullptr);
    ~CrashHandler();

    // Crash handling setup
    static void installSignalHandlers();
    static void uninstallSignalHandlers();
    
    // Heartbeat system
    void startHeartbeat();
    void stopHeartbeat();
    void updateHeartbeat();
    
    // Crash detection
    bool detectPreviousCrash();
    QString getLastCrashReport();
    void clearCrashData();
    
    // Recovery configuration
    void setAutoRestart(bool enabled);
    void setMaxRestartAttempts(int maxAttempts);
    void setRestartDelay(int delaySeconds);
    
    // Safe shutdown
    void requestSafeShutdown();
    void forceSafeShutdown();
    
    // System state
    void saveSystemState();
    void restoreSystemState();

public slots:
    void onApplicationAboutToQuit();

signals:
    void crashDetected(const QString& crashInfo);
    void safeShutdownRequested();
    void systemStateRestored();

private slots:
    void onHeartbeatTimer();
    void checkForCrash();

private:
    static void signalHandler(int signal);
    static void generateCrashReport(int signal);
    static void performEmergencyShutdown();
    
    void initializeCrashHandler();
    void setupHeartbeatFile();
    void writeHeartbeat();
    bool isHeartbeatStale();
    void createCrashReport(const QString& reason, const QString& details = QString());
    void attemptRestart();
    
    // Heartbeat system
    QTimer* m_heartbeatTimer;
    QString m_heartbeatFilePath;
    QString m_crashReportPath;
    QString m_systemStatePath;
    int m_heartbeatInterval;
    qint64 m_lastHeartbeat;
    
    // Recovery configuration
    bool m_autoRestart;
    int m_maxRestartAttempts;
    int m_restartDelay;
    int m_currentRestartAttempts;
    
    // Crash detection
    bool m_crashDetected;
    QString m_lastCrashInfo;
    
    // Safe shutdown
    bool m_safeShutdownInProgress;
    QTimer* m_shutdownTimer;
    
    // Static members for signal handling
    static CrashHandler* s_instance;
    static bool s_signalHandlersInstalled;
    static QString s_crashReportPath;
    
    // Constants
    static const int DEFAULT_HEARTBEAT_INTERVAL = 5000;  // 5 seconds
    static const int DEFAULT_RESTART_DELAY = 10;         // 10 seconds
    static const int DEFAULT_MAX_RESTART_ATTEMPTS = 3;
    static const int HEARTBEAT_STALE_THRESHOLD = 15000;  // 15 seconds
    static const int SAFE_SHUTDOWN_TIMEOUT = 30000;     // 30 seconds
};

#endif // CRASHHANDLER_H
