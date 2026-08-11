// Red Monkey MPG HID report diagnostic. No USB keyboard output is present.

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "red_monkey_mpg/descriptor_identity.h"
#include "red_monkey_mpg/input_diagnostic.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#ifndef RED_MONKEY_MPG_CONTROLLER_ADDRESS
#error "RED_MONKEY_MPG_CONTROLLER_ADDRESS must be supplied by CMake"
#endif

#define HID_DESCRIPTOR_BYTES 512

static bd_addr_t controller_address;
static uint8_t descriptor_storage[HID_DESCRIPTOR_BYTES];
static uint16_t hid_cid;
static uint16_t last_hid_cid;
static btstack_packet_callback_registration_t hci_callback;
static btstack_timer_source_t reconnect_timer;
static bool descriptor_ready;
static uint8_t last_report[64];
static uint16_t last_report_len;

static void schedule_connect(uint32_t delay_ms);

static void connect_controller(btstack_timer_source_t *timer) {
  (void)timer;
  if (hid_cid != 0) return;
  printf("Connecting to %s...\n", RED_MONKEY_MPG_CONTROLLER_ADDRESS);
  const uint8_t status = hid_host_connect(
      controller_address, HID_PROTOCOL_MODE_REPORT, &hid_cid);
  if (status != ERROR_CODE_SUCCESS) {
    if (status == ERROR_CODE_COMMAND_DISALLOWED && last_hid_cid != 0) {
      // A prior attempt can leave its control channel closing asynchronously.
      hid_host_disconnect(last_hid_cid);
    }
    hid_cid = 0;
    printf("Connect request failed: 0x%02X; retrying.\n", status);
    schedule_connect(5000);
  } else {
    // The host owns a fresh connection object now. Never let cleanup from an
    // older failed attempt act on a CID that BTstack may later reuse.
    last_hid_cid = 0;
  }
}

static void schedule_connect(uint32_t delay_ms) {
  btstack_run_loop_remove_timer(&reconnect_timer);
  btstack_run_loop_set_timer_handler(&reconnect_timer, connect_controller);
  btstack_run_loop_set_timer(&reconnect_timer, delay_ms);
  btstack_run_loop_add_timer(&reconnect_timer);
}

static void print_hex(const uint8_t *data, uint16_t length) {
  for (uint16_t i = 0; i < length; ++i) {
    printf("%02X%s", data[i], i + 1 == length ? "" : " ");
  }
  printf("\n");
}

static void print_decoded_report(const uint8_t *report, uint16_t length) {
  // The Classic HID interrupt transaction begins with 0xA1. The HID parser
  // expects the following report ID and payload.
  if (!descriptor_ready || length < 2 || report[0] != 0xA1) return;

  btstack_hid_parser_t parser;
  btstack_hid_parser_init(
      &parser, hid_descriptor_storage_get_descriptor_data(hid_cid),
      hid_descriptor_storage_get_descriptor_len(hid_cid),
      HID_REPORT_TYPE_INPUT, report + 1, length - 1);
  uint32_t buttons = 0;
  int32_t hat = -1;
  int32_t x = -1;
  int32_t y = -1;
  int32_t z = -1;
  int32_t rz = -1;
  int32_t c4 = -1;
  int32_t c5 = -1;
  while (btstack_hid_parser_has_more(&parser)) {
    uint16_t usage_page;
    uint16_t usage;
    int32_t value;
    btstack_hid_parser_get_field(&parser, &usage_page, &usage, &value);
    if (usage_page == 0x0009 && usage >= 1 && usage <= 20 && value != 0) {
      buttons |= 1u << (usage - 1);
    } else if (usage_page == 0x0001) {
      if (usage == 0x0039) hat = value;
      if (usage == 0x0030) x = value;
      if (usage == 0x0031) y = value;
      if (usage == 0x0032) z = value;
      if (usage == 0x0035) rz = value;
    } else if (usage_page == 0x0002) {
      if (usage == 0x00C4) c4 = value;
      if (usage == 0x00C5) c5 = value;
    }
  }
  printf("STATE buttons=%05lX hat=%" PRId32 " X=%" PRId32
         " Y=%" PRId32 " Z=%" PRId32 " Rz=%" PRId32
         " C4=%" PRId32 " C5=%" PRId32 "\n",
         (unsigned long)buttons, hat, x, y, z, rz, c4, c5);
}

