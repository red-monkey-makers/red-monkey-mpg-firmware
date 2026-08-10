// Live Lite 2 -> CNC-profile USB bridge for validation on a normal computer.
// Do not connect this bench image to a machine controller.

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "btstack.h"
#include "hardware/watchdog.h"
#include "openmpg/control_mapper.hpp"
#include "openmpg/controller_profile.hpp"
#include "openmpg/descriptor_identity.h"
#include "openmpg/masso_keyboard.hpp"
#if OPENMPG_ENABLE_CONFIG_CDC
#include "openmpg/config_service.hpp"
#include "openmpg/persistent_config.hpp"
#include "openmpg/production_config.hpp"
#include "pico/unique_id.h"
#endif
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "usb_keyboard_device.h"

#ifndef OPENMPG_CONTROLLER_ADDRESS
#error OPENMPG_CONTROLLER_ADDRESS must be supplied by CMake
#endif
#ifndef OPENMPG_LITE2_DESCRIPTOR_SHA256
#define OPENMPG_LITE2_DESCRIPTOR_SHA256 ""
#endif
#if OPENMPG_ENABLE_CONFIG_CDC && !defined(OPENMPG_POLL_BTSTACK)
#error Configurable receivers require single-threaded polled BTstack servicing
#endif

namespace {

constexpr std::size_t kDescriptorBytes = 512;
constexpr std::uint32_t kReconnectMs = 3000;
constexpr std::uint32_t kStaleMs = 150;
constexpr std::int16_t kCenterLimit = 9000;

bd_addr_t controller_address{};
std::uint8_t descriptor_storage[kDescriptorBytes]{};
std::uint16_t hid_cid{};
std::uint16_t last_hid_cid{};
btstack_packet_callback_registration_t hci_callback{};
btstack_timer_source_t reconnect_timer{};
#if OPENMPG_ENABLE_CONFIG_CDC
btstack_timer_source_t pairing_scan_timer{};
#endif
std::atomic_bool descriptor_ready{};
std::atomic_bool controller_connected{};
std::atomic_bool controller_address_valid{};
std::atomic_bool rearm_required{true};
std::atomic_uint32_t last_valid_report_ms{};

const openmpg::ControllerProfile* active_profile{};
openmpg::ControlMapper mapper{};

#if OPENMPG_ENABLE_CONFIG_CDC
void start_pairing_scan();
bool select_pairing_candidate(const char* address);
bool confirm_pairing_candidate();
void cancel_pairing();

openmpg::ProductionMapping production_mapping{};
openmpg::PersistentConfig persistent_config{};
openmpg::ConfigService config_service{
    production_mapping, persistent_config,
    {start_pairing_scan, select_pairing_candidate, confirm_pairing_candidate,
     cancel_pairing}};
char receiver_serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1]{};
char active_controller_address[18]{};
char candidate_controller_address[18]{};
char active_controller_name[64]{};
char candidate_controller_name[64]{};
std::uint32_t active_controller_device_class{};
std::uint32_t candidate_controller_device_class{};
std::atomic_bool pairing_scan_active{};
std::atomic_bool pairing_scan_requested{};
std::atomic_bool pairing_candidate_selected{};
std::atomic_bool pairing_chord_confirmed{};
std::uint32_t pairing_chord_started_ms{};
std::uint32_t pairing_scan_requested_ms{};
#endif

critical_section_t report_lock{};
std::array<std::uint8_t, 8> desired_report{};
std::array<std::uint8_t, 8> sent_report{};

void publish_report(const openmpg::KeyboardReport& report) {
  critical_section_enter_blocking(&report_lock);
  desired_report = report;
  critical_section_exit(&report_lock);
}

void publish_release() { publish_report({}); }

openmpg::KeyboardReport read_desired_report() {
  openmpg::KeyboardReport copy{};
  critical_section_enter_blocking(&report_lock);
  copy = desired_report;
  critical_section_exit(&report_lock);
  return copy;
}

