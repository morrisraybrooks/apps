# Vacuum Controller GUI System - Project Structure

This document provides a comprehensive overview of the project structure and organization.

## Root Directory Structure

```
vacuum-controller/
├── README.md                    # Main project documentation
├── LICENSE                      # MIT License
├── CHANGELOG.md                 # Version history and changes
├── CONTRIBUTING.md              # Contribution guidelines
├── PROJECT_STRUCTURE.md         # This file
├── CMakeLists.txt              # Main CMake configuration
├── .gitignore                  # Git ignore rules
├── build.sh                    # Build script for Raspberry Pi
├── config/                     # Configuration files
├── docs/                       # Documentation
├── src/                        # Source code
├── tests/                      # Test suite
├── scripts/                    # Utility scripts
└── resources/                  # Resources and assets
```

## Source Code Organization (`src/`)

```
src/
├── main.cpp                    # Application entry point
├── VacuumController.h/.cpp     # Main controller class
├── hardware/                   # Hardware abstraction layer
│   ├── HardwareManager.h/.cpp  # Hardware management
│   ├── SensorInterface.h/.cpp  # Sensor communication
│   ├── ActuatorControl.h/.cpp  # Actuator control
│   └── MCP3008.h/.cpp         # ADC interface
├── safety/                     # Safety-critical systems
│   ├── SafetyManager.h/.cpp    # Safety coordination
│   ├── AntiDetachmentMonitor.h/.cpp # Anti-detachment system
│   └── EmergencyStop.h/.cpp    # Emergency stop system
├── patterns/                   # Pattern execution system
│   ├── PatternEngine.h/.cpp    # Pattern execution engine
│   ├── PatternDefinitions.h/.cpp # Pattern definitions
│   ├── PatternValidator.h/.cpp # Pattern validation
│   └── PatternTemplateManager.h/.cpp # Template management
├── gui/                        # User interface
│   ├── MainWindow.h/.cpp       # Main application window
│   ├── PressureMonitor.h/.cpp  # Pressure visualization
│   ├── PatternSelector.h/.cpp  # Pattern selection UI
│   ├── SafetyPanel.h/.cpp      # Safety status display
│   ├── ParameterAdjustmentPanel.h/.cpp # Parameter controls
│   ├── SystemDiagnosticsPanel.h/.cpp # Diagnostics interface
│   ├── SettingsDialog.h/.cpp   # Settings and calibration
│   ├── CustomPatternDialog.h/.cpp # Custom pattern creation
│   └── components/             # Reusable UI components
│       ├── TouchButton.h/.cpp  # Touch-optimized buttons
│       ├── PressureGauge.h/.cpp # Pressure gauge widget
│       ├── StatusIndicator.h/.cpp # Status indicators
│       ├── MultiStatusIndicator.h/.cpp # Multi-status display
│       └── PressureChart.h/.cpp # Real-time charts
├── threading/                  # Multi-threading system
│   ├── ThreadManager.h/.cpp    # Thread coordination
│   ├── DataAcquisitionThread.h/.cpp # Data acquisition
│   ├── GuiUpdateThread.h/.cpp  # GUI updates
│   └── SafetyMonitorThread.h/.cpp # Safety monitoring
├── error/                      # Error handling system
│   ├── ErrorManager.h/.cpp     # Error management
│   └── CrashHandler.h/.cpp     # Crash detection
├── logging/                    # Data logging system
│   └── DataLogger.h/.cpp       # Comprehensive logging
├── reporting/                  # Report generation
│   ├── DataExporter.h/.cpp     # Data export
│   └── ReportGenerator.h/.cpp  # Report generation
└── performance/                # Performance optimization
    ├── PerformanceMonitor.h/.cpp # Performance monitoring
    └── MemoryManager.h/.cpp    # Memory management
```

## Test Suite Organization (`tests/`)

```
tests/
├── CMakeLists.txt              # Test build configuration
├── TestMain.cpp                # Test runner main
├── TestRunnerMain.cpp          # Comprehensive test runner
├── TestFramework.h/.cpp        # Test framework
├── TestRunner.h/.cpp           # Test automation
├── SafetySystemTests.h/.cpp    # Safety system validation
├── HardwareTests.h/.cpp        # Hardware interface tests
├── PatternTests.h/.cpp         # Pattern execution tests
├── GUITests.h/.cpp             # GUI functionality tests
├── PerformanceTests.h/.cpp     # Performance validation
├── IntegrationTests.h/.cpp     # System integration tests
├── UserInterfaceTests.h/.cpp   # UI testing for 50-inch displays
├── mocks/                      # Mock objects for testing
│   ├── MockHardwareManager.h/.cpp
│   ├── MockSensorInterface.h/.cpp
│   ├── MockActuatorControl.h/.cpp
│   └── MockVacuumController.h/.cpp
└── data/                       # Test data files
    ├── test_config.json
    ├── test_patterns.json
    └── safety_test_data.json
```

