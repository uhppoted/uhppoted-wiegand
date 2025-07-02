#include <stdlib.h>

#include <pico/time.h>

#include "SK6812.pio.h"

#include <M5.h>
#include <SK6812.h>
#include <log.h>

#define LOGTAG "SK6812"

extern const constants HW;

const uint32_t BLINK = 200; // ms

struct {
    PIO pio;
    int sm;
    uint gpio;
    uint32_t rgb;
} SK6812 = {
    .pio = pio0,
    .sm = 0,
    .rgb = 0x000000,
};

typedef struct task {
    uint32_t rgb;
    uint32_t count;
} task;

int64_t SK6812_unblink(alarm_id_t, void *);

void SK6812_init() {
    SK6812.pio = HW.SK6812.pio;
    SK6812.sm = HW.SK6812.sm;
    SK6812.gpio = HW.SK6812.gpio;

    pio_sm_claim(SK6812.pio, SK6812.sm);
    uint offset = pio_add_program(SK6812.pio, &sk6812_program);

    sk6812_program_init(SK6812.pio, SK6812.sm, offset, SK6812.gpio, 800000, true);

    // NTS: the SK6812 draws power from the ISOW7841 which needs ~2ms for power-on soft-start
    sleep_ms(5);
    pio_sm_put_blocking(SK6812.pio, SK6812.sm, 0u);
}

void SK6812_set(uint8_t red, uint8_t green, uint8_t blue) {
    SK6812.rgb = (green << 16u) | (red << 8u) | (blue << 0u);
    uint32_t rgb = SK6812.rgb << 8u;

    pio_sm_put_blocking(SK6812.pio, SK6812.sm, rgb);
}

void SK6812_blink(uint8_t red, uint8_t green, uint8_t blue, uint8_t blinks) {
    uint32_t rgb = (green << 16u) | (red << 8u) | (blue << 0u);
    task *t = (task *)calloc(1, sizeof(task));

    t->rgb = rgb;
    t->count = 2 * blinks;

    pio_sm_put_blocking(SK6812.pio, SK6812.sm, rgb << 8u);
    add_alarm_in_ms(BLINK, SK6812_unblink, t, true);
}

int64_t SK6812_unblink(alarm_id_t id, void *data) {
    task *t = (task *)data;

    if (t != NULL && t->count > 1) {
        t->count--;

        if ((t->count % 2) == 0) {
            pio_sm_put_blocking(SK6812.pio, SK6812.sm, t->rgb << 8u);
            return BLINK * 1000;
        } else {
            pio_sm_put_blocking(SK6812.pio, SK6812.sm, SK6812.rgb << 8u);
            return (750 - BLINK) * 1000;
        }
    }

    free(data);

    // ... default - revert to current colour
    uint32_t rgb = SK6812.rgb << 8u;

    pio_sm_put_blocking(SK6812.pio, SK6812.sm, rgb);

    return 0;
}
