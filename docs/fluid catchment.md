# V-Contour Fluid Collection and Measurement System

## 1. Overview

The fluid catchment system attaches to the V-Contour vacuum cup to measure and track female sexual fluids produced during stimulation sessions. This document covers sensor selection, implementation design, and correlation with arousal data.

### 1.1 Fluid Types to Measure

| Fluid Type | Source | Typical Volume | Production Rate | Trigger |
|------------|--------|----------------|-----------------|---------|
| **Arousal Lubrication** | Bartholin's glands, vaginal transudation | 0.5-5 mL/session | Continuous during arousal | Baseline → Plateau arousal |
| **Pre-Orgasmic Fluid** | Skene's glands (paraurethral) | 0.1-2 mL | Burst at high arousal | Pre-orgasm phase |
| **Orgasmic Fluid** | Skene's glands, bladder (diluted urine for squirting) | 0-50+ mL | Rapid burst during orgasm | Orgasm event |
| **Total Session Fluid** | Combined above | 1-100+ mL | Variable | Full session |

### 1.2 Measurement Requirements

- **Resolution**: ±0.5 mL minimum (detect subtle lubrication changes)
- **Range**: 0-150 mL (accommodate squirting events)
- **Response Time**: <500 ms (detect orgasmic fluid bursts)
- **Accuracy**: ±5% or ±1 mL (whichever is larger)
- **Compatibility**: Must work with vacuum system (no air leaks)
- **Hygiene**: Easy to clean/sterilize, body-safe materials

---

## 2. Sensor Technology Comparison

### 2.1 Recommended: Load Cell (Weight-Based)

**Principle**: Strain gauge measures mass of collection reservoir; convert to volume using fluid density (~1.0 g/mL for aqueous fluids).

| Specification | Recommended |
|---------------|-------------|
| Type | TAL220 or HX711-compatible load cell |
| Capacity | 200g-500g (0-200+ mL fluid) |
| Resolution | 0.01g (0.01 mL) |
| Interface | HX711 ADC → GPIO |
| Cost | $5-15 |

**Advantages**:
- Highest accuracy for cumulative measurement
- Works with any reservoir shape
- No contact with fluid (hygienic)
- Temperature-stable
- Simple calibration (tare empty, calibrate with known mass)

**Disadvantages**:
- Cannot detect flow rate directly (needs derivative)
- Vibration-sensitive (needs damping/filtering)
- Position-dependent (must be level)

**Wiring**:
```
Load Cell → HX711 ADC Board → Raspberry Pi
  E+ (Red)    → E+
  E- (Black)  → E-
  A- (White)  → A-
  A+ (Green)  → A+

HX711 → Raspberry Pi
  VCC → 3.3V
  GND → GND
  DT  → GPIO 26
  SCK → GPIO 19
```

### 2.2 Alternative: Capacitive Level Sensor

**Principle**: Measures capacitance change as fluid level rises in reservoir.

| Specification | Recommended |
|---------------|-------------|
| Type | Non-contact capacitive probe |
| Range | 0-100 mm (linear) |
| Resolution | 0.5 mm (~0.5 mL with proper reservoir geometry) |
| Interface | I2C or analog (0-3.3V) |
| Cost | $10-30 |

**Advantages**:
- Continuous level measurement
- No moving parts
- Fast response

**Disadvantages**:
- Requires consistent reservoir geometry
- Affected by fluid conductivity/dielectric
- More complex calibration
- May detect foam/bubbles incorrectly

### 2.3 Alternative: Optical Level Sensor (IR)

**Principle**: Infrared emitter/detector measures reflection at fluid surface.

**Not recommended** for this application due to:
- Fluid opacity variations
- Foam interference
- Requires clear reservoir
- Lower accuracy

### 2.4 Alternative: Flow Rate Sensor

**Principle**: Measures volumetric flow through tube.

**Not recommended** as primary sensor because:
- Requires flow through restricted channel
- Cannot measure static accumulated volume
- Hygiene concerns with internal wetted parts
- May impede drainage

**Could be secondary sensor** for flow rate bursts during orgasm.

---

## 3. Recommended Hardware Design

