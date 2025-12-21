# Test Framework CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find Qt Test module
find_package(Qt5 REQUIRED COMPONENTS Test)

# Include parent directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/hardware)
include_directories(${CMAKE_SOURCE_DIR}/src/safety)
include_directories(${CMAKE_SOURCE_DIR}/src/control)
include_directories(${CMAKE_SOURCE_DIR}/src/patterns)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/src/gui/components)
include_directories(${CMAKE_SOURCE_DIR}/src/error)
include_directories(${CMAKE_SOURCE_DIR}/src/logging)
include_directories(${CMAKE_SOURCE_DIR}/src/performance)
include_directories(${CMAKE_SOURCE_DIR}/src/reporting)

# Test framework sources (core + active test suites)
set(TEST_FRAMEWORK_SOURCES
    TestFramework.cpp
    SafetySystemTests.cpp
    OrgasmControlAlgorithmTests.cpp
)

set(TEST_FRAMEWORK_HEADERS
    TestFramework.h
    SafetySystemTests.h
    OrgasmControlAlgorithmTests.h
)

# Mock objects for testing
set(MOCK_SOURCES
    mocks/MockHardwareManager.cpp
    mocks/MockSensorInterface.cpp
    mocks/MockActuatorControl.cpp
    mocks/MockVacuumController.cpp
    mocks/MockOrgasmControlAlgorithm.cpp
)

set(MOCK_HEADERS
    mocks/MockHardwareManager.h
    mocks/MockSensorInterface.h
    mocks/MockActuatorControl.h
    mocks/MockVacuumController.h
    mocks/MockOrgasmControlAlgorithm.h
)

# Create test framework library
add_library(VacuumTestFramework STATIC
    ${TEST_FRAMEWORK_SOURCES}
    ${TEST_FRAMEWORK_HEADERS}
    ${MOCK_SOURCES}
    ${MOCK_HEADERS}
)

target_link_libraries(VacuumTestFramework
    Qt5::Core
    Qt5::Test
    Qt5::Widgets
    Qt5::Charts
    VacuumControllerLib
)

# Standalone safety system test runner
#
# This executable uses the custom TestFramework/TestRunner pipeline and
# runs the SafetySystemTests suite. It is wired into CTest so that
# `ctest -R SafetySystemTests` exercises all safety invariants,
# including the AVL seal behavior and tissue-damage threshold logic.
add_executable(SafetySystemTests
    TestRunner.cpp
)

target_link_libraries(SafetySystemTests
    VacuumTestFramework
    Qt5::Core
    Qt5::Test
)

add_test(NAME SafetySystemTests COMMAND SafetySystemTests --suite SafetySystem)

# NOTE: SafetySystemTests, HardwareTests, PatternTests, GUITests, PerformanceTests, IntegrationTests
# are temporarily disabled - they need API updates to match the new TestFramework

# GUI Feature Parity Tests (Phase 1-4)
add_executable(ExecutionModeSelectorTests
    gui/test_ExecutionModeSelector.cpp
)

target_link_libraries(ExecutionModeSelectorTests
    VacuumTestFramework
    Qt5::Test
    Qt5::Widgets
    Qt5::Charts
)

add_test(NAME ExecutionModeSelectorTests COMMAND ExecutionModeSelectorTests)

add_executable(ArousalMonitorTests
    gui/test_ArousalMonitor.cpp
)

target_link_libraries(ArousalMonitorTests
    VacuumTestFramework
    Qt5::Test
    Qt5::Widgets
    Qt5::Charts
)

add_test(NAME ArousalMonitorTests COMMAND ArousalMonitorTests)

add_executable(SettingsPanelArousalTests
    gui/test_SettingsPanel_Arousal.cpp
)

target_link_libraries(SettingsPanelArousalTests
    VacuumTestFramework
    Qt5::Test
    Qt5::Widgets
    Qt5::Charts
)

add_test(NAME SettingsPanelArousalTests COMMAND SettingsPanelArousalTests)

# Test data and configuration files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/test_config.json
               ${CMAKE_CURRENT_BINARY_DIR}/test_config.json COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/test_patterns.json
               ${CMAKE_CURRENT_BINARY_DIR}/test_patterns.json COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/safety_test_data.json
               ${CMAKE_CURRENT_BINARY_DIR}/safety_test_data.json COPYONLY)

# Custom test targets
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS SafetySystemTests ExecutionModeSelectorTests ArousalMonitorTests SettingsPanelArousalTests
    COMMENT "Running all vacuum controller tests"
)

add_custom_target(run_safety_tests
    COMMAND SafetySystemTests
    DEPENDS SafetySystemTests
    COMMENT "Running safety system tests"
)

add_custom_target(run_gui_feature_tests
    COMMAND ExecutionModeSelectorTests
    COMMAND ArousalMonitorTests
    COMMAND SettingsPanelArousalTests
    DEPENDS ExecutionModeSelectorTests ArousalMonitorTests SettingsPanelArousalTests
    COMMENT "Running GUI feature parity tests"
)

# NOTE: run_integration_tests disabled - needs API updates

# Test coverage (if gcov is available)
option(ENABLE_COVERAGE "Enable test coverage reporting" OFF)

if(ENABLE_COVERAGE)
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${LCOV_PATH} --list coverage.info
            COMMAND ${GENHTML_PATH} -o coverage coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating test coverage report"
        )
    endif()
endif()

# Valgrind memory testing (if valgrind is available)
find_program(VALGRIND_PATH valgrind)

if(VALGRIND_PATH)
    add_custom_target(memcheck
        COMMAND ${VALGRIND_PATH} --tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes --error-exitcode=1 $<TARGET_FILE:SafetySystemTests>
        DEPENDS SafetySystemTests
        COMMENT "Running memory leak detection"
    )
endif()

# Test result formatting
add_custom_target(test_report
    COMMAND VacuumTestRunner --output-format html --output-path test_report.html
    DEPENDS VacuumTestRunner
    COMMENT "Generating comprehensive test report"
)

# Continuous integration support
add_custom_target(ci_tests
    COMMAND VacuumTestRunner --ci-mode --output-format xml --output-path test_results.xml
    DEPENDS VacuumTestRunner
    COMMENT "Running tests in CI mode"
)
