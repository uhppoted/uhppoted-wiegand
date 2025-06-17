#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include <pico/stdlib.h>
// #include <pico/sync.h>

// #include <hardware/pio.h>
// #include <hardware/ticks.h>

#include <sys.h>
#include <wiegand.h>

#include "ws2812.pio.h"

extern bool sysinit();

void put_rgb(uint8_t red, uint8_t green, uint8_t blue);

struct {
    bool LED;
} sys = {
    .LED = false,
};

bool sys_init() {
    // ... WS2812 LED
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, 16, 800000, true);

    put_rgb(128, 12, 0);

    // ... initialise
    if (!sysinit()) {
        return false;
    }

    // ... startup message
    printf("-----  WIEGAND   v%02x.%02x\n", (VERSION >> 8) & 0x00ff, (VERSION >> 0) & 0x00ff);

    return true;
}

/* Blinks SYSLED.
 *
 */
void sys_tick() {
    sys.LED = !sys.LED;

    if (sys.LED) {
        put_rgb(0, 8, 0);
    } else {
        put_rgb(0, 0, 0); // off
    }
}

// NTS: mutex because the LED was occasionally used for debugging
void put_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t rgb = (red << 16u) | (green << 8u) | (blue / 16 << 0u);

    pio_sm_put_blocking(pio0, 0, rgb << 8u);
}
