#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "../include/reader.h"
#include "../include/wiegand.h"
#include <IN.pio.h>

typedef struct reader {
    uint32_t card;
    uint32_t bits;
    int32_t timer;
    absolute_time_t start;
    absolute_time_t delta;
} reader;

int64_t timeout(alarm_id_t, void *);

const uint32_t READ_TIMEOUT = 100;

void reader_initialise() {
    PIO pio = PIO_IN;
    uint sm = 0;
    uint offset = pio_add_program(pio, &reader_program);

    reader_program_init(pio, sm, offset, D0, D1);

    irq_set_exclusive_handler(PIO0_IRQ_0, rxi);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_set_irq0_source_enabled(pio, pis_sm0_rx_fifo_not_empty, true);
}

void rxi() {
    const uint sm = 0;

    static reader rdr = {
        .card = 0,
        .bits = 0,
        .timer = -1,
        .start = 0,
    };

    if (rdr.bits == 0) {
        rdr.start = get_absolute_time();
        rdr.bits = 0;
        rdr.timer = add_alarm_in_ms(READ_TIMEOUT, timeout, (reader *)&rdr, true);
    }

    uint32_t value = reader_program_get(PIO_IN, sm);

    switch (value) {
    case 1:
        rdr.card <<= 1;
        rdr.card |= 0x00000001;
        rdr.bits++;
        break;

    case 2:
        rdr.card <<= 1;
        rdr.bits++;
        break;
    }

    if (rdr.bits >= 26) {
        if (rdr.timer != -1) {
            cancel_alarm(rdr.timer);
        }

        uint32_t v = MSG_CARD_READ | (rdr.card & 0x0fffffff);
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }

        rdr.card = 0;
        rdr.bits = 0;
        rdr.timer = -1;
    }
}

int64_t timeout(alarm_id_t id, void *data) {
    reader *r = (reader *)data;
    r->bits = 0;
    r->card = 0;

    blink((LED *)&TIMEOUT_LED);
    cancel_alarm(id);
}
