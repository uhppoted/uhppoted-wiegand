#pragma once

#include "pico/util/queue.h"

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

extern void sys_start();
extern void sys_ok();
extern void sys_settime(char *);

extern void blink(LED *);

extern int timef(const datetime_t *, char *, int);
