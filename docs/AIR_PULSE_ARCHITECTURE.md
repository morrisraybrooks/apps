# Air Pulse Pattern Architecture

## Overview

This document explains how the Air Pulse pattern correctly routes high-speed valve control to the ClitoralOscillator class, solving the "Commercial Toy Emulation" problem.

## Problem Statement

Commercial clitoral stimulation toys (Womanizer, Satisfyer) use high-frequency air pulses (8-13 Hz) to induce orgasm. Our initial implementation incorrectly tried to generate these pulses using the PatternEngine's step-based system, which:

1. Created thousands of individual PatternStep objects
2. Drove the main pump (SOL1) instead of the clitoral cylinder (SOL4/SOL5)
3. Could not achieve the required timing precision (77ms period @ 13Hz)

## Solution: Hardware-Accelerated Oscillation

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      PatternEngine                          │
│  - Manages "macro steps" (5-second segments)                │
│  - Controls outer chamber (SOL1/SOL2) for seal             │
│  - Delegates inner chamber to ClitoralOscillator            │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ step.parameters["clitoral_oscillation"] = true
                            │ step.parameters["clitoral_frequency"] = 12.0
                            │ step.parameters["clitoral_amplitude"] = 40.0
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   ClitoralOscillator                        │
│  - Runs at 1ms timer resolution (Qt::PreciseTimer)         │
│  - Controls inner chamber (SOL4/SOL5) at 5-13 Hz           │
│  - 4-phase cycle: SUCTION → HOLD → VENT → TRANSITION       │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    HardwareManager                          │
│  - SOL1/SOL2: Outer chamber (PatternEngine control)        │
│  - SOL4/SOL5: Inner chamber (ClitoralOscillator control)   │
│  - Separate mutexes prevent conflicts                      │
└─────────────────────────────────────────────────────────────┘
```

### Code Flow

1. **Pattern Building** (`buildAirPulsePattern`)
   - Creates 5-second "macro steps" instead of millisecond steps
   - Each step specifies oscillation parameters (frequency, amplitude)
   - Outer chamber pressure maintained for seal integrity

2. **Step Execution** (`executeStep`)
   - Detects AIR_PULSE pattern type
   - Configures ClitoralOscillator with step parameters
   - Starts/updates oscillator in real-time
   - Applies outer chamber pressure via `applyPressureTarget()`

3. **Oscillation Control** (`ClitoralOscillator::onTimerTick`)
   - Runs at 1ms resolution (1000 Hz timer)
   - Executes 4-phase valve timing
   - Measures peak pressure and adjusts duty cycle
   - Independent from PatternEngine's step loop

## Valve Timing Strategy

### 4-Phase Asymmetric Cycle

For a 12 Hz oscillation (83ms period):

| Phase      | Duration | SOL4 (Vacuum) | SOL5 (Vent) | Purpose                    |
|------------|----------|---------------|-------------|----------------------------|
| SUCTION    | 25ms     | OPEN          | CLOSED      | Rapid vacuum build         |
| HOLD       | 8ms      | CLOSED        | CLOSED      | Peak pressure maintained   |
| VENT       | 25ms     | CLOSED        | OPEN        | Rapid pressure release     |
| TRANSITION | 25ms     | CLOSED        | CLOSED      | Settling before next cycle |

**Why 4 phases instead of 2?**

- **HOLD phase**: Allows pressure to propagate to tissue before release
- **TRANSITION phase**: Prevents valve chattering and creates smoother waveform
- **Result**: More sine-wave-like pressure oscillation vs. harsh square wave

### Duty Cycle Modulation

Amplitude is controlled by adjusting the SUCTION phase duration:
- Low amplitude (20 mmHg): 15ms suction, 10ms hold, 25ms vent, 33ms transition
- High amplitude (60 mmHg): 35ms suction, 5ms hold, 25ms vent, 18ms transition

## Conflict Prevention

### Segregation of Duties

| Component          | Controls      | Frequency | Purpose                |
|--------------------|---------------|-----------|------------------------|
| PatternEngine      | SOL1/SOL2     | 0.1-4 Hz  | Outer seal maintenance |
| ClitoralOscillator | SOL4/SOL5     | 5-13 Hz   | Air pulse stimulation  |
| AntiDetachment     | SOL1 override | As needed | Emergency seal restore |

### Mutex Strategy

```cpp
// HardwareManager.cpp
void HardwareManager::setSOL1(bool open) {
    QMutexLocker locker(&m_outerChamberMutex);  // Separate mutex
    // ... control SOL1 ...
}

void HardwareManager::setSOL4(bool open) {
    QMutexLocker locker(&m_innerChamberMutex);  // Separate mutex
    // ... control SOL4 ...
}
```

## PID Control vs. Timed Switching

**Question**: Do we need PID control for air pulse patterns?

**Answer**: No. Use timed switching with iterative correction.

### Why PID Fails at High Frequencies

- PID loops typically run at 10-50 Hz
- Trying to PID control a 13 Hz wave means the PID reacts to the wave itself
- Result: Oscillation fighting, instability, poor performance

### Iterative Learning Control (Current Implementation)

```cpp
void ClitoralOscillator::adjustAmplitude() {
    double measuredPeak = m_hardware->readClitoralPressure();
    double error = m_targetAmplitude - measuredPeak;
    
    if (std::abs(error) > 2.0) {  // 2 mmHg tolerance
        // Adjust duty cycle for next cycle
        m_dutyCycle += error * 0.01;  // Small correction
        calculatePhaseDurations();
    }
}
```

This approach:
- Measures peak pressure achieved in each cycle
- Adjusts duty cycle for the **next** cycle (not current)
- Converges over 5-10 cycles to target amplitude
- Perfect for repetitive waveforms

## Testing Recommendations

1. **Frequency Sweep Test**: Verify 5-13 Hz range with oscilloscope on pressure sensor
2. **Amplitude Linearity**: Confirm 20-60 mmHg range with different duty cycles
3. **Chamber Isolation**: Ensure outer chamber (SOL1) doesn't affect inner oscillation
4. **Long-Duration Stability**: Run 5-minute air pulse pattern, verify no drift

## References

- `src/patterns/PatternEngine.cpp` - Pattern building and execution
- `src/hardware/ClitoralOscillator.cpp` - High-speed oscillation control
- `src/hardware/HardwareManager.cpp` - Valve control and mutex management
- Research: Optimal orgasm induction frequency is 8-13 Hz (see research/orgasm_control.md)