bool report_has_key(const openmpg::KeyboardReport& report) {
  for (const auto byte : report) {
    if (byte != 0) return true;
  }
  return false;
}

void reset_mapper(std::uint32_t now_ms) {
  openmpg::GamepadState disconnected{};
  disconnected.sample_ms = now_ms;
  (void)mapper.update(disconnected, now_ms);
}

bool controls_are_safe_to_arm(const openmpg::GamepadState& input) {
  const auto magnitude = [](std::int16_t value) {
    return value < 0 ? -static_cast<int>(value) : static_cast<int>(value);
  };
  return !input.deadman && magnitude(input.left_x) <= kCenterLimit &&
         magnitude(input.left_y) <= kCenterLimit &&
         magnitude(input.right_y) <= kCenterLimit;
}

void consume_controller_report(const std::uint8_t* report,
                               std::uint16_t length,
                               std::uint32_t now_ms) {
  openmpg::GamepadState input{};
  if (!descriptor_ready.load() || active_profile == nullptr ||
      !active_profile->parse_report(report, length, now_ms, input)) {
    rearm_required = true;
    last_valid_report_ms = 0;
    reset_mapper(now_ms);
    publish_release();
    return;
  }

#if OPENMPG_ENABLE_CONFIG_CDC
  input = openmpg::apply_production_mapping(input, production_mapping);
  if (pairing_candidate_selected.load()) {
    if (config_service.active()) {
      constexpr std::uint32_t kConfirmationChord =
          0x00100 | 0x00200 | 0x00800;
      if ((input.buttons & kConfirmationChord) == kConfirmationChord) {
        if (pairing_chord_started_ms == 0) pairing_chord_started_ms = now_ms;
        if (!pairing_chord_confirmed.load() &&
            now_ms - pairing_chord_started_ms >= 3000) {
          pairing_chord_confirmed = true;
          config_service.emit_pairing_state("physical_confirmation_complete");
        }
      } else {
        pairing_chord_started_ms = 0;
      }
    }
    // An uncommitted candidate is never a motion source, including during the
    // short interval while a setup disconnect is being observed by main().
    publish_release();
    return;
  }
  if (config_service.active()) {
    publish_release();
    return;
  }
#endif

  last_valid_report_ms = now_ms;
  if (rearm_required.load()) {
    reset_mapper(now_ms);
    // Latch any already-held non-motion button so it cannot become a false
    // rising edge immediately after the neutral re-arm report.
    (void)mapper.update(input, now_ms);
    publish_release();
    if (controls_are_safe_to_arm(input)) rearm_required = false;
    return;
  }

  publish_report(openmpg::cnc_controller_keyboard_report(
      production_mapping.cnc_profile, mapper.update(input, now_ms)));
}

void schedule_connect(std::uint32_t delay_ms);

#if OPENMPG_ENABLE_CONFIG_CDC
void begin_pairing_scan(btstack_timer_source_t* timer) {
  (void)timer;
  if (!pairing_scan_requested.load()) return;

  const std::uint32_t now_ms = to_ms_since_boot(get_absolute_time());
  if (now_ms - pairing_scan_requested_ms > 65000) {
    pairing_scan_requested = false;
    config_service.emit_pairing_state("scan_failed");
    return;
  }

  const std::uint8_t status = gap_inquiry_start(47);
  if (status == ERROR_CODE_SUCCESS) {
    pairing_scan_active = true;
    pairing_scan_requested = false;
    return;
  }

  // A just-cancelled HID connection may still own the HCI command channel.
  // Retry after it settles instead of leaving the setup UI falsely scanning.
  btstack_run_loop_set_timer_handler(&pairing_scan_timer, begin_pairing_scan);
  btstack_run_loop_set_timer(&pairing_scan_timer, 500);
  btstack_run_loop_add_timer(&pairing_scan_timer);
}
#endif

