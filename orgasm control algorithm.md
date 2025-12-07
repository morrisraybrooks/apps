# Orgasm Control Algorithm Design

## V-Contour Dual-Chamber Vacuum System

### Adaptive Arousal Detection and Stimulation Control

---

## 1. Overview

This document describes the orgasm control algorithm for the V-Contour system, implementing:

1. **Arousal Detection** - Real-time estimation using pressure sensor biofeedback
2. **Adaptive Edging** - Sensor-driven approach/deny cycles
3. **Forced Orgasm** - Relentless stimulation through multiple climaxes
4. **Safety Integration** - Emergency stop and tissue protection

---

## 2. Arousal Detection Algorithm

### 2.1 Physiological Basis

During female sexual arousal, clitoral engorgement causes measurable changes:

| Arousal Stage | Clitoral Volume Change | Pressure Signature |
|---------------|------------------------|-------------------|
| Baseline | 0% | Stable vacuum reading |
| Early Arousal | 25-50% increase | Slight pressure fluctuation |
| Plateau | 75-150% increase | Sustained pressure rise, rhythmic variations |
| Pre-Orgasm | 200-300% increase | Rapid pressure oscillations, micro-contractions |
| Orgasm | Peak + rhythmic contractions | Large amplitude pressure waves (0.8-1.2 Hz) |
| Resolution | Gradual decrease | Slow pressure normalization |

### 2.2 Heart Rate as Arousal Indicator

Heart rate provides a reliable, non-invasive physiological marker of sexual arousal:

| Arousal Stage | Heart Rate (BPM) | HRV (RMSSD) | Physiological Basis |
|---------------|------------------|-------------|---------------------|
| Baseline | 60-80 | 50-100 ms | Resting parasympathetic dominance |
| Early Arousal | 80-100 | 40-60 ms | Sympathetic activation begins |
| Plateau | 100-130 | 30-50 ms | Sustained sympathetic tone |
| Pre-Orgasm | 130-160 | 20-35 ms | Peak sympathetic, reduced variability |
| Orgasm | 150-180+ | < 25 ms | Autonomic surge, rhythmic contractions |
| Resolution | 100→70 | 40-80 ms | Parasympathetic recovery |

**Heart Rate Variability (HRV)**: Decreases during arousal as sympathetic nervous system dominates. Low HRV + high HR = strong arousal indicator.

### 2.3 Sensor-Based Arousal Indicators

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                       MULTI-SENSOR AROUSAL DETECTION                                │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│   PRESSURE SENSORS                              HEART RATE SENSOR                   │
│   ┌───────────┐ ┌───────────┐ ┌───────────┐    ┌─────────────────┐                 │
│   │ AVL (CH0) │ │Clitoral   │ │ Tank (CH1)│    │ Pulse Sensor    │                 │
│   │ Outer seal│ │ (CH2)     │ │ Reference │    │ (Analog/MAX30102)│                 │
│   └─────┬─────┘ └─────┬─────┘ └─────┬─────┘    └────────┬────────┘                 │
│         │             │             │                    │                          │
│         ▼             ▼             ▼                    ▼                          │
│   ┌───────────┐ ┌───────────┐ ┌───────────┐    ┌─────────────────┐                 │
│   │ Low-Pass  │ │ Bandpass  │ │ Seal      │    │ Peak Detection  │                 │
│   │ Filter    │ │ 0.5-3 Hz  │ │ Check     │    │ BPM Calculation │                 │
│   └─────┬─────┘ └─────┬─────┘ └─────┬─────┘    └────────┬────────┘                 │
│         │             │             │                    │                          │
│         ▼             ▼             ▼                    ▼                          │
│   ┌────────────────────────────────────────────────────────────────────────────┐   │
│   │                         AROUSAL ESTIMATOR                                   │   │
│   │                                                                             │   │
│   │   Pressure Features (70%):              Heart Rate Features (30%):         │   │
│   │   • Baseline deviation     (25%)        • HR Zone (50% of HR)              │   │
│   │   • Pressure variance      (15%)        • HRV inverted (25% of HR)         │   │
│   │   • Contraction power      (20%)        • HR acceleration (25% of HR)      │   │
│   │   • Rate of change         (10%)                                           │   │
│   │   × Seal integrity                      Orgasm signature detection:        │   │
│   │                                         HR > 150 + HRV < 30 + sustained    │   │
│   │                                                                             │   │
│   └────────────────────────────────┬────────────────────────────────────────────┘   │
│                                    │                                                 │
│                                    ▼                                                 │
│                          ┌───────────────────┐                                      │
│                          │  Arousal Level    │                                      │
│                          │  (0.0 - 1.0)      │                                      │
│                          └───────────────────┘                                      │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.4 Arousal Estimation Formula (Multi-Sensor Fusion)

```cpp
// Multi-sensor weighted arousal estimation
double calculateArousalLevel() {
    // === PRESSURE FEATURES ===
    double baselineDeviation = (m_currentClitoralPressure - m_baselineClitoral) / m_baselineClitoral;
    double pressureVariance = calculateVariance(m_pressureHistory, VARIANCE_WINDOW_SAMPLES);
    double contractionPower = calculateBandPower(m_pressureHistory, 0.8, 1.2);  // Hz
    double rateOfChange = calculateDerivative(m_pressureHistory);
    double sealIntegrity = m_currentAVLPressure / m_targetAVLPressure;

    // Normalize pressure features to 0.0-1.0 range
    double normDeviation = clamp(baselineDeviation / MAX_DEVIATION, 0.0, 1.0);
    double normVariance = clamp(pressureVariance / MAX_VARIANCE, 0.0, 1.0);
    double normContraction = clamp(contractionPower / MAX_CONTRACTION_POWER, 0.0, 1.0);
    double normROC = clamp(abs(rateOfChange) / MAX_RATE_OF_CHANGE, 0.0, 1.0);

    // Pressure-based arousal (70% when HR enabled)
    double pressureArousal =
        WEIGHT_DEVIATION * normDeviation +      // 0.25
        WEIGHT_VARIANCE * normVariance +        // 0.15
        WEIGHT_CONTRACTION * normContraction +  // 0.20
        WEIGHT_ROC * normROC;                   // 0.10

    // === HEART RATE FEATURES (if enabled) ===
    double heartRateArousal = 0.0;
    if (m_heartRateEnabled && m_heartRateSensor->hasPulseSignal()) {
        double hrNormalized = m_heartRateSensor->getHeartRateNormalized();   // 0-1 based on zone
        double hrvNormalized = m_heartRateSensor->getHRVNormalized();        // Inverted: low HRV = high arousal
        double hrAcceleration = m_heartRateSensor->getHeartRateAcceleration();
        double normAccel = clamp(abs(hrAcceleration) / 10.0, 0.0, 1.0);      // Max 10 BPM/sec

        // Heart rate arousal component (30% of total)
        heartRateArousal =
            WEIGHT_HR_ZONE * hrNormalized +     // 0.50 of HR weight
            WEIGHT_HRV * hrvNormalized +        // 0.25 of HR weight
            WEIGHT_HR_ACCEL * normAccel;        // 0.25 of HR weight
    }

    // === SENSOR FUSION ===
    double effectivePressureWeight = 1.0 - m_heartRateWeight;  // 0.70 when HR enabled
    double arousal = effectivePressureWeight * pressureArousal +
                     m_heartRateWeight * heartRateArousal;     // 0.30 when HR enabled

    // Apply seal integrity penalty
    arousal *= sealIntegrity;

    // Exponential smoothing to prevent jitter
    m_smoothedArousal = AROUSAL_ALPHA * arousal + (1.0 - AROUSAL_ALPHA) * m_smoothedArousal;

    return clamp(m_smoothedArousal, 0.0, 1.0);
}
```

