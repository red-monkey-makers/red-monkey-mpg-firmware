#include "red_monkey_mpg/control_mapper.hpp"
#include "red_monkey_mpg/lite2_report.hpp"
#include "red_monkey_mpg/masso_keyboard.hpp"
#include "red_monkey_mpg/production_config.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>

using namespace red_monkey_mpg;

namespace {

std::uint32_t random_state = 0x4f4d5047u;

std::uint32_t next_random() {
  random_state = random_state * 1664525u + 1013904223u;
  return random_state;
}

std::int16_t random_axis() {
  return static_cast<std::int16_t>(next_random() & 0xffffu);
}

bool report_is_bounded(const KeyboardReport& report) {
  if (report[0] != 0 && report[0] != 0x02) return false;
  if (report[1] != 0) return false;
  for (std::size_t i = 3; i < report.size(); ++i) {
    if (report[i] != 0) return false;
  }
  return true;
}

}  // namespace

int main() {
  Lite2ReportParser parser{};
  const std::array<std::uint8_t, 11> valid = {
      0xA1, 0x01, 0x00, 0x00, 0x80, 0x7F,
      0x7F, 0x7F, 0x80, 0x00, 0x00};

  GamepadState untouched{};
  untouched.left_x = 1234;
  untouched.buttons = 0xabcdefu;
  untouched.connected = true;
  for (std::size_t length = 0; length < 20; ++length) {
    if (length == valid.size()) continue;
    GamepadState output = untouched;
    assert(!parser.parse(valid.data(), length, 10, output));
    assert(output.left_x == untouched.left_x);
    assert(output.buttons == untouched.buttons);
    assert(output.connected == untouched.connected);
  }
  GamepadState output = untouched;
  assert(!parser.parse(nullptr, valid.size(), 10, output));
  auto invalid_header = valid;
  invalid_header[0] = 0;
  assert(!parser.parse(invalid_header.data(), invalid_header.size(), 10, output));
  invalid_header = valid;
  invalid_header[1] = 2;
  assert(!parser.parse(invalid_header.data(), invalid_header.size(), 10, output));

  // Exhaust every possible raw value in each analog byte. Normalization must
  // remain in int16 range and accepted reports must always replace the output.
  for (std::size_t byte = 5; byte <= 8; ++byte) {
    for (unsigned raw = 0; raw <= 255; ++raw) {
      auto report = valid;
      report[byte] = static_cast<std::uint8_t>(raw);
      GamepadState parsed{};
      assert(parser.parse(report.data(), report.size(), raw, parsed));
      assert(parsed.connected && parsed.sample_ms == raw);
    }
  }

  constexpr std::array<std::uint32_t, 4> controls = {
      0x00040, 0x00080, 0x00100, 0x00200};
  for (const auto deadman : controls) {
    for (const auto continuous : controls) {
      ProductionMapping mapping{};
      mapping.deadman_mask = deadman;
      mapping.continuous_mask = continuous;
      const bool aliases = deadman == continuous ||
                           deadman == mapping.precision_mask ||
                           continuous == mapping.precision_mask;
      assert((validate_production_mapping(mapping) == ConfigValidation::valid) ==
             !aliases);
    }
  }
  for (unsigned action = 0; action <= 255; ++action) {
    ProductionMapping mapping{};
    mapping.face_a = static_cast<ButtonAction>(action);
    const bool valid_action = action <= static_cast<unsigned>(ButtonAction::step_1_00);
    assert((validate_production_mapping(mapping) == ConfigValidation::valid) ==
           valid_action);
  }

  // Deterministic fuzzing of the pure safety mapper. Every motion frame must
  // have fresh connected input, an asserted dead-man, and no cancel request.
  for (unsigned iteration = 0; iteration < 20000; ++iteration) {
    ControlMapper mapper{};
    GamepadState input{};
    input.connected = (next_random() & 1u) != 0;
    input.deadman = (next_random() & 1u) != 0;
    input.rapid = (next_random() & 1u) != 0;
    input.precision = (next_random() & 1u) != 0;
    input.cancel = (next_random() & 0x1fu) == 0;
    input.cycle_resolution = (next_random() & 0x1fu) == 0;
    input.select_resolution = (next_random() & 0x1fu) == 0;
    input.left_x = random_axis();
    input.left_y = random_axis();
    input.right_x = random_axis();
    input.right_y = random_axis();
    const std::uint32_t now = next_random();
    const bool stale = (next_random() & 7u) == 0;
    input.sample_ms = stale ? now - 151u : now;

    const OutputFrame frame = mapper.update(input, now);
    if (frame.has_jog) {
      assert(input.connected && input.deadman && !input.cancel && !stale);
      assert(!frame.release_all);
      assert(!frame.has_step_resolution);
    }
    if (!input.deadman || !input.connected || input.cancel || stale) {
      assert(!frame.has_jog);
    }
    const auto keyboard = masso_g3_keyboard_report(frame);
    assert(report_is_bounded(keyboard));
    if (frame.release_all) assert(keyboard == KeyboardReport{});
  }

  // Unsigned time subtraction must remain correct across the millisecond wrap.
  ControlMapper wrap_mapper{};
  GamepadState wrapped{};
  wrapped.connected = true;
  wrapped.deadman = true;
  wrapped.left_x = 20000;
  wrapped.sample_ms = std::numeric_limits<std::uint32_t>::max() - 50u;
  assert(wrap_mapper.update(wrapped, 25u).has_jog);  // 76 ms old
  assert(!wrap_mapper.update(wrapped, 200u).has_jog);  // 251 ms old
}
