# Milking Cycle Mode - Implementation Design

## Phase 2: Technical Design Document

Based on the research findings and existing code patterns in `OrgasmControlAlgorithm`.

---

## 1. Overview

### 1.1 Mode Definition

The Milking Cycle mode maintains arousal in a sustained "milking zone" (75-90%) to extract pleasure WITHOUT allowing orgasm. Orgasm is a FAILURE condition.

### 1.2 Key Differences from Existing Modes

| Aspect | Edging/Denial | Milking |
|--------|---------------|---------|
| Arousal pattern | Peaks and valleys (85% → 70% → 85%) | Sustained plateau (75-90%) |
| Intensity control | On/off (boost/back-off) | Continuous PID-like adjustment |
| Orgasm | Allowed (edging) or prevented (denial) | FAILURE condition |
| State cycle | BUILD → EDGE → BACKOFF → HOLD → repeat | BUILD → MILKING (sustained) |

---

## 2. Enum Modifications

### 2.1 Mode Enum Addition

```cpp
// In OrgasmControlAlgorithm.h, line ~61-67
enum class Mode {
    MANUAL,
    ADAPTIVE_EDGING,
    FORCED_ORGASM,
    MULTI_ORGASM,
    DENIAL,
    MILKING           // NEW: Sustained sub-orgasmic stimulation
};
```

### 2.2 ControlState Enum Addition

```cpp
// In OrgasmControlAlgorithm.h, line ~47-57
enum class ControlState {
    STOPPED,
    CALIBRATING,
    BUILDING,
    BACKING_OFF,
    HOLDING,
    FORCING,
    MILKING,           // NEW: Maintaining arousal in milking zone
    DANGER_REDUCTION,  // NEW: Arousal too high, reducing stimulation
    ORGASM_FAILURE,    // NEW: Unwanted orgasm detected
    COOLING_DOWN,
    ERROR
};
```

---

## 3. New Public Methods

### 3.1 Session Start Method

```cpp
// In OrgasmControlAlgorithm.h, public section (~line 74-78)

/**
 * @brief Start a milking session
 * @param durationMs Session duration in milliseconds (default 30 minutes)
 * @param failureMode How to handle orgasm failure (0=stop, 1=ruined, 2=punish, 3=continue)
 */
void startMilking(int durationMs = 1800000, int failureMode = 0);
```

### 3.2 Configuration Methods

```cpp
// In OrgasmControlAlgorithm.h, public section (~line 81-86)

void setMilkingZoneLower(double threshold);  // Default 0.75
void setMilkingZoneUpper(double threshold);  // Default 0.90
void setDangerThreshold(double threshold);   // Default 0.92
void setMilkingFailureMode(int mode);        // 0=stop, 1=ruined, 2=punish, 3=continue
```

### 3.3 Status Getters

```cpp
// In OrgasmControlAlgorithm.h, public section (~line 93-103)

double getMilkingZoneTime() const { return m_milkingZoneTime; }
int getDangerZoneEntries() const { return m_dangerZoneEntries; }
bool isMilkingSuccess() const { return m_milkingOrgasmCount == 0; }
```

---

## 4. New Signals

```cpp
// In OrgasmControlAlgorithm.h, Q_SIGNALS section (~line 105-148)

// Milking mode signals
void milkingZoneEntered(double arousalLevel);
void milkingZoneMaintained(qint64 durationMs, double avgArousal);
void dangerZoneEntered(double arousalLevel);
void dangerZoneExited(double arousalLevel);
void unwantedOrgasm(int orgasmCount, qint64 sessionDurationMs);
void milkingIntensityAdjusted(double newIntensity, double arousalError);
void milkingSessionComplete(qint64 durationMs, bool success, int dangerEntries);
```

---

## 5. New Private Methods

```cpp
// In OrgasmControlAlgorithm.h, private section (~line 166-169)

void runMilking();              // Main milking loop (called by onUpdateTick)
void startMilkingInternal(int durationMs, int failureMode);
double calculateMilkingIntensityAdjustment();  // PID-like control
void handleMilkingOrgasmFailure();             // Failure response
```

