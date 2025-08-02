# Hardware Testing Guide

This document describes the comprehensive hardware testing capabilities of the Vacuum Controller system.

## Overview

The Vacuum Controller includes a built-in hardware testing system that can validate all components including sensors, actuators, safety systems, and communication interfaces. Tests can be run from the command line or integrated into automated testing workflows.

## Command Line Testing

### Basic Usage

```bash
# Run sensor tests
./vacuum_controller --test-sensors

# Run actuator tests  
./vacuum_controller --test-actuators

# Run comprehensive tests
./vacuum_controller --test-all
```

### Advanced Options

```bash
# Enable verbose output
./vacuum_controller --test-all --verbose

# Save results to file
./vacuum_controller --test-sensors --output sensor_test_results

# Set custom timeout (in seconds)
./vacuum_controller --test-all --timeout 60

# Continue testing after failures
./vacuum_controller --test-all --continue-on-failure

# Combined example
./vacuum_controller --test-all --verbose --output full_test --timeout 120 --continue-on-failure
```

## Test Categories

### 1. Sensor Tests (`--test-sensors`)

Tests all pressure sensors for functionality and accuracy:

- **AVL Sensor Test**: Validates AVL pressure sensor readings
  - Takes multiple samples to check stability
  - Calculates coefficient of variation (CV)
  - Passes if CV < 5%

- **Tank Sensor Test**: Validates tank pressure sensor readings
  - Similar stability testing as AVL sensor
  - Checks for consistent readings over time

**Expected Results:**
- Stable pressure readings
- Low coefficient of variation
- No communication errors

### 2. Actuator Tests (`--test-actuators`)

Tests all actuators for proper operation:

- **Pump Control Test**: Validates vacuum pump operation
  - Tests enable/disable functionality
  - Tests speed control at multiple setpoints (25%, 50%, 75%, 100%)
  - Verifies actual speed matches target within 2% tolerance

- **SOL1 Valve Test**: Tests AVL valve operation
  - Tests open/close functionality
  - Verifies state feedback

- **SOL2 Valve Test**: Tests AVL vent valve operation
- **SOL3 Valve Test**: Tests tank vent valve operation

**Expected Results:**
- Accurate speed control
- Reliable valve operation
- Proper state feedback

### 3. Communication Tests

Tests hardware communication interfaces:

- **SPI Communication Test**: Validates MCP3008 ADC communication
  - Tests reading from both sensor channels
  - Verifies data integrity

- **GPIO Test**: Validates GPIO pin functionality
  - Tests all valve control pins
  - Verifies input/output operations

**Expected Results:**
- Successful SPI transactions
- Reliable GPIO operations
- No communication timeouts

### 4. Safety Tests

Tests safety system functionality:

- **Emergency Stop Test**: Validates emergency stop system
  - Tests emergency stop activation
  - Verifies system shutdown

**Expected Results:**
- Immediate emergency stop response
- Proper system state changes

## Test Results

### Output Format

Tests generate detailed results in multiple formats:

1. **Console Output**: Real-time test progress and results
2. **JSON Reports**: Detailed test data for analysis
3. **Summary Reports**: High-level test statistics

### Result Interpretation

Each test can have one of these results:

- **PASSED**: Test completed successfully
- **FAILED**: Test failed - hardware issue detected
- **WARNING**: Test completed with minor issues
- **SKIPPED**: Test was not executed
- **IN PROGRESS**: Test is currently running

### Sample Output

```
=== Test Results ===
Total Tests: 9
Passed: 8
Failed: 1
Warnings: 0
Skipped: 0
Duration: 45.2 seconds

=== Failed Tests ===
- Pump Control Test: Speed control error: target=75%, actual=72%, error=4%
```

## Automated Testing Script

Use the provided script for comprehensive testing:

```bash
# Run the automated test script
./scripts/test_hardware.sh
```

This script:
- Runs all test categories
- Generates detailed reports
- Creates timestamped result directories
- Provides recommendations based on results

## Troubleshooting

### Common Issues

1. **SPI Communication Failures**
   - Check SPI wiring connections
   - Verify MCP3008 power supply
   - Ensure correct GPIO pin configuration

2. **Sensor Reading Instability**
   - Check sensor power supply
   - Verify analog connections
   - Consider environmental factors (temperature, vibration)

3. **Actuator Control Issues**
   - Check GPIO connections
   - Verify actuator power supply
   - Test manual actuator operation

4. **Permission Errors**
   - Ensure user has GPIO access permissions
   - Run with appropriate privileges if needed

### Hardware Validation Checklist

Before running tests, verify:

- [ ] All hardware connections are secure
- [ ] Power supplies are stable
- [ ] GPIO permissions are configured
- [ ] SPI interface is enabled
- [ ] No other processes are using GPIO pins

## Integration with CI/CD

The testing system can be integrated into automated workflows:

```bash
# Example CI script
#!/bin/bash
set -e

# Build the application
mkdir -p build && cd build
cmake .. && make

# Run hardware tests
../vacuum_controller --test-all --output ci_test_results --timeout 300

# Check exit code
if [ $? -eq 0 ]; then
    echo "All hardware tests passed"
    exit 0
else
    echo "Hardware tests failed"
    exit 1
fi
```

## Performance Benchmarks

Expected test durations:

- Sensor Tests: ~15 seconds
- Actuator Tests: ~20 seconds
- Communication Tests: ~5 seconds
- Safety Tests: ~10 seconds
- Comprehensive Tests: ~45 seconds

## Safety Considerations

⚠️ **Important Safety Notes:**

- Tests may activate actuators and change system state
- Ensure system is in a safe configuration before testing
- Emergency stop should be easily accessible during testing
- Some tests may generate vacuum or pressure changes
- Always follow proper safety procedures

## Support

For issues with hardware testing:

1. Check the troubleshooting section above
2. Review test output files for detailed error information
3. Verify hardware connections and configuration
4. Consult the main system documentation

## Test Development

To add new tests:

1. Add test case to `HardwareTester::setupTestCases()`
2. Implement test logic in appropriate `perform*Test()` method
3. Update command line options if needed
4. Add documentation to this guide
