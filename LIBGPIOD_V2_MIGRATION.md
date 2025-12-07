# libgpiod v2.x Migration Guide for C++

## 🚀 **You're Now Running libgpiod v2.2.1!**

This guide shows the key differences between libgpiod v1.x and v2.x for C++ programming.

## 📋 **Key Changes Summary**

### **1. Header Files**
```cpp
// OLD (v1.x)
#include <gpiod.hpp>

// NEW (v2.x) - Same header, but different namespace
#include <gpiod.hpp>
```

### **2. Namespace Changes**
```cpp
// OLD (v1.x)
using namespace gpiod;

// NEW (v2.x) - More specific namespaces
using namespace gpiod;  // Still works, but more organized
```

### **3. Chip Opening**
```cpp
// OLD (v1.x)
auto chip = gpiod::chip("gpiochip0");

// NEW (v2.x)
auto chip = gpiod::chip("/dev/gpiochip0");
// or
auto chip = gpiod::chip("gpiochip0");  // Still works
```

### **4. Line Request (Most Important Change)**
```cpp
// OLD (v1.x)
auto line = chip.get_line(18);
line.request({"vacuum-controller", gpiod::line_request::DIRECTION_OUTPUT});
line.set_value(1);

// NEW (v2.x) - Request-based API
auto request = chip.prepare_request()
    .set_consumer("vacuum-controller")
    .add_line_settings(18, gpiod::line_settings()
        .set_direction(gpiod::line::direction::OUTPUT))
    .do_request();

request.set_value(18, gpiod::line::value::ACTIVE);
```

### **5. Reading Input Lines**
```cpp
// OLD (v1.x)
auto line = chip.get_line(21);  // Emergency stop button
line.request({"vacuum-controller", gpiod::line_request::DIRECTION_INPUT});
int value = line.get_value();

// NEW (v2.x)
auto request = chip.prepare_request()
    .set_consumer("vacuum-controller")
    .add_line_settings(21, gpiod::line_settings()
        .set_direction(gpiod::line::direction::INPUT)
        .set_bias(gpiod::line::bias::PULL_UP))
    .do_request();

auto value = request.get_value(21);
```

### **6. Multiple Lines (Bulk Operations)**
```cpp
// OLD (v1.x)
auto lines = chip.get_lines({17, 27, 22});  // SOL1, SOL2, SOL3
lines.request({"vacuum-controller", gpiod::line_request::DIRECTION_OUTPUT});
lines.set_values({1, 0, 1});

// NEW (v2.x)
auto request = chip.prepare_request()
    .set_consumer("vacuum-controller")
    .add_line_settings({17, 27, 22}, gpiod::line_settings()
        .set_direction(gpiod::line::direction::OUTPUT))
    .do_request();

request.set_values({{17, gpiod::line::value::ACTIVE},
                    {27, gpiod::line::value::INACTIVE},
                    {22, gpiod::line::value::ACTIVE}});
```

### **7. Event Handling**
```cpp
// OLD (v1.x)
line.request({"vacuum-controller", gpiod::line_request::EVENT_BOTH_EDGES});
auto event = line.event_wait(std::chrono::seconds(1));

// NEW (v2.x)
auto request = chip.prepare_request()
    .set_consumer("vacuum-controller")
    .add_line_settings(21, gpiod::line_settings()
        .set_direction(gpiod::line::direction::INPUT)
        .set_edge_detection(gpiod::line::edge::BOTH))
    .do_request();

auto events = request.read_edge_events();
```

## 🔧 **Updated Vacuum Controller GPIO Code**

Here's how your vacuum controller GPIO code should look with v2.x:

```cpp
#include <gpiod.hpp>
#include <chrono>
#include <iostream>

class VacuumControllerGPIO {
private:
    gpiod::chip chip_;
    gpiod::request output_request_;
    gpiod::request input_request_;
    
    // GPIO pin definitions
    static constexpr int SOL1_PIN = 17;  // Applied Vacuum Line
    static constexpr int SOL2_PIN = 27;  // AVL vent valve
    static constexpr int SOL3_PIN = 22;  // Tank vent valve
    static constexpr int PUMP_PIN = 25;  // Pump enable
    static constexpr int EMERGENCY_STOP_PIN = 21;  // Emergency stop

public:
    VacuumControllerGPIO() : chip_("/dev/gpiochip0") {
        // Setup output pins
        output_request_ = chip_.prepare_request()
            .set_consumer("vacuum-controller")
            .add_line_settings({SOL1_PIN, SOL2_PIN, SOL3_PIN, PUMP_PIN}, 
                gpiod::line_settings()
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE))
            .do_request();
        
        // Setup input pins
        input_request_ = chip_.prepare_request()
            .set_consumer("vacuum-controller")
            .add_line_settings(EMERGENCY_STOP_PIN,
                gpiod::line_settings()
                    .set_direction(gpiod::line::direction::INPUT)
                    .set_bias(gpiod::line::bias::PULL_UP)
                    .set_edge_detection(gpiod::line::edge::FALLING))
            .do_request();
    }
    
    void setPump(bool enable) {
        output_request_.set_value(PUMP_PIN, 
            enable ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
    }
    
    void setSolenoid(int solenoid, bool open) {
        int pin = (solenoid == 1) ? SOL1_PIN : 
                  (solenoid == 2) ? SOL2_PIN : SOL3_PIN;
        output_request_.set_value(pin,
            open ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
    }
    
    bool isEmergencyStop() {
        return input_request_.get_value(EMERGENCY_STOP_PIN) == gpiod::line::value::INACTIVE;
    }
    
    void setAllSolenoids(bool sol1, bool sol2, bool sol3) {
        output_request_.set_values({
            {SOL1_PIN, sol1 ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE},
            {SOL2_PIN, sol2 ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE},
            {SOL3_PIN, sol3 ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE}
        });
    }
    
    void emergencyStop() {
        // Turn off pump and close all solenoids
        output_request_.set_values({
            {PUMP_PIN, gpiod::line::value::INACTIVE},
            {SOL1_PIN, gpiod::line::value::INACTIVE},
            {SOL2_PIN, gpiod::line::value::INACTIVE},
            {SOL3_PIN, gpiod::line::value::INACTIVE}
        });
    }
};
```

## 🎯 **Key Benefits of v2.x**

1. **Better Resource Management**: Request-based API ensures proper cleanup
2. **Improved Performance**: Bulk operations are more efficient
3. **Enhanced Safety**: Better error handling and validation
4. **Modern C++**: Uses modern C++ features and patterns
5. **Future-Proof**: Active development and maintenance

## 🔄 **Migration Steps for Your Project**

1. **Update CMakeLists.txt** (already done in your project):
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)
```

2. **Update your GPIO wrapper class** with the new API
3. **Test thoroughly** - the new API is more strict about resource management
4. **Enjoy better performance** and more reliable GPIO operations!

## ✅ **Your Project Status**

- ✅ libgpiod v2.2.1 installed
- ✅ C++ headers available
- ✅ CMake configuration ready
- ✅ Ready for modern GPIO programming!

The new API is more verbose but much more robust and efficient. Your vacuum controller will benefit from better performance and reliability!
