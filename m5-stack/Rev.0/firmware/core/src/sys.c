#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <hardware/watchdog.h>

#include <pico/sync.h>

#include <M5.h>
#include <SK6812.h>
#include <cli.h>
#include <log.h>
#include <sys.h>
#include <uart.h>
#include <wiegand.h>

#define LOGTAG "SYS"
#define PRINT_QUEUE_SIZE 64

extern const char *TERMINAL_QUERY_STATUS;

const uint32_t MSG = 0xf0000000;
const uint32_t MSG_LED = 0x10000000;
const uint32_t MSG_IO6 = 0x20000000;
const uint32_t MSG_IO7 = 0x30000000;
const uint32_t MSG_RX = 0x40000000;
const uint32_t MSG_TTY = 0xc0000000;
const uint32_t MSG_LOG = 0xd0000000;
const uint32_t MSG_WATCHDOG = 0xe0000000;
const uint32_t MSG_TICK = 0xf0000000;

extern queue_t queue;

bool _tick(repeating_timer_t *t);
void _syscheck();
void _sys_trace();

void _push(const char *);
void _flush();
void _print(const char *);

uint32_t get_total_heap();
uint32_t get_free_heap();

struct {
    repeating_timer_t timer;
    bool reboot;

    struct {
        mutex_t lock;
        bool connected;
        int head;
        int tail;
        char list[PRINT_QUEUE_SIZE][128];
    } queue;

    struct {
        float interval;
        absolute_time_t touched;
        repeating_timer_t timer;
    } trace;

    mutex_t guard;
} SYSTEM = {
    .reboot = false,

    .queue = {
        .connected = false,
        .head = 0,
        .tail = 0,
    },

    .trace = {
        .interval = (float)TRACE,
        .touched = 0,
    },
};

bool sysinit() {
    mutex_init(&SYSTEM.guard);
    mutex_init(&SYSTEM.queue.lock);

    if (!add_repeating_timer_ms(1000, _tick, &SYSTEM, &SYSTEM.timer)) {
        return false;
    }

    return true;
}

/* Sets sys.reboot flag to inhibit watchdog reset.
 *
 */
void sys_reboot() {
    SYSTEM.reboot = true;
}

bool _tick(repeating_timer_t *t) {
    push((message){
        .message = MSG_TICK,
        .tag = MESSAGE_NONE,
    });

    return true;
}

void syscheck() {
    _sys_trace();

    // ... kick watchdog
    if (!SYSTEM.reboot) {
        push((message){
            .message = MSG_WATCHDOG,
            .tag = MESSAGE_NONE,
        });
    }
}

void _sys_trace() {
    int64_t interval = (int64_t)(1000.0 * SYSTEM.trace.interval);
    absolute_time_t now = get_absolute_time();
    int64_t delta = absolute_time_diff_us(SYSTEM.trace.touched, now) / 1000;

    if (interval > 0 && delta >= interval) {
        uint32_t heap = get_total_heap();
        uint32_t available = get_free_heap();
        float used = 1.0 - ((float)available / (float)heap);

        debugf(LOGTAG, "trace  queue:%u  heap:%u  heap:%u  used:%.1f%%",
               queue_get_level(&queue),
               heap,
               available,
               100.0f * used);

        SYSTEM.trace.touched = get_absolute_time();
    }
}

void stdout_connected(bool connected) {
    if (connected != SYSTEM.queue.connected) {
        SYSTEM.queue.connected = connected;
        if (connected) {
            print("");
        }
    }
}