### 3.1 System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     FLUID COLLECTION SYSTEM                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   V-Contour Cup                                                         │
│   ┌─────────────┐                                                       │
│   │  Vacuum     │    Drainage Port                                      │
│   │  Chamber    │──────────┐                                            │
│   │             │          │                                            │
│   │  (SOL1-5)   │          ▼                                            │
│   └─────────────┘    ┌───────────┐                                      │
│                      │  Silicone │                                      │
│                      │  Tube     │                                      │
│                      └─────┬─────┘                                      │
│                            │                                            │
│                            ▼                                            │
│   ┌──────────────────────────────────────────────────────────────────┐ │
│   │                    COLLECTION RESERVOIR                           │ │
│   │  ┌────────────────────────────────────────────────────────────┐  │ │
│   │  │   Medical-grade silicone or borosilicate container         │  │ │
│   │  │   150 mL capacity, graduated markings                      │  │ │
│   │  │                                                             │  │ │
│   │  │   ▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░  ← Fluid level     │  │ │
│   │  │                                                             │  │ │
│   │  └────────────────────────────────────────────────────────────┘  │ │
│   │                         │                                         │ │
│   │                         │                                         │ │
│   │  ┌──────────────────────▼────────────────────────────────────┐   │ │
│   │  │              LOAD CELL (TAL220 / 500g)                     │   │ │
│   │  │                                                            │   │ │
│   │  │   [████████████████████████████████████]  ← Strain gauge   │   │ │
│   │  │              E+    E-    A-    A+                          │   │ │
│   │  └──────────────┬─────┬─────┬─────┬──────────────────────────┘   │ │
│   └─────────────────┼─────┼─────┼─────┼──────────────────────────────┘ │
│                     │     │     │     │                                 │
│                     ▼     ▼     ▼     ▼                                 │
│   ┌──────────────────────────────────────────────────────────────────┐ │
│   │                    HX711 ADC                                      │ │
│   │                                                                   │ │
│   │   VCC─────────────────┬─────────── 3.3V (Raspberry Pi)           │ │
│   │   GND─────────────────┴─────────── GND                           │ │
│   │   DT ───────────────────────────── GPIO 26                       │ │
│   │   SCK ──────────────────────────── GPIO 19                       │ │
│   │                                                                   │ │
│   └──────────────────────────────────────────────────────────────────┘ │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Bill of Materials

| Component | Part Number | Quantity | Cost | Notes |
|-----------|-------------|----------|------|-------|
| Load Cell | TAL220 500g | 1 | $8 | 4-wire strain gauge |
| HX711 ADC | HX711 module | 1 | $3 | 24-bit ADC |
| Collection Container | Borosilicate beaker | 1 | $10 | 150 mL, graduated |
| Silicone Tubing | Medical grade 6mm ID | 1 m | $5 | Body-safe |
| Mounting Platform | 3D printed or aluminum | 1 | $5 | Vibration-isolated |
| Jumper Wires | Female-female | 4 | $1 | 20 cm |

**Total: ~$32**

### 3.3 GPIO Pin Assignments

| Signal | GPIO Pin | Description |
|--------|----------|-------------|
| HX711_DT | GPIO 26 | Data from HX711 |
| HX711_SCK | GPIO 19 | Clock to HX711 |

Note: GPIO 26 and 19 are available (not used by pressure sensors, TENS, or heartrate).

---

## 4. Software Architecture

### 4.1 Class Hierarchy

```
FluidSensor (QObject)
├── Sensor Types
│   ├── LOAD_CELL_HX711      (primary, recommended)
│   ├── CAPACITIVE_I2C       (alternative)
│   ├── CAPACITIVE_ANALOG    (alternative)
│   └── SIMULATED            (testing)
│
├── Measurements
│   ├── getCurrentVolumeMl()         // Current reservoir volume
│   ├── getCumulativeVolumeMl()      // Total session volume
│   ├── getFlowRateMlPerMin()        // Derivative-based rate
│   └── getInstantFlowRateMlPerSec() // Fast burst detection
│
├── Fluid Event Detection
│   ├── detectOrgasmicBurst()        // Large rapid increase
│   ├── getArousalLubricationRate()  // Slow steady increase
│   └── getLubricationScore()        // 0.0-1.0 normalized
│
├── Correlation Data
│   ├── correlateWithArousal(double arousalLevel)
│   └── correlateWithOrgasm(int orgasmNumber)
│
└── Calibration
    ├── tare()                        // Zero with empty container
    ├── calibrate(double knownMassG)  // Calibrate with known weight
    └── setDensity(double gPerMl)     // Fluid density (default 1.0)
```

### 4.2 Integration Points

