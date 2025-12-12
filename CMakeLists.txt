cmake_minimum_required(VERSION 3.16)

# Project configuration with semantic versioning
project(VacuumController
    VERSION 1.0.0
    DESCRIPTION "Industrial Vacuum Controller GUI System for Raspberry Pi"
    HOMEPAGE_URL "https://github.com/morrisraybrooks/vacuum-controller"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Platform detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(RASPBERRY_PI TRUE)
    message(STATUS "Building for Raspberry Pi (${CMAKE_SYSTEM_PROCESSOR})")
else()
    set(RASPBERRY_PI FALSE)
    message(STATUS "Building for generic platform (${CMAKE_SYSTEM_PROCESSOR})")
endif()

# Build options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_DOCS "Build documentation" ON)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(ENABLE_SANITIZERS "Enable runtime sanitizers" OFF)
option(RASPBERRY_PI_OPTIMIZATIONS "Enable Raspberry Pi specific optimizations" ${RASPBERRY_PI})
option(INSTALL_SYSTEMD_SERVICE "Install systemd service files" ON)

# Compiler configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra -Wpedantic")
    add_definitions(-DVACUUM_DEBUG_MODE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
    if(RASPBERRY_PI_OPTIMIZATIONS)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=cortex-a72 -mfpu=neon-fp-armv8")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
endif()

# Sanitizers (Debug builds only)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
endif()

# Code coverage (Debug builds only)
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
endif()

# Find required packages
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Charts Sql Multimedia WebSockets SerialPort)
find_package(PkgConfig REQUIRED)

# Optional OpenCV for camera-based motion detection
find_package(OpenCV QUIET COMPONENTS core imgproc video videoio)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    add_definitions(-DHAVE_OPENCV)
else()
    message(STATUS "OpenCV not found - camera motion detection will be disabled")
endif()

# System dependencies - prioritize /usr/local for libgpiod v2.x
set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(GPIODCXX REQUIRED libgpiodcxx)

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/hardware)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/src/gui/components)
include_directories(${CMAKE_SOURCE_DIR}/src/patterns)
include_directories(${CMAKE_SOURCE_DIR}/src/safety)
include_directories(${CMAKE_SOURCE_DIR}/src/threading)
include_directories(${CMAKE_SOURCE_DIR}/src/calibration)
include_directories(${CMAKE_SOURCE_DIR}/src/error)
include_directories(${CMAKE_SOURCE_DIR}/src/testing)
include_directories(${CMAKE_SOURCE_DIR}/src/control)
include_directories(${CMAKE_SOURCE_DIR}/src/game)
include_directories(${CMAKE_SOURCE_DIR}/src/network)
include_directories(${CMAKE_SOURCE_DIR}/src/licensing)
include_directories(${CMAKE_SOURCE_DIR}/src/admin)

# Source files
set(SOURCES
    src/main.cpp
    src/VacuumController.cpp
    src/hardware/HardwareManager.cpp
    src/hardware/SensorInterface.cpp
    src/hardware/ActuatorControl.cpp
    src/hardware/MCP3008.cpp
    src/hardware/ClitoralOscillator.cpp
    src/hardware/TENSController.cpp
    src/hardware/HeartRateSensor.cpp
    src/hardware/FluidSensor.cpp
    src/hardware/MotionSensor.cpp
    src/hardware/CameraMotionSensor.cpp
    src/control/OrgasmControlAlgorithm.cpp
    src/gui/MainWindow.cpp
    src/gui/MotionMonitor.cpp
    src/gui/CameraMonitor.cpp
    src/gui/PressureMonitor.cpp
    src/gui/FluidMonitor.cpp
    src/gui/PatternSelector.cpp
    src/gui/SafetyPanel.cpp
    src/gui/SettingsPanel.cpp
    src/gui/CustomPatternEditor.cpp
    src/gui/SystemDiagnosticsPanel.cpp
    src/gui/CalibrationInterface.cpp
    src/gui/components/PressureGauge.cpp
    src/gui/components/PressureChart.cpp
    src/gui/components/TouchButton.cpp
    src/gui/components/StatusIndicator.cpp
    src/gui/styles/ModernMedicalStyle.cpp
    src/patterns/PatternEngine.cpp
    src/patterns/PatternDefinitions.cpp
    src/patterns/PatternValidator.cpp
    src/patterns/PatternTemplateManager.cpp
    src/safety/SafetyManager.cpp
    src/safety/AntiDetachmentMonitor.cpp
    src/safety/EmergencyStop.cpp
    src/threading/ThreadManager.cpp
    src/threading/DataAcquisitionThread.cpp
    src/threading/GuiUpdateThread.cpp
    src/threading/SafetyMonitorThread.cpp
    src/calibration/CalibrationManager.cpp
    src/error/CrashHandler.cpp
    src/error/ErrorManager.cpp
    src/testing/HardwareTester.cpp
    src/logging/DataLogger.cpp
    src/performance/PerformanceMonitor.cpp
    src/performance/MemoryManager.cpp
    src/gui/ParameterAdjustmentPanel.cpp
    src/reporting/DataExporter.cpp
    src/game/GameTypes.h
    src/game/GameDefinition.cpp
    src/game/GameEngine.cpp
    src/game/AchievementSystem.cpp
    src/game/ConsequenceEngine.cpp
    src/game/ProgressTracker.cpp
    src/network/MultiUserController.cpp
    src/gui/PrivilegePanel.cpp
    src/gui/AdminPanel.cpp
    src/gui/ExecutionModeSelector.cpp
    src/licensing/LicenseManager.cpp
    src/licensing/LicenseServer.cpp
    src/admin/AccountManager.cpp
    src/admin/DeviceRegistry.cpp
    src/admin/RemoteMonitor.cpp
)

