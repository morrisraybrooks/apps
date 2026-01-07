# Automated Orgasm Patterns

## Overview

The automated orgasm patterns are designed to replicate natural physiological arousal and climax cycles based on observed response patterns. These patterns provide complete ~6.5-minute arousal-to-climax-to-recovery sequences with intelligent anti-detachment integration and multi-cycle support.

### 🔑 Key Advantage: Active Engorgement

Unlike commercial air-pulse toys (Womanizer, Satisfyer) that only provide oscillating pressure waves, the V-Contour dual-chamber system provides **sustained vacuum for active tissue engorgement**:

| Chamber | Function | Pattern Integration |
|---------|----------|---------------------|
| **Outer V-seal** (SOL1/SOL2) | Sustained vacuum (30-50 mmHg) for vulva/labia engorgement | Active throughout all phases |
| **Clitoral cylinder** (SOL4/SOL5) | Sustained vacuum OR oscillating air-pulse (5-13 Hz) | Switches between engorgement and stimulation modes |

**This means the V-Contour can actively induce clitoral erection BEFORE stimulation begins**, rather than relying on natural arousal. This reduces time to orgasm and ensures consistent response.

## Pattern Types

### Single Automated Orgasm
- **Duration**: ~6.5 minutes (including recovery)
- **Cycles**: 1 complete arousal-to-climax-to-recovery sequence
- **Activation**: Single button press
- **Description**: Complete physiological progression through 7 distinct phases (0-6)

### Triple Automated Orgasm
- **Duration**: ~18 minutes total
- **Cycles**: 3 consecutive orgasm cycles with recovery periods
- **Activation**: Single button press for full sequence
- **Description**: Multiple cycles with sensitivity adaptation and progressive recovery periods

### Continuous Orgasm Marathon
- **Duration**: INFINITE (until manually stopped)
- **Cycles**: Endless 4-minute orgasm cycles
- **Activation**: Single button press for continuous operation
- **Description**: Optimized endless cycling with minimal recovery periods for marathon sessions

## Physiological Phase Progression

### Phase 0: Active Engorgement (Pre-stimulation, 15-30 seconds)
**Purpose**: Actively induce clitoral and vulvar engorgement BEFORE air-pulse stimulation begins

- **Outer Chamber**: 40-50 mmHg sustained vacuum (SOL1 open, SOL2 closed)
- **Clitoral Cylinder**: 35-45 mmHg sustained vacuum (SOL4 open, SOL5 closed) - NO PULSING
- **Duration**: 15-30 seconds
- **Mode**: Constant vacuum only (no oscillation)
- **Anti-detachment**: Standard monitoring

**Rationale**: Commercial air-pulse toys skip this step entirely—they rely on natural arousal which can take 5-15 minutes. By actively engorging the clitoris with sustained vacuum, the V-Contour ensures optimal tissue erection and sensitivity from the start. This is the KEY DIFFERENTIATOR of the dual-chamber system.

**What Happens Physiologically**:
- Blood is drawn into clitoral erectile tissue (corpus cavernosum)
- Clitoral glans becomes engorged and erect
- Labia minora/majora engorge and become more sensitive
- Nerve endings are exposed and primed for stimulation

### Phase 1: Initial Sensitivity (30-60 seconds)
**Purpose**: Gentle introduction to air-pulse stimulation on pre-engorged tissue

- **Outer Chamber**: Maintains 45 mmHg sustained (keeps tissue engorged)
- **Clitoral Cylinder**: Transitions to air-pulse mode (5-6 Hz, 35% intensity)
- **Start Pressure**: 35% oscillation (very gentle pulsing)
- **Ramp Duration**: 10 seconds to 55%
- **Settling Period**: 20 seconds at moderate level
- **Variation**: ±5% gentle fluctuation
- **Anti-detachment**: Standard monitoring

**Rationale**: With tissue already engorged from Phase 0, even gentle air-pulse stimulation is highly effective. This avoids overwhelming sensitivity while building arousal on optimally prepared tissue.

### Phase 2: Adaptation Period (1-2.5 minutes)
**Purpose**: Consistent moderate intensity during body adaptation

- **Outer Chamber**: Maintains 45-50 mmHg sustained (continuous engorgement)
- **Clitoral Cylinder**: Air-pulse at 6-8 Hz, 60% intensity
- **Pressure**: 60% base level
- **Duration**: 90 seconds
- **Variation**: ±8% slow oscillation (4-second periods)
- **Anti-detachment**: Standard monitoring with seal maintenance

**Rationale**: Corresponds to the observed settling period where muscle tension decreases and body adapts to stimulation. Outer chamber maintains engorgement throughout.

### Phase 3: Arousal Build-up (2.5-4.5 minutes)
**Purpose**: Gradually increase intensity to match building arousal

- **Outer Chamber**: Maintains 50 mmHg sustained (continuous engorgement)
- **Clitoral Cylinder**: Air-pulse at 8-10 Hz, ramping intensity
- **Phase 3a** (60 seconds): 60% → 75% progressive ramp
- **Phase 3b** (60 seconds): 75% → 85% continued buildup
- **Variation**: ±10-12% with increasing frequency
- **Anti-detachment**: Enhanced monitoring activated