void connect_controller(btstack_timer_source_t* timer) {
  (void)timer;
  if (hid_cid != 0 || !controller_address_valid.load()) return;
  const std::uint8_t status =
      hid_host_connect(controller_address, HID_PROTOCOL_MODE_REPORT, &hid_cid);
  if (status != ERROR_CODE_SUCCESS) {
    if (status == ERROR_CODE_COMMAND_DISALLOWED && last_hid_cid != 0) {
      hid_host_disconnect(last_hid_cid);
    }
    hid_cid = 0;
    schedule_connect(5000);
  } else {
    last_hid_cid = 0;
  }
}

void schedule_connect(std::uint32_t delay_ms) {
  btstack_run_loop_remove_timer(&reconnect_timer);
  btstack_run_loop_set_timer_handler(&reconnect_timer, connect_controller);
  btstack_run_loop_set_timer(&reconnect_timer, delay_ms);
  btstack_run_loop_add_timer(&reconnect_timer);
}

void fail_closed() {
  controller_connected = false;
  descriptor_ready = false;
  active_profile = nullptr;
  rearm_required = true;
  last_valid_report_ms = 0;
  publish_release();
}

void packet_handler(std::uint8_t packet_type, std::uint16_t channel,
                    std::uint8_t* packet, std::uint16_t size) {
  (void)channel;
  (void)size;
  if (packet_type != HCI_EVENT_PACKET) return;

  bd_addr_t event_address{};
  switch (hci_event_packet_get_type(packet)) {
#if OPENMPG_ENABLE_CONFIG_CDC
    case GAP_EVENT_INQUIRY_RESULT: {
      if (!pairing_scan_active.load()) break;

      // The Lite 2 does not include its name in every inquiry response. Its
      // verified D-mode class is 0x002508 (peripheral/gamepad). Accept either
      // the exact advertised name or that class, then enforce the known
      // 83-byte HID descriptor and physical confirmation chord before saving.
      constexpr std::uint32_t kLite2DeviceClass = 0x002508;
      const std::uint32_t device_class =
          gap_event_inquiry_result_get_class_of_device(packet);
      char name[64] = "8BitDo Lite 2";
      bool exact_name = false;
      if (gap_event_inquiry_result_get_name_available(packet)) {
        const std::uint8_t name_length =
            gap_event_inquiry_result_get_name_len(packet);
        const std::uint8_t* name_bytes =
            gap_event_inquiry_result_get_name(packet);
        const std::size_t copy_length =
            name_length < sizeof(name) - 1 ? name_length : sizeof(name) - 1;
        std::memcpy(name, name_bytes, copy_length);
        name[copy_length] = '\0';
        exact_name = std::strcmp(name, "8BitDo Lite 2") == 0;
      }
      if (!exact_name && device_class != kLite2DeviceClass) break;
      if (!exact_name) {
        std::snprintf(name, sizeof(name), "%s", "8BitDo Lite 2 (D mode)");
      }
      gap_event_inquiry_result_get_bd_addr(packet, event_address);
      std::snprintf(candidate_controller_address,
                    sizeof(candidate_controller_address), "%s",
                    bd_addr_to_str(event_address));
      std::snprintf(candidate_controller_name,
                    sizeof(candidate_controller_name), "%s", name);
      candidate_controller_device_class = device_class;
      const int rssi = gap_event_inquiry_result_get_rssi_available(packet)
                           ? gap_event_inquiry_result_get_rssi(packet)
                           : -127;
      config_service.emit_scan_result(name, candidate_controller_address, rssi);
      break;
    }

    case GAP_EVENT_INQUIRY_COMPLETE:
      pairing_scan_active = false;
      config_service.emit_pairing_state("scan_complete");
      break;
#endif

    case BTSTACK_EVENT_STATE:
      if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        if (controller_address_valid.load()) schedule_connect(100);
      }
      break;

    case HCI_EVENT_PIN_CODE_REQUEST:
      hci_event_pin_code_request_get_bd_addr(packet, event_address);
      gap_pin_code_negative(event_address);
      break;

    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
      hci_event_user_confirmation_request_get_bd_addr(packet, event_address);
#if OPENMPG_ENABLE_CONFIG_CDC
      // A missing/corrupt link key must never silently turn a normal reconnect
      // into a new bond. Only the controller selected during an active, local
      // setup session may enter SSP; every other request is rejected.
      if (config_service.active() && pairing_candidate_selected.load() &&
          bd_addr_cmp(event_address, controller_address) == 0) {
        gap_ssp_confirmation_response(event_address);
      } else {
        gap_ssp_confirmation_negative(event_address);
      }
#else
      gap_ssp_confirmation_response(event_address);
#endif
      break;

    case HCI_EVENT_HID_META:
      switch (hci_event_hid_meta_get_subevent_code(packet)) {
        case HID_SUBEVENT_INCOMING_CONNECTION: {
          bd_addr_t incoming_address{};
          hid_subevent_incoming_connection_get_address(packet,
                                                        incoming_address);
          const std::uint16_t incoming_cid =
              hid_subevent_incoming_connection_get_hid_cid(packet);
          if (hid_subevent_incoming_connection_get_status(packet) !=
                  ERROR_CODE_SUCCESS ||
              bd_addr_cmp(incoming_address, controller_address) != 0) {
            hid_host_decline_connection(incoming_cid);
            break;
          }
          btstack_run_loop_remove_timer(&reconnect_timer);
          hid_cid = incoming_cid;
          last_hid_cid = 0;
          if (hid_host_accept_connection(incoming_cid,
                                         HID_PROTOCOL_MODE_REPORT) !=
              ERROR_CODE_SUCCESS) {
            hid_cid = 0;
            fail_closed();
            schedule_connect(kReconnectMs);
          }
          break;
        }

        case HID_SUBEVENT_CONNECTION_OPENED: {
          const std::uint8_t status =
              hid_subevent_connection_opened_get_status(packet);
          if (status == ERROR_CODE_SUCCESS) {
            btstack_run_loop_remove_timer(&reconnect_timer);
            hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
            last_hid_cid = 0;
            fail_closed();
            controller_connected = true;
          } else {
            const std::uint16_t failed_cid =
                hid_subevent_connection_opened_get_hid_cid(packet);
            last_hid_cid = failed_cid;
            hid_host_disconnect(failed_cid);
            hid_cid = 0;
            fail_closed();
            schedule_connect(5000);
          }
          break;
        }

        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE: {
          const std::uint16_t descriptor_length =
              hid_descriptor_storage_get_descriptor_len(hid_cid);
          const std::uint8_t* descriptor =
              hid_descriptor_storage_get_descriptor_data(hid_cid);
          const openmpg::ControllerProbe probe{
              openmpg::ControllerTransport::bluetooth_classic_hid,
              active_controller_name[0] != '\0' ? active_controller_name
                                                  : nullptr,
              active_controller_device_class,
              descriptor,
              descriptor_length};
          const auto selection =
              openmpg::builtin_controller_profiles().select(probe);
          bool descriptor_supported =
              hid_subevent_descriptor_available_get_status(packet) ==
                  ERROR_CODE_SUCCESS &&
              selection.status == openmpg::ProfileSelectionStatus::matched;
#if OPENMPG_ENABLE_CONFIG_CDC
          if (descriptor_supported && OPENMPG_LITE2_DESCRIPTOR_SHA256[0] != '\0') {
            descriptor_supported = openmpg_descriptor_matches_sha256(
                descriptor, descriptor_length,
                OPENMPG_LITE2_DESCRIPTOR_SHA256);
          }
#endif
          active_profile = descriptor_supported ? selection.profile : nullptr;
          descriptor_ready = descriptor_supported;
          if (descriptor_supported) {
#if OPENMPG_ENABLE_CONFIG_CDC
            config_service.set_identity(receiver_serial,
                                        OPENMPG_FIRMWARE_VERSION,
                                        active_profile->display_name,
                                        active_controller_address);
#endif
          }
          if (!descriptor_supported) {
            fail_closed();
#if OPENMPG_ENABLE_CONFIG_CDC
            if (pairing_candidate_selected.load()) {
              config_service.emit_pairing_state("unsupported_hid_descriptor");
              // Roll the entire pairing transaction back. Merely clearing the
              // selected flag would leave this uncommitted address as the
              // reconnect target after the setup session closes.
              cancel_pairing();
            } else if (hid_cid != 0) {
              hid_host_disconnect(hid_cid);
            }
#else
            if (hid_cid != 0) hid_host_disconnect(hid_cid);
#endif
          }
          break;
        }

        case HID_SUBEVENT_REPORT:
          consume_controller_report(
              hid_subevent_report_get_report(packet),
              hid_subevent_report_get_report_len(packet),
              to_ms_since_boot(get_absolute_time()));
          break;

        case HID_SUBEVENT_CONNECTION_CLOSED:
          hid_cid = 0;
          last_hid_cid = 0;
          fail_closed();
#if OPENMPG_ENABLE_CONFIG_CDC
          if (pairing_scan_requested.load() || pairing_scan_active.load()) {
            break;
          }
#endif
          if (controller_address_valid.load()) schedule_connect(kReconnectMs);
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }
}