```cpp
// HardwareManager integration
class HardwareManager {
    // Add fluid sensor
    std::unique_ptr<FluidSensor> m_fluidSensor;

public:
    FluidSensor* getFluidSensor() const { return m_fluidSensor.get(); }
    double readFluidVolumeMl();
    double readFluidFlowRate();
};

// OrgasmControlAlgorithm integration
class OrgasmControlAlgorithm {
    // Add fluid tracking
    FluidSensor* m_fluidSensor;
    double m_sessionFluidVolume;
    double m_lubricationBaseline;
    QVector<FluidEvent> m_fluidEvents;

public:
    void setFluidSensor(FluidSensor* sensor);
    double getSessionFluidVolume() const;
    double getLubricationRate() const;

Q_SIGNALS:
    void fluidOrgasmBurstDetected(double volumeMl, int orgasmNumber);
    void lubricationRateChanged(double mlPerMin);
};
```

---

## 5. Data Correlation Model

### 5.1 Arousal vs. Lubrication

```
Lubrication Rate (mL/min)
    ^
2.0 │                              ┌──────────
    │                         ┌────┘
1.5 │                    ┌────┘
    │               ┌────┘
1.0 │          ┌────┘
    │     ┌────┘
0.5 │┌────┘
    │
0.0 └────┬────┬────┬────┬────┬────┬────┬────► Arousal Level
         0   0.2  0.4  0.5  0.6  0.7  0.8  1.0
```

**Model**: `lubricationRate = baseRate + k * arousalLevel^2`

Where:
- `baseRate` = 0.1 mL/min (minimal at rest)
- `k` = 1.5 (empirically tuned)
- At arousal 0.8 (pre-orgasm): ~1.0 mL/min

### 5.2 Orgasm Fluid Burst

```
Volume (mL)
    ^
 30 │            ┌┐
    │           ┌┘└┐
 20 │          ┌┘  └┐
    │         ┌┘    └┐
 10 │        ┌┘      └────┐
    │       ┌┘            └────────
  0 └──────┴─────────────────────────► Time (s)
           t=0 (orgasm onset)

Detection: dV/dt > 5 mL/sec for > 0.5 sec
```

**Orgasm Intensity Correlation**:
- Weak orgasm: 0-5 mL burst
- Medium orgasm: 5-15 mL burst
- Strong orgasm: 15-30+ mL burst
- Squirting: 30-100+ mL burst

### 5.3 Session Tracking

| Metric | Calculation | Purpose |
|--------|-------------|---------|
| Total Volume | Cumulative sum | Overall session measure |
| Lubrication Volume | Slow accumulation (dV/dt < 0.5 mL/s) | Arousal response |
| Orgasmic Volume | Fast bursts (dV/dt > 2 mL/s) | Orgasm intensity |
| Arousal Ratio | Lubrication / Time | Responsiveness indicator |
| Orgasm Ratio | Orgasmic / Total | % from orgasms |

---

## 6. Safety and Calibration

### 6.1 Calibration Procedure

```
1. TARE (Zero):
   - Place empty, dry collection container on load cell
   - Call tare() to set zero reference
   - Expected: 0.0 mL reading

2. CALIBRATE (Scale Factor):
   - Add known volume of water (e.g., 50 mL from graduated cylinder)
   - Call calibrate(50.0)
   - System calculates: scaleFactor = 50.0 / rawDelta
   - Verify with second known volume (e.g., 100 mL)

3. DENSITY (Optional):
   - Default: 1.0 g/mL (water-like)
   - For more viscous fluids: setDensity(1.02)
   - Affects mass→volume conversion
```

### 6.2 Safety Features

| Feature | Threshold | Action |
|---------|-----------|--------|
| Overflow Warning | 120 mL | Alert user, flash GUI |
| Overflow Critical | 140 mL | Pause session, alarm |
| Negative Volume | < -1 mL | Recalibration needed warning |
| Sensor Disconnect | No signal for 5s | Sensor error alert |
| Rapid Drain | > -10 mL/s | Container removed warning |

### 6.3 Hygiene Reminders

- **Post-session**: Prompt to empty and rinse container
- **After 3 sessions**: Prompt for deep clean with medical-grade sanitizer
- **Monthly**: Remind to check tubing and seals for wear

---

## 7. GUI Design

### 7.1 Real-Time Display Panel

