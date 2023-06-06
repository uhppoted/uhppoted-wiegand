#include <pico/cyw43_arch.h>
#include <stdio.h>

#include <wiegand.h>

void init_sysled() {
}

/**
 * Invoking cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,false) from the LED repeating timer
 * callback seems to cause a lockup if the interval is less than about 750ms.
 * Absolutely no idea why - workaround uses a queued MSG.
 *
 * Ref. https://forums.raspberrypi.com/viewtopic.php?t=348664
 */
void set_sysled(bool on) {
    if (on) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    } else {
        uint32_t msg = MSG_SYSLED | (0 & 0x0fffffff);

        if ((msg != 0x00000000) && !queue_is_full(&queue)) {
            queue_try_add(&queue, &msg);
        }
    }
}

void set_sysled_off() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
}