void service_stale_input(std::uint32_t now_ms) {
  const std::uint32_t last_ms = last_valid_report_ms.load();
  // The Lite 2 may be silent while idle. Silence is only hazardous while the
  // bridge is actively holding a keyboard jog report; otherwise a later input
  // report is itself fresh evidence of the controller state.
  const bool jog_key_active = report_has_key(read_desired_report());
  if (jog_key_active && last_ms != 0 && now_ms - last_ms > kStaleMs) {
    rearm_required = true;
    last_valid_report_ms = 0;
    publish_release();
  }
}

void service_usb_keyboard() {
  static bool was_mounted = false;
  const bool mounted = openmpg_usb_keyboard_mounted();
  if (mounted != was_mounted) {
    sent_report = {};
    rearm_required = true;
    publish_release();
    was_mounted = mounted;
  }
  if (!mounted || !openmpg_usb_keyboard_ready()) return;
  const auto desired = read_desired_report();
  if (desired == sent_report) return;

  // Put an explicit all-keys-up frame between two different non-empty reports.
  if (report_has_key(sent_report) && report_has_key(desired)) {
    const openmpg::KeyboardReport release{};
    if (openmpg_usb_keyboard_send(release.data())) {
      sent_report = {};
    } else {
      // Never retry a motion transition after an unexplained HID enqueue
      // failure. Keep retrying only the all-keys-up report and require a fresh
      // neutral controller state before motion can resume.
      rearm_required = true;
      publish_release();
    }
    return;
  }

  if (openmpg_usb_keyboard_send(desired.data())) {
    sent_report = desired;
  } else {
    rearm_required = true;
    publish_release();
  }
}

