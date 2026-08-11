#include "usb_keyboard_device.h"

#include "tusb.h"

// TinyUSB keeps a device mounted across USB suspend. Treat suspend as unusable
// so the application requires a fresh neutral report before emitting keys when
// the host resumes.
static volatile bool usb_suspended = false;

void red_monkey_mpg_usb_keyboard_init(void) {
  tusb_rhport_init_t device_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(0, &device_init);
}

void red_monkey_mpg_usb_keyboard_task(void) { tud_task(); }

bool red_monkey_mpg_usb_keyboard_mounted(void) {
  return tud_mounted() && !usb_suspended;
}

bool red_monkey_mpg_usb_keyboard_ready(void) {
  return red_monkey_mpg_usb_keyboard_mounted() && tud_hid_ready();
}

bool red_monkey_mpg_usb_keyboard_send(const uint8_t report[8]) {
  if (!red_monkey_mpg_usb_keyboard_ready()) return false;
  return tud_hid_keyboard_report(0, report[0], report + 2);
}

bool red_monkey_mpg_usb_config_connected(void) {
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
  return !usb_suspended && tud_cdc_connected();
#else
  return false;
#endif
}

uint32_t red_monkey_mpg_usb_config_available(void) {
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
  return tud_cdc_available();
#else
  return 0;
#endif
}

uint32_t red_monkey_mpg_usb_config_read(void* buffer, uint32_t length) {
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
  return tud_cdc_read(buffer, length);
#else
  (void)buffer;
  (void)length;
  return 0;
#endif
}

uint32_t red_monkey_mpg_usb_config_write(const void* buffer, uint32_t length) {
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
  return tud_cdc_write(buffer, length);
#else
  (void)buffer;
  (void)length;
  return 0;
#endif
}

void red_monkey_mpg_usb_config_flush(void) {
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
  tud_cdc_write_flush();
#endif
}

void tud_mount_cb(void) { usb_suspended = false; }

void tud_umount_cb(void) { usb_suspended = false; }

void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  usb_suspended = true;
}

void tud_resume_cb(void) { usb_suspended = false; }

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}