---

## 6. New Member Variables

### 6.1 Milking State Variables

```cpp
// In OrgasmControlAlgorithm.h, private section (~line 239-245)

// Milking mode state
int m_milkingFailureMode;       // 0=stop, 1=ruined, 2=punish, 3=continue
int m_milkingOrgasmCount;       // Orgasms during milking (should be 0)
int m_dangerZoneEntries;        // Times arousal exceeded danger threshold
qint64 m_milkingZoneTime;       // Cumulative time in milking zone (ms)
double m_milkingAvgArousal;     // Running average arousal
int m_milkingAvgSamples;        // Sample count for average

// PID control state
double m_milkingIntegralError;  // Accumulated error for I term
double m_milkingPreviousError;  // Previous error for D term
double m_milkingTargetArousal;  // Center of milking zone (0.82)
```

### 6.2 Milking Thresholds

```cpp
// In OrgasmControlAlgorithm.h, private section (~line 230-234)

double m_milkingZoneLower;      // Default 0.75
double m_milkingZoneUpper;      // Default 0.90
double m_dangerThreshold;       // Default 0.92
```

---

## 7. New Constants

```cpp
// In OrgasmControlAlgorithm.h, constants section (~line 296-320)

// Milking mode defaults
static constexpr double DEFAULT_MILKING_ZONE_LOWER = 0.75;
static constexpr double DEFAULT_MILKING_ZONE_UPPER = 0.90;
static constexpr double DEFAULT_DANGER_THRESHOLD = 0.92;
static constexpr double MILKING_TARGET_AROUSAL = 0.82;  // Center of zone

// Milking intensity control
static constexpr double MILKING_BASE_INTENSITY = 0.50;
static constexpr double MILKING_BASE_FREQUENCY = 7.0;
static constexpr double MILKING_MIN_INTENSITY = 0.20;
static constexpr double MILKING_MAX_INTENSITY = 0.70;
static constexpr double MILKING_TENS_AMPLITUDE = 0.40;

// PID gains for arousal maintenance
static constexpr double MILKING_PID_KP = 0.30;   // Proportional gain
static constexpr double MILKING_PID_KI = 0.05;   // Integral gain
static constexpr double MILKING_PID_KD = 0.10;   // Derivative gain

// Timing
static constexpr int MILKING_ADJUSTMENT_INTERVAL_MS = 500;
static constexpr int MILKING_ZONE_REPORT_INTERVAL_MS = 10000;  // Report every 10 sec
static constexpr int MILKING_MAX_SESSION_MS = 3600000;  // 60 min max

// Danger zone recovery
static constexpr double DANGER_RECOVERY_THRESHOLD = 0.88;  // Exit danger when below this
static constexpr int DANGER_PAUSE_DURATION_MS = 2000;      // Pause stimulation duration
```

---

## 8. Implementation: runMilking()

### 8.1 Pseudocode

