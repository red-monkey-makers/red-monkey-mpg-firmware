#include "openmpg/production_config.hpp"

namespace openmpg {
namespace {

constexpr std::uint32_t kMotionControls =
    0x00040 | 0x00080 | 0x00100 | 0x00200;  // L1, R1, L2, R2
constexpr std::uint32_t kFaceMasks[] = {0x00001, 0x00002, 0x00008, 0x00010};

bool is_single_mask(std::uint32_t value) {
  return value != 0 && (value & (value - 1)) == 0;
}

bool is_safe_face_action(ButtonAction action) {
  switch (action) {
    case ButtonAction::unassigned:
    case ButtonAction::cancel:
    case ButtonAction::cycle_resolution:
    case ButtonAction::step_0_01:
    case ButtonAction::step_0_10:
    case ButtonAction::step_0_50:
    case ButtonAction::step_1_00:
      return true;
  }
  return false;
}

bool is_step_action(ButtonAction action) {
  return action == ButtonAction::cycle_resolution ||
         action == ButtonAction::step_0_01 ||
         action == ButtonAction::step_0_10 ||
         action == ButtonAction::step_0_50 ||
         action == ButtonAction::step_1_00;
}

}  // namespace

ConfigValidation validate_production_mapping(const ProductionMapping& mapping) {
  const auto* profile = cnc_controller_profile(mapping.cnc_profile);
  if (profile == nullptr) {
    return ConfigValidation::unsupported_cnc_profile;
  }
  if (!is_single_mask(mapping.deadman_mask) ||
      (mapping.deadman_mask & kMotionControls) == 0 ||
      !is_single_mask(mapping.continuous_mask) ||
      (mapping.continuous_mask & kMotionControls) == 0 ||
      !is_single_mask(mapping.precision_mask) ||
      (mapping.precision_mask & kMotionControls) == 0 ||
      mapping.resolution_mask != 0) {
    return ConfigValidation::invalid_control;
  }
  // Precision is intentionally fixed to R1 in schema v1. Allowing either of
  // the configurable motion controls to alias it makes one physical button
  // assert two safety-relevant modes at once.
  if (mapping.deadman_mask == mapping.continuous_mask ||
      mapping.deadman_mask == mapping.precision_mask ||
      mapping.continuous_mask == mapping.precision_mask) {
    return ConfigValidation::duplicate_motion_control;
  }
  if (!is_safe_face_action(mapping.face_a) ||
      !is_safe_face_action(mapping.face_b) ||
      !is_safe_face_action(mapping.face_x) ||
      !is_safe_face_action(mapping.face_y)) {
    return ConfigValidation::unsafe_face_action;
  }
  if (!profile->supports_step_resolution &&
      (is_step_action(mapping.face_a) || is_step_action(mapping.face_b) ||
       is_step_action(mapping.face_x) || is_step_action(mapping.face_y))) {
    return ConfigValidation::unsupported_profile_action;
  }
  return ConfigValidation::valid;
}

GamepadState apply_production_mapping(const GamepadState& raw,
                                      const ProductionMapping& mapping) {
  GamepadState mapped = raw;
  if (validate_production_mapping(mapping) != ConfigValidation::valid) {
    mapped.deadman = false;
    mapped.rapid = false;
    mapped.precision = false;
    mapped.cycle_resolution = false;
    mapped.select_resolution = false;
    mapped.cancel = true;
    return mapped;
  }
  mapped.deadman = (raw.buttons & mapping.deadman_mask) != 0;
  mapped.rapid = (raw.buttons & mapping.continuous_mask) != 0;
  mapped.precision = (raw.buttons & mapping.precision_mask) != 0;
  mapped.cycle_resolution =
      mapping.resolution_mask != 0 &&
      (raw.buttons & mapping.resolution_mask) != 0;
  mapped.cancel = false;
  mapped.select_resolution = false;
  const ButtonAction actions[] = {mapping.face_a, mapping.face_b,
                                  mapping.face_x, mapping.face_y};
  for (int i = 0; i < 4; ++i) {
    if ((raw.buttons & kFaceMasks[i]) == 0) continue;
    switch (actions[i]) {
      case ButtonAction::unassigned:
        break;
      case ButtonAction::cancel:
        mapped.cancel = true;
        break;
      case ButtonAction::cycle_resolution:
        mapped.cycle_resolution = true;
        break;
      case ButtonAction::step_0_01:
        mapped.select_resolution = true;
        mapped.selected_resolution = StepResolution::mm_0_01;
        break;
      case ButtonAction::step_0_10:
        mapped.select_resolution = true;
        mapped.selected_resolution = StepResolution::mm_0_10;
        break;
      case ButtonAction::step_0_50:
        mapped.select_resolution = true;
        mapped.selected_resolution = StepResolution::mm_0_50;
        break;
      case ButtonAction::step_1_00:
        mapped.select_resolution = true;
        mapped.selected_resolution = StepResolution::mm_1_00;
        break;
    }
  }
  return mapped;
}

}  // namespace openmpg
