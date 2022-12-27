#include <stdio.h>

#include "hardware/rtc.h"

#include "../include/led.h"
#include "../include/wiegand.h"
#include <LED.pio.h>

void ledi() {
    PIO pio = PIO_OUT;
    const uint sm = 1;

    uint32_t value = led_program_get(pio, sm);
    uint32_t msg = MSG_LED | (value & 0x0fffffff);

    switch (value) {
    case 21:
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &msg);
        }
        break;

    case 10:
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &msg);
        }
        break;
    }
}

/* Initialises the PIO.
 *
 */
void led_initialise() {
    PIO pio = PIO_OUT;
    uint sm = 1;
    uint offset = pio_add_program(pio, &led_program);

    led_program_init(pio, sm, offset, WRITER_LED);

    irq_set_exclusive_handler(PIO1_IRQ_0, ledi);
    irq_set_enabled(PIO1_IRQ_0, true);
    pio_set_irq0_source_enabled(PIO_OUT, pis_sm1_rx_fifo_not_empty, true);
}

/* Handler for an LED event.
 *
 * 21: ON
 * 10: OFF
 */
void led_event(uint32_t v) {
    char s[64];
    datetime_t now;

    rtc_get_datetime(&now);

    int N = timef(&now, s, sizeof(s));

    switch (v) {
    case 21:
        gpio_put(GREEN_LED, 1);
        snprintf(&s[N], sizeof(s) - N, "  %-4s %s", "LED", "ON");
        break;

    case 10:
        gpio_put(GREEN_LED, 0);
        snprintf(&s[N], sizeof(s) - N, "  %-4s %s", "LED", "OFF");
        break;

    default:
        snprintf(&s[N], sizeof(s) - N, "  %-4s %s", "LED", "???");
    }

    puts(s);
}
