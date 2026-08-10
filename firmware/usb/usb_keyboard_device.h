#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void openmpg_usb_keyboard_init(void);
void openmpg_usb_keyboard_task(void);
bool openmpg_usb_keyboard_mounted(void);
bool openmpg_usb_keyboard_ready(void);
bool openmpg_usb_keyboard_send(const uint8_t report[8]);
bool openmpg_usb_config_connected(void);
uint32_t openmpg_usb_config_available(void);
uint32_t openmpg_usb_config_read(void* buffer, uint32_t length);
uint32_t openmpg_usb_config_write(const void* buffer, uint32_t length);
void openmpg_usb_config_flush(void);

#ifdef __cplusplus
}
#endif
