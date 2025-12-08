# Vacuum Controller GUI System - Complete Code Explanation

---

## What Is This System?

This is an **Automated Vacuum Stimulation Device** - a sophisticated electromechanical system that applies controlled vacuum pressure to the vulva/clitoris for the purpose of inducing sexual arousal and orgasm. It is designed as a "hands-free" device that can automatically bring a user to climax through precisely controlled vacuum patterns.

The system uses a **vacuum cup** that creates an airtight seal against the body. A pump creates negative pressure (suction) inside the cup, which draws blood into the erectile tissues (clitoris, labia), causing engorgement and heightened sensitivity - similar to natural sexual arousal. By varying the vacuum pressure in specific patterns and timing sequences, the device can replicate and optimize the physiological stages of sexual response.

---

## Why Does This System Exist?

### The Core Problem It Solves

Manual stimulation (whether by hand, partner, or vibrator) has inherent limitations:
1. **Fatigue** - Maintaining consistent stimulation is tiring
2. **Inconsistency** - Human hands cannot maintain precise timing/pressure for extended periods
3. **Timing errors** - The critical moments before orgasm require precise positioning and rhythm
4. **Distraction** - Having to consciously control stimulation interferes with arousal

### The Solution: Automation

This system automates the entire arousal-to-orgasm process:
- **Single-button activation** - No manual effort required during the session
- **Physiologically-optimized patterns** - Based on observed response data
- **Anti-detachment monitoring** - Maintains seal automatically during body movements
- **Multiple cycle support** - Can induce sequential orgasms with proper recovery periods

### Design Philosophy

The system was designed based on observation of actual physiological responses:

1. **Initial sensitivity is high** - The device starts gentle to avoid overwhelming sensation
2. **Adaptation period required** - The body needs time to adjust (muscle tension decreases)
3. **Arousal builds progressively** - Intensity increases as natural lubrication develops
4. **Critical climax phase** - Precise positioning and consistent rhythm are essential
5. **Post-orgasm sensitivity** - Recovery periods account for heightened sensitivity

---

## How It Works (Physical Operation)

### Hardware Components

The system is a **Dual-Chamber V-Contour System** that uses 5 solenoid valves for precise control over two independent vacuum zones.

For a detailed explanation of the hardware components, the dual-chamber architecture, and the complete 5-valve vacuum system diagram, please see **Part 2: Dual-Chamber V-Contour System Design** below.

### Pressure Sensing

Two **MPX5010DP differential pressure sensors** measure:
1. **AVL Pressure** - Actual vacuum at the cup (critical for control)
2. **Tank Pressure** - Vacuum in the reservoir (for system monitoring)

These connect to an **MCP3008 ADC** which converts analog voltages to digital readings at 50Hz.

### Pressure Range

- **0 mmHg** - Atmospheric (no vacuum)
- **35-60 mmHg** - Comfortable therapeutic range
- **75 mmHg** - Maximum safe limit (sensor full scale)
- **85-90%** - Peak stimulation during climax phase


---

## The Pattern System (How Orgasms Are Induced)

The **PatternEngine** is the brain of the system. It executes timed sequences of vacuum pressure changes designed to replicate optimal stimulation.

### Pattern Types

| Pattern | Purpose | Duration |
|---------|---------|----------|
| **Pulse** | Rhythmic suction/release cycles | Variable |
| **Wave** | Sinusoidal pressure variation | 5s period |
| **Air Pulse** | Rapid suction bursts (1-20 Hz) | Variable |
| **Milking** | Stroke-like suction waves | 7 strokes |
| **Edging** | Build to edge, release, repeat | 3 cycles |
| **Automated Orgasm** | Full arousal-to-climax sequence | ~5 min |
| **Triple Orgasm** | 3 sequential orgasms with recovery | ~18 min |
| **Continuous Marathon** | Infinite orgasm loops | Until stopped |

### Automated Orgasm Pattern - The Core Algorithm

This is the flagship feature - a complete automated arousal-to-climax sequence:

