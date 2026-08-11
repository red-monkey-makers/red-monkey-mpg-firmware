#include "red_monkey_mpg/platform.hpp"

#include "pico/stdlib.h"

namespace red_monkey_mpg::platform {

// These fail-closed placeholders are the two hardware integration seams.
// Implement poll_gamepad with BTstack HID host and send_keyboard_output with
// TinyUSB after validating report descriptors and the selected CNC profile.
void initialize() { stdio_init_all(); }
std::uint32_t milliseconds() { return to_ms_since_boot(get_absolute_time()); }
GamepadState poll_gamepad() { return {}; }
void send_keyboard_output(const OutputFrame&) {}

}  // namespace red_monkey_mpg::platform
