#include <pico/time.h>

#include "SK6812.pio.h"

#include <M5.h>
#include <SK6812.h>

extern const constants HW;

struct {
    PIO pio;
    int sm;
    uint gpio;
} SK6812 = {
    .pio = pio0,
    .sm = 0,
};

void SK6812_init() {
    SK6812.pio = HW.SK6812.pio;
    SK6812.sm = HW.SK6812.sm;
    SK6812.gpio = HW.SK6812.gpio;

    pio_sm_claim(SK6812.pio, SK6812.sm);
    uint offset = pio_add_program(SK6812.pio, &sk6812_program);

    sk6812_program_init(SK6812.pio, SK6812.sm, offset, SK6812.gpio, 800000, true);

    // The SK6812 draws power from the ISOW7841 which needs ~2ms for power-on soft-start
    sleep_ms(5);
    pio_sm_put_blocking(SK6812.pio, SK6812.sm, 0u);
}

void SK6812_set(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t rgb = (green << 16u) | (red << 8u) | (blue << 0u);

    pio_sm_put_blocking(SK6812.pio, SK6812.sm, rgb << 8u);
}