```
Phase 1: Initial Sensitivity (0-30 seconds)
├── Start at 35% pressure (very gentle)
├── Ramp to 55% over 10 seconds
├── Settle at moderate level for 20 seconds
└── Purpose: Avoid overwhelming initial sensitivity

Phase 2: Adaptation Period (30s - 2 minutes)
├── Hold at 60% with ±8% slow oscillation
├── Duration: 90 seconds
└── Purpose: Allow body to adapt, muscle tension decreases

Phase 3: Arousal Build-up (2-4 minutes)
├── 3a: Ramp from 60% → 75% (60 seconds)
├── 3b: Ramp from 75% → 85% (60 seconds)
├── Increasing oscillation frequency
└── Purpose: Match building arousal, natural lubrication begins

Phase 4: Pre-Climax Tension (4-5 minutes)
├── Maintain 85% with ±8% rapid oscillation
├── 1.5-second timing precision
├── Maximum anti-detachment sensitivity
└── Purpose: Maintain precise positioning for climax
```

### Multi-Cycle Adaptation

After orgasm, sensitivity changes. The system adapts:

| Cycle | Start Pressure | Notes |
|-------|---------------|-------|
| Cycle 1 | 35% | Full progression |
| Cycle 2 | 40% | Reduced sensitivity after first orgasm |
| Cycle 3 | 45% | Extended climax phase (75s vs 60s) |

Recovery periods between cycles:
- After Cycle 1: 45 seconds at 30%
- After Cycle 2: 60 seconds at 25%
- Final cooldown: 90 seconds at 20%

### Continuous Orgasm Marathon

For extended sessions, optimized 4-minute cycles run indefinitely:
- Faster ramp-up (already aroused)
- Higher base pressures (40% start, 65% adaptation)
- Extended 90-second climax phase at 88%
- Brief 30-second recovery at 45% (maintains arousal)
- Loops forever until manually stopped

---

## The Anti-Detachment System (Critical Safety Feature)

During arousal and especially approaching orgasm, the body moves involuntarily (squirming, muscle contractions). This can break the vacuum seal, interrupting stimulation at the critical moment.

### How It Works

The **AntiDetachmentMonitor** runs at **100Hz** (10ms intervals) monitoring AVL pressure:

1. **Seal intact**: Pressure stays stable at target level
2. **Seal breaking**: Pressure suddenly drops toward atmospheric
3. **Response**: Immediately increases vacuum via SOL1 to re-establish seal

### Response Modes

| Mode | Response Time | Used During |
|------|---------------|-------------|
| Standard | 100ms | Phases 1-2 |
| Enhanced | 25ms | Phases 3-4 (arousal/climax) |
| Gentle | 150ms | Recovery periods |

During climax phase, the system responds in **25ms** - fast enough to maintain seal through involuntary body movements.

---

## Software Architecture

### Initialization Flow

```
main()
  └─> QApplication (High DPI, touch support)
  └─> ModernMedicalStyle (medical device UI theme)
  └─> VacuumController::initialize()
        └─> HardwareManager     (GPIO, SPI, sensors, actuators)
        └─> SafetyManager       (pressure limits, emergency stop)
        └─> PatternEngine       (stimulation patterns)
        └─> AntiDetachmentMonitor (seal integrity)
        └─> ThreadManager       (real-time threads)
        └─> CalibrationManager  (sensor calibration)
  └─> MainWindow (50-inch touch GUI)
  └─> startMonitoringThreads()
```

### Subsystem Responsibilities

#### **HardwareManager** (`src/hardware/`)
- **MCP3008.cpp** - SPI communication with ADC
- **SensorInterface.cpp** - Pressure readings with filtering
- **ActuatorControl.cpp** - Pump PWM, valve control via libgpiod

#### **SafetyManager** (`src/safety/`)
- 10Hz monitoring loop
- Pressure limits: 75 mmHg max, 60 mmHg warning
- Emergency stop coordination
- Sensor timeout detection

#### **PatternEngine** (`src/patterns/`)
- Pattern step execution
- Timing control with speed multipliers
- Anti-detachment mode switching per phase
- Infinite loop support for continuous patterns

#### **AntiDetachmentMonitor** (`src/safety/`)
- 100Hz pressure monitoring
- Automatic vacuum correction
- Phase-specific sensitivity adjustment
- Configurable response delay (25-150ms)

#### **ThreadManager** (`src/threading/`)
- **DataAcquisitionThread** - 50Hz sensor sampling
- **GuiUpdateThread** - 30 FPS display updates
- Integrated safety checks in data thread

### GUI Components (`src/gui/`)

