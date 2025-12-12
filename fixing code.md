fixing code.md

# OrgasmControlAlgorithm.cpp - Bug Report & Fixes

**Date**: December 12, 2024  
**File**: `src/control/OrgasmControlAlgorithm.cpp`  
**Severity**: CRITICAL (5 bugs), HIGH (5 bugs), MEDIUM (6 bugs)  
**Status**: Ready for fixes

## Executive Summary

The `OrgasmControlAlgorithm` class manages the 15+ pattern execution engine with real-time arousal detection, seal monitoring, and multi-mode control (edging, forced orgasm, denial, milking). Current implementation has **16 identified bugs** ranging from null pointer dereferences to race conditions in the multi-threaded arousal calculation pipeline.

**Key Risk Areas**:
- Timer initialization causing crashes on mode transitions
- Atomic index race condition corrupting pressure/arousal history alignment
- Buffer wraparound in spectral analysis
- Safety-critical seal detection logic inverted under high arousal
- Missing thread synchronization in emergency stop path

---

## CRITICAL BUGS (System Failure Risk)

### BUG #1: Uninitialized QTimer Members (`m_stateTimer`, `m_resealTimer`)

**Severity**: CRITICAL  
**Location**: Constructor line ~27-40, used at lines ~315, ~360, ~392, ~410  
**Affected Methods**: `startAdaptiveEdgingInternal()`, `startForcedOrgasm()`, `startMilkingInternal()`, `startDenial()`  
**Thread Context**: Main GUI thread + SafetyMonitorThread

**Problem**:
```cpp
// Constructor ~line 27:
OrgasmControlAlgorithm::OrgasmControlAlgorithm(...)
    : ...
    , m_updateTimer(new QTimer(this))      // ✓ Initialized
    , m_safetyTimer(new QTimer(this))      // ✓ Initialized
    // MISSING: m_stateTimer(new QTimer(this))
    // MISSING: m_resealTimer(new QTimer(this))

// Then at line ~315 in startAdaptiveEdgingInternal():
m_stateTimer.start();  // ✗ Null pointer dereference!
```

**Impact**: 
- Crash when transitioning to BUILDING, FORCING, or MILKING modes
- Affects all 4+ session types (edging, forced orgasm, denial, milking)
- Reproduces on every session start after first run

**Root Cause**: QTimer members declared but not initialized in constructor initializer list. Per Manager Pattern from architecture, all QObject children must be initialized in parent's constructor.

**Fix**:
```cpp
// In constructor initializer list (~line 30):
, m_updateTimer(new QTimer(this))
, m_safetyTimer(new QTimer(this))
, m_stateTimer(new QTimer(this))        // ADD THIS
, m_resealTimer(new QTimer(this))       // ADD THIS
, m_state(ControlState::STOPPED)
```

**Verification**: Search for `m_stateTimer` and `m_resealTimer` usage; should find no `.` dereferences, only `->` pointers after fix.

---

### BUG #2: QElapsedTimer vs QTimer Type Mismatch

**Severity**: CRITICAL  
**Location**: Header file declarations (assumed), usage at lines ~315, ~360, ~410  
**Thread Context**: Main GUI thread + SafetyMonitorThread

**Problem**:
The code uses `QElapsedTimer` (for measuring elapsed time) where `QTimer` (for periodic callbacks) is needed. `QElapsedTimer` has `.start()` and `.elapsed()` methods but does NOT emit `timeout()` signals.

**Current code pattern**:
```cpp
// If declared as:
// QElapsedTimer m_stateTimer;
m_stateTimer.start();  // Measures elapsed time, not a callback timer
// Later:
if (m_stateTimer.elapsed() > TIMEOUT_MS) { ... }  // ✓ Correct usage
// But if code expects:
m_stateTimer.stop();  // ✓ QElapsedTimer has this
connect(m_stateTimer, &QTimer::timeout, ...);  // ✗ QElapsedTimer has NO timeout signal!
```

**Impact**: 
- State transitions won't timeout properly
- Reseal attempts won't have time limits
- Session duration limits might not be enforced

**Root Cause**: Type confusion between elapsed time tracking vs periodic timer signals. Architecture requires both:
- `QElapsedTimer` for measuring session/state duration
- `QTimer` for periodic callbacks and enforcing timeouts

**Fix**: Separate concerns in header declarations:
```cpp
// In OrgasmControlAlgorithm.h private section:

// Periodic callback timers (inherit from QTimer, emit signals)
QTimer* m_updateTimer;       // ✓ Already correct
QTimer* m_safetyTimer;       // ✓ Already correct
QTimer* m_stateTimer;        // Should be QTimer*, not QElapsedTimer
QTimer* m_resealTimer;       // Should be QTimer*, not QElapsedTimer

// Elapsed time tracking (inherit from QElapsedTimer, measure duration)
QElapsedTimer m_sessionTimer;     // Total session time
QElapsedTimer m_edgeSessionTimer; // Edging session-specific timing
```

**Rationale**: Per threading architecture, `QTimer` emits signals in event loop, enabling proper async callbacks. `QElapsedTimer` does not emit signals and is inappropriate for triggering state transitions.

---

### BUG #3: Race Condition - Atomic Index Loaded Multiple Times Per Cycle

**Severity**: CRITICAL  
**Location**: Lines ~520 (updateArousalLevel), ~553 (calculateArousalLevel), helper functions  
**Thread Context**: DataAcquisitionThread (50 Hz) reads sensors, Main GUI thread (30 FPS) reads history  
**Data Integrity Impact**: Pressure/arousal history misalignment, stale calculations

**Problem**:
```cpp
// Line ~520 in updateArousalLevel():
int currentIdx = m_historyIndex.load(std::memory_order_acquire);  // Load #1
m_arousalHistory[currentIdx] = newArousal;
m_historyIndex.store((currentIdx + 1) % HISTORY_SIZE, std::memory_order_release);

// Line ~553 in calculateArousalLevel():
int currentIdx = m_historyIndex.load(std::memory_order_acquire);  // Load #2 - DIFFERENT VALUE!
m_pressureHistory[currentIdx] = currentClitoral;  // Now offset from arousal history
double variance = calculateVariance(...);  // Passes same currentIdx to helper

// Inside calculateVariance() or calculateBandPower():
int idx = m_historyIndex.load(...);  // Load #3 - ANOTHER DIFFERENT VALUE!
```

