#pragma once

#include <stdint.h>

#include "wiegand.h"

extern void led_initialise(enum MODE);
extern void led_event(uint32_t);
extern void led_blink(uint8_t);