**Rationale**: Matches the observed period of increasing effectiveness, natural lubrication, and physical responses (squirming, head movement). Research indicates 8-13 Hz is the optimal "orgasm frequency band."

### Phase 4: Pre-Climax Tension (4.5-5.5 minutes)
**Purpose**: Build tension immediately preceding orgasm

- **Outer Chamber**: Maintains 55 mmHg sustained (maximum engorgement)
- **Clitoral Cylinder**: Air-pulse at 10-12 Hz (optimal orgasm frequency), 85% intensity
- **Pressure**: 85% sustained level
- **Duration**: 45 seconds
- **Variation**: ±8% rapid oscillation (1.5-second periods)
- **Anti-detachment**: Maximum sensitivity mode (25ms response time)

**Rationale**: Addresses the critical point where full body tension returns and precise positioning becomes essential for reaching climax.

**What Happens Physiologically**:
- Full body muscle tension increases (particularly pelvic floor, thighs, abdomen)
- Breathing becomes shallow and rapid
- Heart rate peaks at 150-180 BPM
- Clitoral glans reaches maximum engorgement and sensitivity
- Subject may experience "point of no return" sensation

### Phase 5: Climax/Orgasm (5.5-6 minutes)
**Purpose**: Maintain optimal stimulation through orgasmic contractions

- **Outer Chamber**: Maintains 55-60 mmHg sustained (compensates for involuntary movement)
- **Clitoral Cylinder**: Air-pulse at 11-13 Hz (peak orgasm frequency), 90% intensity
- **Pressure**: 90% maximum sustained level
- **Duration**: 30-45 seconds (action: `climax_maintain`)
- **Variation**: ±5% minimal variation (consistency is critical)
- **Anti-detachment**: MAXIMUM sensitivity (25ms response, aggressive correction)

**Rationale**: The 11-13 Hz frequency matches the 0.8-1.2 Hz involuntary pelvic floor contraction frequency observed during orgasm (8-13 contractions over ~10 seconds). Maintaining consistent stimulation through these contractions prolongs and intensifies the orgasm.

**What Happens Physiologically**:
- Rhythmic contractions of pelvic floor muscles (0.8-1.2 Hz)
- Uterine contractions (internal)
- Potential female ejaculation (squirting) - drains through open channel
- Involuntary vocalizations and body movements
- Intense pleasure sensation radiating from clitoris
- Heart rate may spike to 180+ BPM briefly

**Orgasm Detection (Automatic)**:
The system uses the `OrgasmControlAlgorithm` to detect orgasm onset via:
- Pressure variance analysis (arousal fluctuations)
- Contraction band power detection (0.8-1.2 Hz rhythmic pressure oscillations)
- Rate of pressure change
- Optional heart rate sensor data (HR acceleration, HRV changes)

When contractions are detected AND arousal level exceeds threshold, the system:
1. Emits `orgasmDetected` signal with count and timestamp
2. Applies `THROUGH_ORGASM_BOOST` to maintain intensity
3. Records event for fluid tracking (if enabled)

### Phase 6: Post-Climax Recovery (6-7 minutes)
**Purpose**: Gentle cooldown to prevent overstimulation of hypersensitive tissue

- **Outer Chamber**: Reduces to 30-35 mmHg (maintains light seal without pressure)
- **Clitoral Cylinder**: Reduces to 4-5 Hz, 25-30% intensity OR stops oscillation entirely
- **Pressure**: 30% gentle level
- **Duration**: 45-60 seconds (action: `post_climax_recovery`)
- **Variation**: ±3% minimal fluctuation
- **Anti-detachment**: Gentle mode (150ms response time)

**Rationale**: Post-orgasm, the clitoris becomes hypersensitive and continued high-intensity stimulation causes discomfort rather than pleasure. This phase provides a gentle wind-down while maintaining the vacuum seal.

**What Happens Physiologically**:
- Pelvic floor muscles relax
- Heart rate begins returning to baseline
- Clitoral hypersensitivity peaks then gradually subsides
- Blood begins draining from engorged tissue
- Subject experiences post-orgasmic relaxation and warmth

**Recovery Modes**:
- **Single Cycle**: 45 seconds at 30% → pattern complete
- **Multi-Cycle**: 45-60 seconds at 25-30% → transition to next cycle with adapted sensitivity
- **Continuous Marathon**: 30 seconds minimal recovery → immediate restart

## Complete Phase Timeline Summary

