#include <hardware/watchdog.h>

#include <sys.h>
#include <wiegand.h>

// #define LOGTAG "SYS"

const uint32_t MSG = 0xf0000000;
const uint32_t MSG_WATCHDOG = 0xe0000000;
const uint32_t MSG_TICK = 0xf0000000;

extern queue_t queue;

bool _tick(repeating_timer_t *t);
void _syscheck();

struct {
    repeating_timer_t timer;
    bool reboot;
} SYSTEM = {
    .reboot = false,
    };

bool sysinit() {
    if (!add_repeating_timer_ms(1000, _tick, &SYSTEM, &SYSTEM.timer)) {
        return false;
    }

    return true;
}

bool _tick(repeating_timer_t *t) {
    push((message) {
        .message = MSG_TICK,
        .tag = MESSAGE_NONE,
    });

    return true;
}

void syscheck() {
    // ... kick watchdog
    if (!SYSTEM.reboot) {
        push((message) {
            .message = MSG_WATCHDOG,
            .tag = MESSAGE_NONE,
        });
    }
}

void dispatch(uint32_t v) {
    if ((v & MSG) == MSG_TICK) {
        syscheck();
        sys_tick();

        // // ... ping terminal
        // if (mutex_try_enter(&SYSTEM.guard, NULL)) {
        //     _print(TERMINAL_QUERY_STATUS);
        //     mutex_exit(&SYSTEM.guard);
        // }
    }

    if ((v & MSG) == MSG_WATCHDOG) {
        watchdog_update();
    }
}
