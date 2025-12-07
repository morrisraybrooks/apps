# EMC-Optimized Component Layout Design
## Medical Device Vacuum Controller Enclosure

### Layout Philosophy
Minimize electromagnetic interference through strategic component placement, shielding, and signal routing based on EMC best practices for medical devices.

## Enclosure Zones

### Zone 1: Analog/Sensor Section (EMI-Sensitive)
**Location**: Left side of enclosure, maximum distance from switching circuits
**Components**:
- MCP3008 ADC (primary component)
- 2x MPX5010DP Pressure Sensors
- Analog reference circuits
- Sensor signal conditioning

**Shielding**: Grounded metal partition (aluminum or steel)
**Power**: Clean analog power rail with extensive filtering

### Zone 2: Digital Processing Section (Medium EMI)
**Location**: Center of enclosure
**Components**:
- Raspberry Pi 4 (main controller)
- SPI interface circuits
- GPIO control circuits
- Emergency stop interface

**Isolation**: Physical separation from analog and power sections
**Grounding**: Separate digital ground plane

### Zone 3: Power/Switching Section (High EMI Source)
**Location**: Right side of enclosure, maximum distance from sensors
**Components**:
- PWM pump driver (L293D)
- Solenoid valve drivers (3x)
- Power supply circuits
- Heat sinks and cooling

**Containment**: Heavy shielding and filtering
**Ventilation**: Separate airflow path

## Detailed Component Placement

### Critical Spacing Requirements
```
Component Pair                    Minimum Distance    Reason
─────────────────────────────────────────────────────────────
PWM Pump ↔ MCP3008 ADC              8cm            Magnetic field isolation
PWM Pump ↔ Pressure Sensors         6cm            Prevent sensor coupling
Solenoids ↔ MCP3008 ADC            4cm            Reduce switching noise
SPI Lines ↔ Analog Sensors          3cm            Prevent digital coupling
Power Switching ↔ Analog Ref        5cm            Reference stability
Raspberry Pi ↔ Pressure Sensors     4cm            CPU noise isolation
```

### Optimal Component Coordinates (in 20cm x 15cm enclosure)
```
Component                X(cm)   Y(cm)   Z(cm)   Notes
─────────────────────────────────────────────────────────
MCP3008 ADC              2       7.5     2       Center of analog zone
MPX5010DP Sensor 1       1       10      1       AVL sensor, shielded
MPX5010DP Sensor 2       1       5       1       Tank sensor, shielded
Raspberry Pi 4           10      7.5     1       Center of digital zone
PWM Pump Driver          18      10      2       Far from sensors
SOL1 Driver              18      7.5     2       Solenoid control
SOL2 Driver              18      5       2       Solenoid control  
SOL3 Driver              18      2.5     2       Solenoid control
Power Supply             17      12      3       Switching supply
```

## Shielding Strategy

### Primary Shields
1. **Analog Section Shield**
   - Material: 1mm aluminum sheet
   - Coverage: Complete enclosure around MCP3008 and sensors
   - Grounding: Multiple points to chassis ground
   - Openings: Filtered feedthroughs for cables only

2. **PWM Pump Shield**
   - Material: Ferrite-loaded plastic or μ-metal
   - Purpose: Contain magnetic fields from motor switching
   - Design: Cylindrical shield around pump motor
   - Ventilation: Filtered air vents

3. **Solenoid Shields**
   - Material: Soft iron or μ-metal cups
   - Individual shields for each solenoid valve
   - Grounding: Connected to power ground

### Secondary Shields
- **SPI Cable Shield**: Grounded braid around SPI ribbon cable
- **Sensor Cable Shields**: Individual twisted pair with drain wire
- **Power Cable Ferrites**: Ferrite cores on all power connections

## Cable Routing

### High-Priority Signal Paths
```
Signal Type          Route                           Shielding
─────────────────────────────────────────────────────────────
Sensor Analog        Direct, shortest path          Twisted pair + shield
SPI Digital          Away from analog section       Shielded ribbon cable
PWM Power            Separate conduit               Ferrite cores
Solenoid Control     Bundled power routing          Ferrite + snubbers
Ground Returns       Star topology to chassis       Heavy gauge wire
```

### Cable Separation Rules
- **Analog ↔ Digital**: Minimum 2cm separation, cross at 90° if necessary
- **Power ↔ Signal**: Minimum 3cm separation, separate cable trays
- **High Current ↔ Low Current**: Maximum physical separation

## Grounding Architecture

### Ground Plane Design
```
                    [Chassis Ground - Enclosure]
                            |
                    [Main Ground Bus Bar]
                            |
        ┌───────────────────┼───────────────────┐
        |                   |                   |
   [Analog GND]        [Digital GND]       [Power GND]
        |                   |                   |
    ┌───┴───┐           ┌───┴───┐           ┌───┴───┐
    │MCP3008│           │ RPi 4 │           │L293D  │
    │Sensors│           │ SPI   │           │SOL1-3 │
    └───────┘           └───────┘           └───────┘
```

### Ground Connection Points
- **Single Point Ground**: All grounds connect at main bus bar
- **Chassis Connection**: Multiple low-impedance connections to enclosure
- **Shield Termination**: All cable shields terminate at entry point

## Power Distribution

### Clean Power Architecture
```
[Main Power Input] → [EMI Filter] → [Switching Regulator]
                                           |
                    ┌──────────────────────┼──────────────────────┐
                    |                      |                      |
              [Analog Rail]          [Digital Rail]         [Power Rail]
              +3.3V Clean             +5V/3.3V               +12V/24V
                    |                      |                      |
                MCP3008                Raspberry Pi           Motor Drivers
                Sensors                SPI Circuits           Solenoids
```

### Power Supply Filtering
- **Input EMI Filter**: Common-mode choke + X/Y capacitors
- **Switching Noise Filter**: LC filter on each rail
- **Local Decoupling**: 100nF + 10μF at each IC

## Thermal Management

### Heat Source Isolation
- **PWM Pump Driver**: Dedicated heat sink with thermal isolation
- **Solenoid Drivers**: Individual heat sinks
- **Raspberry Pi**: Passive cooling with thermal pads
- **Power Supply**: Separate cooling zone

### Airflow Design
- **Intake**: Filtered air intake at analog section (clean air)
- **Exhaust**: Hot air exhaust at power section
- **Isolation**: Prevent hot air from power section reaching sensors

## Mechanical Considerations

### Vibration Isolation
- **Pump Motor**: Vibration dampening mounts
- **Sensitive Components**: Shock-absorbing mounts for MCP3008
- **Enclosure**: Rigid construction to prevent resonance

### Accessibility
- **Service Access**: Easy access to Raspberry Pi and ADC for maintenance
- **Cable Management**: Removable cable trays for service
- **Test Points**: Accessible test points for EMC validation

## EMC Validation Points

### Test Locations
1. **MCP3008 ADC**: Monitor for switching noise injection
2. **Sensor Inputs**: Measure noise floor and interference
3. **SPI Lines**: Check for digital noise coupling
4. **Power Rails**: Verify clean power delivery
5. **Ground Points**: Measure ground noise and impedance

### Measurement Setup
- **Spectrum Analyzer**: Monitor 10 kHz - 1 GHz range
- **Oscilloscope**: Time-domain analysis of switching events
- **Current Probes**: Measure common-mode currents
- **Near-Field Probes**: Map magnetic field distribution

This optimized layout minimizes electromagnetic interference while maintaining practical manufacturability and serviceability for the medical device vacuum controller.
