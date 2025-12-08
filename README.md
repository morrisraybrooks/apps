# V-Contour Dual-Therapy System

A sophisticated **Automated Vacuum Stimulation Device** - an electromechanical system that applies controlled vacuum pressure to the vulva/clitoris for inducing sexual arousal and orgasm. Designed as a "hands-free" device that automatically brings a user to climax through precisely controlled vacuum patterns.

For a complete and detailed explanation of the system, please see the [detailed explanation](explanation.md).

Built with Qt on Raspberry Pi 4 with a 50-inch HDMI display and modern **libgpiod v2.2.1** for reliable, future-proof GPIO control.

## 🔑 Critical Differentiator: Active Engorgement

> **Commercial air-pulse toys (Womanizer, Satisfyer, LELO) only provide oscillating pressure waves for stimulation—they CANNOT create sustained vacuum for tissue engorgement.** They rely on natural arousal (5-15 minutes) to engorge the clitoris before/during use.

**The V-Contour dual-chamber system separates these functions:**

| Chamber | Valve Control | Capability |
|---------|---------------|------------|
| **Outer V-seal** | SOL1/SOL2 | Sustained vacuum (30-50 mmHg) for vulva/labia engorgement |
| **Clitoral cylinder** | SOL4/SOL5 | Sustained vacuum for clitoral engorgement **OR** oscillating air-pulse (5-13 Hz) for stimulation |

**This allows the V-Contour to actively induce clitoral erection/engorgement in 15-30 seconds, rather than waiting for natural arousal to occur.**

### Active Engorgement Benefits:
1. **Faster arousal**: Engorge the clitoris BEFORE beginning air-pulse stimulation
2. **Maintained engorgement**: Sustained outer vacuum keeps tissue engorged throughout the session
3. **Faster orgasm**: Optimal clitoral erection from the start reduces time to climax
4. **Enhanced sensitivity**: Engorged tissue has ~8,000 nerve endings more exposed
5. **Consistent response**: Does not rely on the user's natural arousal state
6. **Dual-mode flexibility**: Can switch between engorgement and stimulation dynamically

## Overview

This system provides a complete GUI solution for controlling a vacuum therapy device with the following key features:

- **Real-time pressure monitoring** with graphical displays and live data visualization
- **19+ vacuum patterns** including pulse, wave, air pulse, milking, automated orgasm cycles, and specialized therapeutic patterns
- **Automated Orgasm Patterns** with physiological progression cycles for complete arousal-to-climax sequences
- **🔑 Active Engorgement Technology** - Unlike commercial toys that only oscillate, sustained vacuum actively induces clitoral erection
- **Dual-Chamber V-Contour Design** - Separate outer V-seal (engorgement) and clitoral cylinder (air-pulse) chambers
- **Safety-critical anti-detachment system** that prevents cup detachment during therapy
- **Emergency stop functionality** with immediate system shutdown and safe state recovery
- **Touch-optimized embedded widgets** designed for 50-inch medical displays with optimal scaling
- **Complete pattern storage system** with embedded pattern creation and editing (no modal dialogs)
- **Scrollable interface panels** for optimal content management and clinical workflow
- **Comprehensive safety systems** with overpressure protection and sensor monitoring
- **Modern libgpiod v2.2.1 integration** replacing deprecated wiringPi for reliable GPIO control
- **Graceful shutdown** with proper thread cleanup and hardware safety state

## 🏗️ Industry-Standard Development Practices

This project implements comprehensive industry-standard practices for embedded safety-critical systems:

### Build System & Quality
- **Modern CMake**: Multi-configuration, cross-platform build system with Raspberry Pi optimizations
- **Static Analysis**: Integrated clang-tidy, cppcheck, and clang-format
- **Code Coverage**: lcov integration with automated CI reporting
- **Sanitizers**: AddressSanitizer and UndefinedBehaviorSanitizer for debug builds
- **Comprehensive Testing**: Unit, integration, and system tests with Qt Test Framework

