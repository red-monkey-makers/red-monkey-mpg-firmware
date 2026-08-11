#include <array>
#include <cassert>
#include <cstdint>

#include "red_monkey_mpg/control_mapper.hpp"
#include "red_monkey_mpg/masso_keyboard.hpp"
#include "red_monkey_mpg/mobile_protocol.hpp"

namespace {

std::array<std::uint8_t, red_monkey_mpg::kMobileFrameLength> frame(
    std::uint16_t sequence, std::uint8_t direction, std::uint8_t flags,
    std::uint8_t event_sequence = 0, std::uint8_t event = 0) {
  std::array<std::uint8_t, red_monkey_mpg::kMobileFrameLength> bytes{
      0x52,
      red_monkey_mpg::kMobileProtocolVersion,
      static_cast<std::uint8_t>(sequence),
      static_cast<std::uint8_t>(sequence >> 8u),
      direction,
      flags,
      event_sequence,
      event,
      static_cast<std::uint8_t>(red_monkey_mpg::MobileControllerProfile::masso),
      0,
  };
  bytes.back() = red_monkey_mpg::mobile_crc8(bytes.data(), bytes.size() - 1);
  return bytes;
}

red_monkey_mpg::KeyboardReport translate(red_monkey_mpg::MobileInputSession& session,
                                         red_monkey_mpg::ControlMapper& mapper,
                                         const decltype(frame(0, 0, 0))& bytes,
                                         std::uint32_t now_ms) {
  const auto parsed = session.consume(bytes.data(), bytes.size(), now_ms);
  assert(parsed.accepted);
  return red_monkey_mpg::cnc_controller_keyboard_report(
      red_monkey_mpg::CncControllerProfileId::masso_g3_touch_5_13,
      mapper.update(parsed.input, now_ms));
}

void test_single_and_continuous_reports() {
  red_monkey_mpg::MobileInputSession session;
  red_monkey_mpg::ControlMapper mapper;

  auto bytes = frame(1, 0, 0);
  assert(translate(session, mapper, bytes, 1) == red_monkey_mpg::KeyboardReport{});

  bytes = frame(2, 2, 0x01);  // X+, single-step mode.
  auto report = translate(session, mapper, bytes, 2);
  assert(report[0] == 0);
  assert(report[2] == 0x4f);  // Right arrow.

  bytes = frame(3, 0, 0);
  assert(translate(session, mapper, bytes, 3) == red_monkey_mpg::KeyboardReport{});

  bytes = frame(4, 4, 0x03);  // Y+, continuous mode.
  report = translate(session, mapper, bytes, 4);
  assert(report[0] == 0x02);  // Left Shift.
  assert(report[2] == 0x52);  // Up arrow.
}

void test_resolution_and_cancel_release() {
  red_monkey_mpg::MobileInputSession session;
  red_monkey_mpg::ControlMapper mapper;

  auto bytes = frame(1, 0, 0, 1, 1);  // 0.01 mm.
  auto report = translate(session, mapper, bytes, 1);
  assert(report[2] == 0x21);  // MASSO step-selection key 4.

  bytes = frame(2, 0, 0);
  assert(translate(session, mapper, bytes, 2) == red_monkey_mpg::KeyboardReport{});

  bytes = frame(3, 6, 0x03);
  report = translate(session, mapper, bytes, 3);
  assert(report[0] == 0x02);
  assert(report[2] == 0x18);  // Z+ uses U after axis inversion.

  bytes = frame(4, 0, 0, 2, 5);
  assert(translate(session, mapper, bytes, 4) == red_monkey_mpg::KeyboardReport{});
}

}  // namespace

int main() {
  test_single_and_continuous_reports();
  test_resolution_and_cancel_release();
  return 0;
}
