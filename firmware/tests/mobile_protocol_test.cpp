#include <array>
#include <cassert>
#include <cstdint>

#include "openmpg/mobile_protocol.hpp"

namespace {

std::array<std::uint8_t, openmpg::kMobileFrameLength> frame(
    std::uint16_t sequence, std::uint8_t direction = 0,
    std::uint8_t flags = 0, std::uint8_t event_sequence = 0,
    std::uint8_t event = 0,
    std::uint8_t profile = static_cast<std::uint8_t>(
        openmpg::MobileControllerProfile::masso)) {
  std::array<std::uint8_t, openmpg::kMobileFrameLength> bytes{
      0x52,
      openmpg::kMobileProtocolVersion,
      static_cast<std::uint8_t>(sequence),
      static_cast<std::uint8_t>(sequence >> 8u),
      direction,
      flags,
      event_sequence,
      event,
      profile,
      0,
  };
  bytes.back() = openmpg::mobile_crc8(bytes.data(), bytes.size() - 1);
  return bytes;
}

void test_direction_mapping() {
  openmpg::MobileInputSession session;
  const auto bytes = frame(1, 6, 0x03);
  const auto result = session.consume(bytes.data(), bytes.size(), 100);
  assert(result.accepted);
  assert(result.sequence == 1);
  assert(result.controller_profile == openmpg::MobileControllerProfile::masso);
  assert(result.input.connected);
  assert(result.input.deadman);
  assert(result.input.rapid);
  assert(!result.input.precision);
  assert(result.input.right_y == -32767);
  assert(result.input.sample_ms == 100);
}

void test_invalid_frames_fail_closed() {
  openmpg::MobileInputSession session;
  auto bytes = frame(1);
  bytes[9] ^= 1u;
  assert(session.consume(bytes.data(), bytes.size(), 0).error ==
         openmpg::MobileFrameError::bad_crc);

  bytes = frame(1, 7);
  assert(session.consume(bytes.data(), bytes.size(), 0).error ==
         openmpg::MobileFrameError::invalid_direction);

  bytes = frame(1, 0, 0x80);
  assert(session.consume(bytes.data(), bytes.size(), 0).error ==
         openmpg::MobileFrameError::invalid_flags);

  bytes = frame(1, 0, 0x06);
  assert(session.consume(bytes.data(), bytes.size(), 0).error ==
         openmpg::MobileFrameError::ambiguous_rate);

  bytes = frame(1, 0, 0, 1, 8);
  assert(session.consume(bytes.data(), bytes.size(), 0).error ==
         openmpg::MobileFrameError::invalid_event);
  bytes = frame(1);
  assert(session.consume(bytes.data(), bytes.size(), 1).accepted);

  bytes = frame(2, 0, 0, 0, 0, 0x7F);
  assert(session.consume(bytes.data(), bytes.size(), 2).error ==
         openmpg::MobileFrameError::unsupported_profile);
}

void test_replay_and_wrap_handling() {
  openmpg::MobileInputSession session;
  auto bytes = frame(0xFFFFu);
  assert(session.consume(bytes.data(), bytes.size(), 0).accepted);
  assert(session.consume(bytes.data(), bytes.size(), 1).error ==
         openmpg::MobileFrameError::replayed_sequence);
  bytes = frame(0);
  assert(session.consume(bytes.data(), bytes.size(), 2).accepted);
  bytes = frame(0xFFFFu);
  assert(session.consume(bytes.data(), bytes.size(), 3).error ==
         openmpg::MobileFrameError::replayed_sequence);
}

void test_discrete_event_is_deduplicated() {
  openmpg::MobileInputSession session;
  auto bytes = frame(1, 0, 0, 9, 6);
  auto result = session.consume(bytes.data(), bytes.size(), 0);
  assert(result.accepted);
  assert(result.input.override_increase);

  bytes = frame(2, 0, 0, 9, 6);
  result = session.consume(bytes.data(), bytes.size(), 1);
  assert(result.accepted);
  assert(!result.input.override_increase);

  bytes = frame(3, 0, 0, 10, 2);
  result = session.consume(bytes.data(), bytes.size(), 2);
  assert(result.input.select_resolution);
  assert(result.input.selected_resolution == openmpg::StepResolution::mm_0_10);
}

void test_reset_starts_new_sequence_session() {
  openmpg::MobileInputSession session;
  auto bytes = frame(400);
  assert(session.consume(bytes.data(), bytes.size(), 0).accepted);
  session.reset();
  bytes = frame(1);
  assert(session.consume(bytes.data(), bytes.size(), 1).accepted);
}

}  // namespace

int main() {
  test_direction_mapping();
  test_invalid_frames_fail_closed();
  test_replay_and_wrap_handling();
  test_discrete_event_is_deduplicated();
  test_reset_starts_new_sequence_session();
  return 0;
}
