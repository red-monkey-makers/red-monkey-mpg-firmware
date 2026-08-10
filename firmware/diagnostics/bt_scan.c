// Red Monkey MPG Bluetooth discovery diagnostic.
// This program has no keyboard/HID-device output and cannot command MASSO.

#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define INQUIRY_DURATION_1280_MS 5

static btstack_packet_callback_registration_t hci_callback;

static void start_scan(void) {
  printf("\nScanning for Bluetooth Classic devices...\n");
  printf("Put the 8BitDo Lite 2 into pairing mode now.\n");
  const uint8_t status = gap_inquiry_start(INQUIRY_DURATION_1280_MS);
  if (status != ERROR_CODE_SUCCESS) {
    printf("Could not start inquiry (status 0x%02x).\n", status);
  }
}

static void packet_handler(uint8_t packet_type, uint16_t channel,
                           uint8_t *packet, uint16_t size) {
  (void)channel;
  (void)size;
  if (packet_type != HCI_EVENT_PACKET) return;

  const uint8_t event = hci_event_packet_get_type(packet);
  if (event == BTSTACK_EVENT_STATE &&
      btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
    bd_addr_t local_address;
    gap_local_bd_addr(local_address);
    printf("\nRed Monkey MPG Bluetooth scanner ready\n");
    printf("Pico address: %s\n", bd_addr_to_str(local_address));
    start_scan();
    return;
  }

  if (event == GAP_EVENT_INQUIRY_RESULT) {
    bd_addr_t address;
    gap_event_inquiry_result_get_bd_addr(packet, address);
    printf("Found %-17s  class 0x%06lx",
           bd_addr_to_str(address),
           (unsigned long)gap_event_inquiry_result_get_class_of_device(packet));
    if (gap_event_inquiry_result_get_rssi_available(packet)) {
      printf("  RSSI %d dBm",
             (int8_t)gap_event_inquiry_result_get_rssi(packet));
    }
    if (gap_event_inquiry_result_get_name_available(packet)) {
      const uint8_t *name = gap_event_inquiry_result_get_name(packet);
      uint8_t length = gap_event_inquiry_result_get_name_len(packet);
      char safe_name[64];
      if (length >= sizeof(safe_name)) length = sizeof(safe_name) - 1;
      memcpy(safe_name, name, length);
      safe_name[length] = '\0';
      printf("  name \"%s\"", safe_name);
    }
    printf("\n");
    return;
  }

  if (event == GAP_EVENT_INQUIRY_COMPLETE) {
    printf("Scan complete; restarting.\n");
    start_scan();
  }
}

int main(void) {
  stdio_init_all();
  sleep_ms(1500);
  printf("\nRed Monkey MPG diagnostic booting (keyboard output disabled)\n");

  if (cyw43_arch_init() != 0) {
    printf("ERROR: CYW43 Bluetooth initialization failed.\n");
    while (true) tight_loop_contents();
  }

  hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
  hci_callback.callback = &packet_handler;
  hci_add_event_handler(&hci_callback);
  hci_power_control(HCI_POWER_ON);

  while (true) sleep_ms(1000);
}
