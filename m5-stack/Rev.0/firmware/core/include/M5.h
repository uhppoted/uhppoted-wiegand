#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <hwconfig.h>
#include <pico/util/queue.h>

extern const uint32_t MSG;
extern const uint32_t MSG_LED;
extern const uint32_t MSG_IO6;
extern const uint32_t MSG_IO7;
extern const uint32_t MSG_RX;
extern const uint32_t MSG_TTY;
extern const uint32_t MSG_TICK;
extern const uint32_t MSG_WATCHDOG;
extern const uint32_t MSG_LOG;

extern const constants HW;

typedef enum {
    MESSAGE_NONE,
    MESSAGE_BOOLEAN,
    MESSAGE_UINT32,
    MESSAGE_BUFFER,
} msg_type;

struct buffer;

typedef struct message {
    uint32_t message;
    msg_type tag;
    union {
        uint32_t none;
        bool boolean;
        uint32_t u32;
        struct buffer *buffer;
    };
} message;

extern bool push(message m);
