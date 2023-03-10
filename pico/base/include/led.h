#pragma once

#include <stdint.h>

#include "wiegand.h"

enum LED {
    SYS_LED = 1,
    GOOD_CARD,
    BAD_CARD,
    CARD_TIMEOUT,
};

extern void led_initialise(enum MODE);
extern void led_event(uint32_t);
extern void led_blink(uint8_t);

extern void blink(enum LED);
