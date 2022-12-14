#pragma once

#include "hardware/pio.h"

#include "pico/util/queue.h"

extern const PIO PIO_READER;
extern const PIO PIO_EMULATOR;
extern const PIO PIO_LED;
extern const PIO PIO_BLINK;
extern const PIO PIO_BUZZER;

extern const uint SM_READER;
extern const uint SM_EMULATOR;
extern const uint SM_LED;
extern const uint SM_BLINK;
extern const uint SM_BUZZER;

extern const enum pio_interrupt_source IRQ_READER;
extern const enum pio_interrupt_source IRQ_LED;

enum MODE { UNKNOWN = 0,
            READER,
            EMULATOR };

enum ACCESS { // UNKNOWN = 0,
    GRANTED = 1,
    DENIED = 2
};

typedef struct LED {
    uint pin;
    int32_t timer;
} LED;

typedef struct card {
    datetime_t timestamp;
    uint32_t facility_code;
    uint32_t card_number;
    bool ok;
    enum ACCESS granted;
} card;

extern enum MODE mode;

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
extern const uint32_t MSG_TX;
extern const uint32_t MSG_RXI;
extern const uint32_t MSG_DEBUG;

extern const LED TIMEOUT_LED;
extern const uint BUZZER;
extern const uint GREEN_LED;
extern enum MODE mode;
extern queue_t queue;
extern card last_card;

extern void blink(LED *);
extern void cardf(const card *, char *, int);
extern int timef(const datetime_t, char *, int);
extern int bits(uint32_t);
