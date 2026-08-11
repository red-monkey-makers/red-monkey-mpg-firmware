#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be supplied by the Pico SDK
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_PICO
#endif

#define CFG_TUD_ENABLED 1
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_HID 1
#ifndef RED_MONKEY_MPG_USB_DYNAMIC_SERIAL
#define RED_MONKEY_MPG_USB_DYNAMIC_SERIAL 0
#endif
#ifndef RED_MONKEY_MPG_ENABLE_CONFIG_CDC
#define RED_MONKEY_MPG_ENABLE_CONFIG_CDC 0
#endif
#define CFG_TUD_CDC RED_MONKEY_MPG_ENABLE_CONFIG_CDC
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_HID_EP_BUFSIZE 8
#if RED_MONKEY_MPG_ENABLE_CONFIG_CDC
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256
#define CFG_TUD_CDC_EP_BUFSIZE 64
#endif

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#ifdef __cplusplus
}
#endif
