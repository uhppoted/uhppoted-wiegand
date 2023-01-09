#pragma once

#include <stdint.h>

#include "wiegand.h"

extern void buzzer_initialise(enum MODE);
extern void buzzer_beep(uint8_t);
