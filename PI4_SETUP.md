# Raspberry Pi 4 Complete Setup Guide

## Quick Start Checklist

### ✅ Pre-Setup Requirements
- [ ] Raspberry Pi 4B (4GB+ RAM recommended)
- [ ] 32GB+ microSD card (Class 10)
- [ ] 50-inch touch display (1920x1080+)
- [ ] MCP3008 ADC chip
- [ ] Dual pressure sensors
- [ ] 3 solenoid valves + vacuum pump
- [ ] Hardware emergency stop button

### ✅ Software Installation Steps
```bash
# 1. Update system
sudo apt update && sudo apt upgrade -y

# 2. Install build essentials
sudo apt install -y build-essential cmake git

# 3. Install Qt5 (complete package)
sudo apt install -y qt5-default qtbase5-dev qtcharts5-dev libqt5charts5-dev qttools5-dev

# 4. Install Raspberry Pi libraries
sudo apt install -y wiringpi libwiringpi-dev

# 5. Enable hardware interfaces
sudo raspi-config
# → Interface Options → SPI → Enable
# → Interface Options → GPIO → Enable
# → Reboot
```

### ✅ Build and Run
```bash
# 1. Clone repository
git clone https://github.com/morrisraybrooks/apps.git
cd apps

# 2. Build project
mkdir build && cd build
cmake .. -DRASPBERRY_PI_BUILD=ON
make -j4

# 3. Run application
sudo ./VacuumController
```

## Detailed Hardware Setup

### GPIO Wiring Diagram
```
Raspberry Pi 4 GPIO Pinout:
┌─────────────────────────────────┐
│  3V3  (1) (2)  5V               │
│  GPIO2(3) (4)  5V               │
│  GPIO3(5) (6)  GND              │
│  GPIO4(7) (8)  GPIO14           │
│  GND  (9) (10) GPIO15           │
│  GPIO17(11)(12) GPIO18 ← Pump PWM
│  GPIO27(13)(14) GND             │
│  GPIO22(15)(16) GPIO23          │
│  3V3 (17)(18) GPIO24            │
│  GPIO10(19)(20) GND             │
│  GPIO9(21)(22) GPIO25 ← Pump Enable
│  GPIO11(23)(24) GPIO8           │
│  GND (25)(26) GPIO7             │
│  ...                            │
└─────────────────────────────────┘

Connections:
GPIO 17 → SOL1 (AVL Valve)
GPIO 27 → SOL2 (AVL Vent)  
GPIO 22 → SOL3 (Tank Vent)
GPIO 25 → Pump Enable
GPIO 18 → Pump PWM Control
GPIO 21 → Emergency Stop Button
```

### SPI Wiring (MCP3008)
```
MCP3008 ADC Connections:
┌─────────────────┐
│ VDD  VREF  AGND │ ← 3.3V, 3.3V, GND
│ CLK  DOUT  DIN  │ ← GPIO11, GPIO9, GPIO10
│ CS   DGND       │ ← GPIO8, GND
│ CH0  CH1  ...   │ ← AVL Sensor, Tank Sensor
└─────────────────┘

Pi SPI0 Pins:
MOSI (GPIO10) → DIN
MISO (GPIO9)  → DOUT  
SCLK (GPIO11) → CLK
CE0  (GPIO8)  → CS
```

## Display Configuration

### 50-inch Touch Display Setup
```bash
# 1. Configure display resolution
sudo nano /boot/config.txt

# Add these lines:
hdmi_force_hotplug=1
hdmi_group=2
hdmi_mode=82        # 1920x1080 @ 60Hz
hdmi_drive=2
hdmi_boost=4

# For touch screen
dtoverlay=vc4-kms-v3d
max_framebuffers=2

# 2. Install touch calibration
sudo apt install -y xinput-calibrator

# 3. Calibrate touch (run after GUI starts)
xinput_calibrator

# 4. Auto-start application
sudo nano /etc/xdg/lxsession/LXDE-pi/autostart
# Add: @/path/to/VacuumController
```

## Build Troubleshooting

### Common Issues and Solutions

#### Issue 1: Qt5 Not Found
```bash
# Error: Could not find Qt5
# Solution:
sudo apt install -y qt5-default qtbase5-dev qtcharts5-dev
sudo apt install -y libqt5charts5-dev qttools5-dev
```

#### Issue 2: wiringPi Missing
```bash
# Error: wiringPi.h: No such file or directory
# Solution:
sudo apt install -y wiringpi libwiringpi-dev

# Verify installation:
gpio readall
```

