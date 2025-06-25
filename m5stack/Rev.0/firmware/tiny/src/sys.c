#include <stdio.h>

#include <cli.h>
#include <log.h>
#include <sys.h>
#include <usb.h>
#include <wiegand.h>

#include "ws2812.pio.h"

#define LOGTAG "SYS"

extern bool sysinit();
extern const constants IO;

void put_WS2812(uint8_t red, uint8_t green, uint8_t blue);

struct {
    PIO pio;
    int sm;
    bool LED;
} sys = {
    .pio = pio0,
    .sm = 0,
    .LED = false,
};

bool sys_init() {
    { // ... SYS LED
        pio_sm_claim(sys.pio, sys.sm);
        uint offset = pio_add_program(sys.pio, &ws2812_program);

        ws2812_program_init(sys.pio, sys.sm, offset, IO.SYSLED, 800000, true);
        put_WS2812(128, 12, 0);
    }

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
        put_WS2812(0, 8, 0); // green
    } else {
        put_WS2812(0, 0, 0); // off
    }
}

void put_WS2812(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t rgb = (red << 16u) | (green << 8u) | (blue / 16 << 0u);

    pio_sm_put_blocking(sys.pio, sys.sm, rgb << 8u);
}
