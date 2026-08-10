#pragma once

#include <cstdint>
#include "openmpg/types.hpp"

namespace openmpg::platform {

void initialize();
std::uint32_t milliseconds();
GamepadState poll_gamepad();
void send_keyboard_output(const OutputFrame& frame);

}  // namespace openmpg::platform