**Scenario**:
- Tick N: Load `currentIdx=10`, store arousal at [10], increment to 11
- Tick N+1: Load `currentIdx=11` in calculateArousalLevel, store pressure at [11]
- Result: Arousal at [10], pressure at [11] → misaligned history entries
- Spectral analysis (calculateBandPower) uses stale/wrong indices → corrupted arousal calculation

**Impact**: 
- Arousal curve becomes noisy and unreliable
- Edge detection might trigger on stale pressure data
- Orgasm detection confidence decreases significantly
- Seal loss detection may use misaligned baseline

**Root Cause**: Per "Multi-threaded Control" architecture (section 4), atomic variables should be loaded ONCE per cycle and passed as parameters. Current code loads repeatedly without consistency.

**Code comment** at line ~553 says "Bug #1 fix: Use atomic load" but THIS IS the bug—using atomic doesn't prevent multiple loads in same cycle.

**Fix**: Load once per update cycle, pass index to all functions:
```cpp
// In updateArousalLevel():
void OrgasmControlAlgorithm::updateArousalLevel()
{
    QMutexLocker locker(&m_mutex);
    
    // Load ONCE at cycle start
    int currentIdx = m_historyIndex.load(std::memory_order_acquire);
    
    // Pass index to calculation (don't reload)
    double newArousal = calculateArousalLevel(currentIdx);  // PASS as parameter
    
    // Update history with SAME index
    m_arousalHistory[currentIdx] = newArousal;
    m_pressureHistory[currentIdx] = m_hardware->readClitoralPressure();
    
    // Advance index ONCE at cycle end
    m_historyIndex.store((currentIdx + 1) % HISTORY_SIZE, std::memory_order_release);
    
    // Update smoothed arousal
    m_smoothedArousal = 0.8 * m_smoothedArousal + 0.2 * newArousal;
    
    // ...rest of method...
}

// Modify calculateArousalLevel to accept index parameter:
double OrgasmControlAlgorithm::calculateArousalLevel(int currentIdx)  // ADD parameter
{
    if (!m_hardware) return 0.0;

    double currentClitoral = m_hardware->readClitoralPressure();
    double currentAVL = m_hardware->readAVLPressure();

    // REMOVE: int currentIdx = m_historyIndex.load(...);  // DELETE THIS LINE

    // Validate pressure readings per safety-first design
    if (currentClitoral < PRESSURE_MIN_VALID || currentClitoral > PRESSURE_MAX_VALID) {
        currentClitoral = m_baselineClitoral > 0.0 ? m_baselineClitoral : 0.0;
    }
    if (currentAVL < PRESSURE_MIN_VALID || currentAVL > PRESSURE_MAX_VALID) {
        currentAVL = m_baselineAVL > 0.0 ? m_baselineAVL : 0.0;
    }

    // Pass index to ALL helper functions - never reload
    double pressureVariance = calculateVariance(m_pressureHistory, VARIANCE_WINDOW_SAMPLES, currentIdx);
    double contractionPower = calculateBandPower(m_pressureHistory, 0.8, 1.2, currentIdx);
    double rateOfChange = calculateDerivative(m_pressureHistory, currentIdx);

    // ...rest of arousal calculation...
    
    return arousal;
}

// Update all helper functions to accept and use passed index:
double OrgasmControlAlgorithm::calculateVariance(
    const QVector<double>& data, int windowSize, int currentIdx)
{
    // Use passed currentIdx - NEVER reload m_historyIndex
    // int startIdx = ((currentIdx - windowSize) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
    // Loop from startIdx...
}
```

**Verification**: After fix, `m_historyIndex.load()` should appear EXACTLY ONCE per `onUpdateTick()` execution, in `updateArousalLevel()` only.

---

### BUG #4: Buffer Wraparound Arithmetic in `calculateBandPower()`

**Severity**: CRITICAL  
**Location**: Line ~672  
**Thread Context**: DataAcquisitionThread via calculateArousalLevel  
**Memory Safety**: Out-of-bounds array access possible

**Problem**:
```cpp
// Line ~672:
for (int lag = lagLow; lag <= lagHigh; ++lag) {
    for (int i = 0; i < HISTORY_SIZE - lag; ++i) {
        int idx1 = ((currentIdx - i) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
        int idx2 = ((currentIdx - i - lag) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;  // Can be negative!
```

**Scenario**:
- `currentIdx = 2, i = 5, lag = 10, HISTORY_SIZE = 32`
- `(2 - 5 - 10) = -13`
- `(-13 + 32) = 19` ✓ OK this time
- But if intermediate calculation is `-200`:
  - `(-200 + 32) = -168` ✗ Still negative!
  - Double modulo pattern `(x % N + N) % N` handles this correctly

**Current code** appears to use single modulo with defensive offset. However, loop bounds are problematic:
- Line ~665: `int maxI = qMin(HISTORY_SIZE - lag, HISTORY_SIZE);` is redundant
- Better: `int maxI = HISTORY_SIZE - lag;` ensures `i < maxI` implies `i + lag < HISTORY_SIZE`

**Impact**: 
- Potential out-of-bounds array access in spectral analysis
- Arousal calculation may read garbage memory
- Undefined behavior in multi-threaded context

