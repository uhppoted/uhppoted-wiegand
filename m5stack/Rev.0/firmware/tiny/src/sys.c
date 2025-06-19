#include <stdio.h>

#include <cli.h>
#include <log.h>
#include <sys.h>
#include <usb.h>
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

    if (!usb_init()) {
        return false;
    }

    log_init();
    cli_init();

    // ... startup message
    char s[64];

    snprintf(s, sizeof(s), "-----  WIEGAND    %s\n", VERSION);
    print(s);

    return true;
}

/* Blinks SYSLED.
 *
 */
void sys_tick() {
    sys.LED = !sys.LED;

    if (sys.LED) {
        put_rgb(0, 8, 0); // green
    } else {
        put_rgb(0, 0, 0); // off
    }
}

// NTS: mutex because the LED was occasionally used for debugging
void put_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t rgb = (red << 16u) | (green << 8u) | (blue / 16 << 0u);

    pio_sm_put_blocking(pio0, 0, rgb << 8u);
}
