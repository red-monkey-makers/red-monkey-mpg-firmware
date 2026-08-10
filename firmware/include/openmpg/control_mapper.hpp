#pragma once

#include <optional>

#include "openmpg/types.hpp"

namespace openmpg {

struct MapperConfig {
  std::int16_t deadzone{9000};
  std::uint32_t stale_timeout_ms{150};
};

class ControlMapper {
 public:
  explicit ControlMapper(MapperConfig config = {});
  OutputFrame update(const GamepadState& input, std::uint32_t now_ms);

 private:
  MapperConfig config_;
  std::optional<Axis> locked_axis_;
  bool awaiting_center_{};
  bool resolution_button_was_pressed_{};
  bool selection_button_was_pressed_{};
  bool override_increase_was_pressed_{};
  bool override_decrease_was_pressed_{};
  StepResolution next_resolution_{StepResolution::mm_0_01};
};

}  // namespace openmpg