static void packet_handler(uint8_t packet_type, uint16_t channel,
                           uint8_t *packet, uint16_t size) {
  (void)channel;
  (void)size;
  if (packet_type != HCI_EVENT_PACKET) return;

  bd_addr_t event_address;
  const uint8_t event = hci_event_packet_get_type(packet);
  switch (event) {
    case BTSTACK_EVENT_STATE:
      if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        schedule_connect(100);
      }
      break;

    case HCI_EVENT_PIN_CODE_REQUEST:
      hci_event_pin_code_request_get_bd_addr(packet, event_address);
      printf("Legacy PIN requested; refusing.\n");
      gap_pin_code_negative(event_address);
      break;

    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
      hci_event_user_confirmation_request_get_bd_addr(packet, event_address);
      printf("Pairing confirmation value: %" PRIu32 " (accepted)\n",
             little_endian_read_32(packet, 8));
      gap_ssp_confirmation_response(event_address);
      break;

    case HCI_EVENT_HID_META:
      switch (hci_event_hid_meta_get_subevent_code(packet)) {
        case HID_SUBEVENT_INCOMING_CONNECTION: {
          bd_addr_t incoming_address;
          hid_subevent_incoming_connection_get_address(packet,
                                                        incoming_address);
          const uint16_t incoming_cid =
              hid_subevent_incoming_connection_get_hid_cid(packet);
          if (hid_subevent_incoming_connection_get_status(packet) !=
                  ERROR_CODE_SUCCESS ||
              bd_addr_cmp(incoming_address, controller_address) != 0) {
            printf("Declining incoming HID connection from %s.\n",
                   bd_addr_to_str(incoming_address));
            hid_host_decline_connection(incoming_cid);
            break;
          }
          btstack_run_loop_remove_timer(&reconnect_timer);
          hid_cid = incoming_cid;
          last_hid_cid = 0;
          printf("Accepting controller auto-reconnect from %s.\n",
                 bd_addr_to_str(incoming_address));
          const uint8_t accept_status = hid_host_accept_connection(
              incoming_cid, HID_PROTOCOL_MODE_REPORT);
          if (accept_status != ERROR_CODE_SUCCESS) {
            printf("Incoming HID accept failed: 0x%02X\n", accept_status);
            hid_cid = 0;
            schedule_connect(3000);
          }
          break;
        }

        case HID_SUBEVENT_CONNECTION_OPENED: {
          const uint8_t status = hid_subevent_connection_opened_get_status(packet);
          if (status == ERROR_CODE_SUCCESS) {
            btstack_run_loop_remove_timer(&reconnect_timer);
            last_hid_cid = 0;
            descriptor_ready = false;
            last_report_len = 0;
            hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
            printf("HID connected. Waiting for descriptor...\n");
          } else {
            const uint16_t failed_cid =
                hid_subevent_connection_opened_get_hid_cid(packet);
            last_hid_cid = failed_cid;
            hid_host_disconnect(failed_cid);
            hid_cid = 0;
            printf("HID connection failed: 0x%02X\n", status);
            printf("Retrying automatically; keep the controller in pairing mode.\n");
            schedule_connect(5000);
          }
          break;
        }

        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE: {
          const uint8_t status = hid_subevent_descriptor_available_get_status(packet);
          if (status == ERROR_CODE_SUCCESS) {
            descriptor_ready = true;
            const uint8_t *descriptor =
                hid_descriptor_storage_get_descriptor_data(hid_cid);
            const uint16_t descriptor_length =
                hid_descriptor_storage_get_descriptor_len(hid_cid);
            char fingerprint[65];
            printf("HID descriptor ready (%u bytes).\n", descriptor_length);
            if (red_monkey_mpg_descriptor_sha256_hex(descriptor, descriptor_length,
                                              fingerprint)) {
              printf("DESCRIPTOR_SHA256 %s\n", fingerprint);
            } else {
              printf("DESCRIPTOR_SHA256 unavailable\n");
            }
            printf("DESCRIPTOR_BYTES ");
            print_hex(descriptor, descriptor_length);
            printf("Move one control at a time.\n");
          } else {
            printf("HID descriptor failed: 0x%02X\n", status);
          }
          break;
        }

        case HID_SUBEVENT_REPORT: {
          const uint8_t *report = hid_subevent_report_get_report(packet);
          const uint16_t length = hid_subevent_report_get_report_len(packet);
          if (length == last_report_len && length <= sizeof(last_report) &&
              memcmp(report, last_report, length) == 0) {
            break;
          }
          if (length <= sizeof(last_report)) {
            memcpy(last_report, report, length);
            last_report_len = length;
          }
          printf("REPORT %3u: ", length);
          print_hex(report, length);
          print_decoded_report(report, length);
          red_monkey_mpg_diagnose_lite2_report(report, length,
                                        to_ms_since_boot(get_absolute_time()));
          break;
        }

        case HID_SUBEVENT_CONNECTION_CLOSED:
          hid_cid = 0;
          last_hid_cid = 0;
          descriptor_ready = false;
          last_report_len = 0;
          printf("HID disconnected. Retrying automatically...\n");
          schedule_connect(3000);
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }
}

int main(void) {
  stdio_init_all();
  sleep_ms(1500);
  printf("\nRed Monkey MPG raw HID diagnostic (keyboard output disabled)\n");
  printf("Controller: %s\n", RED_MONKEY_MPG_CONTROLLER_ADDRESS);

  if (!sscanf_bd_addr(RED_MONKEY_MPG_CONTROLLER_ADDRESS, controller_address)) {
    printf("ERROR: invalid controller Bluetooth address.\n");
    while (true) tight_loop_contents();
  }
  if (cyw43_arch_init() != 0) {
    printf("ERROR: CYW43 Bluetooth initialization failed.\n");
    while (true) tight_loop_contents();
  }

  l2cap_init();
  hid_host_init(descriptor_storage, sizeof(descriptor_storage));
  hid_host_register_packet_handler(packet_handler);
  gap_set_default_link_policy_settings(
      LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
  hci_set_master_slave_policy(HCI_ROLE_MASTER);
  gap_discoverable_control(0);
  gap_connectable_control(1);

  hci_callback.callback = &packet_handler;
  hci_add_event_handler(&hci_callback);
  hci_power_control(HCI_POWER_ON);

  while (true) sleep_ms(1000);
}
