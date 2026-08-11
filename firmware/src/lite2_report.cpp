#include "openmpg/lite2_report.hpp"

#include <algorithm>
#include <cstring>

#include "openmpg/controller_profile.hpp"

namespace openmpg {

std::int16_t Lite2ReportParser::normalize(std::uint8_t raw, int center) {
  const int scaled = (static_cast<int>(raw) - center) * 256;
  return static_cast<std::int16_t>(std::clamp(scaled, -32768, 32767));
}

bool Lite2ReportParser::parse(const std::uint8_t* report, std::size_t length,
                              std::uint32_t sample_ms,
                              GamepadState& output) const {
  if (report == nullptr || length != 11 || report[0] != 0xA1 ||
      report[1] != 0x01) {
    return false;
  }

  GamepadState parsed{};
  parsed.buttons = static_cast<std::uint32_t>(report[2]) |
                   (static_cast<std::uint32_t>(report[3]) << 8) |
                   (static_cast<std::uint32_t>(report[4] & 0x0F) << 16);
  parsed.hat = report[4] >> 4;
  parsed.left_x = normalize(report[5], 127);
  parsed.left_y = normalize(report[6], 127);
  parsed.right_x = normalize(report[7], 127);
  parsed.right_y = normalize(report[8], 128);
  parsed.right_trigger = report[9];   // HID consumer C4
  parsed.left_trigger = report[10];   // HID consumer C5
  parsed.deadman = (parsed.buttons & lite2_button::left_trigger) != 0;
  parsed.rapid = (parsed.buttons & lite2_button::right_trigger) != 0;
  parsed.precision = (parsed.buttons & lite2_button::right_shoulder) != 0;
  parsed.cycle_resolution = (parsed.buttons & lite2_button::a) != 0;
  parsed.cancel = (parsed.buttons & lite2_button::b) != 0;
  parsed.connected = true;
  parsed.sample_ms = sample_ms;
  output = parsed;
  return true;
}

namespace {

ProfileMatch match_lite2_d(const ControllerProbe& probe) {
  if (probe.transport != ControllerTransport::bluetooth_classic_hid ||
      probe.hid_descriptor == nullptr || probe.hid_descriptor_length != 83) {
    return ProfileMatch::unsupported;
  }
  if (probe.advertised_name != nullptr) {
    if (std::strcmp(probe.advertised_name, "8BitDo Lite 2") == 0 ||
        std::strcmp(probe.advertised_name, "8BitDo Lite 2 (D mode)") == 0) {
      return ProfileMatch::exact;
    }
    return ProfileMatch::unsupported;
  }
  if (probe.bluetooth_device_class == 0x002508) return ProfileMatch::exact;
  // Bonded reconnects do not repeat inquiry identity. The descriptor and the
  // previously stored profile are the available compatibility evidence.
  return ProfileMatch::compatible;
}

bool parse_lite2_d(const std::uint8_t* report, std::size_t length,
                   std::uint32_t sample_ms, GamepadState& output) {
  static const Lite2ReportParser parser{};
  return parser.parse(report, length, sample_ms, output);
}

}  // namespace

const ControllerProfile& lite2_d_profile() {
  static const ControllerProfile profile{
      "8bitdo.lite2.dinput",
      "8BitDo Lite 2 — D-input",
      ControllerTransport::bluetooth_classic_hid,
      ProfileMaturity::community_tested,
      {/*x_axis=*/true, /*y_axis=*/true, /*z_axis=*/true,
       /*held_deadman_candidate=*/true,
       /*continuous_modifier_candidate=*/true,
       /*precision_modifier_candidate=*/true},
      match_lite2_d,
      parse_lite2_d};
  return profile;
}

}  // namespace openmpg
