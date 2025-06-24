#include <pico/time.h>

#include "ws2812.pio.h"

#include "LED.h"

extern const uint WIEGAND_LED;

struct {
    PIO pio;
    int sm;
} LED = {
    .pio = pio0,
    .sm = 0,
};

void LED_init() {
    LED.sm = pio_claim_unused_sm(LED.pio, true);
    uint offset = pio_add_program(LED.pio, &ws2812_program);

    ws2812_program_init(LED.pio, LED.sm, offset, WIEGAND_LED, 800000, true);

    // The SK6812 + ISOW7841 seems to need a couple of ms to stabilise - setting the
    // LED immediately on startup (e.g. in sys_init) does weird things.
    sleep_ms(5);
    pio_sm_put_blocking(LED.pio, LED.sm, 0u);
}

void LED_set(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t rgb = (green << 16u) | (red << 8u) | (blue << 0u);

    pio_sm_put_blocking(LED.pio, LED.sm, rgb << 8u);
}