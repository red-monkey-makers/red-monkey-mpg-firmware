#include "openmpg/control_mapper.hpp"

#include <cstdlib>

namespace openmpg {

ControlMapper::ControlMapper(MapperConfig config) : config_(config) {}

OutputFrame ControlMapper::update(const GamepadState& in,
                                  std::uint32_t now_ms) {
  OutputFrame out{};
  const bool stale = (now_ms - in.sample_ms) > config_.stale_timeout_ms;
  if (!in.connected || stale) {
    locked_axis_.reset();
    awaiting_center_ = false;
    resolution_button_was_pressed_ = false;
    selection_button_was_pressed_ = false;
    return out;
  }

  const auto magnitude = [](std::int16_t value) {
    return std::abs(static_cast<int>(value));
  };

  const bool all_centered = magnitude(in.left_x) <= config_.deadzone &&
                            magnitude(in.left_y) <= config_.deadzone &&
                            magnitude(in.right_y) <= config_.deadzone;

  const bool resolution_pressed = in.cycle_resolution;
  const bool resolution_rising_edge =
      resolution_pressed && !resolution_button_was_pressed_;
  resolution_button_was_pressed_ = resolution_pressed;
  const bool selection_rising_edge =
      in.select_resolution && !selection_button_was_pressed_;
  selection_button_was_pressed_ = in.select_resolution;

  if (in.cancel) {
    locked_axis_.reset();
    awaiting_center_ = true;
    return out;
  }

  // Resolution selection is a non-motion command. Accept one edge only while
  // the dead-man is released and every assigned joystick axis is centered.
  if (!in.deadman) {
    locked_axis_.reset();
    awaiting_center_ = false;
    if (all_centered && selection_rising_edge) {
      out.release_all = false;
      out.has_step_resolution = true;
      out.step_resolution = in.selected_resolution;
    } else if (all_centered && resolution_rising_edge) {
      out.release_all = false;
      out.has_step_resolution = true;
      out.step_resolution = next_resolution_;
      switch (next_resolution_) {
        case StepResolution::mm_0_01:
          next_resolution_ = StepResolution::mm_0_10;
          break;
        case StepResolution::mm_0_10:
          next_resolution_ = StepResolution::mm_0_50;
          break;
        case StepResolution::mm_0_50:
          next_resolution_ = StepResolution::mm_1_00;
          break;
        case StepResolution::mm_1_00:
          next_resolution_ = StepResolution::mm_0_01;
          break;
      }
    }
    return out;
  }

  if (awaiting_center_) {
    if (all_centered) awaiting_center_ = false;
    return out;
  }

  const auto axis_value = [&in](Axis axis) {
    if (axis == Axis::x) return in.left_x;
    if (axis == Axis::y) return in.left_y;
    return in.right_y;
  };

  // Red Monkey MPG emits one jog axis at a time. Lock it until every
  // stick returns to center so diagonal jitter cannot alternate X/Y commands.
  if (!locked_axis_) {
    if (all_centered) return out;
    if (magnitude(in.right_y) > config_.deadzone) {
      locked_axis_ = Axis::z;
    } else {
      locked_axis_ = magnitude(in.left_x) >= magnitude(in.left_y) ? Axis::x
                                                                  : Axis::y;
    }
  }

  const Axis axis = *locked_axis_;
  const std::int16_t value = axis_value(axis);
  if (magnitude(value) <= config_.deadzone) {
    locked_axis_.reset();
    awaiting_center_ = true;
    return out;
  }

  out.release_all = false;
  out.has_jog = true;
  out.jog.axis = axis;
  // Gamepad Y is conventionally positive downward; invert Y and Z.
  const bool positive = axis == Axis::x ? value > 0 : value < 0;
  out.jog.direction = positive ? Direction::positive : Direction::negative;
  out.jog.rate = in.precision ? JogRate::precision
                              : (in.rapid ? JogRate::rapid : JogRate::normal);
  return out;
}

}  // namespace openmpg
