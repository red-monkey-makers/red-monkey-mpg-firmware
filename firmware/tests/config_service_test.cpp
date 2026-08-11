#include "red_monkey_mpg/config_service.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

namespace {

bool usb_connected = true;
std::string usb_input;
std::size_t usb_input_offset = 0;
std::string usb_output;
std::size_t usb_write_limit = static_cast<std::size_t>(-1);

void feed(const char* frame) {
  usb_input.append(frame);
}

bool output_contains(const char* text) {
  return usb_output.find(text) != std::string::npos;
}

void clear_output() { usb_output.clear(); }

}  // namespace

extern "C" {

bool red_monkey_mpg_usb_config_connected(void) { return usb_connected; }

std::uint32_t red_monkey_mpg_usb_config_available(void) {
  return static_cast<std::uint32_t>(usb_input.size() - usb_input_offset);
}

std::uint32_t red_monkey_mpg_usb_config_read(void* buffer, std::uint32_t length) {
  const std::size_t available = usb_input.size() - usb_input_offset;
  const std::size_t amount = available < length ? available : length;
  if (amount == 0) return 0;
  std::memcpy(buffer, usb_input.data() + usb_input_offset, amount);
  usb_input_offset += amount;
  if (usb_input_offset == usb_input.size()) {
    usb_input.clear();
    usb_input_offset = 0;
  }
  return static_cast<std::uint32_t>(amount);
}

std::uint32_t red_monkey_mpg_usb_config_write(const void* buffer,
                                       std::uint32_t length) {
  const std::size_t amount =
      length < usb_write_limit ? length : usb_write_limit;
  usb_output.append(static_cast<const char*>(buffer), amount);
  return static_cast<std::uint32_t>(amount);
}

void red_monkey_mpg_usb_config_flush(void) {}

}  // extern "C"

namespace red_monkey_mpg {

bool PersistentConfig::load(ProductionMapping&) { return false; }

bool PersistentConfig::store(const ProductionMapping& mapping) {
  if (validate_production_mapping(mapping) != ConfigValidation::valid) {
    return false;
  }
  active_tag_ ^= 1u;
  ++sequence_;
  return true;
}

bool PersistentConfig::store_controller_address(const ProductionMapping&,
                                                const char*) {
  return false;
}

}  // namespace red_monkey_mpg

