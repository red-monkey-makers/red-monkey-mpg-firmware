#include "openmpg/config_service.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "usb_keyboard_device.h"

namespace openmpg {
namespace {

const char* find_once(const char* line, const char* key) {
  const char* found = std::strstr(line, key);
  if (found == nullptr || std::strstr(found + 1, key) != nullptr) return nullptr;
  return found + std::strlen(key);
}

bool read_string(const char* line, const char* key, char* output,
                 std::size_t output_size) {
  const char* start = find_once(line, key);
  if (start == nullptr) return false;
  const char* end = std::strchr(start, '"');
  if (end == nullptr || end == start ||
      static_cast<std::size_t>(end - start) >= output_size) {
    return false;
  }
  std::memcpy(output, start, static_cast<std::size_t>(end - start));
  output[end - start] = '\0';
  return true;
}

bool read_unsigned(const char* line, const char* key, unsigned long& output) {
  const char* start = find_once(line, key);
  if (start == nullptr || *start < '0' || *start > '9') return false;
  char* end = nullptr;
  output = std::strtoul(start, &end, 10);
  if (end == start) return false;
  while (*end == ' ' || *end == '\t') ++end;
  return *end == ',' || *end == '}' || *end == '\0';
}

std::uint32_t control_mask(const char* value, bool allow_resolution) {
  if (std::strcmp(value, "L1") == 0) return 0x00040;
  if (std::strcmp(value, "R1") == 0) return 0x00080;
  if (std::strcmp(value, "L2") == 0) return 0x00100;
  if (std::strcmp(value, "R2") == 0) return 0x00200;
  if (allow_resolution && std::strcmp(value, "MINUS") == 0) return 0x00400;
  if (allow_resolution && std::strcmp(value, "PLUS") == 0) return 0x00800;
  if (allow_resolution && std::strcmp(value, "UNASSIGNED") == 0) return 0;
  return 0xffffffffu;
}

ButtonAction button_action(const char* value) {
  if (std::strcmp(value, "UNASSIGNED") == 0) return ButtonAction::unassigned;
  if (std::strcmp(value, "CANCEL") == 0) return ButtonAction::cancel;
  if (std::strcmp(value, "CYCLE_RESOLUTION") == 0)
    return ButtonAction::cycle_resolution;
  if (std::strcmp(value, "STEP_0_01") == 0) return ButtonAction::step_0_01;
  if (std::strcmp(value, "STEP_0_10") == 0) return ButtonAction::step_0_10;
  if (std::strcmp(value, "STEP_0_50") == 0) return ButtonAction::step_0_50;
  if (std::strcmp(value, "STEP_1_00") == 0) return ButtonAction::step_1_00;
  return static_cast<ButtonAction>(0xff);
}

const char* control_name(std::uint32_t mask) {
  switch (mask) {
    case 0x00040: return "L1";
    case 0x00080: return "R1";
    case 0x00100: return "L2";
    case 0x00200: return "R2";
    case 0x00400: return "MINUS";
    case 0x00800: return "PLUS";
    default: return "UNASSIGNED";
  }
}

const char* action_name(ButtonAction action) {
  switch (action) {
    case ButtonAction::unassigned: return "UNASSIGNED";
    case ButtonAction::cancel: return "CANCEL";
    case ButtonAction::cycle_resolution: return "CYCLE_RESOLUTION";
    case ButtonAction::step_0_01: return "STEP_0_01";
    case ButtonAction::step_0_10: return "STEP_0_10";
    case ButtonAction::step_0_50: return "STEP_0_50";
    case ButtonAction::step_1_00: return "STEP_1_00";
  }
  return "UNASSIGNED";
}

bool parse_mapping(const char* line, ProductionMapping& mapping) {
  char deadman[16], continuous[16], resolution[20];
  char face_a[24], face_b[24], face_x[24], face_y[24];
  if (!read_string(line, "\"deadman\":\"", deadman, sizeof(deadman)) ||
      !read_string(line, "\"continuous\":\"", continuous,
                   sizeof(continuous)) ||
      !read_string(line, "\"resolution\":\"", resolution,
                   sizeof(resolution)) ||
      !read_string(line, "\"faceA\":\"", face_a, sizeof(face_a)) ||
      !read_string(line, "\"faceB\":\"", face_b, sizeof(face_b)) ||
      !read_string(line, "\"faceX\":\"", face_x, sizeof(face_x)) ||
      !read_string(line, "\"faceY\":\"", face_y, sizeof(face_y))) {
    return false;
  }
  ProductionMapping candidate = mapping;
  if (std::strstr(line, "\"cncProfile\":\"") != nullptr) {
    char cnc_profile[48];
    if (!read_string(line, "\"cncProfile\":\"", cnc_profile,
                     sizeof(cnc_profile))) {
      return false;
    }
    const auto* selected = cnc_controller_profile_by_key(cnc_profile);
    if (selected == nullptr) return false;
    candidate.cnc_profile = selected->id;
  }
  candidate.deadman_mask = control_mask(deadman, false);
  candidate.continuous_mask = control_mask(continuous, false);
  candidate.resolution_mask = control_mask(resolution, true);
  candidate.face_a = button_action(face_a);
  candidate.face_b = button_action(face_b);
  candidate.face_x = button_action(face_x);
  candidate.face_y = button_action(face_y);
  if (validate_production_mapping(candidate) != ConfigValidation::valid) {
    return false;
  }
  mapping = candidate;
  return true;
}

}  // namespace

ConfigService::ConfigService(ProductionMapping& mapping,
                             PersistentConfig& storage,
                             ConfigServiceHooks hooks)
    : mapping_(mapping), storage_(storage), hooks_(hooks) {}

void ConfigService::set_identity(const char* serial, const char* firmware,
                                 const char* controller,
                                 const char* address) {
  serial_ = serial;
  firmware_ = firmware;
  controller_ = controller;
  address_ = address;
}

bool ConfigService::write_line(const char* line) {
  const std::size_t line_size = std::strlen(line);
  if (line_size > sizeof(tx_) - 1) return false;

  const std::size_t pending = tx_size_ - tx_offset_;
  if (tx_offset_ != 0 && sizeof(tx_) - tx_size_ < line_size + 1) {
    std::memmove(tx_, tx_ + tx_offset_, pending);
    tx_size_ = pending;
    tx_offset_ = 0;
  }
  if (sizeof(tx_) - tx_size_ < line_size + 1) return false;

  std::memcpy(tx_ + tx_size_, line, line_size);
  tx_size_ += line_size;
  tx_[tx_size_++] = '\n';
  service_tx();
  return true;
}

void ConfigService::service_tx() {
  if (tx_offset_ == tx_size_) {
    tx_offset_ = 0;
    tx_size_ = 0;
    return;
  }
  const std::size_t pending = tx_size_ - tx_offset_;
  const auto written = openmpg_usb_config_write(
      tx_ + tx_offset_, static_cast<std::uint32_t>(pending));
  if (written > pending) {
    // A broken USB backend must not move the queue cursor out of bounds.
    return;
  }
  tx_offset_ += written;
  if (written != 0) openmpg_usb_config_flush();
  if (tx_offset_ == tx_size_) {
    tx_offset_ = 0;
    tx_size_ = 0;
  }
}

void ConfigService::respond(bool ok, const char* body) {
  char response[896];
  std::snprintf(response, sizeof(response),
                "{\"protocol\":1,\"ok\":%s,%s}", ok ? "true" : "false",
                body);
  write_line(response);
}

void ConfigService::handle_line(char* line) {
  unsigned long protocol = 0;
  if (!read_unsigned(line, "\"protocol\":", protocol) || protocol != 1) {
    respond(false, "\"error\":\"UNSUPPORTED_PROTOCOL\"");
    return;
  }
  char command[32];
  if (!read_string(line, "\"command\":\"", command, sizeof(command))) {
    respond(false, "\"error\":\"INVALID_REQUEST\"");
    return;
  }
  if (std::strcmp(command, "GET_INFO") == 0) {
    char body[700];
    std::snprintf(
        body, sizeof(body),
        "\"result\":{\"serial\":\"%s\",\"firmware\":\"%s\","
        "\"controller\":%s%s%s,\"address\":%s%s%s,"
        "\"configurationSequence\":%lu}",
        serial_, firmware_, controller_ ? "\"" : "null",
        controller_ ? controller_ : "", controller_ ? "\"" : "",
        address_ ? "\"" : "null", address_ ? address_ : "",
        address_ ? "\"" : "", static_cast<unsigned long>(storage_.sequence()));
    respond(true, body);
    return;
  }
  if (std::strcmp(command, "GET_CONFIG") == 0) {
    char body[700];
    const auto* profile = cnc_controller_profile(mapping_.cnc_profile);
    if (profile == nullptr) {
      respond(false, "\"error\":\"UNSUPPORTED_CNC_PROFILE\"");
      return;
    }
    std::snprintf(
        body, sizeof(body),
        "\"result\":{\"schema\":1,\"mapping\":{"
        "\"cncProfile\":\"%s\","
        "\"deadman\":\"%s\",\"continuous\":\"%s\","
        "\"resolution\":\"%s\",\"faceA\":\"%s\","
        "\"faceB\":\"%s\",\"faceX\":\"%s\",\"faceY\":\"%s\"}}",
        profile->key, control_name(mapping_.deadman_mask),
        control_name(mapping_.continuous_mask),
        control_name(mapping_.resolution_mask), action_name(mapping_.face_a),
        action_name(mapping_.face_b), action_name(mapping_.face_x),
        action_name(mapping_.face_y));
    respond(true, body);
    return;
  }
  if (std::strcmp(command, "GET_CNC_PROFILES") == 0) {
    char body[700];
    std::size_t used = static_cast<std::size_t>(
        std::snprintf(body, sizeof(body), "\"result\":{\"profiles\":["));
    bool complete = used < sizeof(body);
    for (std::size_t index = 0;
         complete && index < cnc_controller_profile_count(); ++index) {
      const auto* profile = cnc_controller_profile_at(index);
      if (profile == nullptr) {
        complete = false;
        break;
      }
      // Registry strings are compile-time constants reviewed for JSON-safe
      // printable content; protocol clients cannot supply them.
      const int written = std::snprintf(
          body + used, sizeof(body) - used,
          "%s{\"id\":\"%s\",\"name\":\"%s\","
          "\"description\":\"%s\","
          "\"supportsStepResolution\":%s,"
          "\"supportsOverrideAdjustment\":%s}",
          index == 0 ? "" : ",", profile->key, profile->name,
          profile->description,
          profile->supports_step_resolution ? "true" : "false",
          profile->supports_override_adjustment ? "true" : "false");
      if (written < 0 || static_cast<std::size_t>(written) >=
                             sizeof(body) - used) {
        complete = false;
      } else {
        used += static_cast<std::size_t>(written);
      }
    }
    if (complete) {
      const int written =
          std::snprintf(body + used, sizeof(body) - used, "]}");
      complete = written >= 0 && static_cast<std::size_t>(written) <
                                     sizeof(body) - used;
    }
    if (!complete) {
      respond(false, "\"error\":\"PROFILE_LIST_TOO_LARGE\"");
      return;
    }
    respond(true, body);
    return;
  }
  if (std::strcmp(command, "SET_CONFIG") == 0) {
    unsigned long schema = 0;
    if (!read_unsigned(line, "\"schema\":", schema) || schema != 1) {
      respond(false, "\"error\":\"UNSUPPORTED_SCHEMA\"");
      return;
    }
    ProductionMapping candidate = mapping_;
    if (!parse_mapping(line, candidate)) {
      respond(false, "\"error\":\"INVALID_MAPPING\"");
      return;
    }
    if (!storage_.store(candidate)) {
      respond(false, "\"error\":\"CONFIG_STORE_FAILED\"");
      return;
    }
    mapping_ = candidate;
    respond(true, "\"result\":{\"saved\":true}");
    return;
  }
  if (std::strcmp(command, "START_SCAN") == 0) {
    if (hooks_.start_scan == nullptr) {
      respond(false, "\"error\":\"PAIRING_NOT_AVAILABLE\"");
    } else {
      hooks_.start_scan();
      respond(true, "\"result\":{\"scanning\":true}");
    }
    return;
  }
  if (std::strcmp(command, "PAIR_CANDIDATE") == 0) {
    char candidate[24];
    if (!read_string(line, "\"address\":\"", candidate,
                     sizeof(candidate)) || hooks_.select_candidate == nullptr ||
        !hooks_.select_candidate(candidate)) {
      respond(false, "\"error\":\"PAIR_CANDIDATE_REJECTED\"");
    } else {
      respond(true, "\"result\":{\"connecting\":true}");
    }
    return;
  }
  if (std::strcmp(command, "CONFIRM_PAIRING") == 0) {
    if (hooks_.confirm_pairing != nullptr && hooks_.confirm_pairing()) {
      respond(true, "\"result\":{\"paired\":true}");
    } else {
      respond(false, "\"error\":\"PAIRING_NOT_CONFIRMED\"");
    }
    return;
  }
  if (std::strcmp(command, "CANCEL_PAIRING") == 0) {
    if (hooks_.cancel_pairing != nullptr) hooks_.cancel_pairing();
    respond(true, "\"result\":{\"cancelled\":true}");
    return;
  }
  respond(false, "\"error\":\"UNKNOWN_COMMAND\"");
}

void ConfigService::service() {
  const bool connected = openmpg_usb_config_connected();
  if (!connected) {
    active_ = false;
    input_size_ = 0;
    tx_size_ = 0;
    tx_offset_ = 0;
    overflow_ = false;
    return;
  }
  active_ = true;
  service_tx();
  // Apply backpressure at complete-frame boundaries. This keeps command
  // responses ordered without ever blocking the motion-safety loop.
  if (tx_size_ != 0) return;
  // Bound work per pass so even a hostile or malfunctioning serial host cannot
  // starve stale-input handling and the all-keys-up USB safety path.
  constexpr std::uint32_t kMaxBytesPerService = 256;
  std::uint32_t processed = 0;
  while (processed < kMaxBytesPerService &&
         openmpg_usb_config_available() != 0) {
    char byte;
    if (openmpg_usb_config_read(&byte, 1) != 1) break;
    ++processed;
    if (byte == '\n') {
      if (!overflow_ && input_size_ != 0) {
        input_[input_size_] = '\0';
        handle_line(input_);
      } else if (overflow_) {
        respond(false, "\"error\":\"FRAME_TOO_LARGE\"");
      }
      input_size_ = 0;
      overflow_ = false;
    } else if (byte != '\r') {
      if (static_cast<unsigned char>(byte) < 0x20) {
        overflow_ = true;
        continue;
      }
      if (input_size_ < sizeof(input_) - 1) {
        input_[input_size_++] = byte;
      } else {
        overflow_ = true;
      }
    }
  }
  service_tx();
}

void ConfigService::emit_scan_result(const char* name, const char* address,
                                     int rssi) {
  if (!active_) return;
  char event[300];
  std::snprintf(event, sizeof(event),
                "{\"protocol\":1,\"event\":\"SCAN_RESULT\","
                "\"candidate\":{\"name\":\"%s\",\"address\":\"%s\","
                "\"rssi\":%d}}",
                name, address, rssi);
  write_line(event);
}

void ConfigService::emit_pairing_state(const char* state) {
  if (!active_) return;
  char event[180];
  std::snprintf(event, sizeof(event),
                "{\"protocol\":1,\"event\":\"PAIRING_STATE\","
                "\"state\":\"%s\"}", state);
  write_line(event);
}

}  // namespace openmpg