void dispatch(uint32_t v) {
    if ((v & MSG) == MSG_TICK) {
        syscheck();
        sys_tick();

        // ... ping terminal
        if (mutex_try_enter(&SYSTEM.guard, NULL)) {
            _print(TERMINAL_QUERY_STATUS);
            mutex_exit(&SYSTEM.guard);
        }

        // ... bump log queue
        push((message){
            .message = MSG_LOG,
            .tag = MESSAGE_NONE,
        });
    }

    if ((v & MSG) == MSG_LED) {
        infof(LOGTAG, "LED %s", (v & 0x0fffffff) == 0x01 ? "on" : "off");

        if ((v & 0x0fffffff) == 0x01) {
            SK6812_set(0, 8, 0);
        } else {
            SK6812_set(0, 0, 0);
        }
    }

    if ((v & MSG) == MSG_IO6) {
        infof(LOGTAG, "IO6 %s", (v & 0x0fffffff) == 0x01 ? "on" : "off");

        if ((v & 0x0fffffff) == 0x01) {
            uint32_t facility_code = CARD_IO6 / 100000;
            uint32_t card = CARD_IO6 % 100000;

            if (!write_card(facility_code, card)) {
                debugf(LOGTAG, "card %u%05u error", facility_code, card);
            }
        }
    }

    if ((v & MSG) == MSG_IO7) {
        infof(LOGTAG, "IO7 %s", (v & 0x0fffffff) == 0x01 ? "on" : "off");

        if ((v & 0x0fffffff) == 0x01) {
            uint32_t facility_code = CARD_IO7 / 100000;
            uint32_t card = CARD_IO7 % 100000;

            if (!write_card(facility_code, card)) {
                debugf(LOGTAG, "card %u%05u error", facility_code, card);
            }
        }
    }

    if ((v & MSG) == MSG_RX) {
        struct rxh *rxh = (struct rxh *)(SRAM_BASE | (v & 0x0fffffff));

        rxh->f(rxh->buffer);
    }

    if ((v & MSG) == MSG_WATCHDOG) {
        watchdog_update();
    }

    if ((v & MSG) == MSG_LOG) {
        _flush();
    }

    if ((v & MSG) == MSG_TTY) {
        struct buffer *b = (struct buffer *)(SRAM_BASE | (v & 0x0fffffff));

        cli_rx(b);
    }
}

void print(const char *msg) {
    _push(msg);
}

void _push(const char *msg) {
    if (mutex_try_enter(&SYSTEM.queue.lock, NULL)) {
        int head = SYSTEM.queue.head;
        int tail = SYSTEM.queue.tail;
        int next = (head + 1) % PRINT_QUEUE_SIZE;

        if (next == SYSTEM.queue.tail) {
            snprintf(SYSTEM.queue.list[tail++], 128, "...\n");
            tail %= PRINT_QUEUE_SIZE;

            snprintf(SYSTEM.queue.list[tail], 128, "...\n");

            SYSTEM.queue.tail = tail;
        }

        if (next != SYSTEM.queue.tail) {
            snprintf(SYSTEM.queue.list[head], 128, "%s", msg);
            SYSTEM.queue.head = next;
        }

        mutex_exit(&SYSTEM.queue.lock);
    }

    push((message){
        .message = MSG_LOG,
        .tag = MESSAGE_NONE,
    });
}

void _flush() {
    if (mutex_try_enter(&SYSTEM.guard, NULL)) {
        if (mutex_try_enter(&SYSTEM.queue.lock, NULL)) {
            int head = SYSTEM.queue.head;
            int tail = SYSTEM.queue.tail;

            while (tail != head) {
                _print(SYSTEM.queue.list[tail++]);
                tail %= PRINT_QUEUE_SIZE;
            }

            SYSTEM.queue.tail = tail;
            mutex_exit(&SYSTEM.queue.lock);
        }

        mutex_exit(&SYSTEM.guard);
    }
}

void _print(const char *msg) {
    int remaining = strlen(msg);
    int ix = 0;
    int N;

    while (remaining > 0) {
        if ((N = fwrite(&msg[ix], 1, remaining, stdout)) <= 0) {
            break;
        } else {
            remaining -= N;
            ix += N;
        }
    }

    fflush(stdout);
}

uint32_t get_total_heap() {
    extern char __StackLimit, __bss_end__;

    return &__StackLimit - &__bss_end__;
}

uint32_t get_free_heap() {
    extern char __StackLimit, __bss_end__;

    uint32_t total = &__StackLimit - &__bss_end__;
    struct mallinfo m = mallinfo();

    return total - m.uordblks;
}
