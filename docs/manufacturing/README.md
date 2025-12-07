# Manufacturing Documentation - Vacuum Cup Assembly

## Overview
Complete manufacturing documentation for the vacuum cup component of the medical device vacuum controller system. Designed for vulva stimulation with anti-detachment monitoring capabilities.

## Files Structure
```
docs/
├── cup_design.svg              # Comprehensive design overview
└── manufacturing/
    ├── cup_exploded.svg        # Exploded assembly with part callouts
    ├── cup_tech_drawing.svg    # Technical drawing with dimensions
    ├── cup_tech_drawing.dxf    # CAD exchange format (DXF R12)
    ├── README.md               # This file
    └── Makefile                # PDF export automation
```

## 3D Model
```
hardware/cup/cup.scad           # Complete OpenSCAD parametric model
```

## Quick Start
1. **View 3D Model**: `openscad hardware/cup/cup.scad`
2. **Generate PDFs**: `make -C docs/manufacturing pdf`
3. **View Drawings**: Open SVG files in browser or Inkscape

## Technical Specifications

### Dimensions
- **Inner Cavity**: 22×14mm oval (clitoral application)
- **Dome Depth**: 14 ±0.3mm
- **Wall Thickness**: 2.0 ±0.2mm uniform
- **Seal Lip**: 6.0 ±0.5mm width, 12° ±2° contact angle

### Materials
- **Shell**: PC/Tritan (Polycarbonate/Copolyester)
  - Medical grade, biocompatible
  - Wall thickness: 2.0-3.0mm
- **Seal Lip**: Platinum-cured Silicone
  - Shore A 10-15 (soft skin contact)
  - Biocompatible per ISO 10993
- **Diaphragm** (Optional): Medical Silicone
  - Shore A 30-40, thickness 0.4-0.6mm
  - Dual-chamber separation

### Pressure Ratings
- **Continuous Operation**: -40 kPa (-300 mmHg)
- **Transient Maximum**: -60 kPa (-450 mmHg)
- **Test Pressure**: -80 kPa (-600 mmHg)

### Compliance Standards
- **ISO 10993**: Biocompatibility (skin contact)
- **ISO 80369-7**: Luer-lock connections
- **Medical Device**: Manufacturing standards compliant

## Port Configuration

### Primary Port (P1)
- **Type**: Female Luer-lock socket
- **Part Number**: 0-0-1 (ISO 80369-7)
- **ID**: 3.9mm minimum (6% taper)
- **Connection**: SOL1 (apply vacuum) / SOL2 (vent/release)

### AVL Sensor Tap
- **ID**: 2.0mm micro-lumen
- **Position**: 6mm from primary port
- **Function**: Applied Vacuum Level monitoring
- **Connection**: MCP3008 ADC Channel 0

### Auxiliary Port (P2) - Optional
- **Type**: Female Luer-lock socket
- **ID**: 3.0mm nominal
- **Connection**: SOL3 (reserved for future use)
- **Status**: Not used in current firmware

## Manufacturing Process

### 1. Shell Production
- Injection molding or 3D printing (SLA/SLS)
- Material: PC/Tritan medical grade
- Post-processing: Surface finishing to Ra 0.8μm internal

### 2. Port Installation
- Press-fit or threaded Luer-lock inserts
- Ensure flush mounting with shell surface
- Apply medical-grade thread locker if required

### 3. Seal Lip Assembly
- Overmolding or separate assembly
- Material: Platinum-cured silicone
- Ensure proper contact angle (12°)

### 4. Quality Control
- Pressure testing to -80 kPa
- Dimensional verification
- Biocompatibility certification
- Visual inspection for defects

## PDF Export
Generate PDF documentation using Inkscape:
```bash
make pdf                        # Generate all PDFs
make cup_design.pdf            # Overview diagram only
make cup_exploded.pdf          # Exploded assembly only
make cup_tech_drawing.pdf      # Technical drawing only
```

## Integration with Vacuum Controller
- **SOL1**: Apply vacuum via primary port
- **SOL2**: Vent/release via primary port
- **SOL3**: Reserved auxiliary connection
- **AVL Sensor**: Anti-detachment monitoring
- **Controller**: Raspberry Pi 4 with MCP3008 ADC

## Safety Notes
⚠️ **Critical Requirements**:
- All materials must be biocompatible (ISO 10993)
- Pressure test every unit before deployment
- Verify proper seal lip contact angle
- Ensure AVL sensor line is unobstructed
- Test anti-detachment system functionality

Controller interface notes:
- Controller max is set to 75 mmHg (MPX5010DP FS), PatternEngine operates 10–90% of nominal max; emergency stop now at ~80 mmHg.
- If sensor is changed later, update SafetyManager/VacuumController max and PatternEngine safety scaling accordingly.
- Air pulse patterns are generated via main line control (pump/SOL1/SOL2); dual‑chamber aux port is optional hardware and unused in current firmware.
