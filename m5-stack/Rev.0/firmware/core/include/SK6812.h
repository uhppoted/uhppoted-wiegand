#pragma once

#include <stdint.h>

extern void SK6812_init();
extern bool SK6812_start();
extern void SK6812_set(uint8_t red, uint8_t green, uint8_t blue);
extern void SK6812_blink(uint8_t red, uint8_t green, uint8_t blue, uint8_t count);