Designed for 50-inch medical touch displays:
- **MainWindow** - Stacked panel navigation
- **PressureMonitor** - Real-time pressure visualization
- **PatternSelector** - Pattern selection with one-touch start
- **SafetyPanel** - Emergency controls
- **SettingsPanel** - System configuration
- **CustomPatternEditor** - User pattern creation

### Keyboard Shortcuts

- **ESC** - Emergency Stop
- **F1** - Main Control Panel
- **F2** - Safety Panel
- **F3** - Settings
- **F4** - Diagnostics

---

## Technical Specifications

| Component | Specification |
|-----------|--------------|
| **Platform** | Raspberry Pi 4 (8GB RAM) |
| **Display** | 50-inch HDMI touch display |
| **Framework** | Qt 5, C++17 |
| **ADC** | MCP3008 (10-bit, 8-channel) |
| **Pressure Sensors** | 2× MPX5010DP (0-75 mmHg) |
| **Pump Control** | PWM via L293D driver |
| **Valves** | 3× 12V solenoid valves |
| **Sensor Sampling** | 50Hz |
| **Safety Monitoring** | 100Hz |
| **GUI Update Rate** | 30 FPS |



---

## Summary

This system transforms vacuum stimulation from a manual, inconsistent process into an automated, physiologically-optimized experience. Key innovations:

1. **Physiological pattern design** - Based on observed arousal response data
2. **Anti-detachment system** - Maintains seal through body movements
3. **Multi-cycle support** - Adapts intensity for sequential orgasms
4. **Continuous mode** - Infinite loops for marathon sessions
5. **Safety-critical design** - Multiple layers of protection
6. **Touch-optimized GUI** - Single-button operation on medical displays

The goal: **One button press → Complete automated orgasm experience**

---

# Part 2: Dual-Chamber V-Contour System Design

---

## 1. Hardware Evolution: Dual-Chamber V-Contour System

### 1.1 The Problem with Traditional Dome Cups

Standard vacuum cups have fundamental design flaws:
- **Generic dome shape** doesn't match vulva anatomy
- **Single chamber** cannot provide both attachment AND targeted stimulation
- **Closed design** traps fluids (urine, vaginal secretions, ejaculate) inside the cup
- **No access** to urethra or vaginal opening for simultaneous play
- **Uniform suction** cannot replicate the localized sensation of oral stimulation

### 1.2 The V-Contour Solution: Anatomical V-Shape Design

The V-Contour is a purpose-built vacuum interface engineered for the female vulva:

```
┌───────────────────────────────────────────────────────────────────┐
│                  V-CONTOUR CUP ANATOMY (Top View)                 │
│                                                                   │
│                     ╭────────────────────╮                        │
│                    ╱                      ╲                       │
│                   ╱    OUTER V-CHAMBER     ╲                      │
│                  ╱     (Labia Seal Zone)    ╲                     │
│                 ╱                            ╲                    │
│                │     ┌─────────────────┐      │                   │
│                │     │                 │      │                   │
│                │     │  CLITORAL       │      │                   │
│                │     │  CYLINDER       │      │                   │
│                │     │  (Air Pulse     │      │                   │
│                │     │   Zone)         │      │                   │
│                │     └─┬─────────────┬─┘      │                   │
│                │       │             │        │                   │
│                │       │             │        │                   │
│                │       │ OPEN CHANNEL│        │                   │
│                │       │ (Urethra +  │        │                   │
│                │       │  Vaginal    │        │                   │
│                │       │  Access)    │        │                   │
│                │       │             │        │                   │
│                │       │             │        │                   │
│                │       │             │        │                   │
│                │       │             │        │                   │
│                │       │             │        │                   │
│                └───────┘             └────────┘                   │
│                                                                   │
│                                                                   │
│  Legend:                                                          │
│  • Outer V-Chamber: Seals around labia majora/minora              │
│  • Open Channel: Fluid drainage + accessory access                │
│  • Clitoral Cylinder: Independent air-pulse stimulation           │
└───────────────────────────────────────────────────────────────────┘
```
### 1.3 Dual-Chamber Architecture

The V-Contour utilizes two independent vacuum zones:

| Zone | Name | Function | Vacuum Type |
|------|------|----------|-------------|
| **Zone 1** | Outer V-Seal Chamber | Attachment, engorgement, labia stimulation | Constant/slow variation |
| **Zone 2** | Clitoral Cylinder | Targeted clitoral air-pulse stimulation | Rapid pulsing (1-20 Hz) OR sustained engorgement |

