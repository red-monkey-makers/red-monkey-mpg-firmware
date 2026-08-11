#pragma once

#include <cstdint>

namespace openmpg {

enum class Axis : std::uint8_t { x, y, z };
enum class Direction : std::int8_t { negative = -1, positive = 1 };
enum class JogRate : std::uint8_t { precision, normal, rapid };
enum class StepResolution : std::uint8_t { mm_0_01, mm_0_10, mm_0_50, mm_1_00 };

struct GamepadState {
  std::int16_t left_x{};
  std::int16_t left_y{};
  std::int16_t right_x{};
  std::int16_t right_y{};
  std::uint32_t buttons{};
  std::uint8_t hat{8};
  std::uint8_t left_trigger{};
  std::uint8_t right_trigger{};
  bool deadman{};       // L2, held
  bool rapid{};         // R2, held
  bool precision{};     // R1, held
  bool cycle_resolution{};
  bool select_resolution{};
  StepResolution selected_resolution{StepResolution::mm_0_01};
  bool cancel{};
  bool connected{};
  std::uint32_t sample_ms{};
};

struct JogCommand {
  Axis axis{Axis::x};
  Direction direction{Direction::positive};
  JogRate rate{JogRate::normal};
};

struct OutputFrame {
  bool release_all{true};
  bool has_jog{};
  JogCommand jog{};
  bool has_step_resolution{};
  StepResolution step_resolution{StepResolution::mm_0_01};
};

}  // namespace openmpg