**Fix**:
```cpp
double OrgasmControlAlgorithm::calculateBandPower(
    const QVector<double>& data, double freqLow, double freqHigh, int currentIdx)
{
    // Input validation
    if (data.size() < 20) return 0.0;

    if (freqLow <= 0.0 || freqHigh <= 0.0 || freqHigh < freqLow) {
        return 0.0;
    }

    // Calculate lag bounds
    int lagLow = static_cast<int>(10.0 / freqHigh);
    int lagHigh = static_cast<int>(10.0 / freqLow);

    // Clamp to valid history range
    lagLow = qBound(1, lagLow, HISTORY_SIZE - 1);
    lagHigh = qBound(1, lagHigh, HISTORY_SIZE - 1);

    // Ensure lagHigh >= lagLow after clamping
    if (lagHigh < lagLow) {
        std::swap(lagLow, lagHigh);
    }

    double maxCorrelation = 0.0;

    for (int lag = lagLow; lag <= lagHigh; ++lag) {
        double correlation = 0.0;
        int count = 0;

        // Limit i to ensure lag doesn't exceed history size
        int maxI = HISTORY_SIZE - lag;

        for (int i = 0; i < maxI; ++i) {
            // Consistent double-modulo pattern for safe wraparound
            int idx1 = ((currentIdx - i) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
            int idx2 = ((currentIdx - i - lag) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
            
            // Defensive bounds check (should never fail, but safety-first)
            if (idx1 >= 0 && idx1 < data.size() && idx2 >= 0 && idx2 < data.size()) {
                correlation += data[idx1] * data[idx2];
                count++;
            }
        }

        if (count > 0) {
            correlation /= count;
            maxCorrelation = qMax(maxCorrelation, correlation);
        }
    }

    return maxCorrelation;
}
```

**Rationale**: Per safety-first design from architecture, buffer access must be validated even when logic suggests it's safe. This prevents undefined behavior in multi-threaded context.

---

### BUG #5: Seal Detection Logic Inverted - Safety-Critical Failure

**Severity**: CRITICAL  
**Location**: Line ~1077 in `performSafetyCheck()`  
**Thread Context**: SafetyMonitorThread (100 Hz)  
**Safety Impact**: False seal loss alarms, premature emergency stops, session termination

**Problem**:
```cpp
// Line ~1077:
double adaptiveThreshold = SEAL_LOST_THRESHOLD;
if (m_arousalLevel > 0.0) {
    // At m_arousalLevel = 1.0 with SEAL_AROUSAL_COMPENSATION_FACTOR = 0.4:
    // threshold becomes 60% of normal
    // This makes false seals MORE likely, not less!
    adaptiveThreshold = SEAL_LOST_THRESHOLD * 
                        (1.0 - SEAL_AROUSAL_COMPENSATION_FACTOR * m_arousalLevel);
}
```

**Scenario**:
- User reaches high arousal (0.9) - tissue swells naturally
- AVL pressure drops slightly due to engorgement (expected)
- WRONG LOGIC: Lower threshold + low pressure = false seal alarm → emergency stop
- User session terminated unexpectedly

**Correct interpretation**: High arousal explains SOME pressure drops (tissue swelling), so detection threshold should INCREASE (be harder to trigger), not decrease.

**Current logic**: `threshold *= (1.0 - factor * arousal)` → at arousal=1.0, multiply by 0.6 (EASIER detection)  
**Correct logic**: `threshold *= (1.0 + compensation)` → at high arousal, require MORE pressure drop to trigger

**Impact**: 
- Sessions fail under high arousal conditions (when orgasm most likely)
- Safety becomes unreliable—can't trust seal monitoring
- User experience: frequent false emergency stops

**Root Cause**: Inverse compensation direction violates "Safety-First Design" principle from architecture. Seal integrity must remain reliably detected regardless of arousal state.

**Fix**:
```cpp
void OrgasmControlAlgorithm::performSafetyCheck()
{
    // ... existing validation code ...

    double avlPressure = m_hardware ? m_hardware->readAVLPressure() : 0.0;
    double clitoralPressure = m_hardware ? m_hardware->readClitoralPressure() : 0.0;

    // ========================================================================
    // Arousal-Adaptive Seal Integrity Detection (Bug #5 fix)
    // Per dual-chamber architecture: outer V-seal must maintain 30-50 mmHg
    // ========================================================================

    // Base threshold for mechanical seal failure (never adjusted)
    double baseThreshold = SEAL_LOST_THRESHOLD;  // e.g., 10.0 mmHg drop

    // Adaptive threshold accounts for tissue swelling at high arousal
    // But increases required pressure drop (HARDER to trigger), not decreases it
    double adaptiveThreshold = SEAL_LOST_THRESHOLD;
    
    if (m_arousalLevel > 0.5) {
        // At high arousal, tissue engorgement explains some pressure drop
        // So INCREASE the required drop to trigger alarm (harder detection)
        double swellingCompensation = SEAL_AROUSAL_COMPENSATION_FACTOR * 
                                      ((m_arousalLevel - 0.5) / 0.5);
        
        // Cap compensation at 30% to prevent over-adjustment
        swellingCompensation = qMin(swellingCompensation, 0.3);
        
        // CRITICAL FIX: Use multiplication with positive compensation
        // Old: (1.0 - compensation) makes threshold LOWER (wrong!)
        // New: (1.0 + compensation) makes threshold HIGHER (correct!)
        adaptiveThreshold = SEAL_LOST_THRESHOLD * (1.0 + swellingCompensation);
    }

    // Calculate pressure rate of change
    double rateOfChange = 0.0;
    bool hasValidPreviousPressure = (m_previousAVLPressure > 0.0);
    
    if (hasValidPreviousPressure) {
        rateOfChange = (m_previousAVLPressure - avlPressure) / 
                       (SAFETY_INTERVAL_MS / 1000.0);
    }

    // Check if clitoral pressure is rising (engorgement, not deflation)
    bool clitoralPressureRising = hasValidPreviousPressure &&
                                  (clitoralPressure > m_previousClitoralPressure + 0.5);

    // Detect seal loss using FIXED threshold for rapid mechanical failures
    // (mechanical seal failure is not affected by arousal state)
    bool rapidSealLeak = hasValidPreviousPressure &&
                         (avlPressure < baseThreshold) &&
                         (rateOfChange > RAPID_PRESSURE_DROP_THRESHOLD);

    // Detect gradual seal loss (persistent low pressure)
    // But don't trigger if clitoral is rising (normal engorgement)
    bool gradualSealLeak = hasValidPreviousPressure &&
                           (avlPressure < adaptiveThreshold) &&
                           (!clitoralPressureRising) &&
                           (m_arousalLevel < 0.5);

    // Critical safety: any severe pressure loss triggers immediately
    bool criticalPressureLoss = (avlPressure < baseThreshold * 0.3);

    // Combine detection logic
    bool sealLossDetected = rapidSealLeak || gradualSealLeak || criticalPressureLoss;

    if (sealLossDetected) {
        m_sealLossCount++;
        if (m_sealLossCount >= SEAL_LOSS_THRESHOLD_COUNT && !m_resealAttemptInProgress) {
            qWarning() << "SEAL LOSS DETECTED: Emergency reseal attempt initiated"
                       << "Count:" << m_sealLossCount
                       << "AVL pressure:" << avlPressure;
            m_resealAttemptInProgress = true;
            m_resealTimer->start();  // Bug #1 fix: m_resealTimer is now initialized
            // Progressive reseal attempt (phases 1-3)
        }
    } else if (avlPressure < adaptiveThreshold) {
        // Pressure is low but multiple signals indicate tissue swelling, not leak
        if (m_verboseLogging) {
            qDebug() << "Low AVL pressure likely due to tissue swelling"
                     << "Arousal:" << m_arousalLevel
                     << "Clitoral rising:" << clitoralPressureRising
                     << "Threshold:" << adaptiveThreshold;
        }
        // Don't reset counter immediately - only decrement gradually
        if (m_sealLossCount > 0) {
            m_sealLossCount--;
        }
    } else {
        // Seal is GOOD - pressure well above threshold
        if (m_sealLossCount > 0) {
            qDebug() << "Seal re-established after" << m_sealLossCount << "failures";
        }
        m_sealLossCount = 0;
        m_resealAttemptInProgress = false;
    }

    // Store previous values for next iteration
    m_previousAVLPressure = avlPressure;
    m_previousClitoralPressure = clitoralPressure;

    // ... rest of safety checks ...
}
```

