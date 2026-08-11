#include "red_monkey_mpg/control_mapper.hpp"
#include "red_monkey_mpg/platform.hpp"

#include "pico/stdlib.h"

int main() {
  red_monkey_mpg::platform::initialize();
  red_monkey_mpg::ControlMapper mapper{};
  while (true) {
    const auto now = red_monkey_mpg::platform::milliseconds();
    const auto input = red_monkey_mpg::platform::poll_gamepad();
    red_monkey_mpg::platform::send_keyboard_output(mapper.update(input, now));
    sleep_ms(5);
  }
}