### CI/CD & Automation
- **GitHub Actions**: Multi-platform builds (x86_64, ARM64) with automated testing
- **Package Generation**: Native Debian packages with proper dependencies
- **Documentation**: Automated Doxygen API docs deployed to GitHub Pages
- **Version Management**: Semantic versioning with automated changelog generation
- **Cross-compilation**: ARM64 packages for Raspberry Pi deployment

### Security & Distribution
- **System Integration**: systemd service with security hardening
- **Hardware Permissions**: Automatic udev rules and GPIO/SPI group setup
- **Package Management**: Professional .deb packages with installation scripts
- **Service Management**: Automatic startup, logging, and monitoring integration

## 🎯 Latest Release: v1.5.0 - Automated Orgasm Patterns & Production Installation

### ✅ **Major Improvements in v1.5.0**
- **Automated Orgasm Patterns**: Complete physiological arousal-to-climax cycles with intelligent progression
- **Continuous Orgasm Marathon**: Infinite cycling pattern for extended sessions with optimized 4-minute cycles
- **Production Installation**: Full system installation with proper library paths and configuration management
- **Graceful Shutdown**: Fixed thread cleanup for clean application exit without manual termination
- **Enhanced Pattern Loading**: Intelligent config file discovery for both development and production environments
- **System Integration**: Professional launcher script with hardware access and Wayland display support
- **Thread Safety**: Improved data acquisition and GUI update thread management with proper cleanup

### ✅ **Previous Major Improvements (v1.4.0)**
- **Embedded Widget Architecture**: Replaced modal dialogs with embedded widgets for seamless medical device UX
- **libgpiod v2.2.1 Integration**: Modern GPIO control replacing deprecated wiringPi for enhanced reliability
- **Touch-Optimized Interface**: Embedded pattern editing eliminates modal interruptions in clinical workflow
- **Enhanced Medical Device Compliance**: Always-visible controls optimized for 50-inch touch displays
- **Future-Proof GPIO**: Modern libgpiod ensures compatibility with current and future Raspberry Pi OS versions

### 🚀 **System Status**
- **Hardware Integration**: MCP3008 ADC, modern libgpiod v2.2.1 GPIO, SPI communication fully operational
- **Safety Systems**: Anti-detachment monitoring, emergency controls, pressure limits active
- **Pattern Engine**: 19 built-in patterns + unlimited custom pattern support with embedded editing
- **Automated Orgasm Patterns**: Complete physiological cycles with intelligent progression
- **Real-time Monitoring**: Live pressure charts, diagnostics, data logging working
- **Multi-threading**: Separate threads for GUI, data acquisition, safety monitoring with graceful shutdown
- **GPIO Architecture**: Modern libgpiod v2.2.1 replaces deprecated wiringPi for enhanced reliability

## 🌟 Automated Orgasm Patterns

The vacuum controller now includes advanced **Automated Orgasm Patterns** that provide complete physiological arousal-to-climax cycles:

### **Available Automated Patterns**
1. **Single Automated Orgasm** (5 minutes)
   - Complete arousal-to-climax cycle with physiological progression
   - Intelligent intensity ramping based on natural response patterns
   - Automatic climax detection and post-orgasm recovery phase

2. **Triple Automated Orgasm** (18 minutes)
   - Three complete orgasm cycles with recovery periods
   - Progressive intensity increase across cycles
   - Optimized timing for multiple climax experiences

3. **Continuous Orgasm Marathon** (Infinite)
   - **Endless cycling pattern** for extended sessions
   - Optimized 4-minute cycles with enhanced anti-detachment monitoring
   - Runs continuously until manually stopped
   - Perfect for extended pleasure sessions

