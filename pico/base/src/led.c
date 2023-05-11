#include <stdio.h>
#include <stdlib.h>

#include "hardware/rtc.h"

#include "../include/TPIC6B595.h"
#include "../include/led.h"
#include "../include/uart.h"
#include "../include/wiegand.h"

const uint32_t BLINK_DELAY = 500;
const uint32_t BLINK_BLINK = 1000;
const uint32_t BLINK_INTERVAL = 250;

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

/* struct for blink state
 *
 */
struct {
    bool initialised;
    int32_t blink;
    int32_t delay;
    queue_t queue;
    repeating_timer_t timer;
} BLINK_STATE = {
    .initialised = false,
    .blink = 0,
};

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

/* 100ms interval READER LED blink callback.
 *
 * Processes active or queued blink events.
 *
 */
bool blink_callback(repeating_timer_t *rt) {
    uint32_t blink;

    if (BLINK_STATE.blink > 0) {
        gpio_put(READER_LED, 0);
        BLINK_STATE.blink -= 100;
    } else if (BLINK_STATE.delay > 0) {
        gpio_put(READER_LED, 1);
        BLINK_STATE.delay -= 100;
    } else if (queue_try_remove(&BLINK_STATE.queue, &blink)) {
        if ((blink & 0x80000000) == 0x80000000) {
            BLINK_STATE.delay = blink & 0x00ffffff;
            gpio_put(READER_LED, 1);
        } else {
            BLINK_STATE.blink = blink & 0x00ffffff;
            gpio_put(READER_LED, 0);
        }
    } else {
        gpio_put(READER_LED, 1);
    }

    return true;
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

    gpio_init(READER_LED);
    gpio_set_dir(READER_LED, GPIO_OUT);
    gpio_put(READER_LED, 1);

    // ... initialise LED input poll timer
    if (add_repeating_timer_us(1000, LED_callback, NULL, &LEDs.timer)) {
        LEDs.initialised = true;
    } else {
        ok = false;
    }

    // ... initialise status LEDs output blink timer
    if (!add_repeating_timer_ms(10, callback, NULL, &led_timer)) {
        ok = false;
    }

    // ... initialise READER LED
    if ((mode == READER) || (mode == CONTROLLER)) {
        queue_init(&BLINK_STATE.queue, sizeof(int32_t), 16);

        if (add_repeating_timer_ms(100, blink_callback, NULL, &BLINK_STATE.timer)) {
            BLINK_STATE.initialised = true;
        } else {
            ok = false;
        }
    }

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

/* Blink the reader LED after a delay. The reader LED is managed by a PIO pin
 * not by the TPIC6B595.
 *
 * NOTE: the blink queue is 16 deep so the maximum number of blinks is 8
 *       (8 beep + 8 intervals).
 */
void led_blink(uint8_t count) {
    if (BLINK_STATE.initialised) {
        int32_t delay = 0x80000000 | BLINK_DELAY;
        int32_t blink = 0x00000000 | BLINK_BLINK;
        int32_t interval = 0x00000000 | BLINK_INTERVAL;

        if (queue_try_add(&BLINK_STATE.queue, &delay)) {
            for (int i = 0; i < count; i++) {
                if (!queue_try_add(&BLINK_STATE.queue, &delay)) {
                    break;
                }

                if (!queue_try_add(&BLINK_STATE.queue, &interval)) {
                    break;
                }
            }
        }
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
