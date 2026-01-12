#ifndef ADAPTIVESEALMONITOR_H
#define ADAPTIVESEALMONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QElapsedTimer>
#include <QQueue>
#include <functional>
#include "SafetyConstants.h"
#include "../core/StatefulComponent.h"

class HardwareManager;
class EmergencyStopCoordinator;
class ISafetyLogger;

/**
 * @brief Enhanced Anti-Detachment System with Adaptive Seal Integrity Monitoring
 *
 * This class extends the basic anti-detachment monitoring with physiological
 * compensation for tissue engorgement during arousal. It implements:
 *
 * 1. ADAPTIVE THRESHOLD CALCULATION
 *    - Compensates for tissue swelling during arousal
 *    - Formula: threshold = baseline * (1 - arousal * AROUSAL_COMPENSATION_FACTOR)
 *    - Prevents false positives during normal arousal progression
 *
 * 2. MULTI-PATH LEAK DETECTION
 *    - Path 1 (Rapid): Rate-of-change > 5 mmHg/100ms = mechanical failure
 *    - Path 2 (Gradual): Below adaptive threshold + no swelling indicators
 *    - Path 3 (Critical): Below absolute minimum (10 mmHg) regardless of state
 *
 * 3. PROGRESSIVE RESEAL RECOVERY
 *    - Attempt 1: 50% pump power, wait 2s
 *    - Attempt 2: 70% pump power, wait 2s
 *    - Attempt 3: 90% pump power, wait 2s
 *    - Session termination if all attempts fail
 *
 * Thread Safety:
 *    - All public methods are thread-safe via mutex protection
 *    - 100Hz monitoring rate with lock-free pressure history
 *
 * Integration:
 *    - Receives arousal level from OrgasmControlAlgorithm
 *    - Coordinates with PatternEngine for critical phase priority
 *    - Uses ISafetyLogger for unified logging
 *    - Integrates with EmergencyStopCoordinator
 */
class AdaptiveSealMonitor : public QObject, public StatefulComponent<int>
{
    Q_OBJECT

public:
    /**
     * @brief Seal monitoring states
     */
    enum SealState {
        SEALED = 0,           // Seal intact, normal operation
        WARNING = 1,          // Pressure approaching threshold
        LEAK_DETECTED = 2,    // Active leak detected, reseal in progress
        RESEALING = 3,        // Progressive reseal attempt underway
        SEAL_LOST = 4,        // All reseal attempts failed
        SYSTEM_ERROR = 5      // Hardware or sensor error
    };
    Q_ENUM(SealState)

    /**
     * @brief Leak detection path that triggered response
     */
    enum LeakPath {
        NONE = 0,
        RAPID_MECHANICAL = 1,   // Path 1: Rapid pressure drop (mechanical failure)
        GRADUAL_LEAK = 2,       // Path 2: Gradual leak (not explained by physiology)
        CRITICAL_LOSS = 3       // Path 3: Below absolute minimum threshold
    };
    Q_ENUM(LeakPath)

    /**
     * @brief Phase priority for enhanced monitoring
     */
    enum PhasePriority {
        NORMAL = 0,           // Standard monitoring (100 Hz)
        ELEVATED = 1,         // Enhanced sensitivity (150 Hz)
        CRITICAL = 2          // Maximum sensitivity (200 Hz, faster response)
    };
    Q_ENUM(PhasePriority)

    explicit AdaptiveSealMonitor(HardwareManager* hardware, QObject* parent = nullptr);
    ~AdaptiveSealMonitor() override;

    // ========================================================================
    // System Control
    // ========================================================================

    bool initialize();
    void shutdown();
    bool isActive() const { return m_active; }

    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();

    // ========================================================================
    // Arousal Integration (called by OrgasmControlAlgorithm)
    // ========================================================================

    /**
     * @brief Update current arousal level for adaptive threshold calculation
     * @param arousalLevel Normalized arousal level (0.0 to 1.0)
     */
    void setArousalLevel(double arousalLevel);
    double getArousalLevel() const { return m_arousalLevel; }

    /**
     * @brief Set clitoral pressure for swelling detection
     * @param pressureMmHg Clitoral cylinder pressure in mmHg
     */
    void setClitoralPressure(double pressureMmHg);

    // ========================================================================
    // Phase Priority (called by PatternEngine)
    // ========================================================================

    /**
     * @brief Set monitoring priority based on pattern phase
     * @param priority Phase priority level
     */
    void setPhasePriority(PhasePriority priority);
    PhasePriority getPhasePriority() const { return m_phasePriority; }

    // ========================================================================
    // Threshold Configuration
    // ========================================================================

