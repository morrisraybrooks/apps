# Vacuum Controller User Manual

## Table of Contents
1. [Getting Started](#getting-started)
2. [Main Interface](#main-interface)
3. [Pattern Selection](#pattern-selection)
4. [Safety Systems](#safety-systems)
5. [System Settings](#system-settings)
6. [Troubleshooting](#troubleshooting)
7. [Maintenance](#maintenance)

## Getting Started

### System Startup
1. **Power On**: Press the power button on the Raspberry Pi
2. **Wait for Boot**: The system will automatically start the vacuum controller application
3. **Initial Check**: The system performs a self-test on startup
4. **Ready State**: Green status indicators show the system is ready for use

### First Time Setup
1. **Calibration**: Navigate to Settings → Calibration and run sensor calibration
2. **Safety Limits**: Review and adjust safety thresholds in Settings → Safety
3. **User Preferences**: Configure display and interface preferences

## Main Interface

### Dashboard Overview
The main dashboard provides real-time monitoring and control:

#### Pressure Monitoring Section
- **AVL Pressure Gauge**: Shows current Applied Vacuum Line pressure
- **Tank Pressure Gauge**: Shows current tank pressure
- **Real-time Chart**: Historical pressure data with configurable time ranges
- **Threshold Lines**: Visual indicators for warning and critical pressure levels

#### Control Panel
- **Pattern Selection**: Choose from 15+ predefined vacuum patterns
- **Emergency Stop**: Large red button for immediate system shutdown
- **Start/Stop Controls**: Begin or end pattern execution
- **Parameter Adjustment**: Real-time intensity, speed, and pressure controls

#### Status Indicators
- **System Health**: Overall system status (Green/Yellow/Red)
- **Safety Systems**: Anti-detachment, overpressure protection status
- **Hardware Status**: Sensor, actuator, and communication status
- **Pattern Status**: Current pattern execution state

### Navigation
- **Touch Interface**: Optimized for 50-inch touch displays
- **Tab Navigation**: Main tabs for Monitoring, Patterns, Safety, Settings
- **Quick Access**: Emergency controls always visible
- **Status Bar**: System time, uptime, and critical alerts

## Pattern Selection

### Available Patterns

#### Pulse Patterns
- **Slow Pulse**: 2-second pulses with 2-second pauses at 60% pressure
- **Medium Pulse**: 1-second pulses with 1-second pauses at 70% pressure
- **Fast Pulse**: 0.5-second pulses with 0.5-second pauses at 75% pressure

#### Wave Patterns
- **Slow Wave**: 10-second gradual pressure waves (30-70% range)
- **Medium Wave**: 5-second pressure waves (40-80% range)
- **Fast Wave**: 2-second pressure waves (50-85% range)

#### Air Pulse Patterns
- **Slow Air Pulse**: 3-second vacuum with 2-second air release
- **Medium Air Pulse**: 2-second vacuum with 1.5-second air release
- **Fast Air Pulse**: 1-second vacuum with 1-second air release

#### Milking Patterns
- **Slow Milking**: 3-second strokes with 2-second release, 7 cycles
- **Medium Milking**: 2-second strokes with 1.5-second release, 8 cycles
- **Fast Milking**: 1.5-second strokes with 1-second release, 10 cycles

#### Constant Patterns
- **Slow Constant**: 70% base pressure with ±15% variation over 3 seconds
- **Medium Constant**: 75% base pressure with ±10% variation over 2 seconds
- **Fast Constant**: 80% base pressure with ±5% variation over 1 second

#### Special Patterns
- **Edging**: 15-second buildup to 85%, 5-second release, 3-second hold, repeated 3 times

### Pattern Controls

#### Selection Process
1. **Browse Categories**: Use category tabs to filter patterns
2. **Pattern Preview**: Click pattern name to see description and parameters
3. **Parameter Adjustment**: Modify intensity, speed, and pressure offset
4. **Preview Mode**: Test pattern with reduced pressure for safety
5. **Start Execution**: Begin full pattern execution

#### Real-time Adjustments
- **Intensity Slider**: Adjust overall pattern intensity (0-100%)
- **Speed Control**: Modify pattern timing (0.1x to 3.0x speed)
- **Pressure Offset**: Add/subtract pressure from base pattern (-20% to +20%)
- **Emergency Stop**: Immediately halt pattern execution

#### Custom Patterns
1. **Create New**: Access custom pattern creation dialog
2. **Step Editor**: Define individual pressure and timing steps
3. **Visual Designer**: Drag-and-drop pattern creation
4. **Validation**: System checks pattern safety before saving
5. **Save/Load**: Store custom patterns for future use

## Safety Systems

### Anti-detachment Monitoring
The most critical safety feature prevents cup detachment:

#### How It Works
- **Continuous Monitoring**: 100Hz pressure monitoring on AVL line
- **Threshold Detection**: Alerts when pressure drops below 50 mmHg (configurable)
- **Automatic Response**: Immediately increases vacuum to prevent detachment
- **Recovery**: Returns to normal operation when pressure stabilizes

#### Status Indicators
- **Green**: Normal operation, cup properly attached
- **Yellow**: Warning - pressure approaching threshold
- **Red**: Anti-detachment activated - vacuum increased automatically

#### Configuration
- **Threshold Setting**: Adjust trigger pressure (20-80 mmHg)
- **Response Delay**: Configure response time (50-500ms)
- **Maximum Increase**: Limit vacuum increase (10-50%)

### Emergency Stop System
Immediate system shutdown capability:

#### Activation Methods
- **Software Button**: Large red button on main interface
- **Hardware Button**: Physical emergency stop button (if connected)
- **Automatic Triggers**: Overpressure, sensor failure, system error

#### Emergency Response
1. **Immediate Shutdown**: All vacuum generation stops instantly
2. **Valve Activation**: Vent valves open to release pressure
3. **System Lock**: Prevents restart until reset
4. **Event Logging**: Records emergency stop event with timestamp

#### Reset Procedure
1. **Identify Cause**: Review system status and error messages
2. **Resolve Issue**: Address the condition that triggered emergency stop
3. **System Check**: Verify all systems are functioning normally
4. **Reset Button**: Press reset to restore normal operation

### Overpressure Protection
Automatic protection against excessive pressure:

#### Monitoring
- **Dual Sensors**: Both AVL and tank pressure monitored
- **Configurable Limits**: Set maximum pressure (50-150 mmHg)
- **Rapid Response**: Immediate shutdown if limits exceeded

#### Protection Actions
1. **Pump Shutdown**: Vacuum pump immediately disabled
2. **Vent Activation**: Pressure relief valves opened
3. **Alert Generation**: Visual and audible warnings
4. **System Lock**: Prevents operation until reset

## System Settings

### Safety Configuration
Access through Settings → Safety tab:

#### Pressure Limits
- **Maximum Pressure**: System shutdown threshold (default: 100 mmHg)
- **Warning Threshold**: Alert level (default: 80 mmHg)
- **Anti-detachment Threshold**: Cup detachment prevention (default: 50 mmHg)

#### Safety Features
- **Emergency Stop**: Enable/disable emergency stop functionality
- **Overpressure Protection**: Enable automatic pressure limiting
- **Auto Shutdown**: Automatic shutdown on system errors
- **Sensor Timeout**: Maximum time between sensor readings

### Calibration
Access through Settings → Calibration tab:

#### Sensor Calibration
1. **Preparation**: Ensure system is at atmospheric pressure
2. **Start Calibration**: Click "Calibrate Sensors" button
3. **Follow Prompts**: System guides through calibration process
4. **Verification**: Test readings against known pressure sources
5. **Save Results**: Store calibration data for future use

#### Calibration Schedule
- **Automatic Reminders**: System prompts for periodic calibration
- **Calibration History**: View past calibration dates and results
- **Validation**: Verify calibration accuracy with test procedures

### Hardware Configuration
Access through Settings → Hardware tab:

#### GPIO Pin Assignment
- **SOL1 (AVL Valve)**: GPIO pin for main vacuum valve
- **SOL2 (AVL Vent)**: GPIO pin for AVL pressure relief
- **SOL3 (Tank Vent)**: GPIO pin for tank pressure relief
- **Pump Control**: GPIO pins for pump enable and PWM control
- **Emergency Button**: GPIO pin for hardware emergency stop

#### SPI Configuration
- **Channel Selection**: SPI channel for sensor communication
- **Speed Setting**: Communication speed (typically 1 MHz)
- **Sensor Mapping**: Assignment of sensors to SPI channels

### Display Settings
Access through Settings → Display tab:

#### Screen Configuration
- **Resolution**: Display resolution settings
- **Fullscreen Mode**: Enable/disable fullscreen operation
- **Touch Calibration**: Calibrate touch screen accuracy
- **Brightness**: Adjust display brightness

#### Interface Preferences
- **Font Size**: Adjust text size for readability
- **Color Theme**: Choose interface color scheme
- **Chart Settings**: Configure pressure chart appearance
- **Alert Settings**: Customize warning and alarm displays

## Troubleshooting

### Common Issues

#### System Won't Start
**Symptoms**: Application fails to launch or crashes on startup
**Solutions**:
1. Check power supply and connections
2. Verify SD card integrity
3. Review system logs: `/var/log/vacuum-controller.log`
4. Run hardware test: `sudo ./VacuumController --test-hardware`

#### Sensor Reading Errors
**Symptoms**: "Sensor Error" alerts or invalid pressure readings
**Solutions**:
1. Check sensor connections and wiring
2. Verify SPI communication: `sudo ./VacuumController --test-spi`
3. Recalibrate sensors through Settings → Calibration
4. Replace faulty sensors if problems persist

#### Pattern Execution Issues
**Symptoms**: Patterns don't start or stop unexpectedly
**Solutions**:
1. Check safety system status - may be preventing operation
2. Verify pressure limits are not exceeded
3. Review pattern parameters for validity
4. Check hardware actuator functionality

#### Performance Problems
**Symptoms**: Slow response, GUI freezing, or delayed updates
**Solutions**:
1. Check system resources: CPU, memory usage
2. Close unnecessary applications
3. Restart the vacuum controller application
4. Consider system reboot if problems persist

### Error Messages

#### "Hardware Not Ready"
- **Cause**: Hardware initialization failed
- **Solution**: Check connections, restart application

#### "Sensor Timeout"
- **Cause**: No response from pressure sensors
- **Solution**: Check SPI connections, recalibrate sensors

#### "Overpressure Detected"
- **Cause**: Pressure exceeded safety limits
- **Solution**: Check for blockages, verify pressure settings

#### "Anti-detachment Activated"
- **Cause**: Cup detachment risk detected
- **Solution**: Check cup attachment, verify threshold settings

### Log Files
System logs provide detailed troubleshooting information:

- **Main Log**: `/var/log/vacuum-controller.log` - General application events
- **Error Log**: `/var/log/vacuum-controller-errors.log` - Error conditions
- **Safety Log**: `/var/log/vacuum-controller-safety.log` - Safety system events
- **Performance Log**: `/var/log/vacuum-controller-performance.log` - Performance metrics

## Maintenance

### Regular Maintenance Tasks

#### Daily Checks
- **Visual Inspection**: Check for physical damage or loose connections
- **System Status**: Verify all status indicators show green
- **Pressure Readings**: Confirm sensor readings are reasonable
- **Safety Test**: Test emergency stop functionality

#### Weekly Maintenance
- **Sensor Cleaning**: Clean pressure sensor ports and connections
- **Calibration Check**: Verify sensor accuracy with known pressure source
- **Log Review**: Check system logs for errors or warnings
- **Performance Check**: Monitor system performance metrics

#### Monthly Maintenance
- **Full Calibration**: Complete sensor calibration procedure
- **Hardware Test**: Run comprehensive hardware test suite
- **Software Update**: Check for and install software updates
- **Backup**: Create backup of system configuration and logs

#### Annual Maintenance
- **Professional Service**: Schedule professional inspection and service
- **Component Replacement**: Replace wear items per manufacturer schedule
- **Compliance Check**: Verify continued compliance with safety standards
- **Documentation Update**: Update maintenance records and procedures

### Preventive Maintenance

#### Sensor Care
- **Regular Cleaning**: Keep sensor ports free of debris
- **Calibration Schedule**: Follow recommended calibration intervals
- **Environmental Protection**: Protect sensors from moisture and contamination
- **Replacement Planning**: Monitor sensor performance and plan replacements

#### Hardware Maintenance
- **Connection Inspection**: Regularly check all electrical connections
- **Valve Service**: Clean and lubricate valves per manufacturer instructions
- **Pump Maintenance**: Follow pump manufacturer maintenance schedule
- **Cable Management**: Ensure cables are properly routed and secured

### Record Keeping
Maintain detailed records of all maintenance activities:

- **Maintenance Log**: Date, time, and description of all maintenance
- **Calibration Records**: Results and dates of all calibrations
- **Component History**: Installation dates and service history
- **Performance Trends**: Track system performance over time
- **Compliance Documentation**: Records required for regulatory compliance
