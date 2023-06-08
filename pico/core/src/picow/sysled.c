#include <pico/cyw43_arch.h>

void init_sysled() {
}

/**
 * Invoking cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,false) from the LED repeating timer
 * callback seems to cause a lockup if the interval is less than about 750ms.
 * Immediate cause is a cyw43 driver loop waiting to acquire a lock (which makes
 * absolutely no sense in the context). Interim workaround (pending more investigation)
 * is to set the LED pin low immediately after initialisation.
 *
 * Ref. https://forums.raspberrypi.com/viewtopic.php?t=348664
 */
void set_sysled(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
}
