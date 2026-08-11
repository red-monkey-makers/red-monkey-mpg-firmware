#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void red_monkey_mpg_usb_keyboard_init(void);
void red_monkey_mpg_usb_keyboard_task(void);
bool red_monkey_mpg_usb_keyboard_mounted(void);
bool red_monkey_mpg_usb_keyboard_ready(void);
bool red_monkey_mpg_usb_keyboard_send(const uint8_t report[8]);
bool red_monkey_mpg_usb_config_connected(void);
uint32_t red_monkey_mpg_usb_config_available(void);
uint32_t red_monkey_mpg_usb_config_read(void* buffer, uint32_t length);
uint32_t red_monkey_mpg_usb_config_write(const void* buffer, uint32_t length);
void red_monkey_mpg_usb_config_flush(void);

#ifdef __cplusplus
}
#endif