## Documentation (`docs/`)

```
docs/
├── USER_MANUAL.md              # Complete user manual
├── API_REFERENCE.md            # Developer API documentation
├── INSTALLATION.md             # Installation guide
├── HARDWARE_SETUP.md           # Hardware configuration
├── TROUBLESHOOTING.md          # Common issues and solutions
├── MAINTENANCE.md              # Maintenance procedures
├── SAFETY_GUIDELINES.md        # Safety considerations
├── PERFORMANCE_TUNING.md       # Performance optimization
├── DEVELOPMENT_GUIDE.md        # Developer setup guide
├── TESTING_GUIDE.md            # Testing procedures
├── DEPLOYMENT_GUIDE.md         # Production deployment
├── images/                     # Documentation images
│   ├── system_architecture.png
│   ├── gui_screenshots/
│   └── hardware_diagrams/
└── examples/                   # Code examples
    ├── basic_usage.cpp
    ├── custom_patterns.cpp
    └── hardware_integration.cpp
```

## Configuration (`config/`)

```
config/
├── default_settings.json      # Default system configuration
├── pattern_definitions.json   # Pattern configurations
├── hardware_config.json       # Hardware pin assignments
├── safety_limits.json         # Safety thresholds
├── display_config.json        # Display settings
├── logging_config.json        # Logging configuration
└── test_config.json          # Test configuration
```

## Scripts (`scripts/`)

```
scripts/
├── build.sh                   # Build script
├── install.sh                 # Installation script
├── setup_raspberry_pi.sh      # Raspberry Pi setup
├── run_tests.sh              # Test execution
├── deploy.sh                 # Deployment script
├── backup_config.sh          # Configuration backup
├── update_system.sh          # System update
└── performance_benchmark.sh   # Performance testing
```

## Resources (`resources/`)

```
resources/
├── icons/                     # Application icons
│   ├── app_icon.png
│   ├── emergency_stop.png
│   ├── pattern_icons/
│   └── status_icons/
├── fonts/                     # Custom fonts
├── styles/                    # Qt stylesheets
│   ├── default_theme.qss
│   ├── dark_theme.qss
│   └── high_contrast.qss
├── sounds/                    # Audio alerts
│   ├── alarm.wav
│   ├── warning.wav
│   └── notification.wav
└── translations/              # Internationalization
    ├── vacuum_controller_en.ts
    ├── vacuum_controller_es.ts
    └── vacuum_controller_fr.ts
```

## Key Components Description

### Core System (`src/`)

- **VacuumController**: Main system coordinator
- **HardwareManager**: Hardware abstraction and control
- **SafetyManager**: Safety system coordination
- **PatternEngine**: Pattern execution and timing

### Safety-Critical Components (`src/safety/`)

- **AntiDetachmentMonitor**: 100Hz monitoring for cup detachment prevention
- **EmergencyStop**: Hardware and software emergency stop system
- **SafetyManager**: Coordinates all safety systems

### User Interface (`src/gui/`)

- **MainWindow**: Main application window optimized for 50-inch displays
- **Touch Components**: Touch-optimized controls and widgets
- **Real-time Displays**: Pressure charts, gauges, and status indicators

### Testing Framework (`tests/`)

- **Comprehensive Testing**: Unit, integration, performance, and UI tests
- **Mock Objects**: Hardware simulation for testing
- **Automated Testing**: Continuous integration support

### Performance Systems (`src/performance/`)

- **Multi-threading**: 50Hz data, 30 FPS GUI, 100Hz safety monitoring
- **Memory Management**: Advanced memory optimization
- **Performance Monitoring**: Real-time performance tracking

## Build System

### CMake Configuration

- **Modular Build**: Separate libraries for each component
- **Cross-platform**: Supports Linux (Raspberry Pi) and development platforms
- **Testing Integration**: Automated test execution
- **Documentation Generation**: Doxygen integration

### Dependencies

- **Qt 6.2+**: GUI framework and core libraries
- **CMake 3.16+**: Build system
- **C++17**: Programming language standard
- **WiringPi**: Raspberry Pi GPIO library (optional)

## Deployment Structure

### Production Deployment

```
/opt/vacuum-controller/
├── bin/VacuumController        # Main executable
├── lib/                        # Shared libraries
├── config/                     # Configuration files
├── logs/                       # Log files
├── data/                       # Runtime data
└── docs/                       # Documentation
```

### Development Environment

```
~/vacuum-controller/
├── build/                      # Build directory
├── install/                    # Local installation
└── [source directories]       # Source code
```

This structure provides a clear separation of concerns, making the codebase maintainable, testable, and suitable for medical device development standards.
