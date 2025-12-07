# EMC Testing and Validation Plan
## Medical Device Vacuum Controller

### Overview
Comprehensive electromagnetic compatibility testing plan to validate interference-free operation and compliance with medical device EMC standards.

## Testing Standards and Requirements

### Primary Standards
- **IEC 60601-1-2:2014**: EMC requirements for medical electrical equipment
- **CISPR 11**: Industrial, scientific and medical equipment - Radio disturbance characteristics
- **IEC 61000-4-x**: Electromagnetic compatibility testing and measurement techniques

### Performance Criteria
- **Pressure Measurement Accuracy**: ±0.5 mmHg under all EMI conditions
- **Anti-Detachment Response**: <100ms response time with EMI present
- **System Stability**: No false triggers, oscillations, or lockups
- **Safety Functions**: Emergency stop must function under all EMI conditions

## Phase 1: Pre-Compliance Testing

### 1.1 Conducted Emissions Testing
**Objective**: Measure noise on power and signal lines

**Test Setup**:
```
[Device Under Test] → [LISN] → [EMI Receiver] → [Computer]
                        |
                   [Ground Plane]
```

**Equipment Required**:
- Line Impedance Stabilization Network (LISN)
- EMI Test Receiver (9kHz - 30MHz)
- Current probes (1MHz - 1GHz)
- Spectrum analyzer

**Test Procedure**:
1. Connect device to LISN on grounded test bench
2. Operate device in all modes (pump PWM, solenoid switching)
3. Measure conducted emissions on power lines
4. Record peak and average emissions vs. frequency
5. Compare to CISPR 11 Class A limits

**Acceptance Criteria**:
- Conducted emissions <6dB below CISPR 11 limits
- No spurious emissions >40dB above noise floor

### 1.2 Radiated Emissions Testing
**Objective**: Measure RF emissions from device and cables

**Test Setup**:
- Semi-anechoic chamber or open area test site
- Biconical antenna (30MHz - 300MHz)
- Log-periodic antenna (300MHz - 1GHz)
- EMI test receiver

**Test Procedure**:
1. Position device 3m from antenna
2. Rotate device and antenna for maximum emissions
3. Test with all cables connected (realistic configuration)
4. Measure emissions 30MHz - 1GHz
5. Identify emission sources using near-field probes

**Acceptance Criteria**:
- Radiated emissions <6dB below CISPR 11 Class A limits
- No resonances from vacuum lines acting as antennas

### 1.3 Power Quality Analysis
**Objective**: Verify clean power delivery to sensitive circuits

**Test Points**:
- MCP3008 VDD and VREF pins
- Raspberry Pi 3.3V and 5V rails
- Analog sensor power supplies
- Ground bus bar noise

**Measurements**:
```
Parameter                Target Value        Test Condition
─────────────────────────────────────────────────────────
Analog 3.3V Ripple      <1mV p-p           Full load, all switching
Digital 5V Ripple       <10mV p-p          Maximum CPU load
Reference Noise         <50μV RMS          10Hz-100kHz bandwidth
Ground Noise            <1mV p-p           Between ground points
Load Regulation         <0.1%              No load to full load
```

## Phase 2: Susceptibility Testing

### 2.1 Conducted Susceptibility (CS)
**Objective**: Inject noise on power and signal lines, verify system stability

**Test Setup**:
```
[Signal Generator] → [Coupling Network] → [Device Power Input]
                                              |
                                        [Performance Monitor]
```

**Test Procedure**:
1. Inject 1V RMS, 150kHz - 80MHz on power lines
2. Monitor pressure measurement accuracy during injection
3. Verify anti-detachment system response time
4. Check for system resets or malfunctions
5. Test emergency stop functionality

**Acceptance Criteria**:
- Pressure accuracy maintained within ±1.0 mmHg
- No system malfunctions or resets
- Anti-detachment response <150ms (50% margin)

### 2.2 Radiated Susceptibility (RS)
**Objective**: Expose device to RF fields, verify immunity

**Test Setup**:
- Anechoic chamber with RF amplifier
- Biconical and log-periodic antennas
- Field strength: 3V/m (80MHz - 1GHz)
- Modulation: 80% AM at 1kHz

**Test Procedure**:
1. Position device in uniform field area
2. Sweep frequency 80MHz - 1GHz
3. Monitor all critical functions during exposure
4. Test worst-case orientations
5. Verify performance during and after exposure

**Acceptance Criteria**:
- No degradation of pressure measurement accuracy
- All safety functions operational
- No permanent damage or calibration drift

### 2.3 Electrical Fast Transient (EFT)
**Objective**: Test immunity to switching transients

**Test Parameters**:
- Voltage: ±2kV on power lines, ±1kV on signal lines
- Rise time: 5ns
- Repetition rate: 5kHz and 100kHz
- Duration: 1 minute per polarity

**Critical Monitoring**:
- MCP3008 ADC readings during transients
- Solenoid valve operation
- System stability and error logging

## Phase 3: System-Level Validation

### 3.1 Magnetic Field Immunity
**Objective**: Test immunity to magnetic fields from solenoids and pump

**Test Setup**:
- Helmholtz coils for uniform magnetic field
- Field strength: 30 A/m at 50Hz, 3 A/m at 1kHz-100kHz
- Monitor pressure sensor accuracy