### **Key Features**
- **🔑 Active Engorgement Phase**: Unlike commercial toys (Womanizer, Satisfyer) that only oscillate, the V-Contour uses sustained vacuum to actively engorge the clitoris BEFORE stimulation begins
- **Dual-Chamber Architecture**: Independent outer V-seal (SOL1/SOL2) for engorgement + clitoral cylinder (SOL4/SOL5) for air-pulse stimulation
- **Physiological Progression**: Patterns follow natural arousal curves with Phase 0 engorgement
- **Intelligent Timing**: Based on research showing 8-13 Hz as the optimal orgasm frequency
- **Safety Integration**: Enhanced monitoring during automated cycles
- **One-Click Activation**: Simple touch interface for complete automation
- **Customizable Parameters**: Adjust intensity, timing, and sensitivity thresholds

### **Why Active Engorgement Matters**
> Commercial air-pulse toys rely on natural arousal (5-15 minutes) to engorge the clitoris. The V-Contour's sustained vacuum actively induces clitoral erection in 15-30 seconds, significantly reducing time to orgasm.

See [docs/AUTOMATED_ORGASM_PATTERNS.md](docs/AUTOMATED_ORGASM_PATTERNS.md) for detailed technical specifications.

## 🔧 Current Build Status

**✅ COMPLETE**: All development tasks finished - 100% production-ready system
**🔧 BUILD STATUS**: CMake configures successfully, ready for Raspberry Pi 4 deployment
**📋 NEXT STEPS**: See [BUILD_STATUS.md](BUILD_STATUS.md) and [PI4_SETUP.md](PI4_SETUP.md) for complete Pi 4 setup guide

## 🚀 Production Installation (Recommended)

### **System Installation**
```bash
# Install dependencies (includes modern libgpiod v2.2.1)
sudo apt update && sudo apt install -y qtbase5-dev qtcharts5-dev libqt5charts5-dev libgpiod-dev pkg-config

# Clone and build
git clone https://github.com/morrisraybrooks/apps.git
cd apps && mkdir build && cd build
cmake .. -DRASPBERRY_PI_BUILD=ON && make -j3  # Use -j3 for Pi 4 memory optimization

# Install system-wide
sudo make install
sudo ldconfig  # Update library cache

# Run from anywhere
vacuum-controller-launcher
```

### **Installation Locations**
- **Executable**: `/usr/local/bin/VacuumController`
- **Launcher**: `/usr/local/bin/vacuum-controller-launcher`
- **Config Files**: `/usr/local/share/vacuum-controller/`
- **Libraries**: `/usr/local/lib/libVacuumControllerLib.so`
- **Documentation**: `/usr/local/share/doc/VacuumController/`

### **Development Mode (Alternative)**
```bash
# For development/testing without system installation
git clone https://github.com/morrisraybrooks/apps.git
cd apps && mkdir build && cd build
cmake .. -DRASPBERRY_PI_BUILD=ON && make -j3
sudo ./VacuumController  # Run from build directory
```

## 🚀 Quick Start for Developers

### 1. Development Environment Setup
```bash
# Clone the repository
git clone https://github.com/morrisraybrooks/apps.git
cd apps

# Set up development environment (installs all tools and dependencies)
./setup-dev-environment.sh

# Build with enhanced build script
./build.sh -c -r  # Clean build with tests
```

### 2. Build Options
```bash
# Development builds
./build.sh -t Debug -c -r --enable-coverage    # Debug with coverage
./build.sh -t Release -c -i                    # Release build and install
./build.sh --enable-static-analysis            # With static analysis

# Package generation
cd build && cpack -G DEB                       # Create .deb package
```

### 3. Development Tools
```bash
# Code formatting
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Static analysis
cppcheck --enable=all --std=c++17 src/

# Memory checking
valgrind --tool=memcheck ./build/VacuumController

# Version management
./scripts/version.sh bump minor               # Bump version
./scripts/version.sh release 1.2.0           # Full release process
```

### 4. Testing & Quality
```bash
# Run all tests
cd build && ctest --output-on-failure

# Generate coverage report
./build.sh -t Debug --enable-coverage -r
cd build && lcov --capture --directory . --output-file coverage.info

# Generate documentation
cd build && make docs
```

## Hardware Requirements

