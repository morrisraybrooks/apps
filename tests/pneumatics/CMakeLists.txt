# Pneumatics unit tests (independent of the legacy BUILD_TESTS suite).
find_package(Qt5 REQUIRED COMPONENTS Core Test)

function(pneumatics_test name)
    add_executable(tst_${name} tst_${name}.cpp)
    set_target_properties(tst_${name} PROPERTIES AUTOMOC ON)
    target_link_libraries(tst_${name} PRIVATE PneumaticsLogic Qt5::Test)
    target_compile_definitions(tst_${name} PRIVATE
        PNEUMATICS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME pneumatics_${name} COMMAND tst_${name})
endfunction()

pneumatics_test(pulsercalibration)
pneumatics_test(tanklogic)
pneumatics_test(chamberregulator)
pneumatics_test(sealwatch)
pneumatics_test(pulserdrive)
pneumatics_test(softwarepwm)
pneumatics_test(pneumaticcontroller)