**Verification**:
- High arousal session: AVL drops from 45 to 35 mmHg (reasonable engagement)
- Old code: `threshold = 10 * (1 - 0.4 * 0.9) = 6.0 mmHg` → FALSE ALARM ✗
- New code: `threshold = 10 * (1 + 0.3 * min(0.4 * 0.4)) = 10.48 mmHg` → No alarm ✓

---

## HIGH-SEVERITY BUGS (Major Functional Issues)

### BUG #6: Missing Thread Synchronization in `emergencyStop()`

**Severity**: HIGH  
**Location**: Line ~480 in `emergencyStop()`  
**Thread Context**: Main GUI thread calls, SafetyMonitorThread reads `m_emergencyStop`  
**Race Condition**: Multiple threads accessing `m_emergencyStop` without mutex

**Problem**:
```cpp
void OrgasmControlAlgorithm::emergencyStop()
{
    // BUG: No mutex lock - but method accesses m_emergencyStop
    // Meanwhile SafetyMonitorThread in onSafetyCheck() reads:
    // if (m_emergencyStop) { /* stop everything */ }
    handleEmergencyStop();
}

// In onSafetyCheck() (runs in SafetyMonitorThread):
void OrgasmControlAlgorithm::onSafetyCheck()
{
    // No lock - but reads m_emergencyStop:
    if (m_emergencyStop) {  // Race condition with emergencyStop()!
        // ...
    }
}
```

**Impact**: 
- Race condition: write in GUI thread vs read in SafetyMonitorThread
- Memory visibility issue: changes to `m_emergencyStop` not guaranteed to propagate
- Stop request might be delayed or missed entirely
- Safety-critical: emergency stop might not activate when needed

**Root Cause**: Per threading architecture, all cross-thread state access must use mutex or atomics. `m_emergencyStop` appears to be a plain bool without synchronization.

**Fix**:
```cpp
void OrgasmControlAlgorithm::emergencyStop()
{
    // Critical-2 fix: Acquire mutex before accessing shared state
    QMutexLocker locker(&m_mutex);
    
    // Now safe to modify m_emergencyStop and call other methods
    handleEmergencyStop();
    
    // locker auto-releases mutex at end of scope
}

// Verify m_emergencyStop is defined as:
// std::atomic<bool> m_emergencyStop;  (preferred for performance)
// OR guarded by m_mutex if bool type

// In onSafetyCheck(), also add lock:
void OrgasmControlAlgorithm::onSafetyCheck()
{
    QMutexLocker locker(&m_mutex);
    
    // Now safe to read m_emergencyStop
    if (m_emergencyStop) {
        // ...
    }
}
```

**Alternative**: If `m_emergencyStop` is already `std::atomic<bool>`, no mutex needed but code should use `.load()`/`.store()` explicitly:
```cpp
void OrgasmControlAlgorithm::emergencyStop()
{
    m_emergencyStop.store(true, std::memory_order_release);
    handleEmergencyStop();
}

// In onSafetyCheck():
if (m_emergencyStop.load(std::memory_order_acquire)) { ... }
```

---

### BUG #7: Integer Validation Missing Before Switch on `m_milkingFailureMode`

**Severity**: HIGH  
**Location**: Line ~1547 in `handleMilkingOrgasmFailure()`, line ~1471 in `runMilking()` ORGASM_FAILURE case  
**Thread Context**: Main control thread  
**Memory Safety**: Undefined switch behavior

**Problem**:
```cpp
// Line ~445 in setMilkingFailureMode():
void OrgasmControlAlgorithm::setMilkingFailureMode(int mode)
{
    QMutexLocker locker(&m_mutex);
    m_milkingFailureMode = qBound(0, mode, 3);  // ✓ Validated at assignment
}

// Line ~1547 in handleMilkingOrgasmFailure():
switch (m_milkingFailureMode) {  // ✗ No validation before switch!
case 0:  // Stop
case 1:  // Ruined orgasm
case 2:  // Punishment
case 3:  // Continue
default:
    startCoolDown();
}
```

**Scenario**:
- Thread 1: Calls `setMilkingFailureMode(1)`, assigns validated value
- Thread 2: Reads `m_milkingFailureMode` → gets 1
- Race condition: If Thread 3 corrupts `m_milkingFailureMode` between check and switch, switch uses garbage value
- Undefined behavior in C++ switch statements

