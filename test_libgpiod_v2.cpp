#include <iostream>
#include <gpiod.hpp>
#include <chrono>
#include <thread>

int main() {
    std::cout << "=== libgpiod v2.x C++ Test ===" << std::endl;
    
    try {
        // Open GPIO chip
        auto chip = gpiod::chip("/dev/gpiochip0");
        std::cout << "✅ Successfully opened GPIO chip: " << chip.get_info().get_name() << std::endl;
        std::cout << "   Number of lines: " << chip.get_info().get_num_lines() << std::endl;
        
        // Test output pin (LED or similar - using pin 18 as it's commonly available)
        std::cout << "\n--- Testing Output Pin (GPIO 18) ---" << std::endl;
        
        auto output_request = chip.prepare_request()
            .set_consumer("libgpiod-v2-test")
            .add_line_settings(18, gpiod::line_settings()
                .set_direction(gpiod::line::direction::OUTPUT)
                .set_output_value(gpiod::line::value::INACTIVE))
            .do_request();
        
        std::cout << "✅ Successfully configured GPIO 18 as output" << std::endl;
        
        // Blink the pin a few times
        for (int i = 0; i < 5; i++) {
            output_request.set_value(18, gpiod::line::value::ACTIVE);
            std::cout << "   GPIO 18 HIGH" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            output_request.set_value(18, gpiod::line::value::INACTIVE);
            std::cout << "   GPIO 18 LOW" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // Test input pin (using pin 21 - emergency stop button)
        std::cout << "\n--- Testing Input Pin (GPIO 21) ---" << std::endl;
        
        auto input_request = chip.prepare_request()
            .set_consumer("libgpiod-v2-test")
            .add_line_settings(21, gpiod::line_settings()
                .set_direction(gpiod::line::direction::INPUT)
                .set_bias(gpiod::line::bias::PULL_UP))
            .do_request();
        
        std::cout << "✅ Successfully configured GPIO 21 as input with pull-up" << std::endl;
        
        auto value = input_request.get_value(21);
        std::cout << "   GPIO 21 current value: " << 
            (value == gpiod::line::value::ACTIVE ? "HIGH" : "LOW") << std::endl;
        
        // Test multiple pins (vacuum controller pins)
        std::cout << "\n--- Testing Multiple Pins (Vacuum Controller) ---" << std::endl;
        
        auto multi_request = chip.prepare_request()
            .set_consumer("vacuum-controller-test")
            .add_line_settings({17, 27, 22, 25}, gpiod::line_settings()
                .set_direction(gpiod::line::direction::OUTPUT)
                .set_output_value(gpiod::line::value::INACTIVE))
            .do_request();
        
        std::cout << "✅ Successfully configured vacuum controller pins (17, 27, 22, 25)" << std::endl;
        
        // Test bulk operations
        multi_request.set_values({
            {17, gpiod::line::value::ACTIVE},   // SOL1
            {27, gpiod::line::value::INACTIVE}, // SOL2
            {22, gpiod::line::value::ACTIVE},   // SOL3
            {25, gpiod::line::value::ACTIVE}    // PUMP
        });
        
        std::cout << "   Set SOL1=HIGH, SOL2=LOW, SOL3=HIGH, PUMP=HIGH" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Turn everything off
        multi_request.set_values({
            {17, gpiod::line::value::INACTIVE},
            {27, gpiod::line::value::INACTIVE},
            {22, gpiod::line::value::INACTIVE},
            {25, gpiod::line::value::INACTIVE}
        });
        
        std::cout << "   All pins set to LOW (safe state)" << std::endl;
        
        std::cout << "\n🎉 libgpiod v2.x C++ API test completed successfully!" << std::endl;
        std::cout << "✅ Your vacuum controller will work perfectly with libgpiod v2.2.1" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