### 2.5 Threshold Values

| Parameter | Value | Description |
|-----------|-------|-------------|
| `BASELINE_SAMPLE_TIME` | 10 seconds | Initial calibration period |
| `VARIANCE_WINDOW_SAMPLES` | 200 (10s @ 20Hz) | Sliding window for variance |
| `MAX_DEVIATION` | 0.5 (50%) | Max expected baseline deviation |
| `MAX_VARIANCE` | 25.0 mmHg² | Max expected pressure variance |
| `MAX_CONTRACTION_POWER` | 10.0 | Normalized 0.8-1.2 Hz band power |
| `MAX_RATE_OF_CHANGE` | 5.0 mmHg/s | Max pressure change rate |
| `AROUSAL_ALPHA` | 0.15 | Smoothing factor |
| `EDGE_THRESHOLD` | 0.85 | Pre-orgasm arousal level |
| `ORGASM_THRESHOLD` | 0.95 | Orgasm detection threshold |
| `DEFAULT_HR_WEIGHT` | 0.30 | Heart rate contribution to arousal |
| `HR_ORGASM_BPM` | 150 | BPM threshold for orgasm signature |
| `HR_ORGASM_HRV` | 30 ms | HRV threshold for orgasm signature |

### 2.5 Arousal State Machine

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       AROUSAL STATE MACHINE                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                          ┌──────────────┐                                  │
│                          │   BASELINE   │◄─────────────────────────┐       │
│                          │  (0.0-0.2)   │                          │       │
│                          └──────┬───────┘                          │       │
│                                 │ arousal > 0.2                    │       │
│                                 ▼                                  │       │
│                          ┌──────────────┐                          │       │
│                          │   WARMING    │                          │       │
│                          │  (0.2-0.5)   │                          │       │
│                          └──────┬───────┘                          │       │
│                                 │ arousal > 0.5                    │       │
│                                 ▼                                  │       │
│                          ┌──────────────┐                          │       │
│                          │   PLATEAU    │                          │       │
│                          │  (0.5-0.85)  │◄──────┐                  │       │
│                          └──────┬───────┘       │                  │       │
│                                 │ arousal > 0.85│ arousal < 0.7   │       │
│                                 ▼               │                  │       │
│                          ┌──────────────┐       │                  │       │
│              ┌───────────│  PRE-ORGASM  │───────┘                  │       │
│              │           │  (0.85-0.95) │                          │       │
│    EDGE MODE │           └──────┬───────┘                          │       │
│   (back off) │                  │ arousal > 0.95                   │       │
│              │                  ▼                                  │       │
│              │           ┌──────────────┐                          │       │
│              │           │   ORGASM     │                          │       │
│              └──────────►│  (0.95-1.0)  │──────────────────────────┘       │
│                          │  + contractions detected                │       │
│                          └──────────────┘                          │       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Edging Algorithm

### 3.1 Adaptive Edging Strategy

Unlike pre-programmed edging patterns, adaptive edging uses real-time arousal feedback:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ADAPTIVE EDGING ALGORITHM                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                          MAIN LOOP                                   │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│   1. BASELINE CALIBRATION (10 seconds)                                     │
│      └─► Record resting clitoral pressure                                  │
│                                                                             │
│   2. BUILD-UP PHASE                                                        │
│      ├─► Start at low intensity (outer: 30%, clitoral: 6 Hz)              │
│      ├─► LOOP:                                                             │
│      │   ├─► Read arousal level                                           │
│      │   ├─► IF arousal < EDGE_THRESHOLD (0.85):                          │
│      │   │   ├─► Increase intensity by RAMP_RATE                          │
│      │   │   └─► Increase oscillation frequency                           │
│      │   ├─► ELSE IF arousal >= EDGE_THRESHOLD:                           │
│      │   │   └─► GOTO BACK-OFF PHASE                                      │
│      │   └─► Wait 100ms                                                   │
│      └─► Continue until edge detected                                     │
│                                                                             │
│   3. BACK-OFF PHASE                                                        │
│      ├─► Rapidly reduce intensity (outer: 10%, clitoral: STOP)            │
│      ├─► Wait for arousal to drop below RECOVERY_THRESHOLD (0.70)         │
│      ├─► Minimum back-off time: 5 seconds                                 │
│      └─► Record edge event (time, intensity at edge)                      │
│                                                                             │
│   4. HOLD PHASE                                                            │
│      ├─► Maintain low stimulation (outer: 20%, clitoral: 5 Hz light)      │
│      ├─► Duration: 3-10 seconds (configurable)                            │
│      └─► User remains highly aroused but not approaching orgasm           │
│                                                                             │
│   5. REPEAT (configurable cycles, default: 5)                             │
│      └─► GOTO step 2 with slightly higher starting intensity              │
│                                                                             │
│   6. OPTIONAL: RELEASE                                                     │
│      └─► After N cycles, allow orgasm (GOTO Forced Orgasm mode)           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Edging Pseudocode