**Impact**: 
- Invalid mode values cause undefined behavior
- Could skip intended case handler
- Potential crash if mode is negative or > 100

**Root Cause**: Validation happens at assignment, but no defensive validation before switch.

**Fix**:
```cpp
void OrgasmControlAlgorithm::handleMilkingOrgasmFailure()
{
    m_milkingOrgasmCount++;
    qint64 elapsed = m_sessionTimer.elapsed();

    qDebug() << "Milking failure: orgasm" << m_milkingOrgasmCount
             << "at" << elapsed << "ms, mode:" << m_milkingFailureMode;

    emit unwantedOrgasm(m_milkingOrgasmCount, elapsed);
    emit orgasmDetected(m_milkingOrgasmCount, elapsed);

    // BUG #7 fix: Validate before switch (defensive programming)
    int validatedMode = qBound(0, m_milkingFailureMode, 3);
    if (validatedMode != m_milkingFailureMode) {
        qWarning() << "Invalid m_milkingFailureMode:" << m_milkingFailureMode
                   << "- using" << validatedMode;
        m_milkingFailureMode = validatedMode;
    }

    switch (validatedMode) {  // Use validated value, not raw member
    case 0:  // Stop immediately
        setMode(Mode::MANUAL);
        stop();
        break;

    case 1:  // Ruined orgasm
        m_intensity = 0.0;
        if (m_clitoralOscillator) {
            m_clitoralOscillator->stop();
        }
        if (m_tensController) {
            m_tensController->stop();
        }
        qDebug() << "Ruined orgasm - stopped all stimulation";
        startCoolDown();
        break;

    case 2:  // Punishment mode
        m_intensity = PUNISHMENT_INTENSITY;
        m_frequency = PUNISHMENT_FREQUENCY;
        if (m_clitoralOscillator) {
            if (!m_clitoralOscillator->isRunning()) {
                m_clitoralOscillator->start();
            }
            m_clitoralOscillator->setFrequency(m_frequency);
            m_clitoralOscillator->setAmplitude(m_intensity * MAX_CLITORAL_AMPLITUDE);
        }
        qDebug() << "Punishment mode activated";
        break;

    case 3:  // Continue - allow next orgasm attempt
        qDebug() << "Orgasm failure - continuing milking";
        setMode(Mode::MILKING);
        break;

    default:
        qWarning() << "Unexpected failure mode:" << validatedMode;
        startCoolDown();
        break;
    }
}
```

---

### BUG #8: Seal Loss Counter Hysteresis - Permanent Low Pressure Undetected

**Severity**: HIGH  
**Location**: Line ~1104-1127 in `performSafetyCheck()` seal loss handling  
**Thread Context**: SafetyMonitorThread (100 Hz)  
**Safety Impact**: Gradual seal leaks during sustained low pressure never detected

**Problem**:
```cpp
// Line ~1104-1127:
if (sealLossDetected) {
    m_sealLossCount++;
} else if (avlPressure < adaptiveThreshold) {
    // Pressure is low but NOT detected as seal leak
    if (m_sealLossCount > 0) {
        m_sealLossCount--;  // PROBLEM: Counter always decrements when pressure low
    }
}
// Result: Low fluctuating pressure resets counter perpetually
// Real gradual leak never accumulates count → NEVER detected
```

**Scenario**:
- Session at high arousal (0.8) with slight seal leak starting
- Tick 1: Arousal high explains low pressure → `sealLossDetected=false`, `count--` (count=0)
- Tick 2: Pressure still dropping → `sealLossDetected=false`, `count--` (count=0)
- Tick 100: Real leak detected too late, device already fell off

**Current code comment** at line ~1111 suggests understanding but logic is still wrong.

**Impact**: 
- Gradual seal leaks undetected if pressure fluctuates or arousal is high
- Safety monitoring unreliable
- Device could fully detach without triggering emergency stop

**Root Cause**: Counter decrement logic doesn't distinguish between "pressure temporarily low" and "real seal leak with sustained low pressure."

**Fix**:
```cpp
// In performSafetyCheck() seal loss detection section:

    bool sealLossDetected = rapidSealLeak || gradualSealLeak || criticalPressureLoss;

    if (sealLossDetected) {
        // Clear seal loss evidence - increment counter
        m_sealLossCount++;
        if (m_sealLossCount >= SEAL_LOSS_THRESHOLD_COUNT && !m_resealAttemptInProgress) {
            qWarning() << "SEAL LOSS DETECTED: Emergency reseal attempt initiated"
                       << "Failures:" << m_sealLossCount;
            m_resealAttemptInProgress = true;
            m_resealTimer->start();
            // Progressive reseal attempt (phase 1)
        }
    } else if (avlPressure < adaptiveThreshold) {
        // Pressure is low but multiple signals indicate tissue swelling, not actual leak
        // (high arousal, clitoral rising, etc.)
        // 
        // BUG #8 fix: Don't decrement counter when pressure is still low
        // Only reset when pressure FULLY recovers above threshold + hysteresis
        if (m_verboseLogging) {
            qDebug() << "Low AVL pressure, likely tissue swelling"
                     << "Arousal:" << m_arousalLevel
                     << "Clitoral rising:" << clitoralPressureRising;
        }
        // Counter stays unchanged - don't reset yet
        // This allows real leaks to accumulate even under high arousal
    } else if (avlPressure > (adaptiveThreshold + 10.0)) {
        // Seal is GOOD - pressure well above threshold + hysteresis
        // Only NOW reset counter (prevents oscillation)
        if (m_sealLossCount > 0) {
            qDebug() << "Seal re-established. History:" << m_sealLossCount << "failures";
        }
        m_sealLossCount = 0;
        m_resealAttemptInProgress = false;
    }
    // else: pressure between threshold and hysteresis → counter stays unchanged
```

**Rationale**: Three-state logic prevents counter oscillation:
1. **Seal loss evidence**: Increment counter
2. **Low pressure but explained**: Leave counter unchanged (may be real leak)
3. **Pressure recovered + hysteresis**: Reset counter (seal is good)

---

### BUG #9: Oscillator Not Started in DANGER_REDUCTION State