**Zone 1: Outer V-Seal Chamber** (SOL1/SOL2)
- Creates peripheral seal around the vulva (labia majora/minora)
- Maintains **sustained vacuum (30-50 mmHg continuous)** for blood engorgement of vulva and labia
- Causes active tissue engorgement → increased sensitivity and clitoral erection
- Anti-detachment system monitors this zone only

**Zone 2: Clitoral Cylinder** (SOL4/SOL5)
- Small cylinder positioned at apex of V, over clitoral glans
- Independent vacuum supply enables two operating modes:
  - **Sustained vacuum mode**: Continuous negative pressure for clitoral engorgement
  - **Air-pulse mode**: Rapid pressure oscillation (5-13 Hz) for stimulation
- Replicates "air pulse" sensation (like Womanizer/Satisfyer) at industrial scale
- Can operate in constant, pulsing, or pattern modes independently

### 🔑 CRITICAL DIFFERENTIATOR: Active Engorgement Capability

> **Commercial air-pulse toys (Womanizer, Satisfyer, LELO) only provide oscillating pressure waves for stimulation—they CANNOT create sustained vacuum for tissue engorgement.** They rely on natural arousal to engorge the clitoris before/during use.

**The V-Contour dual-chamber system separates these functions:**

| Chamber | Valve Control | Capability |
|---------|---------------|------------|
| **Outer V-seal** | SOL1/SOL2 | Sustained vacuum for vulva/labia engorgement |
| **Clitoral cylinder** | SOL4/SOL5 | Sustained vacuum for clitoral engorgement **OR** oscillating air-pulse for stimulation |

**This allows the V-Contour to actively induce clitoral erection/engorgement, rather than waiting for natural arousal to occur.**

#### Active Engorgement Benefits:

1. **Faster arousal**: Engorge the clitoris BEFORE beginning air-pulse stimulation
2. **Maintained engorgement**: Sustained outer vacuum keeps tissue engorged throughout the session
3. **Faster orgasm**: Optimal clitoral erection from the start reduces time to climax
4. **Enhanced sensitivity**: Engorged tissue has ~8,000 nerve endings more exposed
5. **Consistent response**: Does not rely on the user's natural arousal state
6. **Dual-mode flexibility**: Can switch between engorgement and stimulation dynamically

### 1.4 Open-Channel Design Benefits

The V-shape creates an **open channel** by sealing around the periphery of the vulva while leaving the urethra and vaginal opening completely exposed between the two arms of the cup.

```
┌─────────────────────────────────────────────────────────────────┐
│              OPEN-CHANNEL CROSS-SECTION (Side View)             │
│                                                                 │
│                    Clitoral Cylinder                            │
│                    (Air-Pulse Zone)                             │
│                         │                                       │
│                         ▼                                       │
│     V-Cup Outer Wall ╭─────╮                                    │
│            │         │~~~~~│ ← Clitoral Hood Position           │
│            ▼         │~~~~~│                                    │
│         ╭────────────┴─────┴────────────╮                       │
│        ╱          ╲           ╱          ╲                      │
│       │  Vacuum    │ Urethra │   Vacuum   │                     │
│       │  Chamber   │ Vaginal │  Chamber   │                     │
│       │  (sealed)  │ Opening │  (sealed)  │                     │
│        ╲          ╱           ╲          ╱                      │
│     ═════════════════════════════════════════  ← Body Surface   │
│     OPEN CHANNEL Unobstructed access to urethra/vagina          │
│     Fluids drain away from vacuum system                        │
└─────────────────────────────────────────────────────────────────┘
```

**Benefits:**

1. **Hygiene**: Fluids (urine, vaginal lubrication, ejaculate) drain away from vacuum system
2. **Squirting-Compatible**: Female ejaculation doesn't contaminate vacuum lines
3. **Urethral Access**: Enables simultaneous urethral sounding while vacuum active
4. **Vaginal Access**: Allows insertable toys during vacuum therapy
5. **Easy Cleanup**: Vacuum hoses remain dry; only cup exterior needs cleaning

---

## 2. Technical Specifications for Dual-Chamber Implementation

### 2.1 Additional Hardware Components

To upgrade from single-chamber to dual-chamber operation, the following components are required:

| Component | Quantity | Purpose |
|-----------|----------|---------|
| **SOL4 (Solenoid Valve)** | 1 | Clitoral chamber vacuum control |
| **SOL5 (Solenoid Valve)** | 1 | Clitoral chamber vent (for air-pulse release) |
| **MPX5010DP Sensor** | 1 | Clitoral chamber pressure monitoring |
| **Vacuum Line (6mm ID)** | 1 | Clitoral chamber supply line |
| **Y-Splitter** | 1 | Connects clitoral chamber to tank |

### 2.2 Updated GPIO Pin Assignments

| Pin | Function | Chamber |
|-----|----------|---------|
| GPIO 17 | SOL1 (Vacuum to V-seal) | Outer |
| GPIO 27 | SOL2 (V-seal vent) | Outer |
| GPIO 22 | SOL3 (Tank vent) | Shared |
| GPIO 25 | Pump enable | Shared |
| GPIO 18 | Pump PWM | Shared |
| GPIO 21 | Emergency stop button | Shared |
| **GPIO 23** | **SOL4 (Vacuum to clitoral)** | **Inner** |
| **GPIO 24** | **SOL5 (Clitoral vent)** | **Inner** |

### 2.3 Updated Vacuum System Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    DUAL-CHAMBER VACUUM SYSTEM                               │
│                                                                             │
│   ┌─────────┐    ┌─────────┐                                                │
│   │ DC Pump │───►│  TANK   │◄─── Sensor 2 (Tank)                            │
│   │ (PWM)   │    │         │                                                │
│   └─────────┘    └────┬────┘                                                │
│                       │                                                     │
│              ┌────────┴────────┐                                            │
│              │                 │                                            │
│          [SOL3]            ┌───┴───┐                                        │
│          Tank              │       │                                        │
│          Vent          [SOL1]   [SOL4]                                      │
│              ▼         Outer    Clitoral                                    │
│          ┌─────┐           │       │                                        │
│          │ ATM │           ▼       ▼                                        │
│          └─────┘   ┌───────────────────────┐                                │
│                    │     V-CONTOUR CUP     │                                │
│                    │   OUTER V-CHAMBER   ◄─┼─── Sensor 1 (AVL)              │
│                    │  ┌─────────────────┐  │                                │
│                    │  │   ┌─────────┐   │  │                                │
│                    │  │   │CLITORAL │◄──┼──┼─── Sensor 3 (Clitoral)         │
│                    │  │   │CYLINDER │   │  │                                │
│                    │  │   └┬───────┬┘   │  │       ┌─────┐                  │
│                    │  │    │ OPEN  │    │  │   ┌───│ ATM │                  │
│                    │  │    │CHANNEL│    │  │   │   └─────┘                  │
│                    │  │    │       │    │  │ [SOL5]                         │
│                    │  │    │       │    │  │ Clitoral                       │
│                    │  │    │       │    │  │ Vent                           │
│                    │  │    │       │    │  │   │                            │
│                    │  └────┤       ├────┘◄─┼───┘                            │
│                    │       │       │       │                                │
│                    │       │       │       │                                │
│                    └───────┘       └───────┘                                │
│                        ▲                                                    │
│                        │   ┌─────┐                                          │
│                    [SOL2]──│ ATM │                                          │
│                    Outer   └─────┘                                          │
│                    Vent                                                     │
└─────────────────────────────────────────────────────────────────────────────┘
```




### 2.4 Pressure Specifications by Chamber

| Parameter | Outer V-Seal Chamber | Clitoral Cylinder |
|-----------|---------------------|-------------------|
| **Purpose** | Attachment + engorgement | Air-pulse stimulation |
| **Operating Range** | 30-60 mmHg | 20-75 mmHg |
| **Attachment Threshold** | 35 mmHg minimum | N/A |
| **Warning Threshold** | 65 mmHg | 70 mmHg |
| **Maximum Safe Limit** | 75 mmHg | 80 mmHg |
| **Typical Operating** | 40-55 mmHg (constant) | 30-70 mmHg (pulsing) |

### 2.5 Air-Pulse Timing Specifications

The clitoral cylinder operates in **pulsing mode** to replicate air-pulse toy sensations:

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Frequency Range** | 1-20 Hz | User-adjustable |
| **Default Frequency** | 8 Hz | Optimal for most users |
| **Pulse Width** | 25-75 ms | Time at peak vacuum |
| **Duty Cycle** | 30-70% | Ratio of vacuum-on to cycle |
| **Peak Pressure** | 60-75 mmHg | During pulse "on" phase |
| **Trough Pressure** | 20-35 mmHg | During pulse "off" phase |
| **Ramp Time** | 5-15 ms | SOL4/SOL5 switching speed |

**Air-Pulse Waveform (8 Hz example):**
```
Pressure
(mmHg)
      │
   75 ┼────┐    ┌────┐    ┌────┐    ┌────┐
      │    │    │    │    │    │    │    │    ← Peak (SOL4 open, SOL5 closed)
   50 ┼    │    │    │    │    │    │    │
      │    │    │    │    │    │    │    │
   25 ┼    └────┘    └────┘    └────┘    └───
      │                                       ← Trough (SOL4 closed, SOL5 open)
    0 ┼──────────────────────────────────────► Time (ms)
      0   62  125  187  250  312  375  437
           │◄──────────►│
             125ms cycle
              (8 Hz)
