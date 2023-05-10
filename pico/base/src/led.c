#include <stdio.h>
#include <stdlib.h>

#include "hardware/rtc.h"

#include "../include/TPIC6B595.h"
#include "../include/led.h"
#include "../include/uart.h"
#include "../include/wiegand.h"

#include <BLINK.pio.h>

const uint32_t BLINK_DELAY = 1000;

repeating_timer_t led_timer;

struct {
    int sys_led;
    int good_card;
    int bad_card;
    int card_timeout;
    float lpf;
    bool on;
    bool initialised;
    repeating_timer_t timer;
} LEDs = {
    .sys_led = 0,
    .good_card = 0,
    .bad_card = 0,
    .card_timeout = 0,
    .lpf = 0.0,
    .on = false,
    .initialised = false,
};

/* struct for communicating between led_blinks API function and blinki
 * alarm handler. Allocated and initialised in led_blinks and free'd
 * in blinki.
 */
typedef struct blinks {
    uint32_t count;
} blinks;

/* 1ms LED callback.
 *
 * Polls and LPF filters the LED input from the external access controller. The
 * LPF is just a very basic 1st order filter with hysteresis.
 *
 */
bool LED_callback(repeating_timer_t *rt) {
    float v = gpio_get(WRITER_LED) == 0 ? 1.0 : 0.0;

    LEDs.lpf = 0.95 * LEDs.lpf + 0.05 * v;

    bool on = LEDs.lpf > 0.9;
    bool off = LEDs.lpf < 0.1;
    uint32_t msg = 0x00000000;

    if (on && !LEDs.on) {
        LEDs.on = true;
        msg = MSG_LED | (21 & 0x0fffffff);
    }

    if (off && LEDs.on) {
        LEDs.on = false;
        msg = MSG_LED | (10 & 0x0fffffff);
    }

    if ((msg != 0x00000000) && !queue_is_full(&queue)) {
        queue_try_add(&queue, &msg);
    }

    return true;
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
 * program if the mode is READER or CONTROLLER.
 *
 */
bool led_initialise(enum MODE mode) {
    bool ok = true;

    gpio_init(WRITER_LED);
    gpio_set_dir(WRITER_LED, GPIO_IN);
    gpio_pull_up(WRITER_LED);

    // ... initialise LED input poll timer
    if (!add_repeating_timer_us(1000, LED_callback, NULL, &LEDs.timer)) {
        ok = false;
    }

    // ... initialise LED output stuff
    if ((mode == READER) || (mode == CONTROLLER)) {
        uint offset = pio_add_program(PIO_BLINK, &blink_program);

        blink_program_init(PIO_BLINK, SM_BLINK, offset, READER_LED);
    }

    // ... create repeating timer to manage blinking LEDs
    if (!add_repeating_timer_ms(10, callback, NULL, &led_timer)) {
        ok = false;
    }

    LEDs.initialised = ok;

    return ok;
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
        tx("LED    ON");
        break;

    case 10:
        TPIC_set(GREEN_LED, false);
        tx("LED    OFF");
        break;

    default:
        tx("LED    ???");
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
    if (LEDs.initialised) {
        struct blinks *b = malloc(sizeof(struct blinks));

        b->count = count > 8 ? 8 : count;

        add_alarm_in_ms(BLINK_DELAY, blinki, (void *)b, true);
    }
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
