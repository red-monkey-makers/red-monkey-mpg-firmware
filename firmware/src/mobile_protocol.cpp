#include "openmpg/mobile_protocol.hpp"

namespace openmpg {
namespace {

constexpr std::int16_t kFullNegative = -32767;
constexpr std::int16_t kFullPositive = 32767;

MobileFrameResult reject(MobileFrameError error) {
  MobileFrameResult result{};
  result.error = error;
  return result;
}

}  // namespace

std::uint8_t mobile_crc8(const std::uint8_t* data, std::size_t length) {
  std::uint8_t crc = 0;
  for (std::size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit) {
      crc = static_cast<std::uint8_t>(
          (crc & 0x80u) != 0u ? (crc << 1u) ^ 0x07u : crc << 1u);
    }
  }
  return crc;
}

void MobileInputSession::reset() {
  has_sequence_ = false;
  last_sequence_ = 0;
  has_event_ = false;
  last_event_sequence_ = 0;
}

MobileFrameResult MobileInputSession::consume(const std::uint8_t* data,
                                              std::size_t length,
                                              std::uint32_t now_ms) {
  if (data == nullptr || length != kMobileFrameLength) {
    return reject(MobileFrameError::wrong_length);
  }
  if (data[0] != 0x52u) return reject(MobileFrameError::bad_magic);
  if (data[1] != kMobileProtocolVersion) {
    return reject(MobileFrameError::bad_version);
  }
  if (mobile_crc8(data, kMobileFrameLength - 1) != data[9]) {
    return reject(MobileFrameError::bad_crc);
  }
  if (data[8] != 0) return reject(MobileFrameError::nonzero_reserved);
  if ((data[5] & 0xF8u) != 0u) return reject(MobileFrameError::invalid_flags);
  if (data[4] > 6u) return reject(MobileFrameError::invalid_direction);
  if (data[7] > 7u) return reject(MobileFrameError::invalid_event);
  if ((data[5] & 0x06u) == 0x06u) {
    return reject(MobileFrameError::ambiguous_rate);
  }

  const auto sequence = static_cast<std::uint16_t>(
      data[2] | static_cast<std::uint16_t>(data[3]) << 8u);
  if (has_sequence_) {
    const auto delta = static_cast<std::uint16_t>(sequence - last_sequence_);
    if (delta == 0 || delta > 0x7FFFu) {
      return reject(MobileFrameError::replayed_sequence);
    }
  }
  has_sequence_ = true;
  last_sequence_ = sequence;

  MobileFrameResult result{};
  result.accepted = true;
  result.sequence = sequence;
  result.input.connected = true;
  result.input.sample_ms = now_ms;
  result.input.deadman = (data[5] & 0x01u) != 0u;
  result.input.rapid = (data[5] & 0x02u) != 0u;
  result.input.precision = (data[5] & 0x04u) != 0u;

  switch (data[4]) {
    case 0:
      break;
    case 1:
      result.input.left_x = kFullNegative;
      break;
    case 2:
      result.input.left_x = kFullPositive;
      break;
    case 3:
      result.input.left_y = kFullPositive;
      break;
    case 4:
      result.input.left_y = kFullNegative;
      break;
    case 5:
      result.input.right_y = kFullPositive;
      break;
    case 6:
      result.input.right_y = kFullNegative;
      break;
    default:
      return reject(MobileFrameError::invalid_direction);
  }

  const std::uint8_t event = data[7];
  if (event == 0u) return result;
  if (has_event_ && data[6] == last_event_sequence_) return result;
  has_event_ = true;
  last_event_sequence_ = data[6];
  switch (event) {
    case 1:
    case 2:
    case 3:
    case 4:
      result.input.select_resolution = true;
      result.input.selected_resolution =
          static_cast<StepResolution>(event - 1u);
      break;
    case 5:
      result.input.override_decrease = true;
      break;
    case 6:
      result.input.override_increase = true;
      break;
    case 7:
      result.input.cancel = true;
      break;
    default:
      return reject(MobileFrameError::invalid_event);
  }
  return result;
}

}  // namespace openmpg
