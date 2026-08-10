#include "openmpg/controller_profile.hpp"

#include "openmpg/lite2_report.hpp"

namespace openmpg {

bool profile_supports_safe_motion(const ControllerProfile& profile) {
  const auto& capability = profile.capabilities;
  return profile.id != nullptr && profile.display_name != nullptr &&
         profile.match != nullptr && profile.parse_report != nullptr &&
         capability.x_axis && capability.y_axis && capability.z_axis &&
         capability.held_deadman_candidate;
}

ProfileSelection ControllerProfileRegistry::select(
    const ControllerProbe& probe) const {
  const ControllerProfile* best = nullptr;
  ProfileMatch best_match = ProfileMatch::unsupported;
  bool ambiguous = false;

  for (std::size_t i = 0; i < count_; ++i) {
    const ControllerProfile* candidate = profiles_[i];
    if (candidate == nullptr || !profile_supports_safe_motion(*candidate) ||
        candidate->transport != probe.transport) {
      continue;
    }
    const ProfileMatch match = candidate->match(probe);
    if (match == ProfileMatch::unsupported) continue;
    if (best == nullptr || static_cast<unsigned>(match) >
                               static_cast<unsigned>(best_match)) {
      best = candidate;
      best_match = match;
      ambiguous = false;
    } else if (match == best_match) {
      ambiguous = true;
    }
  }

  if (best == nullptr) return {};
  if (ambiguous) return {ProfileSelectionStatus::ambiguous, nullptr};
  return {ProfileSelectionStatus::matched, best};
}

const ControllerProfileRegistry& builtin_controller_profiles() {
  static const ControllerProfile* const profiles[] = {&lite2_d_profile()};
  static const ControllerProfileRegistry registry{
      profiles, sizeof(profiles) / sizeof(profiles[0])};
  return registry;
}

}  // namespace openmpg