```cpp
void OrgasmControlAlgorithm::runAdaptiveEdging(int targetCycles) {
    // Phase 1: Baseline calibration
    calibrateBaseline(BASELINE_DURATION_MS);

    // Phase 2-5: Edging cycles
    int edgeCount = 0;
    double baseIntensity = INITIAL_INTENSITY;  // 0.3 (30%)
    double baseFrequency = INITIAL_FREQUENCY;  // 6.0 Hz

    while (edgeCount < targetCycles && !m_emergencyStop) {
        // BUILD-UP PHASE
        setState(BUILDING);
        double intensity = baseIntensity;
        double frequency = baseFrequency;

        while (m_arousalLevel < EDGE_THRESHOLD && !m_emergencyStop) {
            // Apply current stimulation
            m_hardware->setOuterChamberPressure(intensity * MAX_OUTER_PRESSURE);
            m_clitoralOscillator->setFrequency(frequency);
            m_clitoralOscillator->setAmplitude(intensity * MAX_CLITORAL_AMPLITUDE);

            // Update arousal estimate
            updateArousalLevel();

            // Ramp up if not yet at edge
            if (m_arousalLevel < EDGE_THRESHOLD * 0.9) {
                intensity = qMin(intensity + RAMP_RATE, MAX_INTENSITY);
                frequency = qMin(frequency + FREQ_RAMP_RATE, MAX_FREQUENCY);
            }

            // Emit progress
            emit buildUpProgress(m_arousalLevel, EDGE_THRESHOLD);

            QThread::msleep(UPDATE_INTERVAL_MS);
        }

        // EDGE DETECTED - BACK OFF
        setState(BACKING_OFF);
        emit edgeDetected(edgeCount + 1, intensity);

        // Rapid stimulation reduction
        m_clitoralOscillator->stop();
        m_hardware->setOuterChamberPressure(BACKOFF_PRESSURE);  // 10%

        // Wait for arousal to drop
        QElapsedTimer backoffTimer;
        backoffTimer.start();

        while ((m_arousalLevel > RECOVERY_THRESHOLD ||
                backoffTimer.elapsed() < MIN_BACKOFF_MS) &&
               !m_emergencyStop) {
            updateArousalLevel();
            emit backOffProgress(m_arousalLevel, RECOVERY_THRESHOLD);
            QThread::msleep(UPDATE_INTERVAL_MS);
        }

        // HOLD PHASE
        setState(HOLDING);
        m_hardware->setOuterChamberPressure(HOLD_PRESSURE);  // 20%
        m_clitoralOscillator->setFrequency(HOLD_FREQUENCY);  // 5 Hz
        m_clitoralOscillator->setAmplitude(HOLD_AMPLITUDE);  // Low
        m_clitoralOscillator->start();

        QThread::msleep(HOLD_DURATION_MS);

        // Increment edge count and prepare for next cycle
        edgeCount++;
        emit edgeCycleCompleted(edgeCount, targetCycles);

        // Escalate base intensity for next cycle (user builds tolerance)
        baseIntensity = qMin(baseIntensity + ESCALATION_RATE, MAX_BASE_INTENSITY);
        baseFrequency = qMin(baseFrequency + FREQ_ESCALATION_RATE, MAX_FREQUENCY);
    }

    // All cycles complete
    if (!m_emergencyStop) {
        emit edgingComplete(edgeCount);
    }
}
```

### 3.3 Edging Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `INITIAL_INTENSITY` | 0.30 | Starting intensity (30%) |
| `INITIAL_FREQUENCY` | 6.0 Hz | Starting oscillation frequency |
| `EDGE_THRESHOLD` | 0.85 | Arousal level to trigger back-off |
| `RECOVERY_THRESHOLD` | 0.70 | Target arousal after back-off |
| `RAMP_RATE` | 0.005/update | Intensity increase per 100ms |
| `FREQ_RAMP_RATE` | 0.02 Hz/update | Frequency increase per 100ms |
| `BACKOFF_PRESSURE` | 10% | Outer chamber during back-off |
| `HOLD_PRESSURE` | 20% | Outer chamber during hold |
| `HOLD_FREQUENCY` | 5.0 Hz | Light oscillation during hold |
| `MIN_BACKOFF_MS` | 5000 | Minimum back-off duration |
| `HOLD_DURATION_MS` | 5000 | Hold phase duration |
| `ESCALATION_RATE` | 0.05 | Intensity increase per cycle |

---

## 4. Forced Orgasm Algorithm

### 4.1 Forced Orgasm Strategy

Forced orgasm applies relentless, escalating stimulation that continues through and after orgasm:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     FORCED ORGASM ALGORITHM                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                         MAIN LOOP                                    │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│   1. INITIALIZATION                                                        │
│      ├─► Calibrate baseline (10 seconds)                                  │
│      ├─► Set intensity: FORCED_BASE_INTENSITY (60%)                       │
│      └─► Set frequency: FORCED_BASE_FREQUENCY (10 Hz)                     │
│                                                                             │
│   2. RELENTLESS STIMULATION LOOP                                          │
│      ├─► Apply maximum stimulation:                                       │
│      │   ├─► Outer chamber: 60-80% pressure                               │
│      │   ├─► Clitoral oscillation: 10-13 Hz                               │
│      │   └─► Optional TENS: 20-30 Hz, 80% amplitude                       │
│      ├─► Monitor arousal level continuously                               │
│      ├─► IF arousal decreasing (user trying to escape):                   │
│      │   └─► INCREASE intensity (anti-escape escalation)                  │
│      ├─► IF orgasm detected:                                              │
│      │   ├─► Log orgasm event                                             │
│      │   ├─► DO NOT reduce stimulation                                    │
│      │   └─► Maintain or increase intensity through orgasm                │
│      └─► Continue until target orgasm count OR max duration               │
│                                                                             │
│   3. POST-ORGASM CONTINUATION                                             │
│      ├─► After each orgasm, brief intensity adjustment (2 seconds)        │
│      ├─► Then return to high-intensity stimulation                        │
│      ├─► Sensitivity increases post-orgasm → faster next orgasm           │
│      └─► Track refractory periods (typically shorten with each O)         │
│                                                                             │
│   4. SESSION TERMINATION                                                   │
│      ├─► Target orgasm count reached (e.g., 3-5)                          │
│      ├─► Maximum session duration (e.g., 30 minutes)                      │
│      ├─► OR emergency stop triggered                                      │
│      └─► Gradual cool-down (not abrupt stop)                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Forced Orgasm Pseudocode