### Core Components
- **Raspberry Pi 4 (8GB RAM)** - Main controller
- **50-inch HDMI Display** - User interface display
- **MCP3008 ADC** - 8-channel analog-to-digital converter
- **3x MPX5010DP Pressure Sensors** - Differential pressure sensors (Outer, Tank, Clitoral)
- **5x Solenoid Valves** - Dual-chamber vacuum control and venting
- **L293D Motor Driver** - Vacuum pump control
- **DC Vacuum Pump** - Main vacuum source

┌─────────────────────────────────────────────────────────────────────────────┐
│                    DUAL-CHAMBER VACUUM SYSTEM                               │
│                                                                             │
│   ┌─────────┐    ┌─────────┐                                                │
│   │ DC Pump │───►│  TANK   │◄─── Sensor 2 (Tank)                            │
│   │ (PWM)   │    │         │                                                │
│   └─────────┘    └────┬────┘                                                │
│                       │                                                     │
│              ┌────────┴────────┐                                            │
│              ▲                 │                                            │
│          [SOL3]            ┌───┴───┐                                        │
│       Tank Vent            │       │                                        │
│             │           [SOL1]   [SOL4]                                     │
│          ┌─────┐         (AVL)  (Clitoral)                                  │
│          │ ATM │           │       │                                        │
│          └─────┘           ▼       ▼                                        │
│                    ┌───────────────────────┐                                │
│                    │     V-CONTOUR CUP     │                                │
│                    │   OUTER V-CHAMBER   ◄─┼─── Sensor 1 (AVL)              │
│                    │      ┌─────────┐◄─────┼─── Sensor 3 (Clitoral)         │
│                    │      │CLITORAL │      │        ┌─────┐                 │
│                    │      │CYLINDER │◄─────┼────────│ ATM │  [SOL5]         │
│                    │      └┬───────┬┘      │        └─────┘  Clitoral       │
│                    │       │ OPEN  │       │                  Vent          │
│                    │       │CHANNEL│       │                                │
│                    │       │ DRAIN │       │                                │
│                    │       │   │   │       │                                │
│                    │       │   │   │       │                                │
│                    │       │   │   │       │                                │
│                    │       │   │   │       │                                │
│                    │       │   ▼   │       │                                │
│                    └───────┘       └───────┘                                │
│                                        ▲                                    │
│                                        │   ┌─────┐                          │
│                                    [SOL2]──│ ATM │                          │
│                                    Outer   └─────┘                          │
│                                    Vent                                     │
└─────────────────────────────────────────────────────────────────────────────┘

### GPIO Pin Assignments (Dual-Chamber)
```
GPIO 17 - SOL1 (Outer V-seal vacuum)
GPIO 27 - SOL2 (Outer V-seal vent)
GPIO 22 - SOL3 (Tank vent - safety)
GPIO 23 - SOL4 (Clitoral cylinder vacuum)      ← NEW
GPIO 24 - SOL5 (Clitoral cylinder vent)        ← NEW
GPIO 25 - Pump enable (L293D)
GPIO 18 - Pump PWM control
GPIO 21 - Emergency stop button (optional)
```

### Pressure Specifications by Chamber

| Parameter | Outer V-Seal Chamber | Clitoral Cylinder |
|-----------|---------------------|-------------------|
| **Purpose** | Attachment + engorgement | Air-pulse stimulation |
| **Operating Range** | 30-60 mmHg | 20-75 mmHg |
| **Attachment Threshold** | 35 mmHg minimum | N/A |
| **Maximum Safe Limit** | 75 mmHg | 80 mmHg |
| **Typical Operating** | 40-55 mmHg (constant) | 30-70 mmHg (pulsing 5-13 Hz) |

### SPI Connections (MCP3008)
```
GPIO 11 (SCK)  → CLK
GPIO 10 (MOSI) → DIN
GPIO 9 (MISO)  → DOUT
GPIO 8 (CS)    → CS
```

