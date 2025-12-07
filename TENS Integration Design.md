# TENS Integration Design for V-Contour Clitoral Cup

## Overview

This document details the integration of TENS (Transcutaneous Electrical Nerve Stimulation) electrodes directly into the V-Contour dual-chamber vacuum system's clitoral cup. The design combines oscillating pressure wave stimulation (5-13 Hz) with electrical nerve stimulation (20 Hz) for synergistic arousal enhancement.

---

## 1. Mechanical Design

### 1.1 Clitoral Cup Electrode Configuration

```
                    TOP VIEW (Cup Interior)
                    ═══════════════════════
                    
                         ┌─────────┐
                        /           \
                       /   VACUUM    \
                      /    CHANNEL    \
                     │                 │
            ┌────────┤                 ├────────┐
            │ LEFT   │     TARGET     │  RIGHT │
            │ ELEC.  │    CLITORIS    │  ELEC. │
            │ (-)    │      AREA      │   (+)  │
            └────────┤                 ├────────┘
                     │                 │
                      \     SEAL      /
                       \   RIDGE    /
                        \         /
                         └───────┘
                         
                    Diameter: 15-20mm
```

### 1.2 Electrode Specifications

| Parameter | Specification | Rationale |
|-----------|--------------|-----------|
| **Material** | Medical-grade 316L stainless steel with gold plating | Biocompatible, corrosion-resistant, low impedance |
| **Alternative** | Conductive silicone (carbon-loaded) | Flexible, softer contact for comfort |
| **Shape** | Curved crescents (8mm × 4mm) | Conforms to cup interior; lateral clitoral placement |
| **Spacing** | 10-15mm apart | Optimal for dorsal genital nerve stimulation |
| **Mounting** | Embedded flush with cup interior | No protrusion; maintains vacuum seal integrity |
| **Surface** | Smooth, polished (Ra < 0.8 μm) | Comfortable skin contact; easy cleaning |

### 1.3 Vacuum Seal Integration

```
                    CROSS-SECTION VIEW
                    ═══════════════════
                    
        VACUUM PORT ──►  ┌────────────────┐
                         │    AIR SPACE   │
        ┌────────────────┼────────────────┼────────────────┐
        │  SILICONE      │                │     SILICONE   │
        │  SEAL RIDGE ───┼────────────────┼─── SEAL RIDGE  │
        ├────────────────┤                ├────────────────┤
        │  ELECTRODE (-)  │   TISSUE     │  ELECTRODE (+)  │
        │  ▓▓▓▓▓▓▓▓▓▓▓▓▓ │               │ ▓▓▓▓▓▓▓▓▓▓▓▓▓  │
        │                │               │                 │
        │   WIRE ROUTING │               │   WIRE ROUTING  │
        │   (embedded)   │               │   (embedded)    │
        └────────────────┴───────────────┴─────────────────┘
                              ▲
                         CUP WALL (rigid plastic/silicone)
```

### 1.4 Wire Routing

- **Embedded channels**: Wires routed through molded channels in cup wall
- **Strain relief**: Flexible section at cup exit point
- **Connector**: Medical-grade 2.5mm snap connector (like ECG leads)
- **Seal integrity**: Potting compound (medical-grade epoxy) at wire entry points

---

## 2. Electrical Design

### 2.1 TENS Waveform Generator Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Waveform Type** | Biphasic symmetric square | Zero net DC; prevents tissue damage |
| **Frequency Range** | 1-100 Hz | Default: 20 Hz (clinical standard) |
| **Pulse Width Range** | 50-500 μs | Default: 400 μs |
| **Amplitude Range** | 0-80 mA peak | Adjustable; sensory threshold ~10-30 mA |
| **Output Isolation** | >1500V galvanic | Medical safety standard |
| **Compliance Voltage** | ±60V | Sufficient for 1kΩ skin impedance |

### 2.2 Biphasic Waveform Diagram

