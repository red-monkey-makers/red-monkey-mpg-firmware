#include "openmpg/control_mapper.hpp"
#include "openmpg/platform.hpp"

#include "pico/stdlib.h"

int main() {
  openmpg::platform::initialize();
  openmpg::ControlMapper mapper{};
  while (true) {
    const auto now = openmpg::platform::milliseconds();
    const auto input = openmpg::platform::poll_gamepad();
    openmpg::platform::send_keyboard_output(mapper.update(input, now));
    sleep_ms(5);
  }
}
