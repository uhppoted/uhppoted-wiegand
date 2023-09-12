#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "wiegand.h"

extern void write_initialise(enum MODE);
extern bool write_card(uint32_t facility_code, uint32_t card);
extern bool write_keycode(char digit);