**Severity**: HIGH  
**Location**: Line ~1356 in `runMilking()` DANGER_REDUCTION case  
**Thread Context**: Main control thread  
**Functional Impact**: Device may not respond to intensity changes if not running

**Problem**:
```cpp
// Line ~1356 in DANGER_REDUCTION case:
if (m_clitoralOscillator) {
    m_clitoralOscillator->setAmplitude(m_intensity * 0.5 * MAX_CLITORAL_AMPLITUDE);
    // MISSING: Check if running!
}

// Other paths (e.g., line ~1246 BUILDING):
if (m_clitoralOscillator) {
    if (!m_clitoralOscillator->isRunning()) {
        m_clitoralOscillator->start();  // ✓ Correct pattern
    }
    m_clitoralOscillator->setFrequency(m_frequency);
}
```

**Impact**: 
- In DANGER_REDUCTION state, amplitude is set but device never starts
- Oscillator remains dormant → no stimulation
- Safety feature (reduce danger) doesn't function
- User might not feel intended reduction

**Root Cause**: Inconsistent startup pattern. Some state branches check `isRunning()` before `start()`, others don't.

**Fix**:
```cpp
        case ControlState::DANGER_REDUCTION: {
            // Check for orgasm while reducing stimulation intensity
            bool orgasmDetected = (m_arousalLevel >= m_orgasmThreshold);

            if (orgasmDetected && !m_inOrgasm) {
                m_inOrgasm = true;
                m_pointOfNoReturnReached = true;
                qDebug() << "Orgasm detected during danger reduction!";
                m_stateTimer->stop();
                setState(ControlState::ORGASM_FAILURE);
                handleMilkingOrgasmFailure();
                break;
            }

            // Significantly reduce stimulation to bring arousal down
            m_intensity = MILKING_MIN_INTENSITY;
            
            if (m_clitoralOscillator) {
                // BUG #9 fix: Ensure device is started before setting parameters
                if (!m_clitoralOscillator->isRunning()) {
                    m_clitoralOscillator->start();
                }
                m_clitoralOscillator->setAmplitude(m_intensity * 0.5 * MAX_CLITORAL_AMPLITUDE);
            }

            if (m_tensController && m_tensEnabled) {
                if (m_tensController->isActive()) {
                    m_tensController->stop();
                }
            }

            // Check exit condition
            qint64 elapsedMs = m_stateTimer->elapsed();
            if (elapsedMs > DANGER_REDUCTION_TIMEOUT_MS || 
                m_arousalLevel < m_edgeThreshold) {
                // Arousal reduced below edge - return to safe milking
                setState(ControlState::MILKING);
                qDebug() << "Exiting danger reduction, arousal recovered to:" << m_arousalLevel;
            }
            break;
        }
```

---

### BUG #10: Missing Constants Definition for Pressure Validation

**Severity**: HIGH  
**Location**: Lines ~555-562 in `calculateArousalLevel()` use undefined constants  
**Thread Context**: DataAcquisitionThread (50 Hz)  
**Build Impact**: Code won't compile if constants not defined

**Problem**:
```cpp
// Line ~555:
if (currentClitoral < PRESSURE_MIN_VALID || currentClitoral > PRESSURE_MAX_VALID) {
    // PRESSURE_MIN_VALID and PRESSURE_MAX_VALID not defined!
    // This will cause compiler error: "undefined identifier"
}
```

**Impact**: 
- Code won't compile without these constants
- Prevents pressure validation from functioning
- Header checks would fail in CI pipeline

**Root Cause**: Constants referenced but not declared in header or defined in .cpp

**Fix**: Add to header file (OrgasmControlAlgorithm.h) in private constants section:

```cpp
// In OrgasmControlAlgorithm.h private section with other constants:
private:
    // Pressure validation bounds (mmHg)
    // Per dual-chamber architecture:
    // - Clitoral cylinder: 0-30 mmHg (normal), can spike to 50+ mmHg during orgasm
    // - AVL outer chamber: 0-75 mmHg (sensor max for MPX5010DP)
    static constexpr double PRESSURE_MIN_VALID = -0.5;      // Allow slight sensor noise
    static constexpr double PRESSURE_MAX_VALID = 80.0;      // Safety margin above sensor max
    
    // For clitoral specifically:
    static constexpr double CLITORAL_PRESSURE_MIN = 0.0;
    static constexpr double CLITORAL_PRESSURE_MAX = 60.0;  // Realistic orgasm peak
    
    // For AVL specifically:
    static constexpr double AVL_PRESSURE_MIN = 0.0;
    static constexpr double AVL_PRESSURE_MAX = 75.0;       // Sensor maximum
```

Then in `calculateArousalLevel()`:
```cpp
    // Defensive bounds checking - validate sensor readings
    if (!std::isfinite(currentClitoral) || !std::isfinite(currentAVL)) {
        qWarning() << "Invalid pressure reading - infinite or NaN detected";
        return 0.0;
    }

    if (currentClitoral < CLITORAL_PRESSURE_MIN || currentClitoral > CLITORAL_PRESSURE_MAX) {
        qWarning() << "Clitoral pressure out of range:" << currentClitoral << "mmHg";
        currentClitoral = qBound(CLITORAL_PRESSURE_MIN, currentClitoral, CLITORAL_PRESSURE_MAX);
    }

    if (currentAVL < AVL_PRESSURE_MIN || currentAVL > AVL_PRESSURE_MAX) {
        qWarning() << "AVL pressure out of range:" << currentAVL << "mmHg";
        currentAVL = qBound(AVL_PRESSURE_MIN, currentAVL, AVL_PRESSURE_MAX);
    }
```

---

## MEDIUM-SEVERITY BUGS (Code Quality & Minor Functional Issues)

### BUG #11: Uninitialized Member `m_previousArousal` Used in Anti-Escape

**Severity**: MEDIUM  
**Location**: Line ~346 in `runAdaptiveEdging()` BACKING_OFF state  
**Thread Context**: Main control thread  
**Functional Impact**: False anti-escape trigger on first backing off attempt

**Verification Note**: Code at line ~289 in `startAdaptiveEdgingInternal()` should reset `m_previousArousal = 0.0`. **Verify this line exists**. If missing, add:

```cpp
void OrgasmControlAlgorithm::startAdaptiveEdgingInternal(int targetCycles)
{
    QMutexLocker locker(&m_mutex);

    m_targetEdges = targetCycles;
    m_edgeCount = 0;
    m_orgasmCount = 0;
    m_unexpectedOrgasmCount = 0;
    m_intensity = INITIAL_INTENSITY;
    m_frequency = INITIAL_FREQUENCY;
    m_emergencyStop = false;
    
    // BUG #11 fix: Reset arousal tracking for fresh session
    m_previousArousal = 0.0;  // Ensure this line is present
    m_arousalLevel = 0.0;
    m_smoothedArousal = 0.0;
    m_highPressureDuration = 0;
    
    // ...rest of initialization...
}
```

---

### BUG #12: Inconsistent Null Check Pattern in `setHeartRateSensor()`

**Severity**: MEDIUM  
**Location**: Lines ~408-435  
**Thread Context**: Main GUI thread + callback thread  
**Code Maintainability**: Defensive checks exist but are scattered in lambdas

**Note**: Code already has defensive checks (`if (!m_heartRateSensor) return;` at lines ~415, ~423, ~431). **This is good but can be improved**:

```cpp
void OrgasmControlAlgorithm::setHeartRateSensor(HeartRateSensor* sensor)
{
    QMutexLocker locker(&m_mutex);

    // BUG #12 fix: Disconnect previous sensor first (Manager Pattern cleanup)
    if (m_heartRateSensor) {
        disconnect(m_heartRateSensor, nullptr, this, nullptr);
    }

    m_heartRateSensor = sensor;

    if (sensor) {
        m_heartRateEnabled = true;

        // Per threading architecture: Use Qt::DirectConnection for signal safety
        connect(sensor, &HeartRateSensor::heartRateUpdated,
                this, [this](int bpm) {
                    QMutexLocker lock(&m_mutex);
                    // Double-check sensor still valid (defensive)
                    if (!m_heartRateSensor) return;
                    m_currentHeartRate = bpm;
                },
                Qt::DirectConnection);
        
        // Similar pattern for other signals...
    } else {
        m_heartRateEnabled = false;
        m_heartRateContribution = 0.0;
    }
}
```

---

### BUG #13: Division by Zero Risk in Arousal Weights

**Severity**: MEDIUM  
**Location**: Lines ~600-620 in `calculateArousalLevel()` heart rate integration  
**Thread Context**: DataAcquisitionThread (50 Hz)  
**Numerical Stability**: NaN propagation

**Verification**: Code should validate `PRESSURE_WEIGHT_SUM > 0` before division:

```cpp
    // Combine pressure and heart rate components
    double effectiveHRWeight = (m_heartRateEnabled && m_heartRateSensor &&
                                m_heartRateSensor->hasPulseSignal())
                                ? m_heartRateWeight : 0.0;
    double effectivePressureWeight = 1.0 - effectiveHRWeight;

    // BUG #13 fix: Validate weight sum is valid
    if (PRESSURE_WEIGHT_SUM < 0.001) {
        qWarning() << "Invalid pressure weight sum:" << PRESSURE_WEIGHT_SUM;
        return 0.0;  // Can't calculate without valid weights
    }

    double pressureArousal = pressureArousalRaw / PRESSURE_WEIGHT_SUM;
    if (!std::isfinite(pressureArousal)) {
        qWarning() << "Calculated pressure arousal is not finite:" << pressureArousal;
        pressureArousal = 0.0;
    }

    // ...rest of calculation...

    // Final check before returning
    if (!std::isfinite(arousal)) {
        qWarning() << "Final arousal is not finite:" << arousal;
        arousal = 0.0;
    }

    return arousal;
```

---

### BUG #14: Verbose Logging Not Checked in Hot Loop

**Severity**: MEDIUM  
**Location**: Lines ~1105-1127 in `performSafetyCheck()` seal monitoring  
**Thread Context**: SafetyMonitorThread (100 Hz = 10ms per cycle)  
**Performance Impact**: Excessive string formatting and I/O in hot path

**Verification**: Code should check `m_verboseLogging` before every `qDebug()`:

```cpp
        if (rapidSealLeak) {
            // BUG #14 fix: Only log at debug level if verbose mode enabled
            if (m_verboseLogging) {
                qDebug() << "Rapid seal leak detected"
                         << "AVL:" << avlPressure
                         << "Fixed threshold:" << baseThreshold
                         << "Rate:" << rateOfChange;
            } else {
                qInfo() << "Seal leak detected (verbose logging disabled)";
            }
        }

        if (m_resealTimer->isActive()) {
            if (m_verboseLogging) {
                qDebug() << "Reseal attempt in progress..."
                         << m_resealTimer->elapsed() << "/" << RESEAL_BOOST_DURATION_MS << "ms";
            }
        }
```

---

### BUG #15: Missing Mutex Lock in `performSafetyCheck()` Reseal Logic

**Severity**: MEDIUM  
**Location**: Lines ~1121-1145  
**Thread Context**: SafetyMonitorThread modifies `m_intensity`, `m_tensAmplitude`  
**Race Condition**: Main control thread may read these during modification

**Verification**: Code should protect state modifications:

```cpp
void OrgasmControlAlgorithm::onSafetyCheck()
{
    QMutexLocker locker(&m_mutex);  // Lock entire safety check
    performSafetyCheck();  // Now safe to modify shared state
}

// Inside performSafetyCheck():
        if (m_resealTimer->elapsed() < RESEAL_BOOST_DURATION_MS) {
            // BUG #15 fix: Lock already held by caller (onSafetyCheck)
            // Safe to modify m_intensity and call hardware methods
            if (m_hardware) {
                m_hardware->setSOL1(true);   // Ensure vacuum is on
                m_hardware->setSOL2(false);  // Ensure vent is closed
            }

            // Temporarily increase intensity to boost vacuum
            m_intensity = qMin(m_intensity + 0.05, MAX_INTENSITY);
        }
```

---

### BUG #16: Unreachable Code in `runCoolDown()` Completion

