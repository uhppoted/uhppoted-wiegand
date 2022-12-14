#include <stdio.h>

#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "../include/acl.h"
#include "../include/cli.h"
#include "../include/led.h"
#include "../include/reader.h"
#include "../include/wiegand.h"
#include <READ.pio.h>

typedef struct reader {
    uint32_t card;
    uint32_t bits;
    int32_t timer;
    absolute_time_t start;
    absolute_time_t delta;
} reader;

int64_t read_timeout(alarm_id_t, void *);

const uint32_t READ_TIMEOUT = 100;

void rxi() {
    static uint32_t card = 0;

    uint32_t value = reader_program_get(PIO_READER, SM_READER);

    switch (value) {
    case 1:
        card <<= 1;
        card |= 0x00000001;
        break;

    case 2:
        card <<= 1;
        card |= 0x00000000;
        break;
    }

    uint32_t v = MSG_RXI | (card & 0x0fffffff);
    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }
}

void reader_initialise() {
    uint offset = pio_add_program(PIO_READER, &reader_program);

    reader_program_init(PIO_READER, SM_READER, offset, READER_D0, READER_D1);

    irq_set_exclusive_handler(PIO0_IRQ_0, rxi);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_set_irq0_source_enabled(PIO_READER, IRQ_READER, true);
}

void on_card_rxi(uint32_t v) {
    static reader rdr = {
        .card = 0,
        .bits = 0,
        .timer = 0,
        .start = 0,
    };

    // ... reset reader after 250ms (add_alarm_in_ms is not guaranteed to succeed)
    absolute_time_t now = get_absolute_time();
    int64_t dt = absolute_time_diff_us(rdr.start, now) / 1;

    if (rdr.bits == 0 || dt > 250 * 1000) {
        if (rdr.timer > 0) {
            cancel_alarm(rdr.timer);
        }

        rdr.start = get_absolute_time();
        rdr.bits = 0;
        rdr.timer = add_alarm_in_ms(READ_TIMEOUT, read_timeout, (reader *)&rdr, true);
    }

    rdr.card = v & 0x03ffffff;
    rdr.bits++;

    if (rdr.bits >= 26) {
        if (rdr.timer > 0) {
            cancel_alarm(rdr.timer);
            rdr.timer = 0;
        }

        uint32_t v = MSG_CARD_READ | (rdr.card & 0x03ffffff);
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }

        rdr.card = 0;
        rdr.bits = 0;
        rdr.timer = -1;
    }
}

void on_card_read(uint32_t v) {
    int even = bits(v & 0x03ffe000);
    int odd = bits(v & 0x00001fff);
    uint32_t card = (v >> 1) & 0x00ffffff;

    last_card.facility_code = (card >> 16) & 0x000000ff;
    last_card.card_number = card & 0x0000ffff;
    last_card.ok = (even % 2) == 0 && (odd % 2) == 1;
    last_card.granted = UNKNOWN;

    rtc_get_datetime(&last_card.timestamp);

    if (last_card.ok && mode == READER) {
        if (acl_allowed(last_card.facility_code, last_card.card_number)) {
            last_card.granted = GRANTED;
        } else {
            last_card.granted = DENIED;
        }
    }

    if (last_card.granted == GRANTED) {
        led_blink(1);
    } else if (last_card.granted == DENIED) {
        led_blink(3);
    }
}

int64_t read_timeout(alarm_id_t id, void *data) {
    reader *r = (reader *)data;

    r->bits = 0;
    r->card = 0;
    r->timer = 0;

    blink((LED *)&TIMEOUT_LED);
}