**Test Procedure**:
1. Generate uniform magnetic field around device
2. Operate all solenoids and pump simultaneously
3. Measure pressure sensor accuracy vs. field strength
4. Test different field orientations
5. Verify anti-detachment system performance

**Acceptance Criteria**:
- Pressure accuracy within ±0.5 mmHg in 30 A/m field
- No false detachment alarms

### 3.2 Operational EMC Testing
**Objective**: Validate EMC performance during realistic operation

**Test Scenarios**:
1. **Maximum EMI Condition**: All solenoids switching, pump at maximum PWM
2. **Pattern Operation**: Rapid solenoid switching during stimulation patterns
3. **Anti-Detachment Response**: EMI during pressure transient detection
4. **Emergency Stop**: EMI immunity of safety systems

**Performance Monitoring**:
```cpp
class EMCValidationMonitor {
    struct EMCTestMetrics {
        double pressure_accuracy_mmhg;
        double response_time_ms;
        int false_alarms_count;
        bool emergency_stop_functional;
        double measurement_noise_floor;
    };
    
    EMCTestMetrics runEMCTest(EMCTestScenario scenario);
    void logEMCPerformance();
    bool validateEMCCompliance();
};
```

### 3.3 Long-Term Stability Testing
**Objective**: Verify stable operation over extended periods with EMI

**Test Duration**: 24 hours continuous operation
**EMI Conditions**: Periodic injection of worst-case interference
**Monitoring**:
- Pressure measurement drift
- System error rates
- Temperature effects on EMC performance
- Calibration stability

## Phase 4: Medical Device Compliance

### 4.1 IEC 60601-1-2 Testing
**Essential Performance**: Define critical functions that must not be affected by EMI
- Pressure measurement accuracy: ±0.5 mmHg
- Anti-detachment detection: <100ms
- Emergency stop response: <50ms
- Vacuum control stability: ±2 mmHg

**Test Levels**:
- Conducted RF: 3V RMS (150kHz - 80MHz)
- Radiated RF: 3V/m (80MHz - 2.5GHz)
- ESD: ±8kV contact, ±15kV air
- EFT: ±2kV power, ±1kV signal
- Surge: ±2kV line-line, ±1kV line-ground

### 4.2 Risk Management (ISO 14971)
**EMI Risk Assessment**:
1. **Hazard**: EMI causes false pressure readings
   - **Risk**: Incorrect vacuum application
   - **Mitigation**: Redundant sensors, error detection algorithms

2. **Hazard**: EMI prevents anti-detachment detection
   - **Risk**: Cup detachment without detection
   - **Mitigation**: Multiple detection methods, fail-safe design

3. **Hazard**: EMI disables emergency stop
   - **Risk**: Cannot stop operation in emergency
   - **Mitigation**: Hardware-based emergency stop, EMI-immune design

## Test Equipment and Setup

### Required Test Equipment
```
Equipment                    Specification              Purpose
─────────────────────────────────────────────────────────────────
EMI Test Receiver           9kHz - 30MHz               Conducted emissions
Spectrum Analyzer           30MHz - 1GHz               Radiated emissions
RF Signal Generator         150kHz - 1GHz              Susceptibility testing
RF Power Amplifier          100W, 80MHz - 1GHz         Field generation
Current Probes              1MHz - 1GHz                Current measurement
Near-Field Probes           H-field and E-field        Source identification
LISN                        50Ω/50μH                   Power line testing
ESD Generator               ±30kV                      ESD testing
EFT/Burst Generator         ±4kV                       Transient testing
```

### Test Environment
- **Semi-Anechoic Chamber**: For radiated emissions/immunity
- **Shielded Room**: For conducted testing
- **Ground Plane**: 2m x 2m minimum
- **Environmental Control**: 23°C ±2°C, 45-75% RH

## Data Collection and Analysis

### Automated Test Data Collection
```cpp
class EMCTestDataLogger {
    struct TestResult {
        string test_name;
        double frequency_mhz;
        double emission_level_dbuv;
        double field_strength_vm;
        bool performance_acceptable;
        string notes;
    };
    
    void logTestResult(TestResult result);
    void generateComplianceReport();
    bool assessOverallCompliance();
};
```

### Pass/Fail Criteria
- **Emissions**: Must be ≥6dB below regulatory limits
- **Immunity**: Essential performance maintained during and after exposure
- **Safety**: Emergency functions must remain operational
- **Accuracy**: Pressure measurements within specification

## Corrective Actions

### Common EMI Issues and Solutions
1. **Excessive Conducted Emissions**
   - Add common-mode chokes
   - Improve power supply filtering
   - Reduce switching rise times

2. **Radiated Emissions from Cables**
   - Add ferrite cores
   - Improve cable shielding
   - Optimize cable routing

3. **Susceptibility to Magnetic Fields**
   - Increase sensor separation from solenoids
   - Add magnetic shielding
   - Implement software filtering

4. **Ground Loop Issues**
   - Verify single-point grounding
   - Improve ground impedance
   - Isolate sensitive circuits

This comprehensive EMC testing plan ensures the medical device vacuum controller meets all electromagnetic compatibility requirements while maintaining safe and accurate operation.