    void setBaselineThreshold(double thresholdMmHg);
    double getBaselineThreshold() const { return m_baselineThreshold; }

    double getAdaptiveThreshold() const;  // Returns current arousal-adjusted threshold

    void setHysteresis(double hysteresisMmHg);
    double getHysteresis() const { return m_hysteresis; }

    // ========================================================================
    // Status Queries
    // ========================================================================

    SealState getSealState() const { return static_cast<SealState>(StatefulComponent::getState()); }
    double getCurrentPressure() const { return m_currentAVLPressure; }
    double getPressureRateOfChange() const { return m_pressureRateOfChange; }
    int getResealAttemptCount() const { return m_resealAttemptCount; }
    LeakPath getLastLeakPath() const { return m_lastLeakPath; }

    // Statistics
    int getTotalLeakEvents() const { return m_totalLeakEvents; }
    int getSuccessfulReseals() const { return m_successfulReseals; }
    int getFailedReseals() const { return m_failedReseals; }
    void resetStatistics();

    // ========================================================================
    // Integration Setters
    // ========================================================================

    void setEmergencyStopCoordinator(EmergencyStopCoordinator* coordinator);
    void setSafetyLogger(ISafetyLogger* logger);

Q_SIGNALS:
    // State signals
    void stateChanged(SealState newState);
    void sealIntact();
    void sealWarning(double pressure, double threshold);

    // Leak detection signals
    void leakDetected(LeakPath path, double pressure, double threshold);
    void rapidLeakDetected(double pressure, double rateOfChange);
    void gradualLeakDetected(double pressure, double adaptiveThreshold);
    void criticalPressureLoss(double pressure);

    // Reseal signals
    void resealAttemptStarted(int attemptNumber, double pumpPower);
    void resealSuccessful(int attemptNumber);
    void resealFailed(int attemptNumber);
    void allResealsFailed();
    void sessionTerminated(const QString& reason);

    // System signals
    void systemError(const QString& error);
    void phasePriorityChanged(PhasePriority priority);

private Q_SLOTS:
    void performMonitoringCycle();
    void onResealTimeout();

private:
    // State management
    void setState(SealState newState);
    QString stateToString(int state) const override;
    void onStateTransition(int oldState, int newState);

    // Core monitoring logic
    void processAVLReading(double avlPressure);
    double calculateAdaptiveThreshold() const;
    double calculatePressureRateOfChange();

    // Multi-path leak detection
    LeakPath detectLeak(double avlPressure);
    bool isRapidLeak(double avlPressure, double rateOfChange);
    bool isGradualLeak(double avlPressure, double adaptiveThreshold);
    bool isCriticalLoss(double avlPressure);

    // Progressive reseal logic
    void startResealSequence();
    void executeResealAttempt();
    bool checkResealSuccess();
    void handleResealSuccess();
    void handleResealFailure();
    void terminateSession(const QString& reason);

    // Hardware control
    void closeVent();
    void setPumpPower(double power);

    // Logging
    void logEvent(const QString& event, double pressure);

    // Emergency stop callback
    void onEmergencyStopTriggered(const QString& reason);

    // Hardware interface
    HardwareManager* m_hardware;
    EmergencyStopCoordinator* m_emergencyStopCoordinator;
    ISafetyLogger* m_safetyLogger;

    // System state
    bool m_active;
    bool m_monitoring;
    bool m_paused;

    // Monitoring configuration
    double m_baselineThreshold;
    double m_hysteresis;
    int m_monitoringRateHz;
    PhasePriority m_phasePriority;

    // Arousal integration
    double m_arousalLevel;
    double m_clitoralPressure;
    double m_previousClitoralPressure;
    bool m_clitoralPressureRising;

    // Pressure tracking
    double m_currentAVLPressure;
    double m_previousAVLPressure;
    double m_pressureRateOfChange;
    qint64 m_lastReadingTime;
    QQueue<double> m_pressureHistory;

    // Reseal state
    int m_resealAttemptCount;
    double m_currentResealPower;
    QTimer* m_resealTimer;
    LeakPath m_lastLeakPath;

    // Timers
    QTimer* m_monitoringTimer;
    QElapsedTimer m_resealElapsed;

    // Statistics
    int m_totalLeakEvents;
    int m_successfulReseals;
    int m_failedReseals;

    // Error tracking
    QString m_lastError;
    int m_consecutiveErrors;

    // Thread safety
    mutable QMutex m_mutex;

    // Constants
    static constexpr int PRESSURE_HISTORY_SIZE = 20;  // 200ms at 100Hz
};

#endif // ADAPTIVESEALMONITOR_H

