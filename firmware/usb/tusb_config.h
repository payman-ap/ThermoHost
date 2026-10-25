#pragma once

/* The Pico SDK build supplies the microcontroller selection. */
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be supplied by the build
#endif

/* Use USB controller 0 as a full-speed device. */
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED OPT_MODE_FULL_SPEED

/* Endpoint zero handles USB enumeration and control requests. */
#define CFG_TUD_ENDPOINT0_SIZE 64

/* Enable one vendor interface; disable the standard device classes. */
#define CFG_TUD_VENDOR 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0

/* Reserve fixed-size buffers for vendor bulk transfers. */
#define CFG_TUD_VENDOR_EPSIZE 64
#define CFG_TUD_VENDOR_RX_BUFSIZE 64
#define CFG_TUD_VENDOR_TX_BUFSIZE 64
