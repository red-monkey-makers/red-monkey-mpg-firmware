// Shared iPhone BLE receiver implementation. The diagnostic target contains no
// USB keyboard output; the production target enables the fail-closed keyboard
// bridge with RED_MONKEY_MPG_MOBILE_USB_KEYBOARD.

#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "btstack.h"
#include "mobile_jogger.h"
#include "red_monkey_mpg/control_mapper.hpp"
#include "red_monkey_mpg/masso_keyboard.hpp"
#include "red_monkey_mpg/mobile_protocol.hpp"
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
#include "hardware/watchdog.h"
#include "usb_keyboard_device.h"
#endif
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

namespace {

constexpr std::uint32_t kStaleMs = 150;
constexpr std::uint8_t kStatusBluetoothReady = 1u << 0u;
constexpr std::uint8_t kStatusUsbReady = 1u << 1u;
constexpr std::uint8_t kStatusOutputLocked = 1u << 2u;
constexpr std::uint8_t kStatusNeutralRequired = 1u << 3u;
constexpr std::uint8_t kStatusDiagnostic = 1u << 4u;

constexpr std::uint8_t kErrorNone = 0;
constexpr std::uint8_t kErrorMalformed = 1;
constexpr std::uint8_t kErrorStale = 2;
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
constexpr std::uint8_t kErrorUsbUnavailable = 5;
#endif

red_monkey_mpg::MobileInputSession session{};
hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;
btstack_packet_callback_registration_t hci_callback{};
btstack_packet_callback_registration_t sm_callback{};
std::uint16_t last_sequence{};
std::uint32_t last_valid_ms{};
std::uint8_t active_direction{};
std::uint8_t resolution{1};
std::uint8_t error_code{kErrorNone};
std::uint8_t controller_profile{
    static_cast<std::uint8_t>(red_monkey_mpg::MobileControllerProfile::masso)};
bool notify_enabled{};
bool neutral_required{true};

#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
red_monkey_mpg::ControlMapper mapper{};
red_monkey_mpg::KeyboardReport desired_report{};
red_monkey_mpg::KeyboardReport sent_report{};

bool report_has_key(const red_monkey_mpg::KeyboardReport& report) {
  for (const auto byte : report) {
    if (byte != 0) return true;
  }
  return false;
}

void publish_release() { desired_report = {}; }

void reset_mapper(std::uint32_t now_ms) {
  red_monkey_mpg::GamepadState disconnected{};
  disconnected.sample_ms = now_ms;
  (void)mapper.update(disconnected, now_ms);
}

bool usb_ready() {
  return red_monkey_mpg_usb_keyboard_mounted() &&
         red_monkey_mpg_usb_keyboard_ready();
}
#else
bool usb_ready() { return false; }
#endif

std::array<std::uint8_t, 21> advertisement{
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x02,
    0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
    0xC6, 0x04, 0xE3, 0x7F, 0xA9, 0xF4, 0x91, 0x8A,
    0xE0, 0x44, 0x1F, 0x0B, 0xB6, 0x96, 0x3A, 0x7A};

std::array<std::uint8_t, 16> scan_response{
    0x0F, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'R',  'e',  'd',  ' ',  'M', 'o', 'n', 'k',
    'e',  'y',  ' ',  'M',  'P', 'G'};

static_assert(advertisement.size() <= 31);
static_assert(scan_response.size() <= 31);

std::array<std::uint8_t, red_monkey_mpg::kMobileFrameLength> status_frame() {
  const bool output_locked =
      neutral_required ||
      (RED_MONKEY_MPG_MOBILE_USB_KEYBOARD && !usb_ready());
  std::array<std::uint8_t, red_monkey_mpg::kMobileFrameLength> bytes{
      0x53,
      red_monkey_mpg::kMobileProtocolVersion,
      static_cast<std::uint8_t>(last_sequence),
      static_cast<std::uint8_t>(last_sequence >> 8u),
      static_cast<std::uint8_t>(
          kStatusBluetoothReady |
          (usb_ready() ? kStatusUsbReady : 0) |
          (output_locked ? kStatusOutputLocked : 0) |
          (neutral_required ? kStatusNeutralRequired : 0) |
          (RED_MONKEY_MPG_MOBILE_USB_KEYBOARD ? 0 : kStatusDiagnostic)),
      active_direction,
      resolution,
      error_code,
      controller_profile,
      0,
  };
  bytes.back() = red_monkey_mpg::mobile_crc8(bytes.data(), bytes.size() - 1);
  return bytes;
}

void request_status_notification() {
  if (notify_enabled && connection_handle != HCI_CON_HANDLE_INVALID) {
    att_server_request_can_send_now_event(connection_handle);
  }
}

std::uint16_t read_callback(hci_con_handle_t, std::uint16_t attribute_handle,
                            std::uint16_t offset, std::uint8_t* buffer,
                            std::uint16_t buffer_size) {
  if (attribute_handle !=
      ATT_CHARACTERISTIC_5D5A91CE_A6F6_4AAF_8E60_C728F36E663C_01_VALUE_HANDLE) {
    return 0;
  }
  const auto status = status_frame();
  return att_read_callback_handle_blob(status.data(), status.size(), offset,
                                       buffer, buffer_size);
}

int write_callback(hci_con_handle_t, std::uint16_t attribute_handle,
                   std::uint16_t transaction_mode, std::uint16_t offset,
                   std::uint8_t* buffer, std::uint16_t buffer_size) {
  if (attribute_handle ==
      ATT_CHARACTERISTIC_5D5A91CE_A6F6_4AAF_8E60_C728F36E663C_01_CLIENT_CONFIGURATION_HANDLE) {
    if (transaction_mode != ATT_TRANSACTION_MODE_NONE || offset != 0 ||
        buffer_size != 2) {
      return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
    }
    notify_enabled = little_endian_read_16(buffer, 0) ==
                     GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    request_status_notification();
    return 0;
  }
  if (attribute_handle !=
      ATT_CHARACTERISTIC_7567D371_0CA2_4AA0_8861_E192D90B6A2B_01_VALUE_HANDLE) {
    return 0;
  }
  if (transaction_mode != ATT_TRANSACTION_MODE_NONE || offset != 0) {
    return ATT_ERROR_REQUEST_NOT_SUPPORTED;
  }

  const std::uint32_t now_ms = to_ms_since_boot(get_absolute_time());
  const auto parsed = session.consume(buffer, buffer_size, now_ms);
  if (!parsed.accepted) {
    active_direction = 0;
    neutral_required = true;
    error_code = kErrorMalformed;
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
    reset_mapper(now_ms);
    publish_release();
#endif
    std::printf("REJECT error=%u length=%u\n",
                static_cast<unsigned>(parsed.error), buffer_size);
    request_status_notification();
    return ATT_ERROR_VALUE_NOT_ALLOWED;
  }

  last_sequence = parsed.sequence;
  controller_profile = static_cast<std::uint8_t>(parsed.controller_profile);
  last_valid_ms = now_ms;
  error_code = kErrorNone;
  active_direction = buffer[4];
  if (!parsed.input.deadman || active_direction == 0) {
    active_direction = 0;
    neutral_required = false;
  } else if (neutral_required) {
    active_direction = 0;
  }
  if (parsed.input.select_resolution) {
    resolution = static_cast<std::uint8_t>(parsed.input.selected_resolution);
  }
  if (parsed.input.cancel) {
    active_direction = 0;
    neutral_required = true;
  }

#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
  if (neutral_required && parsed.input.deadman && buffer[4] != 0) {
    reset_mapper(now_ms);
    publish_release();
  } else {
    const auto output = mapper.update(parsed.input, now_ms);
    desired_report = red_monkey_mpg::cnc_controller_keyboard_report(
        red_monkey_mpg::CncControllerProfileId::masso_g3_touch_5_13, output);
    active_direction = output.has_jog ? buffer[4] : 0;
  }
#endif

  std::printf("MOBILE seq=%u profile=%u deadman=%u direction=%u continuous=%u "
              "precision=%u event=%u\n",
              last_sequence, controller_profile, parsed.input.deadman, buffer[4],
              parsed.input.rapid, parsed.input.precision, buffer[7]);
  request_status_notification();
  return 0;
}

void fail_closed(std::uint8_t error) {
  active_direction = 0;
  last_valid_ms = 0;
  neutral_required = true;
  error_code = error;
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
  reset_mapper(to_ms_since_boot(get_absolute_time()));
  publish_release();
#endif
  request_status_notification();
}

void packet_handler(std::uint8_t packet_type, std::uint16_t,
                    std::uint8_t* packet, std::uint16_t) {
  if (packet_type != HCI_EVENT_PACKET) return;
  switch (hci_event_packet_get_type(packet)) {
    case HCI_EVENT_META_GAP:
      if (hci_event_gap_meta_get_subevent_code(packet) ==
          GAP_SUBEVENT_LE_CONNECTION_COMPLETE) {
        connection_handle =
            gap_subevent_le_connection_complete_get_connection_handle(packet);
        session.reset();
        notify_enabled = false;
        fail_closed(kErrorNone);
        std::printf("iPhone BLE connected.\n");
      }
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      connection_handle = HCI_CON_HANDLE_INVALID;
      notify_enabled = false;
      session.reset();
      fail_closed(kErrorNone);
      std::printf("iPhone BLE disconnected; released.\n");
      break;
    case ATT_EVENT_CAN_SEND_NOW:
      if (connection_handle != HCI_CON_HANDLE_INVALID && notify_enabled) {
        const auto status = status_frame();
        att_server_notify(
            connection_handle,
            ATT_CHARACTERISTIC_5D5A91CE_A6F6_4AAF_8E60_C728F36E663C_01_VALUE_HANDLE,
            status.data(), status.size());
      }
      break;
    default:
      break;
  }
}

void security_handler(std::uint8_t packet_type, std::uint16_t,
                      std::uint8_t* packet, std::uint16_t) {
  if (packet_type != HCI_EVENT_PACKET) return;
  if (hci_event_packet_get_type(packet) == SM_EVENT_JUST_WORKS_REQUEST) {
    sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
  }
}

void service_timeout(std::uint32_t now_ms) {
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
  const bool output_active = report_has_key(desired_report);
#else
  const bool output_active = active_direction != 0;
#endif
  if (output_active && last_valid_ms != 0 &&
      now_ms - last_valid_ms > kStaleMs) {
    fail_closed(kErrorStale);
    std::printf("MOBILE stale; released.\n");
  }
}

#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
void service_usb_keyboard() {
  static bool was_mounted{};
  const bool mounted = red_monkey_mpg_usb_keyboard_mounted();
  if (mounted != was_mounted) {
    sent_report = {};
    neutral_required = true;
    reset_mapper(to_ms_since_boot(get_absolute_time()));
    publish_release();
    was_mounted = mounted;
    request_status_notification();
  }
  if (!mounted || !red_monkey_mpg_usb_keyboard_ready()) {
    if (error_code == kErrorNone) error_code = kErrorUsbUnavailable;
    return;
  }
  if (error_code == kErrorUsbUnavailable) error_code = kErrorNone;
  if (desired_report == sent_report) return;

  // Always place an explicit all-keys-up report between two different
  // non-empty reports. This also prevents an axis/direction transition from
  // being interpreted as simultaneous motion by the CNC controller.
  if (report_has_key(sent_report) && report_has_key(desired_report)) {
    const red_monkey_mpg::KeyboardReport release{};
    if (red_monkey_mpg_usb_keyboard_send(release.data())) {
      sent_report = {};
    } else {
      neutral_required = true;
      reset_mapper(to_ms_since_boot(get_absolute_time()));
      publish_release();
    }
    return;
  }

  if (red_monkey_mpg_usb_keyboard_send(desired_report.data())) {
    sent_report = desired_report;
  } else {
    neutral_required = true;
    reset_mapper(to_ms_since_boot(get_absolute_time()));
    publish_release();
  }
}
#endif

}  // namespace

