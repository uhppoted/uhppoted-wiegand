#include <pico/stdlib.h>
#include <pico/time.h>

#include <M5.h>
#include <gpio.h>
#include <log.h>

#define LOGTAG "GPIO"

struct {
    bool io6;
    bool io7;
    repeating_timer_t timer;
} GPIO = {
    .io6 = false,
    .io7 = false,
};

bool GPIO_on_update(repeating_timer_t *);

const int32_t GPIO_TICK = 25; // ms

void GPIO_init() {
    debugf(LOGTAG, "init");

    gpio_init(HW.gpio.IO6);
    gpio_init(HW.gpio.IO7);

    gpio_set_dir(HW.gpio.IO6, GPIO_IN);
    gpio_set_dir(HW.gpio.IO7, GPIO_IN);

    gpio_set_input_hysteresis_enabled(HW.gpio.IO6, true);
    gpio_set_input_hysteresis_enabled(HW.gpio.IO7, true);

    gpio_pull_up(HW.gpio.IO6);
    gpio_pull_up(HW.gpio.IO7);

    infof(LOGTAG, "initialised");
}

void GPIO_start() {
    infof(LOGTAG, "start");

    add_repeating_timer_us(1000 * GPIO_TICK, GPIO_on_update, NULL, &GPIO.timer);
}

bool GPIO_on_update(repeating_timer_t *rt) {
    float io6 = !gpio_get(HW.gpio.IO6);
    float io7 = !gpio_get(HW.gpio.IO7);

    if (io6 != GPIO.io6) {
        GPIO.io6 = io6;

        push((message){
            .message = MSG_IO6,
            .tag = MESSAGE_BOOLEAN,
            .boolean = GPIO.io6,
        });
    }

    if (io7 != GPIO.io7) {
        GPIO.io7 = io7;

        push((message){
            .message = MSG_IO7,
            .tag = MESSAGE_BOOLEAN,
            .boolean = GPIO.io7,
        });
    }

    return true;
}
