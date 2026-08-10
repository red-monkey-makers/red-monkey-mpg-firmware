#pragma once

#include <cstdint>

#include "openmpg/masso_keyboard.hpp"
#include "openmpg/types.hpp"

namespace openmpg {

enum class ButtonAction : std::uint8_t {
  unassigned,
  cancel,
  cycle_resolution,
  step_0_01,
  step_0_10,
  step_0_50,
  step_1_00,
};

struct ProductionMapping {
  CncControllerProfileId cnc_profile{
      CncControllerProfileId::masso_g3_touch_5_13};
  std::uint32_t deadman_mask{0x00100};
  std::uint32_t continuous_mask{0x00200};
  std::uint32_t precision_mask{0x00080};
  std::uint32_t resolution_mask{0};
  ButtonAction face_a{ButtonAction::cycle_resolution};
  ButtonAction face_b{ButtonAction::cancel};
  ButtonAction face_x{ButtonAction::unassigned};
  ButtonAction face_y{ButtonAction::unassigned};
};

enum class ConfigValidation : std::uint8_t {
  valid,
  invalid_control,
  duplicate_motion_control,
  unsafe_face_action,
  unsupported_cnc_profile,
  unsupported_profile_action,
};

ConfigValidation validate_production_mapping(const ProductionMapping& mapping);
GamepadState apply_production_mapping(const GamepadState& raw,
                                      const ProductionMapping& mapping);

}  // namespace openmpg