```
     CURRENT (mA)
         ▲
    +80 ─┤    ┌───┐           ┌───┐           ┌───┐
         │    │   │           │   │           │   │
      0 ─┼────┘   └───────────┘   └───────────┘   └────► TIME
         │            │               │
    -80 ─┤            └───┐           └───┐
         │                │               │
                     ◄──────────────────────►
                          50ms (20 Hz)
                          
              ◄───────►
              400μs pulse width
```

### 2.3 GPIO Pin Assignments

| GPIO | Function | Description |
|------|----------|-------------|
| **GPIO 5** | TENS_ENABLE | Master enable for TENS output stage |
| **GPIO 6** | TENS_PHASE | Polarity control (HIGH = positive, LOW = negative) |
| **GPIO 12** | TENS_PWM | Amplitude control via PWM (hardware PWM channel) |
| **GPIO 16** | TENS_FAULT | Input: Overcurrent/fault detection from driver |

### 2.4 Hardware Block Diagram

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                     RASPBERRY PI                                 │
    │  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐  │
    │  │ GPIO 5   │    │ GPIO 6   │    │ GPIO 12  │    │ GPIO 16  │  │
    │  │ (ENABLE) │    │ (PHASE)  │    │ (PWM)    │    │ (FAULT)  │  │
    │  └────┬─────┘    └────┬─────┘    └────┬─────┘    └────▲─────┘  │
    └───────┼───────────────┼───────────────┼───────────────┼─────────┘
            │               │               │               │
            ▼               ▼               ▼               │
    ┌───────────────────────────────────────────────────────┼─────────┐
    │                 TENS DRIVER BOARD                      │         │
    │  ┌─────────┐   ┌─────────────┐   ┌─────────────────┐  │         │
    │  │  OPTO-  │   │  H-BRIDGE   │   │   ISOLATED DC   │  │ FAULT   │
    │  │ISOLATOR │──►│  DRIVER     │──►│   POWER SUPPLY  │  │ DETECT  │
    │  │ (IL716) │   │  (DRV8871)  │   │   (±60V, 100mA) │◄─┼─────────┤
    │  └─────────┘   └──────┬──────┘   └─────────────────┘  │         │
    │                       │                                │         │
    │                       ▼                                │         │
    │               ┌───────────────┐                        │         │
    │               │  CURRENT SENSE │                       │         │
    │               │  RESISTOR (1Ω) │───────────────────────┘         │
    │               └───────┬───────┘                                  │
    └───────────────────────┼──────────────────────────────────────────┘
                            │
                            ▼
                    ┌───────────────┐
                    │   ELECTRODES   │
                    │  (+)     (-)   │
                    └───────────────┘
```

---

## 3. Safety Design

### 3.1 Safety Interlocks

| Interlock | Condition | Action |
|-----------|-----------|--------|
| **Emergency Stop** | Emergency button pressed | Disable TENS + all valves + pump |
| **Seal Integrity** | Vacuum < 10 mmHg (seal broken) | Disable TENS immediately |
| **Overcurrent** | Current > 85 mA for > 100ms | Disable TENS, set fault flag |
| **Open Circuit** | Impedance > 10 kΩ | Reduce amplitude, warn user |
| **Thermal** | Driver temp > 60°C | Reduce duty cycle or disable |

### 3.2 Hardware Safety Features

1. **Galvanic Isolation**: 1500V isolation between Pi and output stage
2. **Current Limiting**: Hardware current limit at 85 mA (fuse-backed)
3. **Charge Balance**: Biphasic waveform ensures zero net DC current
4. **Soft Start**: Amplitude ramps from 0 to target over 500ms
5. **Auto-Shutoff**: If no control signal for 2 seconds, output disables

### 3.3 Software Safety Checks

```cpp
// Safety check before enabling TENS
bool TENSController::canEnable() const {
    // Check emergency stop
    if (m_hardware->isEmergencyStop()) return false;
    
    // Check vacuum seal integrity (clitoral cup must be sealed)
    double clitoralPressure = m_hardware->readClitoralPressure();
    if (clitoralPressure < MIN_SEAL_PRESSURE_MMHG) return false;  // 10 mmHg
    
    // Check electrode impedance (if measurable)
    if (m_electrodeImpedance > MAX_IMPEDANCE_OHMS) return false;  // 10kΩ
    
    // Check driver fault status
    if (m_faultDetected) return false;
    
    return true;
}
```

---

## 4. Software Architecture

### 4.1 Class Hierarchy

```
HardwareManager
├── ActuatorControl (valves, pump)
├── SensorInterface (pressure sensors)
├── ClitoralOscillator (air-pulse control)
└── TENSController (NEW - electrical stimulation)
        ├── setFrequency(Hz)
        ├── setPulseWidth(μs)
        ├── setAmplitude(mA)
        ├── start() / stop()
        └── Emergency stop integration