```cpp
void OrgasmControlAlgorithm::runMilking()
{
    // Check session timeout
    if (m_sessionTimer.elapsed() >= m_maxDurationMs) {
        emit milkingSessionComplete(m_sessionTimer.elapsed(),
                                    m_milkingOrgasmCount == 0,
                                    m_dangerZoneEntries);
        startCoolDown();
        return;
    }

    // Update arousal
    updateArousalLevel();

    switch (m_state) {
    case ControlState::CALIBRATING:
        // Same as existing calibration logic
        break;

    case ControlState::BUILDING:
        // Increase stimulation until reaching milking zone
        if (m_arousalLevel >= m_milkingZoneLower) {
            emit milkingZoneEntered(m_arousalLevel);
            m_stateTimer.start();
            setState(ControlState::MILKING);
        } else {
            // Gradual intensity increase
            m_intensity = clamp(m_intensity + RAMP_RATE,
                               MILKING_MIN_INTENSITY, MILKING_MAX_INTENSITY);
            applyStimulation();
        }
        break;

    case ControlState::MILKING:
        // Check for orgasm (FAILURE)
        if (m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
            handleMilkingOrgasmFailure();
            return;
        }

        // Check for danger zone
        if (m_arousalLevel >= m_dangerThreshold) {
            m_dangerZoneEntries++;
            emit dangerZoneEntered(m_arousalLevel);
            m_stateTimer.start();
            setState(ControlState::DANGER_REDUCTION);
            return;
        }

        // Check if fell out of zone (too low)
        if (m_arousalLevel < m_milkingZoneLower) {
            // Increase stimulation to get back in zone
            m_intensity = clamp(m_intensity + RAMP_RATE * 2,
                               MILKING_MIN_INTENSITY, MILKING_MAX_INTENSITY);
        } else {
            // In zone - use PID control to maintain
            double adjustment = calculateMilkingIntensityAdjustment();
            m_intensity = clamp(m_intensity + adjustment,
                               MILKING_MIN_INTENSITY, MILKING_MAX_INTENSITY);

            // Track time in zone
            m_milkingZoneTime += UPDATE_INTERVAL_MS;

            // Update running average
            m_milkingAvgArousal = (m_milkingAvgArousal * m_milkingAvgSamples + m_arousalLevel)
                                 / (m_milkingAvgSamples + 1);
            m_milkingAvgSamples++;

            // Periodic report
            if (m_stateTimer.elapsed() >= MILKING_ZONE_REPORT_INTERVAL_MS) {
                emit milkingZoneMaintained(m_milkingZoneTime, m_milkingAvgArousal);
                m_stateTimer.start();
            }
        }

        applyStimulation();
        break;

    case ControlState::DANGER_REDUCTION:
        // Check for orgasm (FAILURE) even in danger reduction
        if (m_arousalLevel >= m_orgasmThreshold && detectContractions()) {
            handleMilkingOrgasmFailure();
            return;
        }

        // Significantly reduce or pause stimulation
        m_intensity = MILKING_MIN_INTENSITY;
        if (m_clitoralOscillator) {
            m_clitoralOscillator->setIntensity(m_intensity * 0.5);  // Half of minimum
        }
        if (m_tensController && m_tensEnabled) {
            m_tensController->stop();
        }

        // Check if safe to return to milking
        if (m_arousalLevel < DANGER_RECOVERY_THRESHOLD) {
            emit dangerZoneExited(m_arousalLevel);
            m_stateTimer.start();
            setState(ControlState::MILKING);
        }
        break;

    case ControlState::ORGASM_FAILURE:
        // Handle based on failure mode
        handleMilkingOrgasmFailure();
        break;

    default:
        break;
    }
}
```

### 8.2 PID Control Implementation

```cpp
double OrgasmControlAlgorithm::calculateMilkingIntensityAdjustment()
{
    // Calculate error (positive = need more stimulation, negative = too much)
    double error = m_milkingTargetArousal - m_arousalLevel;

    // Proportional term
    double pTerm = MILKING_PID_KP * error;

    // Integral term (accumulated error)
    m_milkingIntegralError += error * (UPDATE_INTERVAL_MS / 1000.0);
    // Anti-windup: clamp integral to prevent runaway
    m_milkingIntegralError = clamp(m_milkingIntegralError, -0.5, 0.5);
    double iTerm = MILKING_PID_KI * m_milkingIntegralError;

    // Derivative term (rate of change of error)
    double dError = (error - m_milkingPreviousError) / (UPDATE_INTERVAL_MS / 1000.0);
    double dTerm = MILKING_PID_KD * dError;
    m_milkingPreviousError = error;

    // Combined adjustment
    double adjustment = pTerm + iTerm + dTerm;

    // Emit for debugging/UI
    emit milkingIntensityAdjusted(m_intensity + adjustment, error);

    return adjustment;
}
```

---

## 9. Implementation: handleMilkingOrgasmFailure()