### ADC Channel Assignments
| Sensor | ADC Channel | Purpose | Update Rate |
|--------|-------------|---------|-------------|
| AVL (Outer) | CH0 | V-seal pressure, anti-detachment | 50 Hz |
| Tank | CH1 | Reservoir monitoring | 50 Hz |
| Clitoral | CH2 | Clitoral cylinder pressure | 100 Hz |

> **📝 Important**: This system uses the modern **libgpiod v2.2.1** library for GPIO control instead of the deprecated `wiringPi`. This ensures compatibility with current and future Raspberry Pi OS versions and provides better security, performance, and reliability. The new request-based API offers improved resource management and enhanced safety for medical device applications.

## 🔧 Modern libgpiod v2.2.1 Integration

This project uses **libgpiod v2.2.1**, the modern Linux GPIO interface that replaces the deprecated wiringPi library. This provides significant advantages for medical device applications:

### **Key Benefits of libgpiod v2.2.1**
- **Future-Proof**: Official Linux GPIO interface, actively maintained and developed
- **Enhanced Security**: Character device interface with proper permissions and access control
- **Better Performance**: Request-based API with bulk operations and efficient resource management
- **Improved Reliability**: Robust error handling and automatic resource cleanup
- **Medical Device Ready**: Strict resource management ideal for safety-critical applications

### **GPIO Command Reference**
```bash
# List all GPIO chips and lines (replaces 'gpio readall')
gpioinfo

# Detect available GPIO chips
gpiodetect

# Read GPIO state (replaces 'gpio read')
gpioget gpiochip0 17

# Set GPIO state (replaces 'gpio write')
gpioset gpiochip0 17=1

# Monitor GPIO changes
gpiomon gpiochip0 17

# Test vacuum controller GPIO pins
gpioget gpiochip0 17 27 22 25  # Read SOL1, SOL2, SOL3, PUMP pins
```

### **Migration from wiringPi**
If you're familiar with wiringPi, here are the key differences:
- **wiringPi**: `gpio readall` → **libgpiod**: `gpioinfo`
- **wiringPi**: `gpio read 0` → **libgpiod**: `gpioget gpiochip0 17`
- **wiringPi**: `gpio write 0 1` → **libgpiod**: `gpioset gpiochip0 17=1`
- **wiringPi**: `gpio mode 0 out` → **libgpiod**: Handled in application code with request-based API

## Software Architecture

### Core Components

1. **VacuumController** - Main system coordinator
2. **HardwareManager** - Hardware abstraction layer
3. **SafetyManager** - Safety monitoring and emergency stop
4. **PatternEngine** - Vacuum pattern execution
5. **MainWindow** - Primary GUI interface
6. **PressureMonitor** - Real-time pressure visualization

### Safety Features

- **Overpressure Protection** - Maximum 100 mmHg limit
- **Anti-detachment Monitoring** - Prevents cup detachment
- **Sensor Error Detection** - Automatic error handling
- **Emergency Stop System** - Immediate shutdown capability
- **Hardware Interlocks** - Safe state on power loss

## Building the Application

### Prerequisites

Install required dependencies on Raspberry Pi:

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Qt5 and development tools
sudo apt install -y qtbase5-dev qtcharts5-dev cmake build-essential pkg-config

# Install modern libgpiod v2.2.1 for GPIO control (replaces deprecated wiringPi)
sudo apt install -y libgpiod-dev libgpiod2 gpiod

# Verify libgpiod installation
gpioinfo  # Should list all GPIO chips and lines
gpiodetect  # Should show available GPIO chips

# Install additional development tools
sudo apt install -y doxygen graphviz clang-format clang-tidy cppcheck valgrind lcov
```

### Build Process

1. Clone or copy the project to your Raspberry Pi
2. Navigate to the project directory
3. Run the build script:

```bash
./build.sh
```

The build script will:
- Detect the platform (Raspberry Pi vs development)
- Configure CMake appropriately
- Build the application
- Provide installation instructions

### Manual Build

If you prefer to build manually:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DRASPBERRY_PI_BUILD=ON
make -j3  # Use -j3 for Raspberry Pi 4 memory optimization
```

