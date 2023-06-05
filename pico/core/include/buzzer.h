#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "wiegand.h"

extern bool buzzer_initialise(enum MODE);
extern void buzzer_beep(uint8_t);
