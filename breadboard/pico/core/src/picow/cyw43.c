#include <pico/cyw43_arch.h>
#include <stdio.h>

void setup_cyw43() {
    char s[64];
    int err;

    if ((err = cyw43_arch_init()) != 0) {
        snprintf(s, sizeof(s), "CYW43 initialisation error (%d)", err);
        puts(s);
    } else {
        puts("CYW43 initialised");

        // The cyw43_gpio_set(..) function in the cyw43 driver invokes cyw43_ensure_up(...) which can
        // either take a while on first call or else or needs to be invoked from the main thread (not
        // sure which at this stage). Either which way, it causes the processor to hang inside a
        // wait-acquire-lock loop if invoked from an alarm or timer callback.
        //
        // Setting the LED pin immediately after initialisation seems to preempt the problem.
        //
        // Ref. https://forums.raspberrypi.com/viewtopic.php?t=348664
        //
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
    }
}