## Running the Application

### On Raspberry Pi

The application requires root privileges to access GPIO pins:

```bash
sudo ./VacuumController
```

### Development/Testing

For development on non-Raspberry Pi systems:

```bash
./VacuumController
```

Note: Hardware features will be simulated on non-ARM platforms.

## Configuration

### Pattern Configuration

Vacuum patterns are defined in `config/patterns.json`. The file includes:

- **15+ predefined patterns** with different speeds and characteristics
- **Safety limits** and pressure constraints
- **Pattern categories** for organized selection
- **Customizable parameters** for each pattern type

### System Settings

System configuration is stored in `config/settings.json`:

- **Safety parameters** (pressure limits, thresholds)
- **Hardware configuration** (GPIO pins, SPI settings)
- **UI preferences** (display settings, themes)
- **Calibration data** for pressure sensors
- **Logging and maintenance settings**

## Safety Considerations

⚠️ **IMPORTANT SAFETY INFORMATION** ⚠️

This is a medical device controller with safety-critical features:

1. **Always test thoroughly** before clinical use
2. **Verify all safety systems** are functioning properly
3. **Ensure emergency stop** is easily accessible
4. **Monitor pressure limits** continuously
5. **Follow proper calibration procedures**
6. **Maintain system logs** for safety compliance

### Anti-detachment System

The anti-detachment feature is **CRITICAL** for patient safety:
- Monitors Applied Vacuum Line (AVL) pressure continuously
- Automatically increases vacuum if cup detachment is detected
- Adjustable threshold settings
- Immediate response capability

## User Interface

### Main Features

- **Large Touch-Friendly Buttons** - Optimized for 50-inch medical display with proper touch targets
- **Real-time Pressure Charts** - Historical pressure visualization with live data streaming
- **Embedded Pattern Editor** - Seamless pattern creation and editing without modal interruptions
- **Safety Panel** - Emergency controls and system status with immediate response
- **Embedded Settings Panel** - System configuration and calibration integrated into main interface
- **Diagnostics Panel** - System health monitoring and maintenance tools
- **libgpiod v2.2.1 Integration** - Modern, reliable GPIO control for all hardware interfaces

### Navigation

- **F1** - Main Control Panel
- **F2** - Safety Panel
- **F3** - Settings
- **F4** - Diagnostics
- **Escape** - Emergency Stop

## Development

### Project Structure

```
src/
├── main.cpp                 # Application entry point
├── VacuumController.*       # Main system controller
├── hardware/               # Hardware abstraction layer
│   ├── HardwareManager.*   # Hardware coordination
│   ├── SensorInterface.*   # Pressure sensor interface
│   ├── ActuatorControl.*   # Valve and pump control
│   └── MCP3008.*          # ADC communication
├── safety/                 # Safety systems
│   ├── SafetyManager.*     # Core safety monitoring
│   ├── AntiDetachmentMonitor.* # Anti-detachment system
│   └── EmergencyStop.*     # Emergency stop handling
├── gui/                    # User interface
│   ├── MainWindow.*        # Main application window
│   ├── PressureMonitor.*   # Pressure visualization
│   ├── PatternSelector.*   # Pattern selection widget
│   ├── SafetyPanel.*       # Safety control panel
│   └── SettingsDialog.*    # Configuration dialog
└── patterns/               # Pattern execution
    ├── PatternEngine.*     # Pattern execution engine
    └── PatternDefinitions.* # Pattern definitions
```

### Adding New Patterns

1. Define pattern in `config/patterns.json`
2. Implement pattern logic in `PatternEngine`
3. Add UI controls in `PatternSelector`
4. Test thoroughly with safety systems

## Troubleshooting

### Common Issues

1. **Permission Denied (GPIO with libgpiod)**
   - Run with `sudo` for GPIO access: `sudo ./VacuumController`
   - Check user is in `gpio` group: `sudo usermod -a -G gpio $USER`
   - Verify libgpiod installation: `gpioinfo` should list GPIO chips