```

### 2.6 Sensor Configuration

| Sensor | ADC Channel | Purpose | Update Rate |
|--------|-------------|---------|-------------|
| AVL (Outer) | CH0 | V-seal pressure, anti-detachment | 50 Hz |
| Tank | CH1 | Reservoir monitoring | 50 Hz |
| **Clitoral** | **CH2** | **Clitoral cylinder pressure** | **100 Hz** |

The clitoral sensor runs at **100 Hz** to accurately track rapid air-pulse transitions.

---

## 3. Software Architecture Changes

### 3.1 HardwareManager Updates

#### ActuatorControl Modifications

```cpp
// New valve definitions
enum ValveType {
    SOL1_OUTER_VACUUM,    // Outer V-seal vacuum
    SOL2_OUTER_VENT,      // Outer V-seal vent
    SOL3_TANK_VENT,       // Tank vent (safety)
    SOL4_CLITORAL_VACUUM, // Clitoral cylinder vacuum (NEW)
    SOL5_CLITORAL_VENT    // Clitoral cylinder vent (NEW)
};

// Dual-channel pressure control
void ActuatorControl::setOuterPressure(double targetMmHg);
void ActuatorControl::setClitoralPressure(double targetMmHg);
void ActuatorControl::startAirPulse(double frequencyHz, double dutyCycle);
void ActuatorControl::stopAirPulse();
```

#### SensorInterface Modifications

```cpp
// Triple-sensor support
double SensorInterface::readOuterPressure();   // V-seal chamber
double SensorInterface::readTankPressure();    // Tank reservoir
double SensorInterface::readClitoralPressure(); // Clitoral cylinder (NEW)

