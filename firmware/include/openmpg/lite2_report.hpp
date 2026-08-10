#pragma once

#include <cstddef>
#include <cstdint>

#include "openmpg/types.hpp"

namespace openmpg {

struct ControllerProfile;

namespace lite2_button {
constexpr std::uint32_t a = 0x00001;
constexpr std::uint32_t b = 0x00002;
constexpr std::uint32_t home = 0x00004;
constexpr std::uint32_t x = 0x00008;
constexpr std::uint32_t y = 0x00010;
constexpr std::uint32_t left_shoulder = 0x00040;
constexpr std::uint32_t right_shoulder = 0x00080;
constexpr std::uint32_t left_trigger = 0x00100;
constexpr std::uint32_t right_trigger = 0x00200;
constexpr std::uint32_t minus = 0x00400;
constexpr std::uint32_t plus = 0x00800;
constexpr std::uint32_t left_stick = 0x02000;
constexpr std::uint32_t right_stick = 0x04000;
}  // namespace lite2_button

class Lite2ReportParser {
 public:
  // Parses the 11-byte Classic HID interrupt report, including A1 and report
  // ID 01. Returns false for any other shape and leaves output untouched.
  bool parse(const std::uint8_t* report, std::size_t length,
             std::uint32_t sample_ms, GamepadState& output) const;

 private:
  static std::int16_t normalize(std::uint8_t raw, int center);
};

const ControllerProfile& lite2_d_profile();

}  // namespace openmpg