2. **libgpiod Not Found**
   - Install libgpiod v2.x: `sudo apt install -y libgpiod-dev libgpiod2 gpiod`
   - Verify installation: `gpiodetect` should show gpiochip0
   - Check version: `gpioinfo --version` (should show v2.x)

3. **SPI Communication Errors**
   - Verify SPI is enabled: `sudo raspi-config` → Interface Options → SPI → Enable
   - Check wiring connections to MCP3008
   - Confirm MCP3008 power supply (3.3V)
   - Test SPI devices: `ls /dev/spi*` should show spidev0.0, spidev0.1

4. **Qt Application Won't Start**
   - Install Qt5 development packages: `sudo apt install qtbase5-dev qtcharts5-dev`
   - Check display configuration and Wayland support
   - **For SSH with Wayland**: Use `ssh user@raspberry-pi` and configure Wayland forwarding

5. **Pressure Readings Invalid**
   - Check sensor calibration in settings
   - Verify ADC connections and SPI communication
   - Test with multimeter and `gpioinfo` for GPIO states
   - Verify MCP3008 is receiving proper 3.3V power

6. **GPIO Control Issues (migrating from wiringPi)**
   - Remove any old wiringPi references: `sudo apt remove wiringpi`
   - Ensure libgpiod v2.x is installed: `sudo apt install libgpiod-dev`
   - Use new GPIO commands: `gpioget`, `gpioset`, `gpiomon` instead of `gpio`
   - Check GPIO chip access: `ls -la /dev/gpiochip*`

### Logging

System logs are written to `/var/log/vacuum-controller.log` (configurable).
Enable debug logging in `config/settings.json` for detailed troubleshooting.

## 🖥️ Wayland Display Configuration

This system is designed for modern Wayland-based environments. All display configurations use Wayland protocols.

### Running on Local Display (Raspberry Pi)

#### Production Mode (Recommended for Medical Use)
```bash
# Wayland: Modern compositor with excellent performance
# Best for clinical deployment on dedicated 50-inch display
QT_QPA_PLATFORM=wayland ./VacuumController

# Enable high-DPI scaling for 50-inch displays
QT_SCALE_FACTOR=1.5 QT_QPA_PLATFORM=wayland ./VacuumController

# Auto-detect platform (defaults to Wayland if available)
./VacuumController
```

#### Fallback Mode (If Wayland Issues)
```bash
# EGLFS: Direct framebuffer rendering as fallback only
QT_QPA_PLATFORM=eglfs ./VacuumController
```

#### Platform Advantages
- **Wayland**: Modern architecture, remote access, better debugging, future-proof
- **EGLFS**: Fallback option for embedded systems without Wayland support

### SSH with Wayland Support

For remote access using SSH with Wayland forwarding:

#### Option 1: VNC over Wayland (Recommended)
```bash
# On the Raspberry Pi, install Wayland VNC server
sudo apt install -y wayvnc

# Start VNC server
wayvnc 0.0.0.0 5900

# From your client machine, connect with VNC viewer
# Then run the application on the Pi desktop
./VacuumController
```

#### Option 2: Wayland Socket Forwarding
```bash
# SSH to the Pi without display forwarding
ssh user@raspberry-pi-ip

# On the Pi, ensure Wayland is running
echo $WAYLAND_DISPLAY  # Should show wayland-0

# Run with Wayland platform
QT_QPA_PLATFORM=wayland ./VacuumController
```

#### Option 3: Remote Desktop with Wayland
```bash
# Install RDP server for Wayland
sudo apt install -y xrdp-wayland

# Configure and start RDP service
sudo systemctl enable xrdp-wayland
sudo systemctl start xrdp-wayland

# Connect from client using RDP
# Run application in the remote session
```

### Qt Platform Configuration