# Header files
set(HEADERS
    src/VacuumController.h
    src/hardware/HardwareManager.h
    src/hardware/SensorInterface.h
    src/hardware/ActuatorControl.h
    src/hardware/MCP3008.h
    src/hardware/ClitoralOscillator.h
    src/hardware/TENSController.h
    src/hardware/HeartRateSensor.h
    src/hardware/FluidSensor.h
    src/hardware/MotionSensor.h
    src/hardware/CameraMotionSensor.h
    src/control/OrgasmControlAlgorithm.h
    src/gui/MainWindow.h
    src/gui/MotionMonitor.h
    src/gui/CameraMonitor.h
    src/gui/PressureMonitor.h
    src/gui/FluidMonitor.h
    src/gui/PatternSelector.h
    src/gui/SafetyPanel.h
    src/gui/SettingsPanel.h
    src/gui/components/PressureGauge.h
    src/gui/components/PressureChart.h
    src/gui/components/TouchButton.h
    src/gui/components/StatusIndicator.h
    src/gui/styles/ModernMedicalStyle.h
    src/patterns/PatternEngine.h
    src/patterns/PatternDefinitions.h
    src/patterns/PatternValidator.h
    src/patterns/PatternTemplateManager.h
    src/safety/SafetyManager.h
    src/safety/AntiDetachmentMonitor.h
    src/safety/EmergencyStop.h
    src/threading/ThreadManager.h
    src/threading/DataAcquisitionThread.h
    src/threading/GuiUpdateThread.h
    src/threading/SafetyMonitorThread.h
    src/error/ErrorManager.h
    src/performance/PerformanceMonitor.h
    src/performance/MemoryManager.h
    src/gui/ParameterAdjustmentPanel.h
    src/reporting/DataExporter.h
    src/game/GameTypes.h
    src/game/GameDefinition.h
    src/game/GameEngine.h
    src/game/AchievementSystem.h
    src/game/ConsequenceEngine.h
    src/game/ProgressTracker.h
    src/network/MultiUserController.h
    src/gui/PrivilegePanel.h
    src/gui/AdminPanel.h
    src/gui/ExecutionModeSelector.h
    src/licensing/LicenseManager.h
    src/licensing/LicenseServer.h
    src/admin/AccountManager.h
    src/admin/DeviceRegistry.h
    src/admin/RemoteMonitor.h
)

# Create shared library for better testing and modularity
add_library(VacuumControllerLib SHARED ${SOURCES} ${HEADERS})

# Set library properties
set_target_properties(VacuumControllerLib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${HEADERS}"
)

# Library include directories
target_include_directories(VacuumControllerLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/components
        ${CMAKE_CURRENT_SOURCE_DIR}/src/patterns
        ${CMAKE_CURRENT_SOURCE_DIR}/src/safety
        ${CMAKE_CURRENT_SOURCE_DIR}/src/threading
        ${CMAKE_CURRENT_SOURCE_DIR}/src/calibration
        ${CMAKE_CURRENT_SOURCE_DIR}/src/error
        ${CMAKE_CURRENT_SOURCE_DIR}/src/testing
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logging
        ${CMAKE_CURRENT_SOURCE_DIR}/src/performance
        ${CMAKE_CURRENT_SOURCE_DIR}/src/reporting
        ${CMAKE_CURRENT_SOURCE_DIR}/src/control
        ${CMAKE_CURRENT_SOURCE_DIR}/src/game
)

