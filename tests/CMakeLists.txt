# Test Framework CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find Qt Test module
find_package(Qt6 REQUIRED COMPONENTS Test)

# Include parent directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/hardware)
include_directories(${CMAKE_SOURCE_DIR}/src/safety)
include_directories(${CMAKE_SOURCE_DIR}/src/patterns)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/src/gui/components)
include_directories(${CMAKE_SOURCE_DIR}/src/error)
include_directories(${CMAKE_SOURCE_DIR}/src/logging)
include_directories(${CMAKE_SOURCE_DIR}/src/performance)
include_directories(${CMAKE_SOURCE_DIR}/src/reporting)

# Test framework sources
set(TEST_FRAMEWORK_SOURCES
    TestFramework.cpp
    TestRunner.cpp
    SafetySystemTests.cpp
    HardwareTests.cpp
    PatternTests.cpp
    GUITests.cpp
    PerformanceTests.cpp
    IntegrationTests.cpp
)

set(TEST_FRAMEWORK_HEADERS
    TestFramework.h
    TestRunner.h
    SafetySystemTests.h
    HardwareTests.h
    PatternTests.h
    GUITests.h
    PerformanceTests.h
    IntegrationTests.h
)

# Mock objects for testing
set(MOCK_SOURCES
    mocks/MockHardwareManager.cpp
    mocks/MockSensorInterface.cpp
    mocks/MockActuatorControl.cpp
    mocks/MockVacuumController.cpp
)

set(MOCK_HEADERS
    mocks/MockHardwareManager.h
    mocks/MockSensorInterface.h
    mocks/MockActuatorControl.h
    mocks/MockVacuumController.h
)

# Create test framework library
add_library(VacuumTestFramework STATIC
    ${TEST_FRAMEWORK_SOURCES}
    ${TEST_FRAMEWORK_HEADERS}
    ${MOCK_SOURCES}
    ${MOCK_HEADERS}
)

target_link_libraries(VacuumTestFramework
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Charts
    VacuumControllerLib
)

# Individual test executables
add_executable(SafetySystemTests
    SafetySystemTests.cpp
    TestMain.cpp
)

target_link_libraries(SafetySystemTests
    VacuumTestFramework
    Qt6::Test
)

add_test(NAME SafetySystemTests COMMAND SafetySystemTests)

add_executable(HardwareTests
    HardwareTests.cpp
    TestMain.cpp
)

target_link_libraries(HardwareTests
    VacuumTestFramework
    Qt6::Test
)

add_test(NAME HardwareTests COMMAND HardwareTests)

add_executable(PatternTests
    PatternTests.cpp
    TestMain.cpp
)

target_link_libraries(PatternTests
    VacuumTestFramework
    Qt6::Test
)

add_test(NAME PatternTests COMMAND PatternTests)

add_executable(GUITests
    GUITests.cpp
    TestMain.cpp
)

target_link_libraries(GUITests
    VacuumTestFramework
    Qt6::Test
)

add_test(NAME GUITests COMMAND GUITests)

add_executable(PerformanceTests
    PerformanceTests.cpp
    TestMain.cpp
)

target_link_libraries(PerformanceTests
    VacuumTestFramework
    Qt6::Test
)

add_test(NAME PerformanceTests COMMAND PerformanceTests)

add_executable(IntegrationTests
    IntegrationTests.cpp
    TestMain.cpp
)

target_link_libraries(IntegrationTests
    VacuumTestFramework
    Qt6::Test
)

add_test(NAME IntegrationTests COMMAND IntegrationTests)

# Comprehensive test runner
add_executable(VacuumTestRunner
    TestRunner.cpp
    TestRunnerMain.cpp
)

target_link_libraries(VacuumTestRunner
    VacuumTestFramework
    Qt6::Test
)

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
    DEPENDS SafetySystemTests HardwareTests PatternTests GUITests PerformanceTests IntegrationTests
    COMMENT "Running all vacuum controller tests"
)

add_custom_target(run_safety_tests
    COMMAND SafetySystemTests
    DEPENDS SafetySystemTests
    COMMENT "Running safety system tests"
)

add_custom_target(run_hardware_tests
    COMMAND HardwareTests
    DEPENDS HardwareTests
    COMMENT "Running hardware tests"
)

add_custom_target(run_pattern_tests
    COMMAND PatternTests
    DEPENDS PatternTests
    COMMENT "Running pattern tests"
)

add_custom_target(run_gui_tests
    COMMAND GUITests
    DEPENDS GUITests
    COMMENT "Running GUI tests"
)

add_custom_target(run_performance_tests
    COMMAND PerformanceTests
    DEPENDS PerformanceTests
    COMMENT "Running performance tests"
)

add_custom_target(run_integration_tests
    COMMAND IntegrationTests
    DEPENDS IntegrationTests
    COMMENT "Running integration tests"
)

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
