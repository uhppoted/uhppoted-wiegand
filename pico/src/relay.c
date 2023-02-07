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

    return add_repeating_timer_ms(5, relay_monitor, NULL, &relay_rt);
}

bool relay_monitor(repeating_timer_t *rt) {
    static bool initialised = false;
    static bool normally_open = false;
    static bool normally_closed = false;

    bool NO = (gpio_get(RELAY_NO) == 0);
    bool NC = (gpio_get(RELAY_NC) == 0);

    if (!initialised || normally_open != NO || normally_closed != NC) {
        initialised = true;
        normally_open = NO;
        normally_closed = NC;

        uint32_t v = MSG_RELAY | (normally_open ? 0x01 : 0x00) | (normally_closed ? 0x02 : 0x00);

        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }
    }

    return true;
}

/* Handler for a RELAY event.
 *
 */
void relay_event(uint32_t v) {
    bool normally_open = (v & 0x01) == 0x01;
    bool normally_closed = (v & 0x02) == 0x02;

    if (normally_open && normally_closed) {
        tx("RELAY ERROR (NC + NO)");
    } else if (normally_open) {
        tx("RELAY NORMALLY OPEN");
    } else if (normally_closed) {
        tx("RELAY NORMALLY CLOSED");
    } else {
        tx("RELAY ERROR (NONE)");
    }
}