```cpp
void OrgasmControlAlgorithm::runForcedOrgasm(int targetOrgasms, int maxDurationMs) {
    // Initialization
    calibrateBaseline(BASELINE_DURATION_MS);

    int orgasmCount = 0;
    double intensity = FORCED_BASE_INTENSITY;  // 0.60
    double frequency = FORCED_BASE_FREQUENCY;  // 10.0 Hz
    double tensAmplitude = FORCED_TENS_AMPLITUDE;  // 0.80

    QElapsedTimer sessionTimer;
    sessionTimer.start();

    // Enable all stimulation systems
    m_hardware->setOuterChamberPressure(intensity * MAX_OUTER_PRESSURE);
    m_clitoralOscillator->setFrequency(frequency);
    m_clitoralOscillator->setAmplitude(intensity * MAX_CLITORAL_AMPLITUDE);
    m_clitoralOscillator->start();

    if (m_tensEnabled) {
        m_tensController->setFrequency(TENS_FORCED_FREQUENCY);  // 25 Hz
        m_tensController->setAmplitude(tensAmplitude);
        m_tensController->start();
    }

    setState(FORCING);

    double previousArousal = 0.0;
    bool inOrgasm = false;
    QElapsedTimer orgasmTimer;

    while (orgasmCount < targetOrgasms &&
           sessionTimer.elapsed() < maxDurationMs &&
           !m_emergencyStop) {

        // Update arousal estimation
        updateArousalLevel();

        // ANTI-ESCAPE: If arousal dropping, escalate intensity
        if (m_arousalLevel < previousArousal - AROUSAL_DROP_THRESHOLD) {
            intensity = qMin(intensity + ANTI_ESCAPE_RATE, MAX_INTENSITY);
            frequency = qMin(frequency + ANTI_ESCAPE_FREQ_RATE, MAX_FREQUENCY);

            m_hardware->setOuterChamberPressure(intensity * MAX_OUTER_PRESSURE);
            m_clitoralOscillator->setFrequency(frequency);
            m_clitoralOscillator->setAmplitude(intensity * MAX_CLITORAL_AMPLITUDE);

            emit antiEscapeTriggered(intensity, frequency);
        }

        // ORGASM DETECTION
        if (!inOrgasm && m_arousalLevel >= ORGASM_THRESHOLD) {
            // Check for rhythmic contractions (0.8-1.2 Hz)
            if (detectContractions()) {
                inOrgasm = true;
                orgasmTimer.start();
                orgasmCount++;

                emit orgasmDetected(orgasmCount, sessionTimer.elapsed());

                // MAINTAIN intensity through orgasm (do NOT back off)
                // Optionally increase slightly for more intense experience
                intensity = qMin(intensity + THROUGH_ORGASM_BOOST, MAX_INTENSITY);
                m_hardware->setOuterChamberPressure(intensity * MAX_OUTER_PRESSURE);
            }
        }

        // ORGASM RECOVERY (brief adjustment, then continue)
        if (inOrgasm) {
            if (orgasmTimer.elapsed() > ORGASM_DURATION_MS) {
                // Brief post-orgasm adjustment
                if (orgasmTimer.elapsed() < ORGASM_DURATION_MS + POST_ORGASM_PAUSE_MS) {
                    // Very brief intensity dip (not a full back-off)
                    m_clitoralOscillator->setAmplitude(intensity * 0.7 * MAX_CLITORAL_AMPLITUDE);
                } else {
                    // Resume full intensity for next orgasm
                    inOrgasm = false;
                    m_clitoralOscillator->setAmplitude(intensity * MAX_CLITORAL_AMPLITUDE);

                    // Post-orgasm sensitivity is higher → can often escalate faster
                    frequency = qMin(frequency + POST_ORGASM_FREQ_BOOST, MAX_FREQUENCY);
                    m_clitoralOscillator->setFrequency(frequency);
                }
            }
        }

        previousArousal = m_arousalLevel;

        emit forcedOrgasmProgress(orgasmCount, targetOrgasms,
                                  sessionTimer.elapsed(), maxDurationMs);

        QThread::msleep(UPDATE_INTERVAL_MS);
    }

    // Session complete - gradual cool-down
    runCoolDown(COOLDOWN_DURATION_MS);

    emit forcedOrgasmComplete(orgasmCount, sessionTimer.elapsed());
}
```

### 4.3 Forced Orgasm Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `FORCED_BASE_INTENSITY` | 0.60 | Starting intensity (60%) |
| `FORCED_BASE_FREQUENCY` | 10.0 Hz | Starting oscillation frequency |
| `ORGASM_THRESHOLD` | 0.95 | Arousal level indicating orgasm |
| `CONTRACTION_MIN_FREQ` | 0.8 Hz | Minimum contraction frequency |
| `CONTRACTION_MAX_FREQ` | 1.2 Hz | Maximum contraction frequency |
| `ANTI_ESCAPE_RATE` | 0.02/update | Intensity boost when arousal drops |
| `ANTI_ESCAPE_FREQ_RATE` | 0.1 Hz/update | Frequency boost when escaping |
| `THROUGH_ORGASM_BOOST` | 0.05 | Intensity increase during orgasm |
| `ORGASM_DURATION_MS` | 15000 | Expected orgasm duration |
| `POST_ORGASM_PAUSE_MS` | 2000 | Brief adjustment after orgasm |
| `POST_ORGASM_FREQ_BOOST` | 0.5 Hz | Frequency increase for next O |
| `TENS_FORCED_FREQUENCY` | 25.0 Hz | TENS frequency during forced |
| `COOLDOWN_DURATION_MS` | 60000 | Gradual cool-down period |

---

## 5. TENS Integration

### 5.1 Combined Stimulation Strategy

TENS (dorsal genital nerve stimulation) enhances arousal through neural pathway activation, complementing mechanical vacuum stimulation:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│               TENS + VACUUM COORDINATION                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   AROUSAL LEVEL         VACUUM                    TENS                      │
│   ─────────────────────────────────────────────────────────────────────    │
│   0.0-0.3 (Warming)     30% outer, 6 Hz          OFF (or 10 Hz priming)    │
│   0.3-0.5 (Building)    40% outer, 8 Hz          15 Hz, 40% amplitude      │
│   0.5-0.7 (Plateau)     50% outer, 10 Hz         20 Hz, 60% amplitude      │
│   0.7-0.85 (High)       60% outer, 11 Hz         25 Hz, 75% amplitude      │
│   0.85-0.95 (Edge)      70% outer, 12 Hz         30 Hz, 85% amplitude      │
│   0.95-1.0 (Orgasm)     80% outer, 13 Hz         30 Hz, 90% amplitude      │
│                                                                             │
│   EDGING MODE: TENS stops during back-off (helps arousal drop faster)      │
│   FORCED MODE: TENS never stops (maintains neural activation)              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Phase Synchronization

For enhanced effect, TENS pulses can be synchronized with vacuum oscillation:

```cpp
void OrgasmControlAlgorithm::synchronizeTENSWithOscillation() {
    // Get clitoral oscillator phase
    ClitoralOscillator::Phase vacuumPhase = m_clitoralOscillator->getCurrentPhase();

    // Synchronize TENS to fire during SUCTION phase for maximum effect
    switch (vacuumPhase) {
        case ClitoralOscillator::Phase::SUCTION:
            m_tensController->setPhase(TENSController::Phase::POSITIVE);
            break;
        case ClitoralOscillator::Phase::VENT:
            m_tensController->setPhase(TENSController::Phase::NEGATIVE);
            break;
        default:
            m_tensController->setPhase(TENSController::Phase::INTER_PULSE);
            break;
    }
}
```

---

## 6. Safety Integration

### 6.1 Emergency Stop Handling