PatternEngine
├── PatternDefinitions (includes new TENS+Vacuum patterns)
└── Coordinated execution of vacuum + TENS
```

### 4.2 TENSController Interface

```cpp
class TENSController : public QObject {
    Q_OBJECT
public:
    enum class Waveform { BIPHASIC_SYMMETRIC, BIPHASIC_ASYMMETRIC, BURST };

    // Configuration
    void setFrequency(double hz);         // 1-100 Hz
    void setPulseWidth(int microseconds); // 50-500 μs
    void setAmplitude(double milliamps);  // 0-80 mA
    void setWaveform(Waveform type);

    // Control
    void start();
    void stop();
    void emergencyStop();
    bool isRunning() const;

    // Presets (from research)
    void setPresetArousal();   // 20 Hz, 400 μs, medium amplitude
    void setPresetClimax();    // 30 Hz, 300 μs, higher amplitude
    void setPresetAfterGlow(); // 10 Hz, 500 μs, low amplitude

    // Status
    double getCurrentAmplitude() const;
    double getElectrodeImpedance() const;
    bool isFaultDetected() const;

signals:
    void stimulationStarted();
    void stimulationStopped();
    void amplitudeChanged(double mA);
    void faultDetected(const QString& reason);
    void electrodeContact(bool good);
};
```

### 4.3 Combined Pattern Step Structure

```cpp
struct CombinedPatternStep {
    // Vacuum parameters (existing)
    double vacuumPressurePercent;  // 0-100%
    int vacuumDurationMs;

    // Clitoral oscillation parameters (existing)
    bool enableOscillation;
    double oscillationFrequencyHz;  // 5-13 Hz
    double oscillationAmplitude;

    // TENS parameters (NEW)
    bool enableTENS;
    double tensFrequencyHz;         // 1-100 Hz (typically 20 Hz)
    int tensPulseWidthUs;           // 50-500 μs (typically 400 μs)
    double tensAmplitudePercent;    // 0-100% of max (80 mA)

    // Coordination
    TENSPhaseSync tensSync;  // CONTINUOUS, SYNC_SUCTION, SYNC_VENT
};
```

---

## 5. Coordination Strategies

### 5.1 TENS + Vacuum Oscillation Timing

Three coordination modes:

#### Mode 1: Continuous TENS (Default)
```
TENS:      ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
           20 Hz continuous throughout oscillation

VACUUM:    ███░░░███░░░███░░░███░░░███░░░███░░░
           SUCTION  VENT   (10 Hz oscillation)
```

#### Mode 2: Synced with Suction Phase
```
TENS:      ▓▓▓▓░░░▓▓▓▓░░░▓▓▓▓░░░▓▓▓▓░░░▓▓▓▓░░░▓▓▓▓
           TENS active only during suction (engorgement)

VACUUM:    ███░░░███░░░███░░░███░░░███░░░███░░░
           SUCTION  VENT   (10 Hz oscillation)
```

#### Mode 3: Alternating (Contrast Stimulation)
```
TENS:      ░░░░▓▓▓▓░░░░▓▓▓▓░░░░▓▓▓▓░░░░▓▓▓▓░░░░▓▓▓▓
           TENS active during VENT phase (contrast effect)

VACUUM:    ███░░░███░░░███░░░███░░░███░░░███░░░
           SUCTION  VENT   (10 Hz oscillation)
```

### 5.2 Arousal Progression Pattern

```
PHASE 1 - Warmup (0-2 min)
├── Vacuum: 30% pressure, sustained
├── Oscillation: OFF
└── TENS: 10 Hz, 200 μs, 20% amplitude