int main() {
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
  watchdog_enable(4000, true);
#endif
  stdio_init_all();
  sleep_ms(1200);
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
  std::printf("\nRed Monkey MPG mobile production receiver\n");
  red_monkey_mpg_usb_keyboard_init();
#else
  std::printf("\nRed Monkey MPG mobile BLE bench\n");
  std::printf("USB keyboard output is not present in this image.\n");
#endif

  if (cyw43_arch_init() != 0) {
    std::printf("ERROR: CYW43 initialization failed.\n");
    while (true) tight_loop_contents();
  }

  l2cap_init();
  sm_init();
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
  sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION |
                                     SM_AUTHREQ_BONDING);
  att_server_init(profile_data, read_callback, write_callback);

  hci_callback.callback = packet_handler;
  hci_add_event_handler(&hci_callback);
  att_server_register_packet_handler(packet_handler);
  sm_callback.callback = security_handler;
  sm_add_event_handler(&sm_callback);

  bd_addr_t null_address{};
  gap_advertisements_set_params(0x0030, 0x0060, 0, 0, null_address, 0x07,
                                0x00);
  gap_advertisements_set_data(advertisement.size(), advertisement.data());
  gap_scan_response_set_data(scan_response.size(), scan_response.data());
  gap_advertisements_enable(1);
  hci_power_control(HCI_POWER_ON);

  while (true) {
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
    watchdog_update();
#endif
    cyw43_arch_poll();
#if RED_MONKEY_MPG_MOBILE_USB_KEYBOARD
    red_monkey_mpg_usb_keyboard_task();
    service_usb_keyboard();
#endif
    service_timeout(to_ms_since_boot(get_absolute_time()));
    sleep_ms(2);
  }
}
