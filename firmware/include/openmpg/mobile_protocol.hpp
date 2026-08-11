#pragma once

#include <cstddef>
#include <cstdint>

#include "openmpg/types.hpp"

namespace openmpg {

constexpr std::size_t kMobileFrameLength = 10;
constexpr std::uint8_t kMobileProtocolVersion = 1;

enum class MobileFrameError : std::uint8_t {
  none,
  wrong_length,
  bad_magic,
  bad_version,
  bad_crc,
  nonzero_reserved,
  invalid_flags,
  invalid_direction,
  invalid_event,
  ambiguous_rate,
  replayed_sequence,
};

struct MobileFrameResult {
  bool accepted{};
  MobileFrameError error{MobileFrameError::none};
  std::uint16_t sequence{};
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
