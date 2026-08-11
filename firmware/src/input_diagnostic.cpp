#include "red_monkey_mpg/input_diagnostic.h"

#include <cstdio>

#include "red_monkey_mpg/control_mapper.hpp"
#include "red_monkey_mpg/controller_profile.hpp"
#include "red_monkey_mpg/lite2_report.hpp"

extern "C" void red_monkey_mpg_diagnose_lite2_report(const std::uint8_t* report,
                                                std::uint16_t length,
                                                std::uint32_t now_ms) {
  static red_monkey_mpg::ControlMapper mapper;
  red_monkey_mpg::GamepadState input{};
  if (!red_monkey_mpg::lite2_d_profile().parse_report(report, length, now_ms,
                                                input)) {
    std::printf("MAPPED invalid Lite 2 report\n");
    return;
  }

  const auto frame = mapper.update(input, now_ms);
  std::printf("MAPPED LX=%d LY=%d RX=%d RY=%d deadman=%u rate=%s ",
              input.left_x, input.left_y, input.right_x, input.right_y,
              input.deadman ? 1u : 0u,
              input.precision ? "precision" : (input.rapid ? "rapid" : "normal"));
  if (!frame.has_jog) {
    std::printf("ACTION=RELEASE\n");
    return;
  }
  const char axis = frame.jog.axis == red_monkey_mpg::Axis::x
                        ? 'X'
                        : (frame.jog.axis == red_monkey_mpg::Axis::y ? 'Y' : 'Z');
  const char direction = frame.jog.direction == red_monkey_mpg::Direction::positive
                             ? '+'
                             : '-';
  std::printf("ACTION=JOG_%c%c\n", axis, direction);
}
