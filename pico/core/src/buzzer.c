#include <stdio.h>
#include <stdlib.h>

#include "../include/buzzer.h"
#include "../include/wiegand.h"

const uint32_t BUZZER_DELAY = 100;
const uint32_t BUZZER_BEEP = 1250;
const uint32_t BUZZER_INTERVAL = 500;

/* struct for buzzer state
 *
 */
struct {
    bool initialised;
    int32_t beep;
    int32_t delay;
    queue_t queue;
    repeating_timer_t timer;
} BUZZER_STATE = {
    .initialised = false,
    .beep = 0,
};

/* 100ms interval buzzer callback.
 *
 * Processes active or queued buzzer events.
 *
 */
bool buzzer_callback(repeating_timer_t *rt) {
    uint32_t beep;

    if (BUZZER_STATE.beep > 0) {
        gpio_put(BUZZER, 0);
        BUZZER_STATE.beep -= 100;

    } else if (BUZZER_STATE.delay > 0) {
        gpio_put(BUZZER, 1);
        BUZZER_STATE.delay -= 100;

    } else if (queue_try_remove(&BUZZER_STATE.queue, &beep)) {

        if ((beep & 0x80000000) == 0x80000000) {
            BUZZER_STATE.delay = beep & 0x00ffffff;
            gpio_put(BUZZER, 1);
        } else {
            BUZZER_STATE.beep = beep & 0x00ffffff;
            gpio_put(BUZZER, 0);
        }

    } else {
        gpio_put(BUZZER, 1);
    }

    return true;
}

/* Initialises the PIO for the buzzer. The PIO is initialised
 * in both 'reader' and 'writer' modes.
 *
 */
bool buzzer_initialise(enum MODE mode) {
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 1);

    queue_init(&BUZZER_STATE.queue, sizeof(int32_t), 16);

    if (add_repeating_timer_ms(100, buzzer_callback, NULL, &BUZZER_STATE.timer)) {
        BUZZER_STATE.initialised = true;
        return true;
    }

    return false;
}

/* Handler for a buzzer beep.
 *
 * NOTE: the buzzer queue is 16 deep so the maximum number of beeps is 8 (8 beep + 8 intervals).
 *       Which seems like enough.
 */
void buzzer_beep(uint8_t count) {
    if (BUZZER_STATE.initialised) {
        int32_t delay = 0x80000000 | BUZZER_DELAY;
        int32_t beep = 0x00000000 | BUZZER_BEEP;
        int32_t interval = 0x00000000 | BUZZER_INTERVAL;

        if (queue_try_add(&BUZZER_STATE.queue, &delay)) {
            for (int i = 0; i < count; i++) {
                if (!queue_try_add(&BUZZER_STATE.queue, &beep)) {
                    break;
                }

                if (!queue_try_add(&BUZZER_STATE.queue, &interval)) {
                    break;
                }
            }
        }
    }
}
