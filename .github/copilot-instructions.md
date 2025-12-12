# AI Coding Agent Instructions for Vacuum Controller GUI

This document guides AI agents on essential patterns, architecture, and workflows for the V-Contour dual-therapy vacuum controller system.

## Project Overview

**V-Contour** is an electromechanical vacuum therapy device GUI running on Raspberry Pi 4 with Qt 5. The system coordinates precise hardware control, safety monitoring, pattern execution, and real-time visualization across multiple threads.

### Critical Differentiator: Dual-Chamber Architecture
- **Outer V-seal**: Sustained vacuum (30-50 mmHg) for tissue engorgement via SOL1/SOL2
- **Clitoral cylinder**: Sustained vacuum OR oscillating air-pulse (5-13 Hz) via SOL4/SOL5
- This enables active engorgement *before* stimulation, unlike commercial air-pulse toys

## Architecture Overview

### Core System Layers
1. **Hardware Abstraction** (`src/hardware/`): Unified interface to GPIO, sensors, actuators, MCP3008 ADC
2. **Pattern Engine** (`src/patterns/`): State machine-driven execution of 15+ vacuum patterns with precise timing
3. **Safety Management** (`src/safety/`): Overpressure protection, emergency stops, anti-detachment monitoring
4. **Multi-threaded Control** (`src/threading/`): 50Hz sensors, 30 FPS GUI, 100Hz safety checks running in parallel
5. **GUI Layer** (`src/gui/`): Touch-optimized for 50-inch displays with embedded widgets (no modals)

### Data Flow
```
VacuumController (main hub)
├─ HardwareManager → SensorInterface (MCP3008 pressure readings)
├─ PatternEngine → ActuatorControl (solenoid/pump signals)
├─ SafetyManager → monitors pressure/sensors, triggers emergency stops
├─ ThreadManager → coordinates DataAcquisitionThread, GuiUpdateThread, SafetyMonitorThread
└─ PatternDefinitions/PatternTemplateManager → JSON config-driven patterns
```

## Build & Development Workflow

### Build System (CMake 3.16+)
**Key files**: `CMakeLists.txt`, `setup-dev-environment.sh`

