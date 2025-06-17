// #include <stdio.h>
// #include <stdlib.h>
#include <stdint.h>

#include <pico/binary_info.h>
// #include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

// #include <bsp/board_api.h>
// #include <hardware/watchdog.h>

#include <log.h>
#include <sys.h>

#define LOGTAG "SYS"
#define _VERSION "v0.0"

const uint32_t WATCHDOG_TIMEOUT = 5000; // ms

queue_t queue;

int main() {
    bi_decl(bi_program_description("uhppoted-wiegang-emulator"));
    bi_decl(bi_program_version_string(_VERSION));

    // ... kernel
    stdio_init_all();

    queue_init(&queue, sizeof(uint32_t), 64);
    alarm_pool_init_default();

    // ... initialise system
    if (!sys_init()) {
        warnf(LOGTAG, "*** ERROR INITIALISING SYSTEM");
        return -1;
    }

    // syserr_set(ERR_RESTART, LOGTAG, "power-on");

    // if (watchdog_caused_reboot()) {
    //     syserr_set(ERR_WATCHDOG, LOGTAG, "watchdog reboot");
    // }

    // watchdog_enable(WATCHDOG_TIMEOUT, true);

    sleep_ms(2500); // FIXME remove - delay to let USB initialise

    // ... run loop
    while (true) {
        uint32_t v;

        queue_remove_blocking(&queue, &v);
        dispatch(v);
    }

    return 0;
}