#### Wayland Configuration (Primary Platform)
```bash
# Set Wayland as default Qt platform
export QT_QPA_PLATFORM=wayland

# High-DPI configuration for 50-inch medical displays
export QT_SCALE_FACTOR=1.5
export QT_AUTO_SCREEN_SCALE_FACTOR=1
export QT_FONT_DPI=120

# Enable Wayland-specific features for medical device
export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
export QT_WAYLAND_FORCE_DPI=120

# Optimize for touch interface
export QT_IM_MODULE=qtvirtualkeyboard

# Run the application
./VacuumController
```

#### EGLFS Configuration (Fallback Only)
```bash
# Direct framebuffer rendering as fallback
export QT_QPA_PLATFORM=eglfs

# High-DPI configuration
export QT_SCALE_FACTOR=1.5
export QT_FONT_DPI=120

# Disable cursor for touch-only interface
export QT_QPA_EGLFS_HIDECURSOR=1

# Run the application
./VacuumController
```

#### Why Wayland is Recommended

**Wayland Advantages:**
- ✅ **Modern architecture** - Future-proof display protocol
- ✅ **Excellent performance** - Hardware-accelerated compositing
- ✅ **Remote access support** - SSH with VNC for maintenance
- ✅ **Better security** - Isolated application rendering
- ✅ **Multi-application** - Can run diagnostic tools alongside
- ✅ **Development friendly** - Remote debugging and testing
- ✅ **Touch optimization** - Native touch and gesture support
- ✅ **High-DPI support** - Perfect for 50-inch medical displays

**EGLFS as Fallback:**
- ⚠️ **Use only if** Wayland is not available or has issues
- ⚠️ **Limited functionality** - No remote access, single application
- ⚠️ **Legacy approach** - Being phased out in favor of Wayland

### Wayland Environment Setup

Ensure proper Wayland environment:

```bash
# Check Wayland session
echo $XDG_SESSION_TYPE  # Should show 'wayland'
echo $WAYLAND_DISPLAY   # Should show 'wayland-0'

# Install Wayland development packages
sudo apt install -y libwayland-dev wayland-protocols

# Install Qt Wayland support
sudo apt install -y qtwayland5
```

### Troubleshooting Wayland Issues

1. **"Cannot connect to Wayland display" error**:
   ```bash
   echo $WAYLAND_DISPLAY  # Should show wayland-0
   ls -la /run/user/$(id -u)/wayland-*  # Check socket exists
   ```

2. **Qt Wayland platform not found**:
   ```bash
   sudo apt install -y qtwayland5
   export QT_QPA_PLATFORM=wayland
   ```

3. **Application debugging with Wayland**:
   ```bash
   QT_LOGGING_RULES="qt.qpa.wayland.*=true" ./VacuumController
   ```

4. **EGLFS issues (black screen, no display)**:
   ```bash
   # Check framebuffer device
   ls -la /dev/fb*

   # Test with specific framebuffer
   QT_QPA_EGLFS_FB=/dev/fb0 ./VacuumController

   # Debug EGLFS initialization
   QT_LOGGING_RULES="qt.qpa.eglfs.*=true" ./VacuumController
   ```

5. **Platform switching for debugging**:
   ```bash
   # Primary: Use Wayland (recommended)
   QT_QPA_PLATFORM=wayland ./VacuumController

   # Fallback: Use EGLFS only if Wayland fails
   QT_QPA_PLATFORM=eglfs ./VacuumController

   # Force software rendering if GPU issues
   QT_QUICK_BACKEND=software ./VacuumController
   ```

6. **High-DPI display configuration**:
   ```bash
   # For 50-inch displays with EGLFS
   export QT_SCALE_FACTOR=1.5
   export QT_FONT_DPI=120
   export QT_QPA_EGLFS_HIDECURSOR=1
   ./VacuumController
   ```

## License

This project is developed for medical device applications. Ensure compliance with relevant medical device regulations and safety standards before clinical use.

## Support

For technical support or questions about this vacuum controller system, please refer to the project documentation or contact the development team.

---

**⚠️ Medical Device Warning**: This system is designed for medical/therapeutic applications. Proper testing, validation, and regulatory compliance are required before clinical use.
