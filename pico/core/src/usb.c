#include <stdlib.h>

#include <pico/time.h>
#include <tusb.h>

#include "../include/wiegand.h"

repeating_timer_t usb_timer;

bool on_usb_rx(repeating_timer_t *rt) {
    if (tud_cdc_connected()) {
        uint8_t buffer[32];
        int ix = 0;

        while (tud_cdc_available() && ix < sizeof(buffer)) {
            uint8_t ch;
            uint32_t N = tud_cdc_read(&ch, 1);

            if (N > 0) {
                buffer[ix++] = ch;
            }
        }

        if (ix > 0) {
            char *b;

            if ((b = calloc(ix + 1, 1)) != NULL) {
                memmove(b, buffer, ix);
                uint32_t msg = MSG_RX | ((uint32_t)b & 0x0fffffff); // SRAM_BASE is 0x20000000
                if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
                    free(b);
                }
            }
        }
    }

    tud_task();

    return true;
}

void setup_usb() {
    add_repeating_timer_ms(10, on_usb_rx, NULL, &usb_timer);
}
