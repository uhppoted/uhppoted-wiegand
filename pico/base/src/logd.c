#include <stdio.h>
#include <stdlib.h>

#include "hardware/rtc.h"

#include <common.h>
#include <uart.h>
#include <wiegand.h>

#include "logd.h"

/* Internal state for the logging daemon.
 *
 */
typedef struct LOGD {
} LOGD;

/* Initialises the logging daemon.
 *
 */
bool logd_initialise(enum MODE mode) {
    return true;
}

void logd_terminate() {
}

void logd_log(const char *message) {
    static int size = 128;
    char *s;

    if ((s = calloc(size, 1)) != NULL) {
        datetime_t now;
        int N;
        uint32_t msg;

        rtc_get_datetime(&now);

        N = timef(now, s, size);

        snprintf(&s[N], size - N, "  %s\n", message);

        msg = MSG_LOG | ((uint32_t)s & 0x0fffffff); // SRAM_BASE is 0x20000000
        if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
            free(s);
        }
    }
}