```
┌────────────────────────────────────────────────────────────────────────┐
│ FLUID COLLECTION                                              [⚙ Tare] │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  Current Volume           Flow Rate            Session Total           │
│  ┌──────────────┐        ┌──────────────┐     ┌──────────────┐        │
│  │    12.5      │        │     0.8      │     │    45.2      │        │
│  │     mL       │        │   mL/min     │     │     mL       │        │
│  └──────────────┘        └──────────────┘     └──────────────┘        │
│                                                                        │
│  Reservoir Level:                                                      │
│  ░░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  30%        │
│  0 mL                        75 mL                        150 mL       │
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────────┐ │
│  │ Volume over Time                                                  │ │
│  │ 50├                                          ●──●                 │ │
│  │   │                                     ●────┘                    │ │
│  │ 30├                              ●─────┘                          │ │
│  │   │                      ●──────┘                                 │ │
│  │ 10├         ●───●───●───┘                                         │ │
│  │   │    ●────┘                                                     │ │
│  │  0├────┘                                                          │ │
│  │   └──┬──────┬──────┬──────┬──────┬──────┬──────┬──────┬          │ │
│  │      0     5m     10m    15m    20m    25m    30m   Time          │ │
│  └──────────────────────────────────────────────────────────────────┘ │
│                                                                        │
│  Orgasm Bursts: ▓ 8.2 mL (O#1)   ▓ 12.4 mL (O#2)   ▓ 15.1 mL (O#3)  │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 8. Implementation Files

### 8.1 New Files to Create

| File | Purpose |
|------|---------|
| `src/hardware/FluidSensor.h` | Fluid sensor class definition |
| `src/hardware/FluidSensor.cpp` | Implementation |
| `src/gui/FluidMonitor.h` | GUI panel definition |
| `src/gui/FluidMonitor.cpp` | GUI implementation |

### 8.2 Files to Modify

| File | Changes |
|------|---------|
| `src/hardware/HardwareManager.h` | Add FluidSensor member |
| `src/hardware/HardwareManager.cpp` | Initialize/shutdown FluidSensor |
| `src/control/OrgasmControlAlgorithm.h` | Add fluid tracking |
| `src/control/OrgasmControlAlgorithm.cpp` | Integrate fluid correlation |
| `src/gui/MainWindow.cpp` | Add FluidMonitor panel |
| `CMakeLists.txt` | Add new source files |

---

## 9. Future Enhancements

1. **Machine Learning**: Train model to predict orgasm intensity from fluid patterns
2. **Personal Baselines**: Track individual user's lubrication response over sessions
3. **Pattern Optimization**: Adjust stimulation patterns based on fluid feedback
4. **Export/Analytics**: Session-over-session fluid data visualization
5. **Secondary Flow Sensor**: Add inline flow sensor for burst rate measurement

I need to add fluid collection, measurement, and tracking capabilities to the V-Contour system. Specifically:

1. **Hardware Integration**: I have a physical fluid catchment/collection system that attaches to the vacuum cup. I need to design the sensor interface and measurement system for it.

2. **Fluid Measurement Sensor**: Add support for measuring fluid volume expelled during sessions. This should track:
   - **Arousal lubrication**: Vaginal lubrication fluid produced during stimulation (baseline to plateau arousal)
   - **Orgasmic fluid**: Fluid expelled during orgasm events
   - **Total session fluid**: Cumulative fluid volume for the entire session

3. **Sensor Type Selection**: Help me determine the best sensor approach:
   - Weight/load cell sensor (measure mass, convert to volume)
   - Capacitive fluid level sensor
   - Optical/ultrasonic level sensor
   - Flow rate sensor
   - Other options?

4. **Software Implementation**: Create the necessary components:
   - `FluidSensor` class (similar to `HeartRateSensor`, `SensorInterface`)
   - Integration with `HardwareManager` for sensor reading
   - Integration with `OrgasmControlAlgorithm` to correlate fluid production with arousal levels and orgasm events
   - Data logging and tracking over time
   - GUI display in `MainWindow` or new panel to show real-time and cumulative fluid measurements

5. **Data Correlation**: Track relationships between:
   - Arousal level vs. lubrication rate
   - Orgasm intensity (from pressure/HR sensors) vs. fluid volume expelled
   - Stimulation patterns vs. fluid production
   - Session duration vs. total fluid volume

6. **Safety & Calibration**: Include calibration procedures, overflow detection, and cleaning/maintenance reminders.

Please provide:
- Recommended sensor hardware specifications
- `FluidSensor.h` and `FluidSensor.cpp` implementation
- Integration points with existing `OrgasmControlAlgorithm` and `HardwareManager`
- GUI components for display and data visualization
- Updates to CMakeLists.txt and relevant documentation