| Phase | Time Range | Duration | Outer Chamber | Clitoral Cylinder | Intensity |
|-------|------------|----------|---------------|-------------------|-----------|
| **0: Engorgement** | 0:00-0:30 | 30s | 45 mmHg sustained | 40 mmHg sustained (no pulse) | N/A |
| **1: Sensitivity** | 0:30-1:30 | 60s | 45 mmHg sustained | 5-6 Hz air-pulse | 35%→55% |
| **2: Adaptation** | 1:30-3:00 | 90s | 45-50 mmHg sustained | 6-8 Hz air-pulse | 60% |
| **3: Build-up** | 3:00-4:30 | 90s | 50 mmHg sustained | 8-10 Hz air-pulse | 60%→85% |
| **4: Pre-Climax** | 4:30-5:15 | 45s | 55 mmHg sustained | 10-12 Hz air-pulse | 85% |
| **5: Climax** | 5:15-5:45 | 30s | 55-60 mmHg sustained | 11-13 Hz air-pulse | 90% |
| **6: Recovery** | 5:45-6:30 | 45s | 30 mmHg sustained | 4-5 Hz or OFF | 30%→0% |

**Total Single Cycle Duration**: ~6.5 minutes (includes recovery)

## Multi-Cycle Adaptations

### Sensitivity Adaptation Between Cycles

**Cycle 1**: Full sensitivity progression
- Standard phase progression as described above
- Complete 5-minute sequence

**Cycle 2**: Reduced initial sensitivity
- Start pressure: 40% (higher than Cycle 1)
- Faster progression through early phases
- Accounts for reduced sensitivity after first climax

**Cycle 3**: Adapted final progression
- Start pressure: 45% (highest initial level)
- Extended climax phase (75 seconds vs 60 seconds)
- Optimized for final climax achievement

### Recovery Periods

**Recovery 1** (after Cycle 1): 45 seconds
- Pressure: 30% with minimal variation
- Purpose: Initial post-climax sensitivity reduction

**Recovery 2** (after Cycle 2): 60 seconds
- Pressure: 25% with gentle variation
- Purpose: Extended recovery for continued stimulation

**Final Cooldown** (after Cycle 3): 90 seconds
- Pressure: 20% with minimal variation
- Purpose: Complete session recovery and gentle conclusion

## Anti-Detachment Integration

### Enhanced Monitoring Modes

**Standard Mode**: Used during Phases 1-2
- Response time: 100ms
- Threshold: 50 mmHg
- Gentle correction to maintain seal

**Enhanced Mode**: Used during Phases 3-4
- Response time: 25ms (4x faster)
- Threshold: 50 mmHg
- Aggressive correction to prevent detachment during critical phases

**Gentle Mode**: Used during recovery periods
- Response time: 150ms
- Reduced sensitivity to prevent over-correction during sensitive recovery

### Phase-Specific Anti-Detachment

- **Climax Phase**: Maximum anti-detachment sensitivity
- **Arousal Buildup**: Enhanced monitoring begins
- **Recovery Periods**: Gentle mode to avoid discomfort
- **Between Cycles**: Maintains seal without over-stimulation

## Technical Implementation

### Pattern Structure
```cpp
// Phase progression with timing control
PatternStep phase1;
phase1.pressurePercent = 35.0;
phase1.durationMs = 10000;
phase1.action = "gentle_ramp";
phase1.parameters["ramp_to"] = 55.0;
phase1.parameters["anti_detachment_priority"] = true;
```

### Anti-Detachment Control
```cpp
// Enhanced anti-detachment during critical phases
if (step.action == "climax_maintain") {
    m_antiDetachmentMonitor->setEnhancedMode(true);
    m_antiDetachmentMonitor->setResponseDelay(25);
}
```

### Cycle Management
```cpp
// Sensitivity adaptation between cycles
double sensitivityMultiplier = 1.0 + (cycle * 0.15);
double initialIntensity = 35.0 + (cycle * 10.0);
```

## Usage Instructions

### Single Cycle Activation
1. Select "Single Automated Orgasm" pattern
2. Single button press to start
3. System automatically progresses through all 7 phases (0-6)
4. ~6.5-minute complete cycle with automatic recovery and conclusion

### Multi-Cycle Activation
1. Select "Triple Automated Orgasm" pattern
2. Single button press to start full sequence
3. System automatically manages all 3 cycles with recovery periods
4. ~18-minute complete sequence with automatic conclusion

### Safety Features
- Automatic emergency stop if anti-detachment fails
- Progressive intensity limits (maximum 90% pressure)
- Gentle recovery periods to prevent over-stimulation
- Real-time monitoring and adjustment

## Benefits

1. **Physiological Accuracy**: Matches natural arousal progression
2. **Consistent Results**: Eliminates timing guesswork
3. **Enhanced Safety**: Integrated anti-detachment prevents positioning loss
4. **Reduced Sensitivity Issues**: Controlled recovery periods
5. **Single-Button Operation**: Simple activation for complete cycles
6. **Multi-Cycle Support**: Consecutive orgasms with proper recovery
7. **Adaptive Progression**: Adjusts for changing sensitivity between cycles

## Configuration Options

- Cycle count (1-3 cycles)
- Phase duration adjustments
- Pressure level customization
- Anti-detachment sensitivity settings
- Recovery period lengths
- Variation patterns and frequencies

This system transforms the vacuum controller from a manual stimulation device into an intelligent automated system that replicates and optimizes natural physiological response patterns.