```cpp
void OrgasmControlAlgorithm::handleEmergencyStop() {
    m_emergencyStop = true;

    // Immediate hardware shutdown
    m_clitoralOscillator->stop();
    m_hardware->setOuterChamberPressure(0);
    m_hardware->setSOL2(true);   // Vent outer chamber
    m_hardware->setSOL5(true);   // Vent clitoral chamber

    if (m_tensEnabled) {
        m_tensController->emergencyStop();  // Immediate TENS disable
    }

    setState(STOPPED);
    emit emergencyStopActivated();
}
```

### 6.2 Safety Checks (100ms interval)

```cpp
void OrgasmControlAlgorithm::performSafetyCheck() {
    // 1. Seal integrity check
    double avlPressure = m_hardware->readAVLPressure();
    if (avlPressure < SEAL_LOST_THRESHOLD) {
        emit sealIntegrityWarning(avlPressure);
        // Reduce intensity but don't stop entirely
        m_intensity *= 0.5;
    }

    // 2. Overpressure protection
    double clitoralPressure = m_hardware->readClitoralPressure();
    if (clitoralPressure > MAX_SAFE_CLITORAL_PRESSURE) {
        m_hardware->setSOL5(true);  // Emergency vent
        emit overpressureWarning(clitoralPressure);
    }

    // 3. TENS impedance check
    if (m_tensEnabled && m_tensController->isFault()) {
        m_tensController->stop();
        emit tensFault(m_tensController->getFaultReason());
    }

    // 4. Session duration limit
    if (m_sessionTimer.elapsed() > MAX_SESSION_DURATION_MS) {
        runCoolDown(COOLDOWN_DURATION_MS);
        emit sessionTimeoutWarning();
    }

    // 5. Tissue protection (prevent sustained high pressure)
    if (m_highPressureDuration > MAX_HIGH_PRESSURE_DURATION_MS) {
        // Brief pressure release
        m_hardware->setOuterChamberPressure(m_intensity * 0.5 * MAX_OUTER_PRESSURE);
        m_highPressureDuration = 0;
        emit tissueProtectionTriggered();
    }
}
```

### 6.3 Safety Thresholds

| Parameter | Value | Description |
|-----------|-------|-------------|
| `SEAL_LOST_THRESHOLD` | 10 mmHg | AVL pressure indicating seal loss |
| `MAX_SAFE_CLITORAL_PRESSURE` | 80 mmHg | Maximum clitoral chamber pressure |
| `MAX_SESSION_DURATION_MS` | 1800000 | 30 minute max session |
| `MAX_HIGH_PRESSURE_DURATION_MS` | 120000 | 2 min max at >70% intensity |
| `COOLDOWN_DURATION_MS` | 60000 | Gradual 1-minute cooldown |

---

## 7. C++ Implementation

### 7.1 OrgasmControlAlgorithm Class Header

