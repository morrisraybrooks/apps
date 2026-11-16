# Hardware-free pneumatics logic (tank, chambers, seal watch, pulser).
# Unit-tested in tests/pneumatics; linked into VacuumControllerLib.
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(Threads REQUIRED)

add_library(PneumaticsLogic STATIC
    PulserCalibration.cpp
    TankLogic.cpp
    ChamberRegulator.cpp
    SealWatch.cpp
    PulserDrive.cpp
    SoftwarePwm.cpp
)
set_target_properties(PneumaticsLogic PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON)
target_include_directories(PneumaticsLogic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/pneumatics>)
target_link_libraries(PneumaticsLogic PUBLIC Qt5::Core Threads::Threads)

# Part of the installed VacuumControllerLib export set (it links PneumaticsLogic publicly)
install(TARGETS PneumaticsLogic EXPORT VacuumControllerTargets ARCHIVE DESTINATION lib)
