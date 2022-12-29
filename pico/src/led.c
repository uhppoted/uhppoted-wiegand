#include <stdio.h>
#include <stdlib.h>

#include "hardware/rtc.h"

#include "../include/led.h"
#include "../include/wiegand.h"
#include <BLINK.pio.h>
#include <LED.pio.h>

const uint32_t BLINK_DELAY = 1000;

/* struct for communicating between led_blinks API function and blinki
 * alarm handler. Allocated and initialiesd in led_blinks and free'd
 * in blinki.
 */
typedef struct blinks {
    uint32_t count;
} blinks;

/* Interrupt handler for emulator LED input. Queue an LED ON/OFF message
 * to the message handler.
 *
 */
void ledi() {
    PIO pio = PIO_OUT;
    const uint sm = 1;

    uint32_t value = led_program_get(pio, sm);
    uint32_t msg = MSG_LED | (value & 0x0fffffff);

    switch (value) {
    case 21:
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &msg);
        }
        break;

    case 10:
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &msg);
        }
        break;
    }
}

/* Alarm handler for 'end of blink start delay'. Queues up 'count'
 * reader LED blinks to the BLINK FIFO..
 *
 */
int64_t blinki(alarm_id_t id, void *data) {
    uint32_t count = ((blinks *)data)->count;

    free(data);

    while (count-- > 0) {
        blink_program_blink(PIO_OUT, 2);
    }

    return 0;
}

/* Initialises the PIO for the emulator LED input and the reader LED
 * output. The PIO is only initialised with the reader LED output
 * program if the mode is 'READER'.
 *
 */
void led_initialise(enum MODE mode) {
    PIO pio = PIO_OUT;
    uint sm = 1;
    uint offset = pio_add_program(pio, &led_program);

    led_program_init(pio, sm, offset, WRITER_LED);

    irq_set_exclusive_handler(PIO1_IRQ_0, ledi);
    irq_set_enabled(PIO1_IRQ_0, true);
    pio_set_irq0_source_enabled(PIO_OUT, pis_sm1_rx_fifo_not_empty, true);

    if (mode == READER) {
        pio = PIO_OUT;
        sm = 2;
        offset = pio_add_program(pio, &blink_program);

        blink_program_init(pio, sm, offset, READER_LED);
    }
}

/* Handler for an LED event.
 *
 * 21: ON
 * 10: OFF
 */
void led_event(uint32_t v) {
    char s[64];
    datetime_t now;

    rtc_get_datetime(&now);

    int N = timef(&now, s, sizeof(s));

    switch (v) {
    case 21:
        gpio_put(GREEN_LED, 0);
        snprintf(&s[N], sizeof(s) - N, "  %-4s %s", "LED", "ON");
        break;

    case 10:
        gpio_put(GREEN_LED, 1);
        snprintf(&s[N], sizeof(s) - N, "  %-4s %s", "LED", "OFF");
        break;

    default:
        snprintf(&s[N], sizeof(s) - N, "  %-4s %s", "LED", "???");
    }

    puts(s);
}

/* Handler for an LED blink.
 *
 * NOTE: because the PIO FIFO is only 8 deep, the maximum number of blinks
 *       is 8. Could probably get complicated about this but no reason to
 *       at the moment.
 */
void led_blink(uint8_t count) {
    struct blinks *b = malloc(sizeof(struct blinks));

    b->count = count > 8 ? 8 : count;

    add_alarm_in_ms(BLINK_DELAY, blinki, (void *)b, true);
}
