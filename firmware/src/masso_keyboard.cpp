#include "openmpg/masso_keyboard.hpp"

#include <cstring>

namespace openmpg {
namespace {

constexpr std::uint8_t kLeftShift = 0x02;
constexpr std::uint8_t kKeyD = 0x07;
constexpr std::uint8_t kKeyU = 0x18;
constexpr std::uint8_t kKeyRight = 0x4f;
constexpr std::uint8_t kKeyLeft = 0x50;
constexpr std::uint8_t kKeyDown = 0x51;
constexpr std::uint8_t kKeyUp = 0x52;
constexpr std::uint8_t kKey1 = 0x1e;
constexpr std::uint8_t kKey2 = 0x1f;
constexpr std::uint8_t kKey3 = 0x20;
constexpr std::uint8_t kKey4 = 0x21;

constexpr CncControllerProfile kProfiles[] = {{
    CncControllerProfileId::masso_g3_touch_5_13,
    "masso-g3-touch-5.13",
    "MASSO G3 Touch 5.13",
    "Arrow/U/D jogging, Shift continuous jog, 1-4 step selection",
    true,
    masso_g3_keyboard_report,
}};

}  // namespace

std::size_t cnc_controller_profile_count() {
  return sizeof(kProfiles) / sizeof(kProfiles[0]);
}

const CncControllerProfile* cnc_controller_profile_at(std::size_t index) {
  return index < cnc_controller_profile_count() ? &kProfiles[index] : nullptr;
}

const CncControllerProfile* cnc_controller_profile(CncControllerProfileId id) {
  for (const auto& profile : kProfiles) {
    if (profile.id == id) return &profile;
  }
  return nullptr;
}

const CncControllerProfile* cnc_controller_profile_by_key(const char* key) {
  if (key == nullptr) return nullptr;
  for (const auto& profile : kProfiles) {
    if (std::strcmp(profile.key, key) == 0) return &profile;
  }
  return nullptr;
}

bool is_supported_cnc_controller_profile(CncControllerProfileId id) {
  return cnc_controller_profile(id) != nullptr;
}

KeyboardReport cnc_controller_keyboard_report(CncControllerProfileId id,
                                              const OutputFrame& frame) {
  const auto* profile = cnc_controller_profile(id);
  return profile == nullptr ? KeyboardReport{} : profile->keyboard_report(frame);
}

KeyboardReport masso_g3_keyboard_report(const OutputFrame& frame) {
  KeyboardReport report{};
  if (frame.release_all) {
    return report;
  }

  if (frame.has_step_resolution) {
    switch (frame.step_resolution) {
      case StepResolution::mm_0_01:
        report[2] = kKey4;
        break;
      case StepResolution::mm_0_10:
        report[2] = kKey3;
        break;
      case StepResolution::mm_0_50:
        report[2] = kKey2;
        break;
      case StepResolution::mm_1_00:
        report[2] = kKey1;
        break;
    }
    return report;
  }

  if (!frame.has_jog) return report;

  // MASSO defines Shift + direction as rapid/continuous jogging. Precision and
  // normal are both emitted as unmodified step-jog keys.
  if (frame.jog.rate == JogRate::rapid) {
    report[0] = kLeftShift;
  }

  switch (frame.jog.axis) {
    case Axis::x:
      report[2] = frame.jog.direction == Direction::positive ? kKeyRight
                                                              : kKeyLeft;
      break;
    case Axis::y:
      report[2] = frame.jog.direction == Direction::positive ? kKeyUp
                                                              : kKeyDown;
      break;
    case Axis::z:
      report[2] = frame.jog.direction == Direction::positive ? kKeyU : kKeyD;
      break;
  }
  return report;
}

}  // namespace openmpg
