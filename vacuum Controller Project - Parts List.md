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
- Vacuum Pump (DC motor type)
  - Controlled via PWM on GPIO 18
  - PWM frequency: 5000Hz
- Vacuum Storage Tank
- Silicone Tubing (3mm ID/5mm OD) for vacuum lines
- T-connectors for vacuum routing
- Cup-Cylinders for vacuum application:

## Valves & Actuators
- 3x Solenoid Valves (for vacuum application/release)
  - SOL1 (GPIO 17): AVL-(Applied Vacuum Line)
  - SOL2 (GPIO 27): AVL vent valve
  - SOL3 (GPIO 22): Tank vent valve


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
- Raspbian OS
- C++ with:
  - RPi.GPIO
  - spidev (for MCP3008)
  - PyBluez (for Bluetooth)

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