```cpp
void OrgasmControlAlgorithm::handleMilkingOrgasmFailure()
{
    m_milkingOrgasmCount++;
    qint64 elapsed = m_sessionTimer.elapsed();

    emit unwantedOrgasm(m_milkingOrgasmCount, elapsed);
    emit orgasmDetected(m_milkingOrgasmCount, elapsed);

    switch (m_milkingFailureMode) {
    case 0:  // Stop - end session immediately
        if (m_clitoralOscillator) m_clitoralOscillator->stop();
        if (m_tensController) m_tensController->stop();
        if (m_hardware) m_hardware->setSOL2(true);  // Vent vacuum
        emit milkingSessionComplete(elapsed, false, m_dangerZoneEntries);
        setState(ControlState::COOLING_DOWN);
        break;

    case 1:  // Ruined orgasm - stop AT orgasm onset
        // Stop all stimulation immediately to deny full pleasure
        if (m_clitoralOscillator) m_clitoralOscillator->stop();
        if (m_tensController) m_tensController->stop();
        if (m_hardware) m_hardware->setSOL2(true);
        // Wait for orgasm to subside, then resume milking
        m_stateTimer.start();
        setState(ControlState::ORGASM_FAILURE);
        // After ORGASM_DURATION_MS, transition back to BUILDING
        break;

    case 2:  // Punishment - continue/intensify through hypersensitivity
        // BOOST stimulation during hypersensitive period
        m_intensity = clamp(m_intensity + THROUGH_ORGASM_BOOST * 2, 0.0, MAX_INTENSITY);
        if (m_clitoralOscillator) {
            m_clitoralOscillator->setIntensity(m_intensity);
            m_clitoralOscillator->setFrequency(m_frequency * 1.2);
        }
        // Continue for POST_UNEXPECTED_ORGASM_RECOVERY_MS then resume
        m_stateTimer.start();
        setState(ControlState::ORGASM_FAILURE);
        break;

    case 3:  // Continue - log and resume milking after brief pause
        // Brief pause
        if (m_clitoralOscillator) m_clitoralOscillator->stop();
        m_stateTimer.start();
        setState(ControlState::ORGASM_FAILURE);
        // After POST_ORGASM_PAUSE_MS, resume at BUILDING
        break;

    default:
        // Default to stop
        startCoolDown();
        break;
    }
}
```

---

## 10. Implementation: startMilking()

```cpp
void OrgasmControlAlgorithm::startMilking(int durationMs, int failureMode)
{
    QMutexLocker locker(&m_mutex);

    if (m_state != ControlState::STOPPED) {
        qWarning() << "Cannot start milking: algorithm already running";
        return;
    }

    startMilkingInternal(durationMs, failureMode);
}

void OrgasmControlAlgorithm::startMilkingInternal(int durationMs, int failureMode)
{
    qDebug() << "Starting milking session:" << durationMs << "ms, failure mode:" << failureMode;

    // Reset session state
    m_edgeCount = 0;
    m_orgasmCount = 0;
    m_milkingOrgasmCount = 0;
    m_dangerZoneEntries = 0;
    m_milkingZoneTime = 0;
    m_milkingAvgArousal = 0.0;
    m_milkingAvgSamples = 0;
    m_highPressureDuration = 0;
    m_sealLossCount = 0;
    m_resealAttemptInProgress = false;

    // Reset PID state
    m_milkingIntegralError = 0.0;
    m_milkingPreviousError = 0.0;
    m_milkingTargetArousal = MILKING_TARGET_AROUSAL;

    // Set session parameters
    m_maxDurationMs = qMin(durationMs, MILKING_MAX_SESSION_MS);
    m_milkingFailureMode = failureMode;

    // Set initial intensity
    m_intensity = MILKING_BASE_INTENSITY;
    m_frequency = MILKING_BASE_FREQUENCY;
    if (m_tensEnabled) {
        m_tensAmplitude = MILKING_TENS_AMPLITUDE;
    }

    // Set mode and state
    setMode(Mode::MILKING);
    setState(ControlState::CALIBRATING);

    // Reset and start timers
    m_sessionTimer.start();
    m_stateTimer.start();
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_safetyTimer->start(SAFETY_INTERVAL_MS);

    // Initialize calibration
    m_calibSumClitoral = 0.0;
    m_calibSumAVL = 0.0;
    m_calibSamples = 0;
}
```

