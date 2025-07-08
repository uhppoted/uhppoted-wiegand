#include <stdint.h>
#include <stdio.h>

#include <pico/binary_info.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include <hardware/watchdog.h>

#include <GPIO.h>
#include <LED.h>
#include <SK6812.h>
#include <log.h>
#include <sys.h>
#include <wiegand.h>

#define LOGTAG "SYS"

const uint32_t WATCHDOG_TIMEOUT = 5000; // ms

queue_t queue;

int main() {
    bi_decl(bi_program_description("uhppoted-wiegand-emulator"));
    bi_decl(bi_program_version_string(VERSION));

    // ... kernel
    stdio_init_all();
    queue_init(&queue, sizeof(uint32_t), 64);
    alarm_pool_init_default();

    // ... initialise system
    if (!sys_init()) {
        warnf(LOGTAG, "*** ERROR INITIALISING SYSTEM");
        return -1;
    }

    watchdog_enable(WATCHDOG_TIMEOUT, true);

    SK6812_init();
    LED_init();
    wiegand_init();
    GPIO_init();

    if (!SK6812_start()) {
        warnf(LOGTAG, "*** ERROR INITIALISING SK6912");
        return -1;
    }

    LED_start();
    GPIO_start();

    sleep_ms(2500); // FIXME remove - delay to let USB initialise

    // ... run loop
    while (true) {
        uint32_t v;

        queue_remove_blocking(&queue, &v);
        dispatch(v);
    }

    return 0;
}
