cmake_minimum_required(VERSION 3.16)
project(VacuumController VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

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

# Source files
set(SOURCES
    src/main.cpp
    src/VacuumController.cpp
    src/hardware/HardwareManager.cpp
    src/hardware/SensorInterface.cpp
    src/hardware/ActuatorControl.cpp
    src/hardware/MCP3008.cpp
    src/gui/MainWindow.cpp
    src/gui/PressureMonitor.cpp
    src/gui/PatternSelector.cpp
    src/gui/SafetyPanel.cpp
    src/gui/SettingsDialog.cpp
    src/gui/components/PressureGauge.cpp
    src/gui/components/PressureChart.cpp
    src/gui/components/TouchButton.cpp
    src/gui/components/StatusIndicator.cpp
    src/patterns/PatternEngine.cpp
    src/patterns/PatternDefinitions.cpp
    src/safety/SafetyManager.cpp
    src/safety/AntiDetachmentMonitor.cpp
    src/safety/EmergencyStop.cpp
    src/threading/ThreadManager.cpp
    src/threading/DataAcquisitionThread.cpp
    src/threading/GuiUpdateThread.cpp
)

# Header files
set(HEADERS
    src/VacuumController.h
    src/hardware/HardwareManager.h
    src/hardware/SensorInterface.h
    src/hardware/ActuatorControl.h
    src/hardware/MCP3008.h
    src/gui/MainWindow.h
    src/gui/PressureMonitor.h
    src/gui/PatternSelector.h
    src/gui/SafetyPanel.h
    src/gui/SettingsDialog.h
    src/gui/components/PressureGauge.h
    src/gui/components/PressureChart.h
    src/gui/components/TouchButton.h
    src/gui/components/StatusIndicator.h
    src/patterns/PatternEngine.h
    src/patterns/PatternDefinitions.h
    src/safety/SafetyManager.h
    src/safety/AntiDetachmentMonitor.h
    src/safety/EmergencyStop.h
    src/threading/ThreadManager.h
    src/threading/DataAcquisitionThread.h
    src/threading/GuiUpdateThread.h
)

# Create executable
add_executable(VacuumController ${SOURCES} ${HEADERS})

# Link Qt libraries
target_link_libraries(VacuumController 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Charts
    pthread
    wiringPi
)

# Compiler flags for Raspberry Pi optimization
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    target_compile_options(VacuumController PRIVATE -mcpu=cortex-a72 -mfpu=neon-fp-armv8)
endif()

# Install target
install(TARGETS VacuumController DESTINATION bin)

# Copy configuration files
install(FILES config/patterns.json DESTINATION share/vacuum-controller)
install(FILES config/settings.json DESTINATION share/vacuum-controller)