// High-speed clitoral monitoring (100 Hz)
void SensorInterface::enableHighSpeedClitoralMonitoring();
```

### 3.2 PatternEngine Updates

#### Dual-Zone Pattern Definitions

Patterns now specify independent targets for each chamber:

```cpp
struct DualZonePatternStep {
    double outerPressurePercent;     // V-seal chamber target (0-100%)
    double clitoralPressurePercent;  // Clitoral cylinder target (0-100%)
    int durationMs;                  // Step duration
    QString outerAction;             // e.g., "maintain_seal"
    QString clitoralAction;          // e.g., "air_pulse_8hz"
    double airPulseFrequency;        // Hz (0 = constant, >0 = pulsing)
    double airPulseDutyCycle;        // 0.0-1.0
};
```

#### New Pattern Types

| Pattern | Outer Chamber | Clitoral Cylinder |
|---------|--------------|-------------------|
| **Attachment Only** | 45% constant | 0% (off) |
| **Gentle Pulse** | 45% constant | 40% @ 4 Hz |
| **Intense Pulse** | 50% constant | 70% @ 12 Hz |
| **Wave + Pulse** | 40-55% wave | 50-70% @ 8 Hz |
| **Edging Mode** | 50% constant | Builds to 75%, stops before climax |
| **Automated Dual-Orgasm** | Phase-matched | Air-pulse intensifies with arousal |

#### Air-Pulse Pattern Generation

```cpp
void PatternEngine::buildAirPulsePattern(double frequency, double dutyCycle) {
    // Calculate timing
    double periodMs = 1000.0 / frequency;
    double onTimeMs = periodMs * dutyCycle;
    double offTimeMs = periodMs - onTimeMs;

    // Generate pulse steps
    for (int cycle = 0; cycle < totalCycles; ++cycle) {
        // Vacuum ON (SOL4 open, SOL5 closed)
        m_clitoralSteps.append(ClitoralStep(peakPressure, onTimeMs, "pulse_on"));
        // Vent (SOL4 closed, SOL5 open)
        m_clitoralSteps.append(ClitoralStep(troughPressure, offTimeMs, "pulse_off"));
    }
}
```

#### Edging Mode Implementation

Using sensors to detect impending orgasm and pause air pulses for "milking" or "edging" effect:

```cpp
void PatternEngine::executeEdgingMode() {
    // Outer chamber: Maintain constant engorgement
    m_actuators->setOuterPressure(50.0);  // Keep vulva engorged

    // Clitoral chamber: Build intensity then pause before climax
    while (m_edgingActive) {
        // Build phase: Increase air-pulse intensity
        for (int i = 0; i < buildupSteps; ++i) {
            double intensity = 40.0 + (i * 5.0);  // 40% → 75%
            m_actuators->setClitoralPressure(intensity);

            // Check for impending orgasm via pressure/timing analysis
            if (detectApproachingClimax()) {
                // PAUSE: Stop clitoral stimulation, maintain outer seal
                m_actuators->stopAirPulse();
                wait(recoveryPeriodMs);  // Let arousal subside
                break;  // Restart build cycle
            }
        }
    }
}
```

### 3.3 AntiDetachmentMonitor Updates

The anti-detachment system monitors **only the outer V-seal chamber**. Clitoral cylinder pressure variations are expected during air-pulse operation.

```cpp
void AntiDetachmentMonitor::checkSealIntegrity() {
    // ONLY monitor outer chamber for detachment
    double outerPressure = m_sensors->readOuterPressure();

    // Ignore clitoral chamber - it fluctuates during air-pulse
    // double clitoralPressure = m_sensors->readClitoralPressure(); // NOT USED

    if (outerPressure < m_detachmentThreshold) {
        // Seal breaking detected - increase outer vacuum only
        emit sealWarning(outerPressure);
        correctOuterVacuum();
    }
}
```

### 3.4 SafetyManager Updates

New safety rules for dual-chamber operation:

| Rule | Outer Chamber | Clitoral Cylinder |
|------|--------------|-------------------|
| **Max Pressure** | 75 mmHg | 80 mmHg |
| **Overpressure Action** | Open SOL2 (vent) | Open SOL5 (vent) |
| **Sensor Timeout** | Emergency stop all | Stop clitoral only |
| **Emergency Stop** | Vent both chambers | Vent both chambers |
| **Detachment Response** | Increase SOL1 | No action (independent) |

```cpp
void SafetyManager::checkDualChamberSafety() {
    double outer = m_sensors->readOuterPressure();
    double clitoral = m_sensors->readClitoralPressure();

    // Independent overpressure checks
    if (outer > OUTER_MAX_PRESSURE) {
        m_actuators->openValve(SOL2_OUTER_VENT);
        emit outerOverpressure(outer);
    }

    if (clitoral > CLITORAL_MAX_PRESSURE) {
        m_actuators->openValve(SOL5_CLITORAL_VENT);
        emit clitoralOverpressure(clitoral);
    }
}
```

### 3.5 GUI Updates

New controls for dual-chamber operation:

- **Dual Pressure Display**: Two gauges showing outer and clitoral pressure
- **Air-Pulse Frequency Slider**: 1-20 Hz adjustment
- **Duty Cycle Slider**: 30-70% adjustment
- **Chamber Enable Toggles**: Independent on/off for each chamber
- **Pattern Mode Selector**: Combined patterns with dual-zone visualization
- **Edging Mode Toggle**: Enable/disable orgasm detection and pause




---

## 4. Product Naming and Marketing Summary

### 4.1 Recommended Product Name

**Primary Name:** `V-Contour Dual-Therapy System`

**Alternative Names:**
| Name | Best For |
|------|----------|
| V-Station | Emphasizes the console/tech aspect |
| V-Pulse | Emphasizes the air-pulse feature |
| Anatomical V-Seal | Emphasizes the anatomical fit |
| Dry-Seal Pulse System | Emphasizes hygiene + air-pulse |
| The Lotus V | Boutique/sensual branding |

### 4.2 Key Technical Selling Points

| Feature | Technical Description | User Benefit |
|---------|----------------------|--------------|
| **Open-V Architecture** | Peripheral seal with central open channel | Hygiene, fluid drainage, simultaneous access |
| **Dual-Zone Technology** | Independent vacuum chambers with separate controls | Attachment + targeted stimulation |
| **Air-Pulse Engine** | 1-20 Hz rapid vacuum oscillation in clitoral cylinder | Replicates oral stimulation sensation |
| **Bio-Feedback System** | Triple pressure sensors with 100 Hz clitoral monitoring | Precise, responsive pressure control |
| **Anti-Detachment** | 100 Hz seal monitoring with 25ms response time | Maintains seal through body movements |
| **Edging Mode** | Detects impending climax, pauses stimulation | Prolonged arousal, intensified release |
| **Smart Safety** | Independent overpressure protection per chamber | Hard-coded safety limits |
| **Squirting-Compatible** | Open-channel drains fluids away from vacuum | Worry-free climax |
| **Access-Enabled** | Urethra/vagina unobstructed during operation | Simultaneous sounding/insertion |

### 4.3 Target Market Tags

- Squirting Friendly
- Urethral Play Compatible
- Open-Access Design
- Hygienic/Easy Clean
- Hands-Free Operation
- Programmable Patterns
- Medical-Grade Safety
- Edging/Milking Capable

### 4.4 Technical Specifications Summary

| Specification | Value |
|---------------|-------|
| **Platform** | Raspberry Pi 4 (8GB RAM) |
| **Display** | 50-inch HDMI touch display |
| **Framework** | Qt 5, C++17 |
| **Vacuum Chambers** | 2 (Outer V-seal + Clitoral cylinder) |
| **Solenoid Valves** | 5 (SOL1-SOL5) |
| **Pressure Sensors** | 3 (Outer, Tank, Clitoral) |
| **Air-Pulse Frequency** | 1-20 Hz |
| **Max Outer Pressure** | 75 mmHg |
| **Max Clitoral Pressure** | 80 mmHg |
| **Safety Monitoring** | 100 Hz |
| **Clitoral Sensor Rate** | 100 Hz |
| **PWM Motor Driver** | 5000 Hz for smooth pressure modulation |

### 4.5 Pattern Modes Summary

| Mode | Description |
|------|-------------|
| **Bio-Mimetic Nursing** | Rhythmic tugging pattern for blood flow stimulation |
| **Sensory Tease (Edging)** | Builds to brink, automatically backs off |
| **The Wave** | Rolling tidal pressure modulation |
| **Air Pulse** | Rapid-fire 1-20 Hz clitoral stimulation |
| **Plateau Mode** | Maintains precise pressure at peak sensitivity |
| **Automated Orgasm** | Full arousal-to-climax 5-minute sequence |
| **Continuous Marathon** | Infinite 4-minute orgasm loops |

---

## Summary: Evolution from Single to Dual-Chamber

This document describes the evolution from the current **single-chamber vacuum system** to an advanced **dual-chamber V-Contour system**:

| Aspect | Current (Single) | Proposed (Dual) |
|--------|-----------------|-----------------|
| **Vacuum Zones** | 1 (AVL only) | 2 (Outer V-seal + Clitoral) |
| **Valves** | 3 (SOL1-SOL3) | 5 (SOL1-SOL5) |
| **Sensors** | 2 (AVL, Tank) | 3 (Outer, Tank, Clitoral) |
| **Stimulation** | Uniform suction | Targeted air-pulse + **active engorgement** |
| **Cup Design** | Dome | Anatomical V-shape |
| **Fluid Handling** | Trapped in cup | Drained via open channel |
| **Access** | Blocked | Urethra/vagina unobstructed |
| **Engorgement** | Passive (relies on natural arousal) | **Active (sustained vacuum induces engorgement)** |

### Key Advantage Over Commercial Air-Pulse Toys

> **Commercial toys (Womanizer, Satisfyer, etc.) only oscillate—they cannot sustain vacuum for tissue engorgement.**

The V-Contour's unique dual-chamber design provides:
- **Sustained vacuum** in outer chamber for vulva/labia engorgement
- **Sustained OR oscillating vacuum** in clitoral cylinder
- **Active clitoral erection** before stimulation begins
- **Faster time to orgasm** by eliminating the arousal delay

**The goal:** Combine the attachment/engorgement capabilities of vacuum therapy with the targeted, intense clitoral stimulation of air-pulse toys—at industrial scale, with full automation, active engorgement, and safety.

---

**End of Dual-Chamber V-Contour Design Document**
