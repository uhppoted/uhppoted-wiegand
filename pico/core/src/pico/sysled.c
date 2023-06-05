#include "hardware/gpio.h"

void init_sysled() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

void set_sysled(bool on) {
    gpio_put(PICO_DEFAULT_LED_PIN, on);
}