int main() {
  using namespace red_monkey_mpg;
  ProductionMapping mapping{};
  PersistentConfig storage{};
  ConfigService service{mapping, storage};
  service.set_identity("0123456789ABCDEF", "test", nullptr, nullptr);

  feed("{\"protocol\":1,\"command\":\"GET_INFO\"}\n");
  service.service();
  assert(service.active());
  assert(output_contains("\"ok\":true"));
  assert(output_contains("0123456789ABCDEF"));

  clear_output();
  feed("{\"protocol\":10,\"command\":\"GET_INFO\"}\n");
  service.service();
  assert(output_contains("UNSUPPORTED_PROTOCOL"));

  clear_output();
  feed("{\"protocol\":1,\"protocol\":1,\"command\":\"GET_INFO\"}\n");
  service.service();
  assert(output_contains("UNSUPPORTED_PROTOCOL"));

  clear_output();
  feed("{\"protocol\":1,\"command\":\"SET_CONFIG\",\"schema\":2}\n");
  service.service();
  assert(output_contains("UNSUPPORTED_SCHEMA"));

  clear_output();
  feed("{\"protocol\":1,\"command\":\"GET_CNC_PROFILES\"}\n");
  service.service();
  assert(output_contains("masso-g3-touch-5.13"));

  constexpr const char* kValidMapping =
      "{\"protocol\":1,\"command\":\"SET_CONFIG\",\"schema\":1,"
      "\"mapping\":{\"cncProfile\":\"masso-g3-touch-5.13\","
      "\"deadman\":\"L2\",\"continuous\":\"R2\","
      "\"resolution\":\"UNASSIGNED\",\"faceA\":\"CYCLE_RESOLUTION\","
      "\"faceB\":\"CANCEL\",\"faceX\":\"UNASSIGNED\","
      "\"faceY\":\"UNASSIGNED\"}}\n";
  clear_output();
  feed(kValidMapping);
  service.service();
  assert(output_contains("\"saved\":true"));
  assert(storage.sequence() == 1);

  clear_output();
  feed("{\"protocol\":1,\"command\":\"SET_CONFIG\",\"schema\":1,"
       "\"mapping\":{\"cncProfile\":\"unsupported\","
       "\"deadman\":\"L2\",\"continuous\":\"R2\","
       "\"resolution\":\"UNASSIGNED\",\"faceA\":\"CYCLE_RESOLUTION\","
       "\"faceB\":\"CANCEL\",\"faceX\":\"UNASSIGNED\","
       "\"faceY\":\"UNASSIGNED\"}}\n");
  service.service();
  assert(output_contains("INVALID_MAPPING"));
  assert(storage.sequence() == 1);

  constexpr const char* kPrecisionAlias =
      "{\"protocol\":1,\"command\":\"SET_CONFIG\",\"schema\":1,"
      "\"mapping\":{\"deadman\":\"R1\",\"continuous\":\"R2\","
      "\"resolution\":\"UNASSIGNED\",\"faceA\":\"CYCLE_RESOLUTION\","
      "\"faceB\":\"CANCEL\",\"faceX\":\"UNASSIGNED\","
      "\"faceY\":\"UNASSIGNED\"}}\n";
  clear_output();
  feed(kPrecisionAlias);
  service.service();
  assert(output_contains("INVALID_MAPPING"));
  assert(storage.sequence() == 1);

  // Work is bounded to 256 input bytes per service call.
  clear_output();
  feed(std::string(300, 'x').c_str());
  service.service();
  assert(red_monkey_mpg_usb_config_available() == 44);
  feed("\n");
  service.service();
  assert(output_contains("UNSUPPORTED_PROTOCOL"));

  clear_output();
  feed(std::string(1100, 'x').c_str());
  feed("\n");
  while (red_monkey_mpg_usb_config_available() != 0) service.service();
  assert(output_contains("FRAME_TOO_LARGE"));

  // TinyUSB is allowed to accept only part of a write. Verify that a response
  // is retained and delivered exactly once instead of being truncated.
  clear_output();
  usb_write_limit = 7;
  feed("{\"protocol\":1,\"command\":\"GET_INFO\"}\n");
  service.service();
  assert(usb_output.size() == 14);  // write_line() plus end-of-service drain.
  for (int attempt = 0; attempt < 200 &&
       (usb_output.empty() || usb_output.back() != '\n'); ++attempt) {
    service.service();
  }
  assert(!usb_output.empty() && usb_output.back() == '\n');
  assert(output_contains("0123456789ABCDEF"));
  assert(usb_output.find('\n') == usb_output.size() - 1);
  usb_write_limit = static_cast<std::size_t>(-1);

  // A second command remains unread until the prior response is drained.
  clear_output();
  usb_write_limit = 1;
  feed("{\"protocol\":1,\"command\":\"GET_CONFIG\"}\n");
  service.service();
  feed("{\"protocol\":1,\"command\":\"GET_INFO\"}\n");
  const auto queued_input = red_monkey_mpg_usb_config_available();
  service.service();
  assert(red_monkey_mpg_usb_config_available() == queued_input);
  usb_write_limit = static_cast<std::size_t>(-1);
  service.service();
  service.service();
  assert(output_contains("\"mapping\""));
  assert(output_contains("0123456789ABCDEF"));

  // A control character invalidates the complete frame instead of allowing a
  // C-string truncation to turn it into a different command.
  clear_output();
  std::string control_frame =
      "{\"protocol\":1,\"command\":\"GET_INFO\"}";
  control_frame.push_back('\0');
  control_frame.push_back('\n');
  feed(control_frame.c_str());
  // c_str() intentionally ends at NUL, so feed the exact bytes as a second step.
  usb_input.push_back('\0');
  usb_input.push_back('\n');
  service.service();
  assert(output_contains("FRAME_TOO_LARGE"));

  // Disconnecting clears an incomplete request and leaves the service inactive.
  clear_output();
  feed("{\"protocol\":1");
  service.service();
  usb_connected = false;
  service.service();
  assert(!service.active());
  usb_connected = true;
  feed(",\"command\":\"GET_INFO\"}\n");
  service.service();
  assert(output_contains("UNSUPPORTED_PROTOCOL"));
}