```cpp
// src/control/OrgasmControlAlgorithm.h

#ifndef ORGASMCONTROLALGORITHM_H
#define ORGASMCONTROLALGORITHM_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <cmath>

class HardwareManager;
class SensorInterface;
class ClitoralOscillator;
class TENSController;

class OrgasmControlAlgorithm : public QObject
{
    Q_OBJECT

public:
    // Arousal states
    enum class ArousalState {
        BASELINE,      // 0.0-0.2
        WARMING,       // 0.2-0.5
        PLATEAU,       // 0.5-0.85
        PRE_ORGASM,    // 0.85-0.95
        ORGASM         // 0.95-1.0
    };
    Q_ENUM(ArousalState)

    // Algorithm states
    enum class ControlState {
        STOPPED,
        CALIBRATING,
        BUILDING,
        BACKING_OFF,
        HOLDING,
        FORCING,
        COOLING_DOWN,
        ERROR
    };
    Q_ENUM(ControlState)

    // Mode
    enum class Mode {
        MANUAL,
        ADAPTIVE_EDGING,
        FORCED_ORGASM,
        MULTI_ORGASM,
        DENIAL
    };
    Q_ENUM(Mode)

    explicit OrgasmControlAlgorithm(HardwareManager* hardware, QObject* parent = nullptr);
    ~OrgasmControlAlgorithm();

    // Control
    void startAdaptiveEdging(int targetCycles = 5);
    void startForcedOrgasm(int targetOrgasms = 3, int maxDurationMs = 1800000);
    void startDenial(int durationMs = 600000);  // Tease without allowing orgasm
    void stop();
    void emergencyStop();

    // Configuration
    void setEdgeThreshold(double threshold);      // Default: 0.85
    void setOrgasmThreshold(double threshold);    // Default: 0.95
    void setRecoveryThreshold(double threshold);  // Default: 0.70
    void setTENSEnabled(bool enabled);
    void setAntiEscapeEnabled(bool enabled);

    // Status
    ControlState getState() const { return m_state; }
    Mode getMode() const { return m_mode; }
    double getArousalLevel() const { return m_arousalLevel; }
    ArousalState getArousalState() const { return m_arousalState; }
    int getEdgeCount() const { return m_edgeCount; }
    int getOrgasmCount() const { return m_orgasmCount; }
    double getCurrentIntensity() const { return m_intensity; }
    double getCurrentFrequency() const { return m_frequency; }

    // Arousal history (for UI visualization)
    QVector<double> getArousalHistory() const;

Q_SIGNALS:
    // Arousal signals
    void arousalLevelChanged(double level);
    void arousalStateChanged(ArousalState state);

    // Edging signals
    void edgeDetected(int edgeNumber, double intensityAtEdge);
    void edgeCycleCompleted(int current, int total);
    void edgingComplete(int totalEdges);
    void buildUpProgress(double arousal, double threshold);
    void backOffProgress(double arousal, double threshold);

    // Forced orgasm signals
    void orgasmDetected(int orgasmNumber, qint64 timeMs);
    void antiEscapeTriggered(double newIntensity, double newFrequency);
    void forcedOrgasmProgress(int orgasms, int target, qint64 elapsed, qint64 max);
    void forcedOrgasmComplete(int totalOrgasms, qint64 durationMs);

    // Safety signals
    void emergencyStopActivated();
    void sealIntegrityWarning(double pressure);
    void overpressureWarning(double pressure);
    void tissueProtectionTriggered();
    void sessionTimeoutWarning();
    void tensFault(const QString& reason);

    // State signals
    void stateChanged(ControlState state);
    void modeChanged(Mode mode);

private Q_SLOTS:
    void onUpdateTick();
    void onSafetyCheck();

private:
    // Arousal detection
    void calibrateBaseline(int durationMs);
    void updateArousalLevel();
    double calculateArousalLevel();
    double calculateVariance(const QVector<double>& data, int windowSize);
    double calculateBandPower(const QVector<double>& data, double freqLow, double freqHigh);
    double calculateDerivative(const QVector<double>& data);
    bool detectContractions();
    void updateArousalState();

    // Algorithm phases
    void runAdaptiveEdging(int targetCycles);
    void runForcedOrgasm(int targetOrgasms, int maxDurationMs);
    void runCoolDown(int durationMs);

    // Safety
    void performSafetyCheck();
    void handleEmergencyStop();

    // Helpers
    void setState(ControlState state);
    void setMode(Mode mode);
    double clamp(double value, double min, double max);

    // Hardware
    HardwareManager* m_hardware;
    SensorInterface* m_sensorInterface;
    ClitoralOscillator* m_clitoralOscillator;
    TENSController* m_tensController;

    // Timers
    QTimer* m_updateTimer;
    QTimer* m_safetyTimer;
    QElapsedTimer m_sessionTimer;

    // State
    ControlState m_state;
    Mode m_mode;
    bool m_emergencyStop;
    bool m_tensEnabled;
    bool m_antiEscapeEnabled;

    // Arousal tracking
    double m_arousalLevel;
    double m_smoothedArousal;
    ArousalState m_arousalState;
    double m_baselineClitoral;
    double m_baselineAVL;
    QVector<double> m_pressureHistory;
    QVector<double> m_arousalHistory;
    int m_historyIndex;

    // Stimulation parameters
    double m_intensity;
    double m_frequency;
    double m_tensAmplitude;

    // Counters
    int m_edgeCount;
    int m_orgasmCount;
    qint64 m_highPressureDuration;

    // Thresholds
    double m_edgeThreshold;
    double m_orgasmThreshold;
    double m_recoveryThreshold;

    // Thread safety
    mutable QMutex m_mutex;

    // Constants
    static const int UPDATE_INTERVAL_MS = 100;     // 10 Hz update rate
    static const int SAFETY_INTERVAL_MS = 100;     // 10 Hz safety checks
    static const int BASELINE_DURATION_MS = 10000; // 10 second calibration
    static const int HISTORY_SIZE = 200;           // 20 seconds at 10 Hz
    static const int VARIANCE_WINDOW_SAMPLES = 100; // 10 seconds

    // Arousal weights
    static constexpr double WEIGHT_DEVIATION = 0.35;
    static constexpr double WEIGHT_VARIANCE = 0.25;
    static constexpr double WEIGHT_CONTRACTION = 0.30;
    static constexpr double WEIGHT_ROC = 0.10;
    static constexpr double AROUSAL_ALPHA = 0.15;

    // Normalization maxima
    static constexpr double MAX_DEVIATION = 0.5;
    static constexpr double MAX_VARIANCE = 25.0;
    static constexpr double MAX_CONTRACTION_POWER = 10.0;
    static constexpr double MAX_RATE_OF_CHANGE = 5.0;

    // Edging defaults
    static constexpr double INITIAL_INTENSITY = 0.30;
    static constexpr double INITIAL_FREQUENCY = 6.0;
    static constexpr double RAMP_RATE = 0.005;
    static constexpr double FREQ_RAMP_RATE = 0.02;
    static constexpr double BACKOFF_PRESSURE = 0.10;
    static constexpr double HOLD_PRESSURE = 0.20;
    static constexpr double HOLD_FREQUENCY = 5.0;
    static constexpr double HOLD_AMPLITUDE = 0.30;
    static constexpr double ESCALATION_RATE = 0.05;
    static constexpr double FREQ_ESCALATION_RATE = 0.3;
    static constexpr int MIN_BACKOFF_MS = 5000;
    static constexpr int HOLD_DURATION_MS = 5000;

    // Forced orgasm defaults
    static constexpr double FORCED_BASE_INTENSITY = 0.60;
    static constexpr double FORCED_BASE_FREQUENCY = 10.0;
    static constexpr double FORCED_TENS_AMPLITUDE = 0.80;
    static constexpr double AROUSAL_DROP_THRESHOLD = 0.05;
    static constexpr double ANTI_ESCAPE_RATE = 0.02;
    static constexpr double ANTI_ESCAPE_FREQ_RATE = 0.1;
    static constexpr double THROUGH_ORGASM_BOOST = 0.05;
    static constexpr double TENS_FORCED_FREQUENCY = 25.0;
    static constexpr int ORGASM_DURATION_MS = 15000;
    static constexpr int POST_ORGASM_PAUSE_MS = 2000;
    static constexpr double POST_ORGASM_FREQ_BOOST = 0.5;
    static constexpr int COOLDOWN_DURATION_MS = 60000;

    // Safety limits
    static constexpr double SEAL_LOST_THRESHOLD = 10.0;
    static constexpr double MAX_SAFE_CLITORAL_PRESSURE = 80.0;
    static constexpr double MAX_OUTER_PRESSURE = 75.0;
    static constexpr double MAX_CLITORAL_AMPLITUDE = 60.0;
    static constexpr double MAX_INTENSITY = 0.95;
    static constexpr double MAX_FREQUENCY = 13.0;
    static constexpr double MAX_BASE_INTENSITY = 0.80;
    static constexpr int MAX_SESSION_DURATION_MS = 1800000;
    static constexpr int MAX_HIGH_PRESSURE_DURATION_MS = 120000;
};

#endif // ORGASMCONTROLALGORITHM_H
```

---

## 8. Pattern Definitions

### 8.1 New Pattern Types

Add to `PatternDefinitions.cpp`:

```cpp
void PatternDefinitions::createOrgasmControlPatterns()
{
    // ========================================================================
    // ADAPTIVE EDGING PATTERN (Sensor-Driven)
    // ========================================================================
    PatternInfo adaptiveEdging;
    adaptiveEdging.name = "Adaptive Edging";
    adaptiveEdging.type = "orgasm_control";
    adaptiveEdging.category = "Orgasm Control";
    adaptiveEdging.description = "Sensor-driven edging with automatic edge detection and denial";
    adaptiveEdging.basePressure = 30.0;
    adaptiveEdging.intensity = 70.0;
    adaptiveEdging.isValid = true;

    // Parameters for OrgasmControlAlgorithm
    adaptiveEdging.parameters["mode"] = "adaptive_edging";
    adaptiveEdging.parameters["target_edges"] = 5;
    adaptiveEdging.parameters["edge_threshold"] = 0.85;
    adaptiveEdging.parameters["recovery_threshold"] = 0.70;
    adaptiveEdging.parameters["initial_intensity"] = 0.30;
    adaptiveEdging.parameters["initial_frequency"] = 6.0;
    adaptiveEdging.parameters["ramp_rate"] = 0.005;
    adaptiveEdging.parameters["min_backoff_ms"] = 5000;
    adaptiveEdging.parameters["hold_duration_ms"] = 5000;
    adaptiveEdging.parameters["tens_enabled"] = true;
    adaptiveEdging.parameters["allow_release"] = true;  // Allow orgasm after target edges

    m_patterns["Adaptive Edging"] = adaptiveEdging;

    // ========================================================================
    // FORCED ORGASM PATTERN
    // ========================================================================
    PatternInfo forcedOrgasm;
    forcedOrgasm.name = "Forced Orgasm";
    forcedOrgasm.type = "orgasm_control";
    forcedOrgasm.category = "Orgasm Control";
    forcedOrgasm.description = "Relentless stimulation continuing through and after orgasm";
    forcedOrgasm.basePressure = 60.0;
    forcedOrgasm.intensity = 90.0;
    forcedOrgasm.isValid = true;

    forcedOrgasm.parameters["mode"] = "forced_orgasm";
    forcedOrgasm.parameters["target_orgasms"] = 3;
    forcedOrgasm.parameters["max_duration_ms"] = 1800000;  // 30 minutes
    forcedOrgasm.parameters["base_intensity"] = 0.60;
    forcedOrgasm.parameters["base_frequency"] = 10.0;
    forcedOrgasm.parameters["anti_escape_enabled"] = true;
    forcedOrgasm.parameters["tens_enabled"] = true;
    forcedOrgasm.parameters["tens_frequency"] = 25.0;
    forcedOrgasm.parameters["tens_amplitude"] = 0.80;
    forcedOrgasm.parameters["cooldown_duration_ms"] = 60000;

    m_patterns["Forced Orgasm"] = forcedOrgasm;

    // ========================================================================
    // MULTI-ORGASM TRAINING
    // ========================================================================
    PatternInfo multiOrgasm;
    multiOrgasm.name = "Multi-Orgasm Training";
    multiOrgasm.type = "orgasm_control";
    multiOrgasm.category = "Orgasm Control";
    multiOrgasm.description = "Training pattern for achieving multiple sequential orgasms";
    multiOrgasm.basePressure = 50.0;
    multiOrgasm.intensity = 80.0;
    multiOrgasm.isValid = true;

    multiOrgasm.parameters["mode"] = "multi_orgasm";
    multiOrgasm.parameters["target_orgasms"] = 5;
    multiOrgasm.parameters["max_duration_ms"] = 2400000;  // 40 minutes
    multiOrgasm.parameters["post_orgasm_pause_ms"] = 3000;  // Brief pause between
    multiOrgasm.parameters["post_orgasm_freq_boost"] = 0.5;
    multiOrgasm.parameters["refractory_adaptation"] = true;  // Adapt to shortening refractory
    multiOrgasm.parameters["tens_enabled"] = true;

    m_patterns["Multi-Orgasm Training"] = multiOrgasm;

    // ========================================================================
    // DENIAL MODE (Pure Edging, No Release)
    // ========================================================================
    PatternInfo denial;
    denial.name = "Denial Mode";
    denial.type = "orgasm_control";
    denial.category = "Orgasm Control";
    denial.description = "Extended teasing without allowing orgasm - for BDSM denial play";
    denial.basePressure = 40.0;
    denial.intensity = 75.0;
    denial.isValid = true;

    denial.parameters["mode"] = "denial";
    denial.parameters["duration_ms"] = 1200000;  // 20 minutes
    denial.parameters["edge_threshold"] = 0.80;  // More conservative edge detection
    denial.parameters["recovery_threshold"] = 0.50;  // Drop further before resuming
    denial.parameters["max_edges"] = 20;  // High edge count before auto-stop
    denial.parameters["allow_release"] = false;  // Never allow orgasm
    denial.parameters["frustration_escalation"] = true;  // Gradually increase intensity
    denial.parameters["tens_enabled"] = true;

    m_patterns["Denial Mode"] = denial;

    // ========================================================================
    // GENTLE EDGING (Beginner Friendly)
    // ========================================================================
    PatternInfo gentleEdging;
    gentleEdging.name = "Gentle Edging";
    gentleEdging.type = "orgasm_control";
    gentleEdging.category = "Orgasm Control";
    gentleEdging.description = "Slower, gentler edging with longer back-off periods";
    gentleEdging.basePressure = 25.0;
    gentleEdging.intensity = 60.0;
    gentleEdging.isValid = true;

    gentleEdging.parameters["mode"] = "adaptive_edging";
    gentleEdging.parameters["target_edges"] = 3;
    gentleEdging.parameters["edge_threshold"] = 0.80;
    gentleEdging.parameters["recovery_threshold"] = 0.50;
    gentleEdging.parameters["initial_intensity"] = 0.20;
    gentleEdging.parameters["initial_frequency"] = 5.0;
    gentleEdging.parameters["ramp_rate"] = 0.003;  // Slower ramp
    gentleEdging.parameters["min_backoff_ms"] = 10000;  // Longer back-off
    gentleEdging.parameters["hold_duration_ms"] = 8000;  // Longer hold
    gentleEdging.parameters["tens_enabled"] = false;  // No TENS for gentle mode
    gentleEdging.parameters["allow_release"] = true;

    m_patterns["Gentle Edging"] = gentleEdging;

    // ========================================================================
    // INTENSE FORCED (Maximum Intensity)
    // ========================================================================
    PatternInfo intenseForced;
    intenseForced.name = "Intense Forced";
    intenseForced.type = "orgasm_control";
    intenseForced.category = "Orgasm Control";
    intenseForced.description = "Maximum intensity forced orgasm with aggressive anti-escape";
    intenseForced.basePressure = 70.0;
    intenseForced.intensity = 95.0;
    intenseForced.isValid = true;

    intenseForced.parameters["mode"] = "forced_orgasm";
    intenseForced.parameters["target_orgasms"] = 5;
    intenseForced.parameters["max_duration_ms"] = 1800000;
    intenseForced.parameters["base_intensity"] = 0.75;
    intenseForced.parameters["base_frequency"] = 12.0;
    intenseForced.parameters["anti_escape_enabled"] = true;
    intenseForced.parameters["anti_escape_rate"] = 0.04;  // Aggressive escalation
    intenseForced.parameters["through_orgasm_boost"] = 0.10;  // Larger boost during O
    intenseForced.parameters["tens_enabled"] = true;
    intenseForced.parameters["tens_frequency"] = 30.0;
    intenseForced.parameters["tens_amplitude"] = 0.90;

    m_patterns["Intense Forced"] = intenseForced;
}
```

### 8.2 Update PatternEngine for Orgasm Control

Add to `PatternEngine.h`:

```cpp
// Forward declaration
class OrgasmControlAlgorithm;

// Add to PatternType enum:
enum PatternType {
    // ... existing types ...
    ORGASM_CONTROL    // Adaptive arousal-based patterns
};

// Add member:
private:
    std::unique_ptr<OrgasmControlAlgorithm> m_orgasmControl;

// Add methods:
public:
    OrgasmControlAlgorithm* getOrgasmControlAlgorithm() const { return m_orgasmControl.get(); }

    // Orgasm control signals
Q_SIGNALS:
    void arousalLevelChanged(double level);
    void edgeDetected(int edgeNumber);
    void orgasmDetected(int orgasmNumber);
```

---

## 9. Algorithm Flowchart

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                  COMPLETE ORGASM CONTROL FLOWCHART                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   START                                                                     │
│     │                                                                       │
│     ▼                                                                       │
│   ┌─────────────────┐                                                      │
│   │ Select Mode:    │                                                      │
│   │ • Adaptive Edge │                                                      │
│   │ • Forced Orgasm │                                                      │
│   │ • Multi-Orgasm  │                                                      │
│   │ • Denial        │                                                      │
│   └────────┬────────┘                                                      │
│            │                                                                │
│            ▼                                                                │
│   ┌─────────────────┐                                                      │
│   │ CALIBRATE       │  Record baseline clitoral pressure (10s)             │
│   │ BASELINE        │                                                      │
│   └────────┬────────┘                                                      │
│            │                                                                │
│            ▼                                                                │
│   ┌─────────────────┐          ┌─────────────────┐                         │
│   │ Read Sensors    │◄─────────│ Safety Check    │ (every 100ms)           │
│   │ • AVL Pressure  │          │ • Seal OK?      │                         │
│   │ • Clitoral P    │          │ • Overpressure? │                         │
│   │ • Tank P        │          │ • TENS Fault?   │                         │
│   └────────┬────────┘          └────────┬────────┘                         │
│            │                            │                                   │
│            ▼                            │ FAIL                              │
│   ┌─────────────────┐                   ▼                                  │
│   │ Calculate       │          ┌─────────────────┐                         │
│   │ Arousal Level   │          │ EMERGENCY STOP  │                         │
│   │ (0.0 - 1.0)     │          │ Vent all valves │                         │
│   └────────┬────────┘          └─────────────────┘                         │
│            │                                                                │
│            ▼                                                                │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │                                                       │                 │
│   │  ADAPTIVE EDGING          │    FORCED ORGASM         │                 │
│   │                           │                          │                 │
│   │  arousal < edge?          │    arousal dropping?     │                 │
│   │  ├─YES: Increase stim     │    ├─YES: Anti-escape    │                 │
│   │  └─NO : Back off          │    │       escalate      │                 │
│   │                           │    └─NO : Maintain       │                 │
│   │  arousal < recovery?      │                          │                 │
│   │  ├─YES: Hold phase        │    Orgasm detected?      │                 │
│   │  └─NO : Continue backoff  │    ├─YES: Log + maintain │                 │
│   │                           │    └─NO : Continue       │                 │
│   │  Edge count reached?      │                          │                 │
│   │  ├─YES: Release or Stop   │    Target reached?       │                 │
│   │  └─NO : Next cycle        │    ├─YES: Cool down      │                 │
│   │                           │    └─NO : Continue       │                 │
│   │                           │                          │                 │
│   └──────────────────────────────────────────────────────┘                 │
│            │                                                                │
│            ▼                                                                │
│   ┌─────────────────┐                                                      │
│   │ COOL DOWN       │  Gradual intensity reduction (60s)                   │
│   └────────┬────────┘                                                      │
│            │                                                                │
│            ▼                                                                │
│   ┌─────────────────┐                                                      │
│   │ SESSION         │  Log: edges, orgasms, duration, peak arousal         │
│   │ COMPLETE        │                                                      │
│   └─────────────────┘                                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Integration Notes

### 10.1 CMakeLists.txt Update

```cmake
# Add to src/CMakeLists.txt
set(CONTROL_SOURCES
    control/OrgasmControlAlgorithm.cpp
)

set(CONTROL_HEADERS
    control/OrgasmControlAlgorithm.h
)

# Add to target sources
target_sources(${PROJECT_NAME} PRIVATE
    ${CONTROL_SOURCES}
    ${CONTROL_HEADERS}
)
```

### 10.2 PatternEngine Integration

```cpp
// In PatternEngine::initializePattern()
if (type == "orgasm_control") {
    m_currentPatternType = ORGASM_CONTROL;

    QString mode = patternData["parameters"]["mode"].toString();

    if (mode == "adaptive_edging") {
        int targetEdges = patternData["parameters"]["target_edges"].toInt(5);
        m_orgasmControl->startAdaptiveEdging(targetEdges);
    } else if (mode == "forced_orgasm") {
        int targetOrgasms = patternData["parameters"]["target_orgasms"].toInt(3);
        int maxDuration = patternData["parameters"]["max_duration_ms"].toInt(1800000);
        m_orgasmControl->startForcedOrgasm(targetOrgasms, maxDuration);
    } else if (mode == "denial") {
        int duration = patternData["parameters"]["duration_ms"].toInt(1200000);
        m_orgasmControl->startDenial(duration);
    }
}
```

### 10.3 UI Integration Recommendations

The following signals should be connected to UI elements:

| Signal | UI Element |
|--------|-----------|
| `arousalLevelChanged(double)` | Real-time arousal gauge (0-100%) |
| `arousalStateChanged(ArousalState)` | State indicator (color-coded) |
| `edgeDetected(int, double)` | Edge counter + notification |
| `orgasmDetected(int, qint64)` | Orgasm counter + timestamp |
| `buildUpProgress(double, double)` | Progress bar to edge threshold |
| `antiEscapeTriggered(double, double)` | Warning/notification |
| `emergencyStopActivated()` | Modal warning dialog |

---

## 11. References

1. **Komisaruk, B. R., & Whipple, B.** (2005). Functional MRI of the brain during orgasm in women. *Annual Review of Sex Research*, 16(1), 62-86.

2. **Meston, C. M., & Buss, D. M.** (2007). Why humans have sex. *Archives of Sexual Behavior*, 36(4), 477-507.

3. **Levin, R. J.** (2004). An orgasm is... who defines what an orgasm is? *Sexual and Relationship Therapy*, 19(1), 101-107.

4. **Georgiadis, J. R., et al.** (2006). Regional cerebral blood flow changes associated with clitorally induced orgasm in healthy women. *European Journal of Neuroscience*, 24(11), 3305-3316.

5. **F-Machine Tremblr Documentation** - Vacuum oscillation mechanism for sustained stimulation

6. **Venus 2000 Patent** (US5501650) - Pneumatic sexual stimulation device mechanics

---

## 12. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-12-07 | V-Contour Team | Initial algorithm design |