---

## 11. Modifications to onUpdateTick()

```cpp
void OrgasmControlAlgorithm::onUpdateTick()
{
    QMutexLocker locker(&m_mutex);

    if (m_emergencyStop) {
        handleEmergencyStop();
        return;
    }

    switch (m_mode) {
    case Mode::ADAPTIVE_EDGING:
    case Mode::DENIAL:
        runAdaptiveEdging();
        break;
    case Mode::FORCED_ORGASM:
    case Mode::MULTI_ORGASM:
        runForcedOrgasm();
        break;
    case Mode::MILKING:       // NEW
        runMilking();
        break;
    case Mode::MANUAL:
    default:
        break;
    }
}
```

---

## 12. State Transition Diagram

```
                         ┌─────────────────┐
                         │    STOPPED      │
                         └────────┬────────┘
                                  │ startMilking()
                                  ▼
                         ┌─────────────────┐
                         │  CALIBRATING    │
                         └────────┬────────┘
                                  │ calibration complete
                                  ▼
                         ┌─────────────────┐
                         │    BUILDING     │
                         └────────┬────────┘
                                  │ arousal ≥ 0.75
                                  ▼
    ┌─────────────────────────────────────────────────────────┐
    │                                                         │
    │                      MILKING                            │◄───┐
    │              (maintain 0.75-0.90)                       │    │
    │                                                         │    │
    └────────┬─────────────────┬──────────────────────────────┘    │
             │                 │                                    │
  arousal>0.92                 │ orgasm detected            arousal<0.88
             ▼                 ▼                                    │
    ┌─────────────────┐ ┌─────────────────┐                        │
    │ DANGER_REDUCTION│ │ ORGASM_FAILURE  │                        │
    │  (reduce stim)  │ │  (handle fail)  │                        │
    └────────┬────────┘ └────────┬────────┘                        │
             │                   │                                  │
             └───────────────────┼──────────────────────────────────┘
                                 │
                    session timeout or failure mode=stop
                                 │
                                 ▼
                         ┌─────────────────┐
                         │  COOLING_DOWN   │
                         └────────┬────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │    STOPPED      │
                         └─────────────────┘
```

---

## 13. Files to Modify

| File | Changes |
|------|---------|
| `OrgasmControlAlgorithm.h` | Add Mode::MILKING, new ControlStates, signals, methods, variables, constants |
| `OrgasmControlAlgorithm.cpp` | Add startMilking(), runMilking(), PID control, failure handling |

---

## 14. Testing Plan

### 14.1 Unit Tests

1. **Zone Maintenance**: Verify arousal stays in 75-90% range
2. **PID Response**: Test intensity adjustments respond correctly to arousal changes
3. **Danger Zone Entry/Exit**: Verify state transitions at thresholds
4. **Orgasm Detection**: Verify failure is detected correctly
5. **Failure Modes**: Test all 4 failure modes behave correctly

### 14.2 Integration Tests

1. **Full Session**: Run 30-minute session, verify zone time tracking
2. **Orgasm Failure**: Trigger orgasm, verify failure mode response
3. **Multiple Danger Entries**: Push to danger zone multiple times

---

## 15. Summary

| Component | Count |
|-----------|-------|
| New enum values | 1 Mode + 3 ControlStates |
| New public methods | 5 |
| New signals | 7 |
| New private methods | 4 |
| New member variables | 11 |
| New constants | 14 |

---

*Design Document: December 2024*
*For: OrgasmControlAlgorithm Milking Cycle Mode Implementation*
*Ready for Phase 3: Implementation*