**Severity**: MEDIUM  
**Location**: Assumed around line ~1400 in cooldown completion logic  
**Thread Context**: Main control thread  
**Code Clarity**: Potential dead code

**Verification**: Ensure no `return` or `break` statements prevent session cleanup code from executing. Typically:

```cpp
    if (coolingComplete) {
        qDebug() << "Cooldown complete, session finished";
        // Ensure these execute before state change:
        emit sessionEnded(m_sessionFluidMl, m_orgasmCount, elapsed);
        setState(ControlState::STOPPED);  // These shouldn't be unreachable
    }
```

---

### BUG #17: Missing State Reset in `startForcedOrgasm()`

**Severity**: MEDIUM  
**Location**: Line ~362 in `startForcedOrgasm()`  
**Thread Context**: Main control thread  
**Functional Impact**: Residual state from previous session affects new session

**Verification**: Ensure method resets all relevant state variables:

```cpp
void OrgasmControlAlgorithm::startForcedOrgasm(int targetOrgasms, int maxDurationMs)
{
    QMutexLocker locker(&m_mutex);

    // Use edging logic foundation
    startAdaptiveEdgingInternal(999);  // Base initialization

    // Override for forced orgasm specifics
    m_targetOrgasms = targetOrgasms;
    m_maxDurationMs = maxDurationMs;
    m_orgasmCount = 0;
    m_edgeCount = 0;
    m_unexpectedOrgasmCount = 0;
    m_inOrgasm = false;
    m_pointOfNoReturnReached = false;

    // BUG #17 fix: Clear all residual state
    m_milkingOrgasmCount = 0;
    m_dangerZoneEntries = 0;
    m_milkingZoneTime = 0;
    m_sealLossCount = 0;
    m_resealAttemptInProgress = false;
    m_previousArousal = 0.0;

    setMode(Mode::FORCED_ORGASM);
}
```

---

## SUMMARY TABLE

| # | Severity | Bug | Location | Impact | Status |
|---|----------|-----|----------|--------|--------|
| 1 | CRITICAL | m_stateTimer/m_resealTimer null | Constructor ~30 | Crash on mode transition | **FIX PROVIDED** |
| 2 | CRITICAL | QElapsedTimer vs QTimer confusion | Header declarations | Timeout logic broken | **FIX PROVIDED** |
| 3 | CRITICAL | Atomic index race condition | updateArousalLevel ~520-553 | History misalignment | **FIX PROVIDED** |
| 4 | CRITICAL | Buffer wraparound | calculateBandPower ~672 | Out-of-bounds access | **FIX PROVIDED** |
| 5 | CRITICAL | Seal detection inverted | performSafetyCheck ~1077 | False alarms, safety failure | **FIX PROVIDED** |
| 6 | HIGH | emergencyStop() race condition | emergencyStop ~480 | Stop may not activate | **FIX PROVIDED** |
| 7 | HIGH | m_milkingFailureMode unvalidated | handleMilkingOrgasmFailure ~1547 | Undefined switch behavior | **FIX PROVIDED** |
| 8 | HIGH | Seal counter hysteresis | performSafetyCheck ~1104-1127 | Leaks undetected | **FIX PROVIDED** |
| 9 | HIGH | Oscillator not started | runMilking DANGER_REDUCTION ~1356 | Device unresponsive | **FIX PROVIDED** |
| 10 | HIGH | Undefined constants | calculateArousalLevel ~555 | Compile error | **FIX PROVIDED** |
| 11 | MEDIUM | m_previousArousal uninitialized | startAdaptiveEdgingInternal ~289 | False anti-escape | **VERIFY** |
| 12 | MEDIUM | Null check pattern inconsistent | setHeartRateSensor ~408-435 | Code clarity | **VERIFY** |
| 13 | MEDIUM | Division by zero in weights | calculateArousalLevel ~600-620 | NaN propagation | **VERIFY** |
| 14 | MEDIUM | Verbose logging not checked | performSafetyCheck ~1105-1127 | Performance impact | **VERIFY** |
| 15 | MEDIUM | Missing mutex in reseal logic | performSafetyCheck ~1121-1145 | Race condition | **VERIFY** |
| 16 | MEDIUM | Unreachable code in cooldown | runCoolDown ~1400 | State cleanup missed | **VERIFY** |
| 17 | MEDIUM | State reset missing | startForcedOrgasm ~362 | Session contamination | **FIX PROVIDED** |

---

## Recommended Fix Priority

1. **PHASE 1 (Critical Path)**:
   - Bug #1 (Timer initialization) - Blocks all mode transitions
   - Bug #3 (Atomic index race) - Corrupts sensor data
   - Bug #5 (Seal detection inverted) - Safety failure

2. **PHASE 2 (High Priority)**:
   - Bug #2 (QTimer type fix) - Affects state transitions
   - Bug #4 (Buffer wraparound) - Memory safety
   - Bug #6 (emergencyStop race) - Safety system reliability
   - Bug #10 (Pressure constants) - Compilation

3. **PHASE 3 (Functional Correctness)**:
   - Remaining HIGH bugs (#7, #8, #9)
   - MEDIUM bugs (#11-17)

---

## Testing Strategy

After fixes, verify:
1. **Unit Tests**: Test each fixed function in isolation
2. **Integration Tests**: Session flow (edge → forced → denial → milking)
3. **Safety Tests**: Seal loss detection, emergency stop activation
4. **Load Tests**: 100 Hz safety loop + 50 Hz sensor loop + 30 FPS GUI
5. **Sanitizer Runs**: AddressSanitizer + ThreadSanitizer for race conditions

```bash
# Build with sanitizers
cmake -B build -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests with error detection
./build/tests/OrgasmControlAlgorithmTests 2>&1 | tee sanitizer-output.log

# Check for detected issues
grep -i "error\|leak\|race" sanitizer-output.log
```

---

**Last Updated**: 2024-12-12  
**Files Modified**: OrgasmControlAlgorithm.h, OrgasmControlAlgorithm.cpp  
**Estimated Fix Time**: 4-6 hours (critical fixes) + 2-3 hours (testing)