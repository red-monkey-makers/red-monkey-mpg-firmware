#include "openmpg/control_mapper.hpp"
#include "openmpg/lite2_report.hpp"
#include "openmpg/masso_keyboard.hpp"
#include "openmpg/production_config.hpp"

#include <cassert>

using namespace openmpg;

int main() {
  Lite2ReportParser parser{};
  GamepadState parsed{};
  const std::uint8_t centered[] = {0xA1, 0x01, 0x00, 0x00, 0x80, 0x7F,
                                   0x7F, 0x7F, 0x80, 0x00, 0x00};
  assert(parser.parse(centered, sizeof(centered), 50, parsed));
  assert(parsed.connected && parsed.sample_ms == 50);
  assert(parsed.left_x == 0 && parsed.left_y == 0);
  assert(parsed.right_x == 0 && parsed.right_y == 0);
  assert(parsed.hat == 8 && !parsed.deadman);

  const std::uint8_t active[] = {0xA1, 0x01, 0x00, 0x03, 0x80, 0xFF,
                                 0x00, 0x00, 0xFF, 0xFF, 0xFF};
  assert(parser.parse(active, sizeof(active), 75, parsed));
  assert(parsed.left_x == 32767 && parsed.left_y == -32512);
  assert(parsed.right_x == -32512 && parsed.right_y == 32512);
  assert(parsed.deadman && parsed.rapid && !parsed.precision);
  assert(parsed.left_trigger == 255 && parsed.right_trigger == 255);

  const std::uint8_t face_a[] = {0xA1, 0x01, 0x01, 0x00, 0x80, 0x7F,
                                 0x7F, 0x7F, 0x80, 0x00, 0x00};
  assert(parser.parse(face_a, sizeof(face_a), 80, parsed));
  assert(parsed.cycle_resolution && !parsed.deadman);
  const std::uint8_t plus[] = {0xA1, 0x01, 0x00, 0x08, 0x80, 0x7F,
                               0x7F, 0x7F, 0x80, 0x00, 0x00};
  assert(parser.parse(plus, sizeof(plus), 81, parsed));
  assert((parsed.buttons & lite2_button::plus) != 0 &&
         !parsed.cycle_resolution);

  ControlMapper mapper{};
  GamepadState input{};
  input.connected = true;
  input.sample_ms = 1000;
  input.left_x = 20000;

  assert(!mapper.update(input, 1000).has_jog);  // dead-man required
  input.deadman = true;
  auto out = mapper.update(input, 1000);
  assert(out.has_jog && out.jog.axis == Axis::x);
  assert(out.jog.direction == Direction::positive);
  assert(!mapper.update(input, 1200).has_jog);  // stale input releases
  input.sample_ms = 1200;
  input.left_x = 0;
  input.right_y = -20000;
  out = mapper.update(input, 1200);
  assert(out.has_jog && out.jog.axis == Axis::z);
  assert(out.jog.direction == Direction::positive);
  input.precision = true;
  input.rapid = true;
  assert(mapper.update(input, 1200).jog.rate == JogRate::precision);

  // Diagonal input locks to one axis and cannot switch until fully centered.
  input.precision = false;
  input.rapid = false;
  input.right_y = 0;
  assert(!mapper.update(input, 1200).has_jog);  // release Z lock
  assert(!mapper.update(input, 1200).has_jog);  // acknowledge center
  input.left_x = 24000;
  input.left_y = -18000;
  assert(mapper.update(input, 1200).jog.axis == Axis::x);
  input.left_x = 0;
  assert(!mapper.update(input, 1200).has_jog);
  assert(!mapper.update(input, 1200).has_jog);  // Y held: still waiting
  input.left_y = 0;
  assert(!mapper.update(input, 1200).has_jog);
  input.left_y = -20000;
  assert(mapper.update(input, 1200).jog.axis == Axis::y);

  // A cycles 4, 3, 2, 1 (0.01, 0.10, 0.50, 1.00 mm), once per press.
  ControlMapper resolution_mapper{};
  GamepadState resolution_input{};
  resolution_input.connected = true;
  resolution_input.sample_ms = 2000;
  resolution_input.cycle_resolution = true;
  out = resolution_mapper.update(resolution_input, 2000);
  assert(out.has_step_resolution &&
         out.step_resolution == StepResolution::mm_0_01);
  assert(!resolution_mapper.update(resolution_input, 2000).has_step_resolution);

  resolution_input.cycle_resolution = false;
  assert(!resolution_mapper.update(resolution_input, 2000).has_step_resolution);
  resolution_input.cycle_resolution = true;
  out = resolution_mapper.update(resolution_input, 2000);
  assert(out.step_resolution == StepResolution::mm_0_10);
  resolution_input.cycle_resolution = false;
  (void)resolution_mapper.update(resolution_input, 2000);
  resolution_input.left_x = 20000;
  resolution_input.cycle_resolution = true;
  assert(!resolution_mapper.update(resolution_input, 2000).has_step_resolution);
  resolution_input.cycle_resolution = false;
  resolution_input.left_x = 0;
  (void)resolution_mapper.update(resolution_input, 2000);
  resolution_input.deadman = true;
  resolution_input.cycle_resolution = true;
  assert(!resolution_mapper.update(resolution_input, 2000).has_step_resolution);

  // MASSO G3 Touch USB keyboard translation is pure and fail-closed.
  OutputFrame frame{};
  assert(masso_g3_keyboard_report(frame) == KeyboardReport{});
  frame.release_all = false;
  frame.has_jog = true;
  frame.jog = {Axis::x, Direction::negative, JogRate::normal};
  assert(masso_g3_keyboard_report(frame)[2] == 0x50);  // Left arrow
  frame.jog = {Axis::x, Direction::positive, JogRate::rapid};
  auto keys = masso_g3_keyboard_report(frame);
  assert(keys[0] == 0x02 && keys[2] == 0x4f);  // Shift + Right
  frame.jog = {Axis::y, Direction::negative, JogRate::precision};
  keys = masso_g3_keyboard_report(frame);
  assert(keys[0] == 0 && keys[2] == 0x51);  // Down arrow
  frame.jog = {Axis::z, Direction::positive, JogRate::normal};
  assert(masso_g3_keyboard_report(frame)[2] == 0x18);  // U
  frame.jog.direction = Direction::negative;
  assert(masso_g3_keyboard_report(frame)[2] == 0x07);  // D

  frame = {};
  frame.release_all = false;
  frame.has_step_resolution = true;
  frame.step_resolution = StepResolution::mm_0_01;
  assert(masso_g3_keyboard_report(frame)[2] == 0x21);  // 4
  frame.step_resolution = StepResolution::mm_0_10;
  assert(masso_g3_keyboard_report(frame)[2] == 0x20);  // 3
  frame.step_resolution = StepResolution::mm_0_50;
  assert(masso_g3_keyboard_report(frame)[2] == 0x1f);  // 2
  frame.step_resolution = StepResolution::mm_1_00;
  assert(masso_g3_keyboard_report(frame)[2] == 0x1e);  // 1

  // Profile dispatch preserves the verified first translator and releases
  // every key for an unknown stored profile ID.
  keys = masso_g3_keyboard_report(frame);
  assert(cnc_controller_keyboard_report(
             CncControllerProfileId::masso_g3_touch_5_13, frame) == keys);
  assert(cnc_controller_keyboard_report(
             static_cast<CncControllerProfileId>(0xff), frame) ==
         KeyboardReport{});

  ProductionMapping production{};
  assert(validate_production_mapping(production) == ConfigValidation::valid);
  GamepadState raw{};
  raw.buttons = 0x00100 | 0x00200 | 0x00001 | 0x00800;
  const auto mapped = apply_production_mapping(raw, production);
  assert(mapped.deadman && mapped.rapid && mapped.cycle_resolution);
  production.deadman_mask = production.continuous_mask;
  assert(validate_production_mapping(production) ==
         ConfigValidation::duplicate_motion_control);
  assert(!apply_production_mapping(raw, production).deadman);  // fail closed

  // R1 is the fixed precision control in schema v1. Configurable motion
  // controls must not alias it.
  production = {};
  production.deadman_mask = production.precision_mask;
  assert(validate_production_mapping(production) ==
         ConfigValidation::duplicate_motion_control);
  production = {};
  production.continuous_mask = production.precision_mask;
  assert(validate_production_mapping(production) ==
         ConfigValidation::duplicate_motion_control);
  production = {};
  production.cnc_profile = static_cast<CncControllerProfileId>(0xff);
  assert(validate_production_mapping(production) ==
         ConfigValidation::unsupported_cnc_profile);
  assert(apply_production_mapping(raw, production).cancel);
}
