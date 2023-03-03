#include <stdio.h>

#include "hardware/rtc.h"

#include "../include/TPIC6B595.h"
#include "../include/cli.h"
#include "../include/relays.h"

enum RELAY_STATE {
    TRANSITION = 1,
    NORMALLY_OPEN = 2,
    NORMALLY_CLOSED = 3,
    RELAY_ERROR = 4,
    FAILED = 5
};

enum DOOR_CONTACT_STATE {
    DOOR_OPEN = 1,
    DOOR_CLOSED = 2
};

bool relay_monitor(repeating_timer_t *);
int64_t relay_timeout(alarm_id_t, void *);

/* Initialises the monitor for relay normally open ('NO') and normally
 * closed ('NC') inputs.
 *
 */
bool relay_initialise(enum MODE mode) {
    static repeating_timer_t relay_rt;

    return add_repeating_timer_ms(1, relay_monitor, NULL, &relay_rt);
}

float lpf(bool b, float v) {
    return v += b ? 0.01 * (1.0 - v) : 0.01 * (0.0 - v);
}

bool relay_monitor(repeating_timer_t *rt) {
    static enum RELAY_STATE state = -1;
    static enum DOOR_CONTACT_STATE state2 = -1;

    static float vfailed = 0.5;
    static float vunknown = 0.5;
    static float verror = 0.5;
    static float vopen = 0.5;
    static float vclosed = 0.5;
    static float vdoor = 0.5;
    static bool normally_open = false;
    static bool normally_closed = false;

    enum RELAY_STATE current = 0xff;
    enum DOOR_CONTACT_STATE current2 = 0xff;

    bool no = gpio_get(RELAY_NO) == 0;
    bool nc = gpio_get(RELAY_NC) == 0;
    bool door = gpio_get(DOOR_SENSOR) == 0;

    verror = lpf(no && nc, verror);
    vopen = lpf(no && !nc, vopen);
    vclosed = lpf(!no && nc, vclosed);
    vunknown = lpf(!no && !nc, vunknown);
    vfailed = lpf(verror < 0.9 && vopen < 0.9 && vclosed < 0.9 && vunknown < 0.9, vfailed);
    vdoor = lpf(door, vdoor);

    if (verror > 0.9 && vopen < 0.9 && vclosed < 0.9 && vunknown < 0.9) {
        current = RELAY_ERROR;
    } else if (verror < 0.9 && vopen > 0.9 && vclosed < 0.9 && vunknown < 0.9) {
        current = NORMALLY_OPEN;
    } else if (verror < 0.9 && vopen < 0.9 && vclosed > 0.9 && vunknown < 0.9) {
        current = NORMALLY_CLOSED;
    } else if (verror < 0.9 && vopen < 0.9 && vclosed < 0.9 && vunknown > 0.9) {
        current = UNKNOWN;
    } else if (vfailed > 0.9) {
        current = FAILED;
    }

    if (vdoor > 0.9) {
        current2 = DOOR_CLOSED;
    } else if (vdoor < 0.1) {
        current2 = DOOR_OPEN;
    }

    if (current != 0xff && current != state) {
        uint32_t msg = MSG_RELAY;

        if (current == UNKNOWN) {
            msg |= 0x0000;
        } else if (current == NORMALLY_OPEN) {
            msg |= 0x0001;
        } else if (current == NORMALLY_CLOSED) {
            msg |= 0x0002;
        } else if (current == RELAY_ERROR) {
            msg |= 0x0003;
        } else if (current == FAILED) {
            msg |= 0x0004;
        } else {
            msg |= 0xffff;
        }

        if (!queue_is_full(&queue) && queue_try_add(&queue, &msg)) {
            state = current;
        }
    }

    if (current2 != 0xff && current2 != state2) {
        uint32_t msg = MSG_DOOR;

        if (current2 == UNKNOWN) {
            msg |= 0x0000;
        } else if (current2 == DOOR_OPEN) {
            msg |= 0x0001;
        } else if (current2 == DOOR_CLOSED) {
            msg |= 0x0002;
        } else {
            msg |= 0xffff;
        }

        if (!queue_is_full(&queue) && queue_try_add(&queue, &msg)) {
            state2 = current2;
        }
    }

    return true;
}

/* Handler for a RELAY event.
 *
 */
void relay_event(uint32_t v) {
    if (v == 0x0000) {
        tx("RELAY UNKNOWN");
    } else if (v == 0x0001) {
        tx("RELAY NORMALLY OPEN");
    } else if (v == 0x0002) {
        tx("RELAY NORMALLY CLOSED");
    } else if (v == 0x0003) {
        tx("RELAY ERROR");
    } else if (v == 0x0004) {
        tx("RELAY FAILED");
    } else {
        tx("RELAY ????");
    }
}

/* Sets the DOOR OPEN relay.
 *
 */
void relay_open(uint32_t delay) {
    if (add_alarm_in_ms(delay, relay_timeout, NULL, false) > 0) {
        TPIC_set(DOOR_RELAY, true);
    }
}

/* Clears the DOOR OPEN relay.
 *
 */
void relay_close() {
    TPIC_set(DOOR_RELAY, false);
}

/* Sets/clears the DOOR CONTACT relay.
 *
 */
void relay_door_contact(bool closed) {
    TPIC_set(DOOR_CONTACT, closed);
}

/* Timeout handler. Clears the DOOR OPEN relay.
 *
 */
int64_t relay_timeout(alarm_id_t id, void *data) {
    relay_close();
    return 0;
}

/* Handler for a DOOR event.
 *
 */
void door_event(uint32_t v) {
    if (v == 0x0000) {
        tx("DOOR UNKNOWN");
    } else if (v == 0x0001) {
        tx("DOOR  OPEN");
    } else if (v == 0x0002) {
        tx("DOOR  CLOSED");
    } else {
        tx("DOOR ????");
    }
}
