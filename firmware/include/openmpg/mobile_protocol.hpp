#pragma once

#include <cstddef>
#include <cstdint>

#include "openmpg/types.hpp"

namespace openmpg {

constexpr std::size_t kMobileFrameLength = 10;
constexpr std::uint8_t kMobileProtocolVersion = 2;

enum class MobileControllerProfile : std::uint8_t {
  masso = 1,
};

enum class MobileFrameError : std::uint8_t {
  none,
  wrong_length,
  bad_magic,
  bad_version,
  bad_crc,
  invalid_flags,
  invalid_direction,
  invalid_event,
  unsupported_profile,
  ambiguous_rate,
  replayed_sequence,
};

struct MobileFrameResult {
  bool accepted{};
  MobileFrameError error{MobileFrameError::none};
  std::uint16_t sequence{};
  MobileControllerProfile controller_profile{MobileControllerProfile::masso};
  GamepadState input{};
};

std::uint8_t mobile_crc8(const std::uint8_t* data, std::size_t length);

class MobileInputSession {
 public:
  MobileFrameResult consume(const std::uint8_t* data, std::size_t length,
                            std::uint32_t now_ms);
  void reset();

 private:
  bool has_sequence_{};
  std::uint16_t last_sequence_{};
  bool has_event_{};
  std::uint8_t last_event_sequence_{};
};

}  // namespace openmpg
