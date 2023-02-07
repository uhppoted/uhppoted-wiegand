#include <stdio.h>

#include "../include/cli.h"
#include "../include/relay.h"

bool relay_monitor(repeating_timer_t *);

/* Initialises the monitor for relay normally open ('NO') and normally
 * closed ('NC') inputs.
 *
 */
bool relay_initialise(enum MODE mode) {
    static repeating_timer_t relay_rt;

    return add_repeating_timer_ms(1, relay_monitor, NULL, &relay_rt);
}

static float vopen = 0.5;
static float vclosed = 0.5;
static bool normally_open = false;
static bool normally_closed = false;

bool relay_monitor(repeating_timer_t *rt) {
    static bool initialised = false;

    vopen += (gpio_get(RELAY_NO) == 0) ? 0.01 * (1.0 - vopen) : 0.01 * (0.0 - vopen);
    vclosed += (gpio_get(RELAY_NC) == 0) ? 0.01 * (1.0 - vclosed) : 0.01 * (0.0 - vclosed);

    bool NO = normally_open;
    bool NC = normally_closed;

    if (vopen > 0.9) {
        NO = true;
    } else if (vopen < 0.1) {
        NO = false;
    }

    if (vclosed > 0.9) {
        NC = true;
    } else if (vclosed < 0.1) {
        NC = false;
    }

    if (!initialised) {
        if ((vopen < 0.1 || vopen > 0.9) && (vclosed < 0.1 || vclosed > 0.9)) {
            initialised = true;
            normally_open = NO;
            normally_closed = NC;

            uint32_t v = MSG_RELAY | (normally_open ? 0x01 : 0x00) | (normally_closed ? 0x02 : 0x00);
            if (!queue_is_full(&queue)) {
                queue_try_add(&queue, &v);
            }
        }
    } else if (normally_open != NO || normally_closed != NC) {
        char s[64];

        normally_open = NO;
        normally_closed = NC;

        uint32_t v = MSG_RELAY | (normally_open ? 0x01 : 0x00) | (normally_closed ? 0x02 : 0x00);

        // snprintf(s, sizeof(s), ">>>> X [%02x]  %d::%.3f %d::%.3f", v, normally_open, vopen, normally_closed, vclosed);
        // tx(s);

        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }
    }

    return true;
}

// 2023-02-07 14:33:03  >>>> X [60000000]  0::0.099 0::0.899
// 2023-02-07 14:33:03  >>>> X [60000002]  0::0.097 1::0.901
// 2023-02-07 14:33:03  >>>> Y [00]  0::0::0.095 1::0::0.903
// 2023-02-07 14:33:03  RELAY ERROR (NONE)
// 2023-02-07 14:33:03  >>>> Y [02]  0::0::0.090 1::1::0.909
// 2023-02-07 14:33:03  RELAY NORMALLY CLOSED
//
// 2023-02-07 14:33:14  >>>> X [60000000]  0::0.894 0::0.099
// 2023-02-07 14:33:14  >>>> Y [00]  0::0::0.898 0::0::0.095
// 2023-02-07 14:33:14  RELAY ERROR (NONE)
//
// 2023-02-07 14:33:14  >>>> X [60000001]  1::0.901 0::0.092
// 2023-02-07 14:33:14  >>>> Y [01]  1::1::0.911 0::0::0.083
// 2023-02-07 14:33:14  RELAY NORMALLY OPEN

/* Handler for a RELAY event.
 *
 */
void relay_event(uint32_t v) {
    bool no = (v & 0x01) == 0x01;
    bool nc = (v & 0x02) == 0x02;
    char s[64];

    // snprintf(s, sizeof(s), ">>>> Y [%02x]  %d::%d::%.3f %d::%d::%.3f", v, normally_open, no, vopen, normally_closed, nc, vclosed);
    // tx(s);

    if (no && nc) {
        tx("RELAY ERROR (NC + NO)");
    } else if (no) {
        tx("RELAY NORMALLY OPEN");
    } else if (nc) {
        tx("RELAY NORMALLY CLOSED");
    } else {
        // FIXME tx("RELAY ERROR (NONE)");
    }
}

void relay_debug() {
    char s[64];

    snprintf(s, sizeof(s), ">>>> ? %d::%.3f %d::%.3f", normally_open, vopen, normally_closed, vclosed);
    tx(s);
}