#### Issue 3: SPI Not Enabled
```bash
# Error: SPI device not found
# Solution:
sudo raspi-config
# → Interface Options → SPI → Enable
sudo reboot

# Verify:
ls /dev/spi*
# Should show: /dev/spidev0.0  /dev/spidev0.1
```

#### Issue 4: Permission Denied
```bash
# Error: Permission denied accessing GPIO
# Solution:
sudo usermod -a -G gpio $USER
sudo usermod -a -G spi $USER
# Logout and login again

# Or run with sudo:
sudo ./VacuumController
```

#### Issue 5: Display Issues
```bash
# Problem: GUI doesn't display properly
# Solution:
export DISPLAY=:0
xhost +local:

# For remote access:
ssh -X pi@raspberry-pi-ip
```

## Testing Procedures

### 1. Hardware Test Sequence
```bash
# Test GPIO pins
gpio readall

# Test SPI communication
# (Build and run SPI test program)

# Test sensors
sudo ./VacuumController --test-sensors

# Test actuators
sudo ./VacuumController --test-actuators
```

### 2. Software Test Sequence
```bash
# Run all tests
make run_all_tests

# Run specific test suites
make run_safety_tests
make run_hardware_tests
make run_gui_tests
```

### 3. Performance Validation
```bash
# Monitor system performance
htop

# Check memory usage
free -h

# Monitor CPU temperature
vcgencmd measure_temp

# Test real-time performance
sudo ./VacuumController --performance-test
```

## Production Deployment

### System Optimization
```bash
# 1. Disable unnecessary services
sudo systemctl disable bluetooth
sudo systemctl disable wifi-powersave

# 2. Set CPU governor to performance
echo 'performance' | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# 3. Increase GPU memory split
sudo raspi-config
# → Advanced Options → Memory Split → 128

# 4. Enable real-time scheduling
echo '@realtime - rtprio 99' | sudo tee -a /etc/security/limits.conf
```

### Auto-Start Configuration
```bash
# 1. Create systemd service
sudo nano /etc/systemd/system/vacuum-controller.service

[Unit]
Description=Vacuum Controller GUI
After=graphical-session.target

[Service]
Type=simple
User=pi
Environment=DISPLAY=:0
ExecStart=/home/pi/apps/build/VacuumController
Restart=always
RestartSec=5

[Install]
WantedBy=graphical-session.target

# 2. Enable service
sudo systemctl enable vacuum-controller.service
sudo systemctl start vacuum-controller.service
```

## Safety Considerations

### Critical Safety Checks
1. **Emergency Stop Testing**
   - Hardware button must immediately stop all vacuum
   - Software emergency stop must be always accessible
   - System must fail to safe state

2. **Pressure Monitoring**
   - Dual sensor redundancy
   - Anti-detachment monitoring at 100Hz
   - Overpressure protection with automatic shutdown

3. **System Validation**
   - All safety systems must be tested before clinical use
   - Regular calibration of pressure sensors required
   - Comprehensive logging for audit trail

### Regulatory Compliance
- System designed for IEC 60601-1 medical device standards
- Comprehensive documentation for regulatory submission
- Full traceability from requirements to implementation
- Risk analysis and mitigation strategies documented

## Performance Targets

### Real-time Requirements
- **Data Acquisition**: 50Hz continuous sampling
- **GUI Refresh**: 30 FPS smooth operation  
- **Safety Monitoring**: 100Hz with <10ms response
- **Touch Response**: <100ms for all controls

### System Resources
- **Memory Usage**: <500MB typical operation
- **CPU Usage**: <50% average load
- **Storage**: <1GB for application and logs
- **Network**: Optional for remote monitoring

## Support and Maintenance

### Regular Maintenance
```bash
# 1. Update system monthly
sudo apt update && sudo apt upgrade

# 2. Check sensor calibration
sudo ./VacuumController --calibrate-sensors

# 3. Verify safety systems
sudo ./VacuumController --safety-test

# 4. Clean log files
sudo ./VacuumController --clean-logs
```

### Backup Procedures
```bash
# 1. Backup configuration
sudo cp -r /etc/vacuum-controller /backup/config/

# 2. Backup application
tar -czf vacuum-controller-backup.tar.gz /home/pi/apps/

# 3. Create SD card image
sudo dd if=/dev/mmcblk0 of=vacuum-controller-image.img bs=4M
```

This setup guide ensures a complete, production-ready deployment of the vacuum controller system on Raspberry Pi 4.
