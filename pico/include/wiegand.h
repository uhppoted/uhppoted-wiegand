#pragma once

#include "pico/util/queue.h"

#define PIO_IN pio0
#define PIO_OUT pio1

enum MODE { UNKNOWN = 0,
            READER,
            EMULATOR };

typedef struct LED {
    uint pin;
    int32_t timer;
} LED;

typedef struct card {
    datetime_t timestamp;
    uint32_t facility_code;
    uint32_t card_number;
    bool ok;
} card;

extern enum MODE mode;

extern const uint MODE_READER;
extern const uint MODE_EMULATOR;
extern const uint READER_D0;
extern const uint READER_D1;
extern const uint READER_LED;
extern const uint WRITER_D0;
extern const uint WRITER_D1;
extern const uint WRITER_LED;

extern const uint32_t MSG;
extern const uint32_t MSG_WATCHDOG;
extern const uint32_t MSG_SYSCHECK;
extern const uint32_t MSG_CARD_READ;
extern const uint32_t MSG_LED;
extern const uint32_t MSG_DEBUG;

extern const LED TIMEOUT_LED;
extern enum MODE mode;
extern queue_t queue;
extern card last_card;

extern void blink(LED *);
extern void cardf(const card *, char *, int);
extern int timef(const datetime_t *, char *, int);
extern int bits(uint32_t);