# Link libraries to the shared library
target_link_libraries(VacuumControllerLib
    PUBLIC
        Qt5::Core
        Qt5::Widgets
        Qt5::Charts
        Qt5::Sql
        Qt5::Multimedia
        Qt5::WebSockets
        Qt5::SerialPort
    PRIVATE
        pthread
        ${GPIOD_LIBRARIES}
        ${GPIODCXX_LIBRARIES}
)

# Link OpenCV if available
if(OpenCV_FOUND)
    target_link_libraries(VacuumControllerLib PRIVATE ${OpenCV_LIBS})
    target_include_directories(VacuumControllerLib PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()

# Add library directories for libgpiod v2.x
target_link_directories(VacuumControllerLib PRIVATE /usr/local/lib)
target_include_directories(VacuumControllerLib PRIVATE /usr/local/include)

# Add compiler definitions
target_compile_definitions(VacuumControllerLib
    PRIVATE
        $<$<CONFIG:Debug>:VACUUM_DEBUG_MODE>
        $<$<BOOL:${RASPBERRY_PI}>:RASPBERRY_PI_BUILD>
)

# Create main executable
add_executable(VacuumController src/main.cpp)

# Link executable to library
target_link_libraries(VacuumController
    PRIVATE
        VacuumControllerLib
)

# Testing configuration
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# Static analysis
if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set_target_properties(VacuumControllerLib PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=-*,readability-*,performance-*,modernize-*"
        )
    endif()

    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXE} --enable=all --std=c++17 --project=${CMAKE_BINARY_DIR}/compile_commands.json
            COMMENT "Running cppcheck static analysis"
        )
    endif()
endif()

# Installation configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install targets
install(TARGETS VacuumController VacuumControllerLib
    EXPORT VacuumControllerTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vacuum-controller
)

# Install configuration files
install(FILES
    config/patterns.json
    config/settings.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/vacuum-controller
)

# Install game definition files
install(DIRECTORY data/games/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/vacuum-controller/games
    FILES_MATCHING PATTERN "*.json"
)

# Install documentation
install(DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    FILES_MATCHING PATTERN "*.md"
)

# Install systemd service file (if enabled)
if(INSTALL_SYSTEMD_SERVICE AND RASPBERRY_PI)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/packaging/systemd/vacuum-controller.service.in
        ${CMAKE_CURRENT_BINARY_DIR}/vacuum-controller.service
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vacuum-controller.service
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system
    )
endif()

# Create and install package configuration files
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/VacuumControllerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(EXPORT VacuumControllerTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/VacuumControllerTargets.cmake"
    NAMESPACE VacuumController::
)

configure_file(cmake/VacuumControllerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/VacuumControllerConfig.cmake"
    @ONLY
)

install(EXPORT VacuumControllerTargets
    FILE VacuumControllerTargets.cmake
    NAMESPACE VacuumController::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VacuumController
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/VacuumControllerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VacuumControllerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VacuumController
)

# CPack configuration for package generation
include(CPack)

set(CPACK_PACKAGE_NAME "vacuum-controller")
set(CPACK_PACKAGE_VENDOR "Morris Brooks")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Industrial Vacuum Controller GUI System")
set(CPACK_PACKAGE_DESCRIPTION "A safety-critical vacuum control system with Qt GUI for Raspberry Pi hardware")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "morrisraybrooks@gmail.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/morrisraybrooks/vacuum-controller")

# Debian package configuration
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Morris Brooks <morrisraybrooks@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.12), libqt5widgets5 (>= 5.12), libqt5charts5 (>= 5.12), libgpiod3 (>= 2.0)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "systemd")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "doxygen, valgrind")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/prerm"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postrm"
)

# RPM package configuration (for other distributions)
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase >= 5.12, qt5-qtcharts >= 5.12, libgpiod >= 2.0")

# Archive packages
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# Component configuration
set(CPACK_COMPONENTS_ALL Runtime Development Documentation)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Vacuum Controller Runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main application and runtime files")
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Files")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Headers and libraries for development")
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "User manual and API documentation")

# Custom targets for development workflow
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code with clang-format"
)

add_custom_target(lint
    COMMAND find ${CMAKE_SOURCE_DIR}/src -name "*.cpp" -o -name "*.h" | xargs clang-tidy
    COMMENT "Running clang-tidy linter"
)

# Print build configuration summary
message(STATUS "")
message(STATUS "=== Vacuum Controller Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Raspberry Pi optimizations: ${RASPBERRY_PI_OPTIMIZATIONS}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build documentation: ${BUILD_DOCS}")
message(STATUS "Enable coverage: ${ENABLE_COVERAGE}")
message(STATUS "Enable static analysis: ${ENABLE_STATIC_ANALYSIS}")
message(STATUS "Enable sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "Install systemd service: ${INSTALL_SYSTEMD_SERVICE}")
message(STATUS "============================================")
message(STATUS "")
