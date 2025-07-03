#include <stdlib.h>

#include <pico/time.h>
#include <pico/util/queue.h>

#include "SK6812.pio.h"

#include <M5.h>
#include <SK6812.h>
#include <log.h>

#define LOGTAG "SK6812"

extern const constants HW;

const uint32_t BLINK_ON = 250;  // ms
const uint32_t BLINK_OFF = 500; // ms
const int32_t TICK = 10;        // ms

struct {
    PIO pio;
    int sm;
    uint gpio;
    uint32_t rgb;

    struct {
        uint32_t rgb;
        uint32_t count;
        int32_t timer;
        queue_t queue;
    } blink;

    repeating_timer_t timer;
} SK6812 = {
    .pio = pio0,
    .sm = 0,
    .rgb = 0x000000,
    .blink = {
        .count = 0,
        .timer = 0,
    },
};

typedef struct task {
    uint32_t rgb;
    uint32_t count;
} task;

bool SK6812_callback(repeating_timer_t *);
void _blink();
bool _pop();

void SK6812_init() {
    SK6812.pio = HW.SK6812.pio;
    SK6812.sm = HW.SK6812.sm;
    SK6812.gpio = HW.SK6812.gpio;

    queue_init(&SK6812.blink.queue, sizeof(task), 32);

    pio_sm_claim(SK6812.pio, SK6812.sm);
    uint offset = pio_add_program(SK6812.pio, &sk6812_program);

    sk6812_program_init(SK6812.pio, SK6812.sm, offset, SK6812.gpio, 800000, true);

    // NTS: the SK6812 draws power from the ISOW7841 which needs ~2ms for power-on soft-start
    sleep_ms(5);
    pio_sm_put_blocking(SK6812.pio, SK6812.sm, 0u);
}

bool SK6812_start() {
    return add_repeating_timer_ms(TICK, SK6812_callback, NULL, &SK6812.timer);
}

void SK6812_set(uint8_t red, uint8_t green, uint8_t blue) {
    SK6812.rgb = (green << 16u) | (red << 8u) | (blue << 0u);

    if (SK6812.blink.count == 0 && SK6812.blink.timer == 0) {
        pio_sm_put_blocking(SK6812.pio, SK6812.sm, SK6812.rgb << 8u);
    }
}

void SK6812_blink(uint8_t red, uint8_t green, uint8_t blue, uint8_t blinks) {
    uint32_t rgb = (green << 16u) | (red << 8u) | (blue << 0u);
    task t = {
        .rgb = rgb,
        .count = blinks,
    };

    queue_try_add(&SK6812.blink.queue, &t);
}

//  timer <= 0 | count > 0  | _pop() | _blink()
// ------------|------------|--------|---------
//     0       |     X      |   X    |    0
//     1       |     0      |   0    |    0
//     1       |     0      |   1    |    1
//     1       |     1      |   0    |    1
//     1       |     1      |   1    |    1
//
// simplified:
//   if (timer <= 0 && (count > 0 || _pop())) { 
//      _blink(); 
//   }
bool SK6812_callback(repeating_timer_t *rt) {
    if (SK6812.blink.timer > 0) {
        SK6812.blink.timer -= TICK;
    }

    if (SK6812.blink.timer <= 0) {
        if (SK6812.blink.count > 0 || _pop()) {
            _blink();
        } else {
            SK6812.blink.timer = 0;
        }
    }

    return true;
}

void _blink() {
    bool on = ((SK6812.blink.count % 2) == 0) ? true : false;
    uint32_t rgb = on ? SK6812.blink.rgb : SK6812.rgb;

    pio_sm_put_blocking(SK6812.pio, SK6812.sm, rgb << 8u);

    SK6812.blink.timer = on ? BLINK_ON : BLINK_OFF;
    SK6812.blink.count--;
}

bool _pop() {
    task t;

    if (queue_try_remove(&SK6812.blink.queue, &t)) {
        SK6812.blink.rgb = t.rgb;
        SK6812.blink.count = 2 * t.count;
        return true;
    }

    return false;
}
