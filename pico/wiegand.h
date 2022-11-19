#pragma once

#define PIO_IN pio0
#define PIO_OUT pio1

typedef struct LED {
    uint pin;
    int32_t timer;
} LED;

extern const uint D0;
extern const uint D1;

extern const uint32_t MSG;
extern const uint32_t MSG_WATCHDOG;
extern const uint32_t MSG_SYSCHECK;
extern const uint32_t MSG_CARD_READ;

extern queue_t queue;
extern const LED TIMEOUT_LED;

extern void blink(LED *);
