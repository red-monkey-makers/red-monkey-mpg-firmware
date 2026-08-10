#include "openmpg/input_diagnostic.h"

#include <cstdio>

#include "openmpg/control_mapper.hpp"
#include "openmpg/controller_profile.hpp"
#include "openmpg/lite2_report.hpp"

extern "C" void openmpg_diagnose_lite2_report(const std::uint8_t* report,
                                                std::uint16_t length,
                                                std::uint32_t now_ms) {
  static openmpg::ControlMapper mapper;
  openmpg::GamepadState input{};
  if (!openmpg::lite2_d_profile().parse_report(report, length, now_ms,
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
  const char axis = frame.jog.axis == openmpg::Axis::x
                        ? 'X'
                        : (frame.jog.axis == openmpg::Axis::y ? 'Y' : 'Z');
  const char direction = frame.jog.direction == openmpg::Direction::positive
                             ? '+'
                             : '-';
  std::printf("ACTION=JOG_%c%c\n", axis, direction);
}
