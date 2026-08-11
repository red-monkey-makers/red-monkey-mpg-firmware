#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "openmpg/types.hpp"

namespace openmpg {

// A boot-protocol keyboard report: modifier, reserved, then six key usages.
using KeyboardReport = std::array<std::uint8_t, 8>;

enum class CncControllerProfileId : std::uint8_t {
  masso_g3_touch_5_13 = 0,
};

struct CncControllerProfile {
  CncControllerProfileId id;
  const char* key;
  const char* name;
  const char* description;
  bool supports_step_resolution;
  KeyboardReport (*keyboard_report)(const OutputFrame& frame);
};

std::size_t cnc_controller_profile_count();
const CncControllerProfile* cnc_controller_profile_at(std::size_t index);
const CncControllerProfile* cnc_controller_profile(CncControllerProfileId id);
const CncControllerProfile* cnc_controller_profile_by_key(const char* key);
bool is_supported_cnc_controller_profile(CncControllerProfileId id);

// Resolves the selected profile and fails closed (all keys released) when the
// stored ID is unsupported.
KeyboardReport cnc_controller_keyboard_report(CncControllerProfileId id,
                                              const OutputFrame& frame);

// Converts the already safety-filtered, one-axis OutputFrame to the official
// MASSO G3 Touch keyboard jogging scheme. Retained as the first built-in
// profile translator and for compatibility with existing diagnostics.
KeyboardReport masso_g3_keyboard_report(const OutputFrame& frame);

}  // namespace openmpg