**Build commands**:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DRASPBERRY_PI_OPTIMIZATIONS=ON
cmake --build build -j$(nproc)
cmake --install build
```

**Build options**:
- `-DENABLE_COVERAGE=ON`: Enable code coverage (debug only)
- `-DENABLE_STATIC_ANALYSIS=ON`: Run clang-tidy, cppcheck
- `-DENABLE_SANITIZERS=ON`: AddressSanitizer + UndefinedBehaviorSanitizer (debug only)
- `-DRASPBERRY_PI_OPTIMIZATIONS=ON`: ARM Cortex-A72 flags for Pi 4

**Compiler settings**:
- Debug: `-g -O0 -DDEBUG -Wall -Wextra -Wpedantic`
- Release (Pi): `-O3 -march=native -mcpu=cortex-a72 -mfpu=neon-fp-armv8`

### Critical Dependencies
- Qt 5.15.2+ (Core, Widgets, Charts, Sql, Multimedia, WebSockets)
- libgpiod v2.2.1+ (modern GPIO, replaces deprecated wiringPi)
- MCP3008 ADC via SPI for pressure sensors
- systemd for service integration on Pi

### Testing & Code Quality
**CI Pipeline** (`.github/workflows/ci.yml`):
- clang-format (code style)
- clang-tidy (modern C++ best practices)
- cppcheck (static analysis)
- Multi-platform builds: Linux x86_64 + ARM64
- Code coverage with lcov

**Run tests**: `./scripts/run_tests.sh` or `cmake --build build --target test`

## Key Design Patterns

### Manager Pattern (QObject-based)
All major subsystems use Manager classes inheriting from QObject with signal/slot architecture:
```cpp
class HardwareManager : public QObject { /* hardware abstraction */ };
class SafetyManager : public QObject { /* safety coordination */ };
class PatternEngine : public QObject { /* pattern execution */ };
class ThreadManager : public QObject { /* thread lifecycle */ };
```
**Pattern usage**: Initialize in `VacuumController::initialize()`, access via public getters, emit signals for status changes.

### Configuration-Driven Patterns
Pattern definitions stored in JSON (`config/patterns.json`, `config/therapeutic_patterns.json`):
```json
{
  "name": "Slow Pulse",
  "type": "pulse",
  "pulse_duration_ms": 2000,
  "pressure_percent": 60
}
```
**Key insight**: PatternEngine/PatternTemplateManager load JSON at runtime—avoid hardcoding pattern logic.

### Thread Architecture
- **DataAcquisitionThread** (50Hz): Reads MCP3008 pressure sensors, updates `m_avlPressure`, `m_tankPressure`
- **GuiUpdateThread** (30 FPS): Emits signals for chart/gauge updates, non-blocking
- **SafetyMonitorThread** (100Hz): Checks overpressure, sensor timeouts, triggers emergency stops
- **Main GUI thread**: Qt event loop handles user input and pattern control

**Critical**: All manager-to-thread communication via signals/slots. Use QMutex for shared state access.

### Safety-First Design
1. **Overpressure limits**: Max 75 mmHg (MPX5010DP sensor range)
2. **Anti-detachment**: Monitors outer chamber pressure drop; triggers emergency stop if cup detaches
3. **Sensor error recovery**: 1000ms timeout before safety action
4. **Emergency stop**: Halts pump, vents all chambers, disables pattern execution immediately

**Pattern in code**: Safety checks happen *before* hardware commands in PatternEngine::executeStep().

## File Organization Reference

### Core Logic
- `src/VacuumController.h/.cpp`: Main coordinator, system state machine
- `src/hardware/HardwareManager.h/.cpp`: GPIO/SPI abstraction, sensor/actuator API
- `src/patterns/PatternEngine.h/.cpp`: Pattern state machine, timing control
- `src/safety/SafetyManager.h/.cpp`: Overpressure/emergency handling
- `src/threading/ThreadManager.h/.cpp`: Thread lifecycle, synchronization

### GUI Components
- `src/gui/MainWindow.h/.cpp`: Top-level window, layout manager
- `src/gui/PressureMonitor.h/.cpp`: Real-time chart widget
- `src/gui/components/PressureGauge.h/.cpp`: Visual gauge display
- `src/gui/PatternSelector.h/.cpp`: Pattern selection embedded widget

### Configuration & Data
- `config/patterns.json`: Standard pattern library
- `config/settings.json`: User preferences
- `config/therapeutic_patterns.json`: Medical/therapeutic patterns

### Testing
- `tests/SafetySystemTests.h/.cpp`: Safety critical testing
- `tests/PatternTests.h/.cpp`: Pattern execution validation
- `tests/mocks/MockHardwareManager.h/.cpp`: Simulation for testing

## Common Development Tasks

### Adding a New Pattern
1. Define JSON in `config/patterns.json` with name, type, durations, pressures
2. PatternTemplateManager auto-loads at startup
3. PatternEngine::executeStep() reads JSON and applies solenoid/pump commands
4. No code changes needed for simple patterns

### Modifying Hardware Interface
1. Update `HardwareManager::setSOL*()` or new sensor read methods
2. Test with `SimulationMode` enabled: `VacuumController::setSimulationMode(true)`
3. Use mock objects from `tests/mocks/` in unit tests

### Debugging Multi-threaded Issues
1. Enable debug build: `-DCMAKE_BUILD_TYPE=Debug`
2. Run with sanitizers: `-DENABLE_SANITIZERS=ON`
3. Check thread state: `ThreadManager::getOverallState()`
4. Use Qt signals/slots debugger or threadstatus logging

### Emergency Stop Handling
- Any code path can call `SafetyManager::triggerEmergencyStop(reason)`
- This vents all chambers and halts PatternEngine immediately
- Recovery: `SafetyManager::resetEmergencyStop()` only after manual verification

## Deployment & CI/CD

### GitHub Actions Workflows
- **CI** (`.github/workflows/ci.yml`): On push/PR—code quality, multi-arch builds, tests
- **Release** (`.github/workflows/release.yml`): Semantic versioning, .deb packages, Doxygen docs

### Packaging for Raspberry Pi
- Debian .deb packages include systemd service, udev rules, configuration files
- Service user: `vacuum-controller` with GPIO/SPI permissions
- Config paths: `/etc/vacuum-controller/`, logs: `/var/log/vacuum-controller/`

### Version Management
Run `./scripts/version.sh bump minor` to increment version in CMakeLists.txt and git tag.

## Conventions & Constraints

- **C++ Standard**: C++17 (use `auto`, smart pointers, structured bindings)
- **Naming**: CamelCase for classes, snake_case for variables/functions
- **Signals**: Emit before state changes; slots handle reactions
- **Pressure units**: Always mmHg internally; never mix with kPa
- **Timing**: Use QTimer with signals, not sleep loops
- **No blocking operations**: Avoid long computations in GUI thread
- **Error handling**: Catch exceptions, log via ErrorManager, emit safety signals

## Useful Commands

```bash
# Full development build with testing
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON -DENABLE_STATIC_ANALYSIS=ON
cmake --build build
cmake --build build --target test
cmake --build build --target coverage

# Cross-compile for Raspberry Pi (requires ARM toolchain)
cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=<path> -DRASPBERRY_PI_OPTIMIZATIONS=ON
cmake --build build-arm

# Format code
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run static analysis
cppcheck --enable=all --std=c++17 src/
clang-tidy -p build src/**/*.cpp
```

## Documentation Resources

- **API Reference**: `docs/API_REFERENCE.md` (Doxygen-generated)
- **User Manual**: `docs/USER_MANUAL.md`
- **Hardware Setup**: `docs/HARDWARE_TESTING.md`
- **Development Practices**: `DEVELOPMENT_PRACTICES.md`
- **Architecture**: `PROJECT_STRUCTURE.md`
- **Pattern Design**: `docs/AUTOMATED_ORGASM_PATTERNS.md`

