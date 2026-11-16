# vacuum Controller Project - Parts List

## Microcontroller
- Raspberry Pi 4 (8GB RAM)
  - Raspbian 32-bit OS
  - Quad-core processor for handling multiple tasks
  - Built-in Wi-Fi and Bluetooth capabilities

## Analog-to-Digital Conversion
- Adafruit MCP3008 (8-channel, 10-bit SPI ADC)
  - Connected via SPI (SCK: GPIO 11, MOSI: GPIO 10, MISO: GPIO 9, CS: GPIO 8)
  - Channel 0: AVL-Line sensor (Applied Vacuum Line)
  - Channel 1: Tank vacuum sensor

## Pressure Sensors
- 2x MPX5010DP Differential Pressure Sensors
  - Sensor 1 = AVL-on MCP3008 channel 0
  - Sensor 2 = Vacuum tank on MCP3008 channel 1

## Display
- 50 inch HDMI display

## Vacuum System
- **Main vacuum pump (in the machine): Marhynchus "Electric Micro Air Pump 2.5-8.0LPM Low Noise Mini Vacuum Diaphragm Pump (DC12V)"**
  - Amazon ASIN [B0BN8DY5FT](https://www.amazon.com/dp/B0BN8DY5FT) — bought Feb 2024 (listing now unavailable)
  - 12 V DC diaphragm pump, 2.5–8.0 L/min, about 80 g, 5 mm air nozzle
  - Max vacuum −60 kPa (**−450 mmHg**) — about 6× the controller's 75 mmHg cap, so a mechanical relief valve on each line (85–100 mmHg) is still needed
  - Max positive pressure 100 kPa; noise ≤ 65 dB at 30 cm; rated life ≥ 500 h continuous
  - Has a built-in pressure switch and relief valve
  - RPM and current draw not listed — measure the current: the L293D only handles about 0.6 A per channel
  - Controlled via software PWM on GPIO 18 using libgpiod (currently on/off only — see `ActuatorControl::setPWM`)
  - PWM frequency: 5000Hz (configurable)
  - Enable control via GPIO 25
- **Spare 12 V pump: YOGULGUL "Mini Air Pump 12V DC, Micro Water Pump Motor with 2 Hose Clamps"**
  - Amazon ASIN [B0B1HTFHZ9](https://www.amazon.com/dp/B0B1HTFHZ9) — bought twice (Feb and Mar 2024; listing now unavailable)
  - 12 V DC mini motor pump, about 50 g, aluminium + polypropylene; flow and max vacuum not listed
  - No pressure switch ("do not block the outlet")
  - Candidate for a first **clitoral air-pulse test**: remove its check valves, seal the exhaust port, and connect the other port by a short stiff tube to a T at the clitoral port (vacuum line joins the T through a small restrictor). Pulse rate = motor RPM ÷ 60. Run it from its own PWM motor driver, not the L293D.
- Vacuum Storage Tank
- Silicone Tubing (3mm ID/5mm OD) for vacuum lines
- T-connectors for vacuum routing
- Cup-Cylinders for vacuum application:

## Valves & Actuators
- 3x Solenoid Valves (for vacuum application/release)
  - SOL1 (GPIO 17): AVL-(Applied Vacuum Line) - controlled via libgpiod
  - SOL2 (GPIO 27): AVL vent valve - controlled via libgpiod
  - SOL3 (GPIO 22): Tank vent valve - controlled via libgpiod


## Motor Driver
- L293D Motor Driver Shield or Module
  - Enable pin connected to Raspberry Pi GPIO 25
  - PWM connected to Raspberry Pi GPIO 18
  - IN2 grounded for single direction operation

### Motor Wiring Instructions
```
Raspberry Pi                  L293D                  Vacuum Pump
-----------------------------------------------------------------
GPIO 25 (Enable) ----> EN (Enable pin)
GPIO 18 (PWM)    ----> IN1 (Input 1)
                       IN2 ----------------> Connect to GND
                       
                       OUT1 ]
                       OUT2 ] ------------> Connect to motor leads
                            ] (either polarity is fine)
                       
                       VCC -----------------> Connect to 5V
                       VM (Motor Power) ----> Connect to 12V
                       GND -----------------> Connect to GND
```

## Control Interface
  - graphical interface
  - Anti detachment (crucial)
  - Pattern selection
  - Custom pattern creation
  - Pulse duration adjustment
  - Pressure monitoring
  - System settings

## Power Supply
- 5V Power Supply (for Raspberry Pi)
- 12V Power Supply (for vacuum pump, solenoid valves, and L293D)
- Common ground between all components

## Additional Components
- Breadboard or Prototyping PCB
- Jumper wires and connecting cables
- Pull-up/pull-down resistors as needed
- Heat shrink tubing or electrical tape for insulation
- Mounting hardware for sensors and pump

## Software Requirements
- Raspbian OS (or Raspberry Pi OS)
- C++ with Qt5 framework:
  - libgpiod (for GPIO control)
  - Linux SPI interface (/dev/spidev for MCP3008)
  - Qt5::Core, Qt5::Widgets, Qt5::Charts
  - pthread (for threading)
- Build system: CMake 3.16+

### Key Library Dependencies
- **libgpiod**: Modern GPIO control library
  - Provides reliable GPIO access through the Linux GPIO character device interface
  - Used for controlling solenoid valves and pump enable/PWM signals
  - More stable and future-proof
- **Linux SPI**: Direct SPI communication via /dev/spidev interface
  - Used for MCP3008 ADC communication
  - No additional libraries required, uses standard Linux SPI ioctls
- **Qt5 Framework**: Cross-platform GUI and application framework
  - Qt5::Widgets for GUI components
  - Qt5::Charts for real-time pressure monitoring graphs
  - Qt5::Core for threading and system integration

### Installation Requirements
To install the required dependencies on Raspberry Pi OS:
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install development tools
sudo apt install -y build-essential cmake git

# Install Qt5 development libraries
sudo apt install -y qtbase5-dev qtcharts5-dev

# Install libgpiod (modern GPIO library)
sudo apt install -y libgpiod-dev gpiod

# Enable SPI interface (if not already enabled)
sudo raspi-config
# Navigate to: Interfacing Options -> SPI -> Enable
```
## Vacuum Patterns
The controller supports multiple vacuum patterns:
1. Slow Pulse
2. medium Pulse
3. Fast Pulse
4. Slow Wave Pattern
5. medium Wave Pattern
6. Fast Wave Pattern
5. Slow Air Pulse
7. medium Air Pulse
8. Fast Air Pulse
6. Slow Milking
7. medium Milking
8. Fast Milking
7. Slow Constant Orgasm
8. Medium Constant Orgasm
9. Fast Constant Orgasm
8. Edging

## Safety Features
- Overpressure protection (MAX_PRESSURE_MMHG = 100.0 mmHg)
- Sensor error detection
- Emergency stop button
- Automatic shutdown on error conditions
- System cannot operate if sensors fail
- Auto-recovery mechanisms implemented in case of crashes


## Wiring Diagram Summary

### SPI Connections (MCP3008)
```
Raspberry Pi     MCP3008
--------------------------
GPIO 11 (SCK)  → CLK
GPIO 10 (MOSI) → DIN
GPIO 9 (MISO)  → DOUT
GPIO 8         → CS
3.3V           → VDD
GND            → GND
```

### Pump Control
```
Raspberry Pi     L293D            Vacuum Pump
-------------------------------------------------
GPIO 25 (Enable) → EN
GPIO 18 (PWM)    → IN1
```

## MCP3008 Integration Notes
The Raspberry Pi has a limited number of ADC pins with inconsistent performance.
The MCP3008 provides 8 stable analog input channels with these benefits:
- Reliable 10-bit resolution across all channels
- Consistent readings across temperature ranges
- Simple SPI interface with good noise immunity
- Expandability for future sensors or controls

### UI Features
- UI control of all vacuum patterns
- Real-time pressure monitoring with graphical display
- Pattern customization interface
- System diagnostics and calibration
- Settings menu for system configuration
- Anti detachment (crucial) this is important it monitors the AVL-vacuum line and if rises above a adjustable threshold it will open the SOL1 (GPIO 17): AVL-(Applied Vacuum Line) to pull more vacuum on the AVL-line this means the system is increasing the vacuum on the  patients skin.this is to make sure the cup does not detach from the patient skin.
### Summary Table
| Component | Purpose |
|-------------|----------------------------------------------|
| MCP3008 | Converts analog sensor signals to digital for the Raspberry Pi (via SPI) |
| Raspberry Pi | Main controller, runs code, etc. |
| MPX5010DP | Analog pressure sensor (needs ADC to be read) |

