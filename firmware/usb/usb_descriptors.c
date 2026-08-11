#include <stddef.h>
#include <string.h>

#include "tusb.h"
#if RED_MONKEY_MPG_USB_DYNAMIC_SERIAL
#include "pico/unique_id.h"
#endif

#ifndef RED_MONKEY_MPG_USB_PRODUCT
#define RED_MONKEY_MPG_USB_PRODUCT "Red Monkey MPG Keyboard Bench Test"
#endif
#ifndef RED_MONKEY_MPG_USB_SERIAL
#define RED_MONKEY_MPG_USB_SERIAL "BENCH-0001"
#endif
#ifndef RED_MONKEY_MPG_USB_PID
#define RED_MONKEY_MPG_USB_PID 0x4010
#endif
#ifndef RED_MONKEY_MPG_USB_VID
// Development only. A commercial build must supply an authorized USB VID.
#define RED_MONKEY_MPG_USB_VID 0xCAFE
#endif
#ifndef RED_MONKEY_MPG_USB_BCD_DEVICE
#define RED_MONKEY_MPG_USB_BCD_DEVICE 0x0001
#endif

enum {
  ITF_NUM_KEYBOARD,
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
  ITF_NUM_CDC,
  ITF_NUM_CDC_DATA,
#endif
  ITF_NUM_TOTAL
};

static tusb_desc_device_t const kDeviceDescriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
#else
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
#endif
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = RED_MONKEY_MPG_USB_VID,
    .idProduct = RED_MONKEY_MPG_USB_PID,
    .bcdDevice = RED_MONKEY_MPG_USB_BCD_DEVICE,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&kDeviceDescriptor;
}

static uint8_t const kKeyboardReportDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return kKeyboardReportDescriptor;
}

#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
#define CONFIG_TOTAL_LEN \
  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)
#else
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#endif

static uint8_t const kConfigurationDescriptor[] = {
    // The receiver never initiates remote wakeup, so do not advertise a USB
    // capability that is neither needed nor exercised by the safety tests.
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(kKeyboardReportDescriptor), 0x81,
                       CFG_TUD_HID_EP_BUFSIZE, 10),
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 0, 0x82, 8, 0x03, 0x83,
                       CFG_TUD_CDC_EP_BUFSIZE),
#endif
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return kConfigurationDescriptor;
}

static char const* const kStringDescriptors[] = {
    (char const[]){0x09, 0x04},
    "Red Monkey MPG",
    RED_MONKEY_MPG_USB_PRODUCT,
    RED_MONKEY_MPG_USB_SERIAL,
};

static uint16_t string_buffer[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t count;
  if (index == 0) {
    memcpy(&string_buffer[1], kStringDescriptors[0], 2);
    count = 1;
  } else {
    if (index >= sizeof(kStringDescriptors) / sizeof(kStringDescriptors[0])) {
      return NULL;
    }
    char const* source = kStringDescriptors[index];
#if RED_MONKEY_MPG_USB_DYNAMIC_SERIAL
    static char unique_serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    if (index == 3) {
      if (unique_serial[0] == '\0') {
        pico_get_unique_board_id_string(unique_serial, sizeof(unique_serial));
      }
      source = unique_serial;
    }
#endif
    count = strlen(source);
    if (count > 31) count = 31;
    for (size_t i = 0; i < count; ++i) string_buffer[1 + i] = source[i];
  }
  string_buffer[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
  return string_buffer;
}