void service_led(std::uint32_t now_ms) {
  bool on;
  if (!controller_connected.load() || !descriptor_ready.load()) {
    on = now_ms % 1000 < 100;
  } else if (rearm_required.load()) {
    on = now_ms % 250 < 125;
  } else {
    on = true;
  }
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
}

#if OPENMPG_ENABLE_CONFIG_CDC
void start_pairing_scan() {
  // Restarting a scan is also a rollback boundary. Without this, starting a
  // second scan after selecting (but not committing) a controller could leave
  // that abandoned controller's link key in flash indefinitely.
  if (pairing_candidate_selected.load()) cancel_pairing();
  publish_release();
  rearm_required = true;
  btstack_run_loop_remove_timer(&reconnect_timer);
  btstack_run_loop_remove_timer(&pairing_scan_timer);
  gap_inquiry_stop();
  pairing_candidate_selected = false;
  pairing_chord_confirmed = false;
  pairing_chord_started_ms = 0;
  candidate_controller_address[0] = '\0';
  candidate_controller_name[0] = '\0';
  candidate_controller_device_class = 0;
  pairing_scan_active = false;
  pairing_scan_requested = true;
  pairing_scan_requested_ms = to_ms_since_boot(get_absolute_time());
  if (hid_cid != 0) {
    hid_host_disconnect(hid_cid);
    hid_cid = 0;
  }
  btstack_run_loop_set_timer_handler(&pairing_scan_timer, begin_pairing_scan);
  btstack_run_loop_set_timer(&pairing_scan_timer, 750);
  btstack_run_loop_add_timer(&pairing_scan_timer);
}

