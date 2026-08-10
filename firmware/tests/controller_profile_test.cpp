#include "openmpg/controller_profile.hpp"
#include "openmpg/control_mapper.hpp"
#include "openmpg/lite2_report.hpp"

#include <array>
#include <cassert>

namespace {

openmpg::ProfileMatch always_compatible(const openmpg::ControllerProbe&) {
  return openmpg::ProfileMatch::compatible;
}

bool inert_parser(const std::uint8_t*, std::size_t, std::uint32_t,
                  openmpg::GamepadState&) {
  return false;
}

openmpg::ControllerProfile fake_profile(const char* id) {
  return {id,
          id,
          openmpg::ControllerTransport::bluetooth_classic_hid,
          openmpg::ProfileMaturity::experimental,
          {/*x_axis=*/true, /*y_axis=*/true, /*z_axis=*/true,
           /*held_deadman_candidate=*/true,
           /*continuous_modifier_candidate=*/false,
           /*precision_modifier_candidate=*/false},
          always_compatible,
          inert_parser};
}

}  // namespace

int main() {
  using namespace openmpg;

  std::array<std::uint8_t, 83> descriptor{};
  ControllerProbe probe{ControllerTransport::bluetooth_classic_hid,
                        "8BitDo Lite 2", 0x002508, descriptor.data(),
                        descriptor.size()};
  auto selected = builtin_controller_profiles().select(probe);
  assert(selected.status == ProfileSelectionStatus::matched);
  assert(selected.profile == &lite2_d_profile());
  assert(profile_supports_safe_motion(*selected.profile));

  const std::uint8_t centered[] = {0xA1, 0x01, 0x00, 0x00, 0x80, 0x7F,
                                   0x7F, 0x7F, 0x80, 0x00, 0x00};
  GamepadState state{};
  assert(selected.profile->parse_report(centered, sizeof(centered), 10,
                                        state));
  assert(state.connected && state.sample_ms == 10);

  probe.hid_descriptor_length = 82;
  selected = builtin_controller_profiles().select(probe);
  assert(selected.status == ProfileSelectionStatus::unsupported);
  assert(selected.profile == nullptr);

  probe.hid_descriptor_length = descriptor.size();
  probe.advertised_name = "Unknown Bluetooth Controller";
  selected = builtin_controller_profiles().select(probe);
  assert(selected.status == ProfileSelectionStatus::unsupported);

  // Unsupported input never reaches the mapper as connected controller state.
  ControlMapper mapper{};
  GamepadState disconnected{};
  assert(!mapper.update(disconnected, 10).has_jog);

  // Equal-strength matches are ambiguous and fail closed instead of depending
  // on registration order.
  const ControllerProfile first = fake_profile("test.first");
  const ControllerProfile second = fake_profile("test.second");
  const ControllerProfile* collision[] = {&first, &second};
  const ControllerProfileRegistry collision_registry{collision, 2};
  selected = collision_registry.select(probe);
  assert(selected.status == ProfileSelectionStatus::ambiguous);
  assert(selected.profile == nullptr);

  // A profile missing mandatory motion capabilities cannot be selected.
  ControllerProfile invalid = fake_profile("test.invalid");
  invalid.capabilities.held_deadman_candidate = false;
  const ControllerProfile* invalid_profiles[] = {&invalid};
  const ControllerProfileRegistry invalid_registry{invalid_profiles, 1};
  selected = invalid_registry.select(probe);
  assert(selected.status == ProfileSelectionStatus::unsupported);
}
