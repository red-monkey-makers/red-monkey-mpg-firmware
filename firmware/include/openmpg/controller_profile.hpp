#pragma once

#include <cstddef>
#include <cstdint>

#include "openmpg/types.hpp"

namespace openmpg {

enum class ControllerTransport : std::uint8_t {
  bluetooth_classic_hid,
  bluetooth_le_hid,
  usb_hid,
};

enum class ProfileMaturity : std::uint8_t {
  experimental,
  community_tested,
  qualified,
};

enum class ProfileMatch : std::uint8_t {
  unsupported,
  compatible,
  exact,
};

struct ControllerCapabilities {
  bool x_axis{};
  bool y_axis{};
  bool z_axis{};
  bool held_deadman_candidate{};
  bool continuous_modifier_candidate{};
  bool precision_modifier_candidate{};
};

struct ControllerProbe {
  ControllerTransport transport{ControllerTransport::bluetooth_classic_hid};
  const char* advertised_name{};
  std::uint32_t bluetooth_device_class{};
  const std::uint8_t* hid_descriptor{};
  std::size_t hid_descriptor_length{};
};

using ControllerMatcher = ProfileMatch (*)(const ControllerProbe&);
using ControllerReportParser = bool (*)(const std::uint8_t*, std::size_t,
                                        std::uint32_t, GamepadState&);

struct ControllerProfile {
  const char* id{};
  const char* display_name{};
  ControllerTransport transport{ControllerTransport::bluetooth_classic_hid};
  ProfileMaturity maturity{ProfileMaturity::experimental};
  ControllerCapabilities capabilities{};
  ControllerMatcher match{};
  ControllerReportParser parse_report{};
};

enum class ProfileSelectionStatus : std::uint8_t {
  matched,
  unsupported,
  ambiguous,
};

struct ProfileSelection {
  ProfileSelectionStatus status{ProfileSelectionStatus::unsupported};
  const ControllerProfile* profile{};
};

class ControllerProfileRegistry {
 public:
  ControllerProfileRegistry(const ControllerProfile* const* profiles,
                            std::size_t count)
      : profiles_(profiles), count_(count) {}

  ProfileSelection select(const ControllerProbe& probe) const;

 private:
  const ControllerProfile* const* profiles_{};
  std::size_t count_{};
};

bool profile_supports_safe_motion(const ControllerProfile& profile);
const ControllerProfileRegistry& builtin_controller_profiles();

}  // namespace openmpg
