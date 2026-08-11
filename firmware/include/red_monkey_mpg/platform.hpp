#pragma once

#include <cstdint>
#include "red_monkey_mpg/types.hpp"

namespace red_monkey_mpg::platform {

void initialize();
std::uint32_t milliseconds();
GamepadState poll_gamepad();
void send_keyboard_output(const OutputFrame& frame);

}  // namespace red_monkey_mpg::platform

