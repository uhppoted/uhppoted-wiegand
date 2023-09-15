#include <stdio.h>
#include <stdlib.h>

#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include <common.h>
#include <led.h>
#include <logd.h>
#include <read.h>

#include <READ.pio.h>

typedef struct buffer {
    uint32_t word;
    uint32_t count;
    alarm_id_t alarm;
} buffer;

void rxi();
int64_t rxii(alarm_id_t, void *);
void on_keycode(char *, int);

const char DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#'};

void read_initialise(enum MODE mode) {
    uint offset = pio_add_program(PIO_READER, &read_program);

    read_program_init(PIO_READER, SM_READER, offset, READER_D0, READER_D1);

    irq_set_exclusive_handler(PIO_READER_IRQ, rxi);
    irq_set_enabled(PIO_READER_IRQ, true);
    pio_set_irq0_source_enabled(PIO_READER, IRQ_READER, true);
}

void on_card_read(uint32_t v) {
    int even = bits(v & 0x03ffe000);
    int odd = bits(v & 0x00001fff);
    uint32_t card = (v >> 1) & 0x00ffffff;

    last_card.ok = (even % 2) == 0 && (odd % 2) == 1;
    last_card.facility_code = (card >> 16) & 0x000000ff;
    last_card.card_number = card & 0x0000ffff;
    last_card.granted = UNKNOWN;

    rtc_get_datetime(&last_card.timestamp);
    blink(last_card.ok ? GOOD_CARD : BAD_CARD);
}

void on_keypad_digit(uint32_t v) {
    static char code[16] = {};
    static uint32_t index = 0;
    static alarm_id_t alarm = 0;

    int keycode = v & 0x0000000f;

    if (keycode < sizeof(DIGITS)) {
        char digit = DIGITS[keycode];

        if (digit == '*' || digit == '#') {
            on_keycode(code, index);
            index = 0;
        } else if (index < sizeof(code)) {
            code[index++] = digit;

            if (index >= sizeof(code)) {
                on_keycode(code, index);
                index = 0;
            }
        }
    }
}

void rxi() {
    static buffer buffer = {
        .word = 0,
        .count = 0,
        .alarm = 0,
    };

    if (buffer.alarm > 0) {
        cancel_alarm(buffer.alarm);
        buffer.alarm = 0;
    }

    uint32_t value = read_program_get(PIO_READER, SM_READER);

    switch (value) {
    case 1:
        buffer.word <<= 1;
        buffer.word |= 0x00000001;
        buffer.count++;
        break;

    case 2:
        buffer.word <<= 1;
        buffer.word |= 0x00000000;
        buffer.count++;
        break;
    }

    buffer.alarm = add_alarm_in_ms(10, rxii, &buffer, true);
}

int64_t rxii(alarm_id_t id, void *data) {
    buffer *b = (buffer *)data;

    if (b->count == 26) {
        uint32_t v = MSG_CARD_READ | (b->word & 0x03ffffff);
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }
    } else if (b->count == 4) {
        uint32_t v = MSG_KEYPAD_DIGIT | (b->word & 0x0000000f);
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }
    } else {
        blink(CARD_TIMEOUT);
    }

    b->word = 0;
    b->count = 0;

    return 0;
}

void on_keycode(char *code, int length) {
    if (length > 0) {
        int N = length + 1;
        char *s;

        if ((s = calloc(N, 1)) != NULL) {
            snprintf(s, N, "%s", code);
            uint32_t msg = MSG_CODE | ((uint32_t)s & 0x0fffffff); // SRAM_BASE is 0x20000000
            if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
                free(s);
            }
        }
    }
}
