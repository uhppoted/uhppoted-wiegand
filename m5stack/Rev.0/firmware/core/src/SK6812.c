#include <pico/time.h>

#include "SK6812.pio.h"
#include <SK6812.h>
#include <wiegand.h>

extern const constants IO;

struct {
    PIO pio;
    int sm;
} sk6812 = {
    .pio = pio0,
    .sm = 0,
};

void SK6812_init() {
    PIO pio = sk6812.pio;
    int sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &sk6812_program);

    sk6812_program_init(pio, sm, offset, IO.SK6812, 800000, true);

    sk6812.sm = sm;

    // The SK6812 + ISOW7841 seems to need a little time to stabilise - setting the LED
    // immediately on startup (e.g. in sys_init) does weird things.
    sleep_ms(5);
    pio_sm_put_blocking(pio, sm, 0u);
}

void SK6812_set(uint8_t red, uint8_t green, uint8_t blue) {
    PIO pio = sk6812.pio;
    int sm = sk6812.sm;
    uint32_t rgb = (green << 16u) | (red << 8u) | (blue << 0u);

    pio_sm_put_blocking(pio, sm, rgb << 8u);
}