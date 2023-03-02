#include <stdio.h>
#include <stdlib.h>

#include "hardware/rtc.h"

#include "../include/TPIC6B595.h"
#include "../include/cli.h"
#include "../include/led.h"
#include "../include/wiegand.h"

#include <BLINK.pio.h>
#include <LED.pio.h>

const uint32_t BLINK_DELAY = 1000;

repeating_timer_t led_timer;

struct {
    int sys_led;
    int good_card;
    int bad_card;
    int card_timeout;
} LEDs = {0, 0, 0, 0};

/* struct for communicating between led_blinks API function and blinki
 * alarm handler. Allocated and initialised in led_blinks and free'd
 * in blinki.
 */
typedef struct blinks {
    uint32_t count;
} blinks;

/* Interrupt handler for writer LED input. Queue an LED ON/OFF message
 * to the message handler.
 *
 */
void ledi() {
    uint32_t value = led_program_get(PIO_LED, SM_LED);
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
        blink_program_blink(PIO_BLINK, SM_BLINK);
    }

    return 0;
}

/* Alarm handler for 'end of blink start delay'. Queues up 'count'
 * reader LED blinks to the BLINK FIFO..
 *
 */
bool callback(repeating_timer_t *rt) {
    if (LEDs.sys_led > 0) {
        LEDs.sys_led -= 10;
        if (LEDs.sys_led <= 0) {
            LEDs.sys_led = 0;
            gpio_put(ONBOARD_LED, 0);
        }
    }

    if (LEDs.good_card > 0) {
        LEDs.good_card -= 10;
        if (LEDs.good_card <= 0) {
            LEDs.good_card = 0;
            TPIC_set(YELLOW_LED, false);
        }
    }

    if (LEDs.bad_card > 0) {
        LEDs.bad_card -= 10;
        if (LEDs.bad_card <= 0) {
            LEDs.bad_card = 0;
            TPIC_set(RED_LED, false);
        }
    }

    if (LEDs.card_timeout > 0) {
        LEDs.card_timeout -= 10;
        if (LEDs.card_timeout <= 0) {
            LEDs.card_timeout = 0;
            TPIC_set(ORANGE_LED, false);
        }
    }

    return true;
}

/* Initialises the PIO for the emulator LED input and the reader LED
 * output. The PIO is only initialised with the reader LED output
 * program if the mode is 'READER'.
 *
 */
void led_initialise(enum MODE mode) {
    // ... initialse PIO for reader/writer LED
    uint offset = pio_add_program(PIO_LED, &led_program);

    led_program_init(PIO_LED, SM_LED, offset, WRITER_LED);

    irq_set_exclusive_handler(PIO_LED_IRQ, ledi);
    irq_set_enabled(PIO_LED_IRQ, true);
    pio_set_irq0_source_enabled(PIO_LED, IRQ_LED, true);

    if (mode == READER) {
        offset = pio_add_program(PIO_BLINK, &blink_program);

        blink_program_init(PIO_BLINK, SM_BLINK, offset, READER_LED);
    }

    // ... create repeating timer to manage blinking LEDs
    add_repeating_timer_ms(10, callback, NULL, &led_timer);
}

/* Handler for an LED event.
 *
 * 21: ON
 * 10: OFF
 */
void led_event(uint32_t v) {
    switch (v) {
    case 21:
        TPIC_set(GREEN_LED, true);
        tx("LED   ON");
        break;

    case 10:
        TPIC_set(GREEN_LED, false);
        tx("LED   OFF");
        break;

    default:
        tx("LED   ???");
    }
}

/* Blink the reader/writer LED after a delay. The reader/writer LED is managed
 * by a PIO pin not by the TPIC6B595.
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

/* 'blink' implementation for TPIC6B595 managed LEDs.
 *
 */
void blink(enum LED led) {
    switch (led) {
    case SYS_LED:
        LEDs.sys_led = 50;
        gpio_put(ONBOARD_LED, 1);
        break;

    case GOOD_CARD:
        LEDs.good_card = 500;
        TPIC_set(YELLOW_LED, true);
        break;

    case BAD_CARD:
        LEDs.bad_card = 500;
        TPIC_set(RED_LED, true);
        break;

    case CARD_TIMEOUT:
        LEDs.card_timeout = 500;
        TPIC_set(ORANGE_LED, true);
        break;
    }
}