bool select_pairing_candidate(const char* address) {
  if (address == nullptr || candidate_controller_address[0] == '\0' ||
      std::strcmp(address, candidate_controller_address) != 0) {
    return false;
  }
  gap_inquiry_stop();
  pairing_scan_requested = false;
  btstack_run_loop_remove_timer(&pairing_scan_timer);
  pairing_scan_active = false;
  if (!sscanf_bd_addr(address, controller_address)) return false;
  controller_address_valid = true;
  std::snprintf(active_controller_address, sizeof(active_controller_address),
                "%s", address);
  std::snprintf(active_controller_name, sizeof(active_controller_name), "%s",
                candidate_controller_name);
  active_controller_device_class = candidate_controller_device_class;
  pairing_candidate_selected = true;
  pairing_chord_confirmed = false;
  pairing_chord_started_ms = 0;
  const char* stored = persistent_config.controller_address();
  if (stored == nullptr || std::strcmp(stored, address) != 0) {
    // Do not let an abandoned earlier setup attempt supply a stale key for a
    // new candidate. A replacement controller must establish a fresh bond.
    gap_drop_link_key_for_bd_addr(controller_address);
  }
  if (hid_cid != 0) hid_host_disconnect(hid_cid);
  hid_cid = 0;
  schedule_connect(500);
  config_service.emit_pairing_state("awaiting_physical_confirmation");
  return true;
}

bool confirm_pairing_candidate() {
  if (!pairing_candidate_selected.load() ||
      !pairing_chord_confirmed.load()) {
    return false;
  }
  link_key_t link_key{};
  link_key_type_t link_key_type{};
  if (!gap_get_link_key_for_bd_addr(controller_address, link_key,
                                    &link_key_type)) {
    return false;
  }

  char previous_address[18]{};
  const char* stored = persistent_config.controller_address();
  if (stored != nullptr) {
    std::snprintf(previous_address, sizeof(previous_address), "%s", stored);
  }
  if (!persistent_config.store_controller_address(
          production_mapping, active_controller_address)) {
    return false;
  }
  if (previous_address[0] != '\0' &&
      std::strcmp(previous_address, active_controller_address) != 0) {
    bd_addr_t old_address{};
    if (sscanf_bd_addr(previous_address, old_address)) {
      gap_drop_link_key_for_bd_addr(old_address);
    }
  }
  pairing_candidate_selected = false;
  pairing_chord_confirmed = false;
  pairing_chord_started_ms = 0;
  config_service.set_identity(receiver_serial, OPENMPG_FIRMWARE_VERSION,
                              active_profile != nullptr
                                  ? active_profile->display_name
                                  : "Compatible controller",
                              persistent_config.controller_address());
  rearm_required = true;
  config_service.emit_pairing_state("saved");
  return true;
}

void cancel_pairing() {
  const bool had_uncommitted_candidate = pairing_candidate_selected.load();
  char rejected_address[18]{};
  if (had_uncommitted_candidate) {
    std::snprintf(rejected_address, sizeof(rejected_address), "%s",
                  active_controller_address);
  }
  gap_inquiry_stop();
  pairing_scan_requested = false;
  btstack_run_loop_remove_timer(&pairing_scan_timer);
  pairing_scan_active = false;
  pairing_candidate_selected = false;
  pairing_chord_confirmed = false;
  pairing_chord_started_ms = 0;
  const char* stored = persistent_config.controller_address();
  if (had_uncommitted_candidate && rejected_address[0] != '\0' &&
      (stored == nullptr || std::strcmp(stored, rejected_address) != 0)) {
    bd_addr_t rejected{};
    if (sscanf_bd_addr(rejected_address, rejected)) {
      gap_drop_link_key_for_bd_addr(rejected);
    }
  }
  const char* fallback = stored != nullptr ? stored : OPENMPG_CONTROLLER_ADDRESS;
  std::snprintf(active_controller_address, sizeof(active_controller_address),
                "%s", fallback);
  active_controller_name[0] = '\0';
  active_controller_device_class = 0;
  controller_address_valid =
      sscanf_bd_addr(active_controller_address, controller_address);
  if (hid_cid != 0) hid_host_disconnect(hid_cid);
  hid_cid = 0;
  if (controller_address_valid.load()) schedule_connect(500);
}
#endif

}  // namespace

