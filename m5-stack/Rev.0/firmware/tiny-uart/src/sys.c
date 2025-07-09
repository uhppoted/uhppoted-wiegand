#include <stdio.h>

#include <M5.h>
#include <cli.h>
#include <log.h>
#include <sys.h>
#include <uart.h>

#include "ws2812.pio.h"

#define LOGTAG "SYS"

extern bool sysinit();
extern const constants HW;

void put_WS2812(uint8_t red, uint8_t green, uint8_t blue);

struct {
    PIO pio;
    int sm;
    uint gpio;
    bool LED;
} sys = {
    .LED = false,
};

bool sys_init() {
    { // ... SYS LED
        sys.pio = HW.WS2812.pio;
        sys.sm = HW.WS2812.sm;
        sys.gpio = HW.WS2812.gpio;

        pio_sm_claim(sys.pio, sys.sm);
        uint offset = pio_add_program(sys.pio, &ws2812_program);

        ws2812_program_init(sys.pio, sys.sm, offset, sys.gpio, 800000, true);
        put_WS2812(128, 12, 0);
    }

    // ... initialise
    if (!sysinit()) {
        return false;
    }

    if (!uart_initialise()) {
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
