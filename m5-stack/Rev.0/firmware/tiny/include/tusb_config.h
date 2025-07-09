#pragma once


#define CFG_TUD_ENABLED (1)
// #define CFG_TUSB_DEBUG (2)

// Legacy RHPORT configuration
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT        (0)
#endif
// end legacy RHPORT

#define CFG_TUD_CDC    (2)
// #define CFG_TUD_VENDOR (1)
// #define CFG_TUD_VENDOR    (0)
// #define CFG_TUD_MSC       (0)
// #define CFG_TUD_HID       (0)
// #define CFG_TUD_MIDI      (0)
// #define CFG_TUD_ECM_RNDIS (0)
// #define CFG_TUD_DFU       (1)

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  (64)
#endif

// CDC FIFO buffer sizes
// NTS: **must** be 64 or USB just doesn't work (presumably CFG_TUD_ENDPOINT0_SIZE ?)
//      <64: minicom and RPCD just hang
//      >64: messages of exactly 64 bytes disappear
#define CFG_TUD_CDC_RX_BUFSIZE  (64)
#define CFG_TUD_CDC_TX_BUFSIZE  (64)