PHASE 2 - Build-up (2-8 min)
├── Vacuum: 50% pressure, sustained
├── Oscillation: 8 Hz, medium amplitude
└── TENS: 20 Hz, 400 μs, 40% amplitude, CONTINUOUS

PHASE 3 - Escalation (8-12 min)
├── Vacuum: 70% pressure, sustained
├── Oscillation: 10 Hz, high amplitude
└── TENS: 25 Hz, 400 μs, 60% amplitude, SYNC_SUCTION

PHASE 4 - Climax (12-15 min)
├── Vacuum: 80% pressure, sustained
├── Oscillation: 12 Hz, maximum amplitude
└── TENS: 30 Hz, 300 μs, 80% amplitude, CONTINUOUS

PHASE 5 - Afterglow (15-20 min)
├── Vacuum: 30% pressure, gentle
├── Oscillation: 5 Hz, low amplitude
└── TENS: 10 Hz, 500 μs, 20% amplitude
```

---

## 6. Component Bill of Materials

### 6.1 TENS Driver Components

| Component | Part Number | Description | Qty |
|-----------|-------------|-------------|-----|
| Optocoupler | IL716 | 2500V isolation, dual channel | 2 |
| H-Bridge Driver | DRV8871 | 3.6A, PWM control, current sense | 1 |
| Current Sense Resistor | - | 1Ω, 1%, 0.5W | 1 |
| Isolated DC-DC | RECOM RH-1515D | ±15V, 1W, 3kV isolation | 1 |
| Boost Converter | TPS61088 | 12V to 60V step-up | 1 |
| Fault Comparator | LM339 | Overcurrent detection | 1 |
| Protection TVS | SMBJ60CA | 60V bidirectional TVS | 2 |
| Fuse | - | 100mA slow-blow, SMD | 1 |

### 6.2 Electrode Assembly

| Component | Specification | Qty |
|-----------|---------------|-----|
| Electrode Material | 316L SS with gold plating | 2 |
| Silicone Overmold | Medical-grade, Shore A 40 | 1 |
| Lead Wire | 26 AWG silicone insulated | 2 |
| Connector | 2.5mm snap (ECG style) | 1 pair |
| Potting Compound | Medical epoxy (EP30MED) | 5g |

---

## 7. Regulatory Considerations

### 7.1 Applicable Standards

| Standard | Description | Relevance |
|----------|-------------|-----------|
| IEC 60601-1 | Medical electrical equipment - General safety | Core safety requirements |
| IEC 60601-2-10 | Nerve/muscle stimulators | TENS-specific requirements |
| IEC 62353 | Medical electrical equipment testing | Recurrent testing |
| ISO 10993 | Biological evaluation of medical devices | Electrode biocompatibility |

### 7.2 Key Compliance Points

1. **Patient leakage current**: < 100 μA normal, < 500 μA single fault
2. **Output characteristics**: Charge per pulse < 25 μC; charge density < 25 μC/cm²
3. **Electrode area**: Minimum 1 cm² per electrode (current density < 2 mA/cm²)
4. **Applied part classification**: Type BF (body floating)

---

## 8. Testing Requirements

### 8.1 Hardware Verification

- [ ] Waveform accuracy: ±5% frequency, ±10% amplitude
- [ ] Galvanic isolation: >1500V dielectric withstand
- [ ] Current limiting: Verified at 85 mA trip point
- [ ] Emergency stop: <50ms disable time
- [ ] Electrode impedance range: 100Ω - 10kΩ operational

### 8.2 Safety Validation

- [ ] Seal detection interlock functional
- [ ] Overcurrent protection functional
- [ ] Soft-start verified (no current spike on enable)
- [ ] Charge balance verified (net DC < 1 μA)
- [ ] Thermal shutdown functional

### 8.3 Functional Testing

- [ ] TENS + vacuum oscillation coordination
- [ ] Pattern execution with combined stimulation
- [ ] User amplitude control responsive
- [ ] Emergency stop disables all outputs