int main() {
  // A wedged Bluetooth/USB stack must reset rather than preserve a stale host
  // key indefinitely. Four seconds leaves ample margin for flash commits.
  watchdog_enable(4000, true);
  critical_section_init(&report_lock);
#if OPENMPG_ENABLE_CONFIG_CDC
  controller_address_valid =
      sscanf_bd_addr(OPENMPG_CONTROLLER_ADDRESS, controller_address);
#else
  controller_address_valid =
      sscanf_bd_addr(OPENMPG_CONTROLLER_ADDRESS, controller_address);
  if (!controller_address_valid.load()) {
    while (true) tight_loop_contents();
  }
#endif
  if (cyw43_arch_init() != 0) {
    while (true) tight_loop_contents();
  }

#if OPENMPG_ENABLE_CONFIG_CDC
  pico_get_unique_board_id_string(receiver_serial, sizeof(receiver_serial));
  (void)persistent_config.load(production_mapping);
  const char* stored_address = persistent_config.controller_address();
  const char* boot_address =
      stored_address != nullptr ? stored_address : OPENMPG_CONTROLLER_ADDRESS;
  std::snprintf(active_controller_address, sizeof(active_controller_address),
                "%s", boot_address);
  controller_address_valid =
      sscanf_bd_addr(active_controller_address, controller_address);
  config_service.set_identity(receiver_serial, OPENMPG_FIRMWARE_VERSION,
                              stored_address != nullptr
                                  ? "8BitDo Lite 2 · D mode"
                                  : nullptr,
                              stored_address);
#endif

  openmpg_usb_keyboard_init();

  l2cap_init();
  // HID L2CAP services require an encrypted link. Lite 2 Just Works cannot
  // provide MITM protection, so this is explicitly Level 2 rather than
  // implying the stronger Level 3 guarantee.
  gap_set_security_level(LEVEL_2);
  hid_host_init(descriptor_storage, sizeof(descriptor_storage));
  hid_host_register_packet_handler(packet_handler);
  gap_set_default_link_policy_settings(
      LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
  gap_ssp_set_auto_accept(0);
  gap_set_required_encryption_key_size(16);
  hci_set_master_slave_policy(HCI_ROLE_MASTER);
  gap_discoverable_control(0);
  gap_connectable_control(1);
  hci_callback.callback = &packet_handler;
  hci_add_event_handler(&hci_callback);
  hci_power_control(HCI_POWER_ON);

  while (true) {
    watchdog_update();
#if defined(OPENMPG_POLL_BTSTACK)
    // Keep Bluetooth callbacks, TinyUSB, configuration parsing, flash state,
    // and mapping updates on this one deterministic main-loop context.
    cyw43_arch_poll();
#endif
    openmpg_usb_keyboard_task();
#if OPENMPG_ENABLE_CONFIG_CDC
    static bool config_was_active = false;
    config_service.service();
    if (config_service.active()) {
      publish_release();
      rearm_required = true;
    } else if (config_was_active &&
               (pairing_scan_requested.load() || pairing_scan_active.load() ||
                pairing_candidate_selected.load())) {
      // Closing/crashing the app is an implicit pairing cancellation. This
      // restores the last committed address and deletes any uncommitted bond.
      cancel_pairing();
    }
    config_was_active = config_service.active();
#endif
    const std::uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    service_stale_input(now_ms);
    service_usb_keyboard();
    service_led(now_ms);
    sleep_us(500);
  }
}
