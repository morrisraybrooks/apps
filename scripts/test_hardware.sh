#!/bin/bash

# Hardware Testing Script for Vacuum Controller
# This script demonstrates the various hardware testing capabilities

echo "=== Vacuum Controller Hardware Testing Script ==="
echo

# Check if the vacuum controller executable exists
VACUUM_CONTROLLER="./build/vacuum_controller"

if [ ! -f "$VACUUM_CONTROLLER" ]; then
    echo "Error: Vacuum controller executable not found at $VACUUM_CONTROLLER"
    echo "Please build the project first using:"
    echo "  mkdir -p build && cd build"
    echo "  cmake .. && make"
    exit 1
fi

# Create test results directory
TEST_DIR="test_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TEST_DIR"
echo "Test results will be saved to: $TEST_DIR"
echo

# Function to run a test and capture results
run_test() {
    local test_name="$1"
    local test_args="$2"
    local output_file="$TEST_DIR/${test_name}_output.txt"
    local report_file="$TEST_DIR/${test_name}"
    
    echo "Running $test_name..."
    echo "Command: $VACUUM_CONTROLLER $test_args --output $report_file --verbose"
    
    if $VACUUM_CONTROLLER $test_args --output "$report_file" --verbose > "$output_file" 2>&1; then
        echo "✓ $test_name completed successfully"
        return 0
    else
        echo "✗ $test_name failed"
        echo "Check $output_file for details"
        return 1
    fi
}

# Test 1: Sensor Tests
echo "1. Testing Sensors..."
run_test "sensor_tests" "--test-sensors"
echo

# Test 2: Actuator Tests
echo "2. Testing Actuators..."
run_test "actuator_tests" "--test-actuators"
echo

# Test 3: Comprehensive Tests
echo "3. Running Comprehensive Tests..."
run_test "comprehensive_tests" "--test-all --continue-on-failure --timeout 60"
echo

# Test 4: Individual Component Tests (if comprehensive tests failed)
echo "4. Individual Component Tests..."

# Check if we can run individual tests
echo "Testing individual components..."

# Test AVL Sensor
echo "  - Testing AVL Sensor..."
if ! $VACUUM_CONTROLLER --test-sensors --output "$TEST_DIR/avl_sensor" --verbose | grep -q "AVL Sensor.*PASSED"; then
    echo "    ⚠ AVL Sensor test issues detected"
fi

# Test Tank Sensor
echo "  - Testing Tank Sensor..."
if ! $VACUUM_CONTROLLER --test-sensors --output "$TEST_DIR/tank_sensor" --verbose | grep -q "Tank Sensor.*PASSED"; then
    echo "    ⚠ Tank Sensor test issues detected"
fi

# Test Pump
echo "  - Testing Pump Control..."
if ! $VACUUM_CONTROLLER --test-actuators --output "$TEST_DIR/pump_test" --verbose | grep -q "Pump.*PASSED"; then
    echo "    ⚠ Pump control test issues detected"
fi

echo

# Generate summary report
echo "5. Generating Summary Report..."
SUMMARY_FILE="$TEST_DIR/test_summary.txt"

cat > "$SUMMARY_FILE" << EOF
=== Vacuum Controller Hardware Test Summary ===
Test Date: $(date)
Test Directory: $TEST_DIR

Test Results:
EOF

# Count passed/failed tests from output files
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

for output_file in "$TEST_DIR"/*_output.txt; do
    if [ -f "$output_file" ]; then
        test_name=$(basename "$output_file" _output.txt)
        echo "  $test_name:" >> "$SUMMARY_FILE"
        
        # Extract test results
        if grep -q "PASSED" "$output_file"; then
            passed=$(grep -c "PASSED" "$output_file")
            PASSED_TESTS=$((PASSED_TESTS + passed))
            echo "    Passed: $passed" >> "$SUMMARY_FILE"
        fi
        
        if grep -q "FAILED" "$output_file"; then
            failed=$(grep -c "FAILED" "$output_file")
            FAILED_TESTS=$((FAILED_TESTS + failed))
            echo "    Failed: $failed" >> "$SUMMARY_FILE"
        fi
        
        echo "" >> "$SUMMARY_FILE"
    fi
done

TOTAL_TESTS=$((PASSED_TESTS + FAILED_TESTS))

cat >> "$SUMMARY_FILE" << EOF

Overall Summary:
  Total Tests: $TOTAL_TESTS
  Passed: $PASSED_TESTS
  Failed: $FAILED_TESTS
  Success Rate: $(( PASSED_TESTS * 100 / TOTAL_TESTS ))%

Files Generated:
EOF

# List all generated files
for file in "$TEST_DIR"/*; do
    if [ -f "$file" ]; then
        echo "  - $(basename "$file")" >> "$SUMMARY_FILE"
    fi
done

echo "✓ Summary report generated: $SUMMARY_FILE"
echo

# Display summary
echo "=== Test Summary ==="
cat "$SUMMARY_FILE"
echo

# Recommendations
echo "=== Recommendations ==="
if [ $FAILED_TESTS -eq 0 ]; then
    echo "✓ All hardware tests passed successfully!"
    echo "  The vacuum controller hardware is functioning correctly."
elif [ $FAILED_TESTS -lt 3 ]; then
    echo "⚠ Some tests failed, but most hardware is functional."
    echo "  Check the failed test details in the output files."
    echo "  Consider recalibrating or checking connections for failed components."
else
    echo "✗ Multiple hardware tests failed."
    echo "  This indicates potential hardware issues that need attention."
    echo "  Review all test output files and check:"
    echo "    - Hardware connections"
    echo "    - Power supply"
    echo "    - Component calibration"
    echo "    - GPIO/SPI configuration"
fi

echo
echo "Test results saved in: $TEST_DIR"
echo "Use the JSON report files for detailed analysis."
echo

# Optional: Open test directory if running on desktop
if command -v xdg-open >/dev/null 2>&1; then
    echo "Opening test results directory..."
    xdg-open "$TEST_DIR" 2>/dev/null &
fi

exit 0
