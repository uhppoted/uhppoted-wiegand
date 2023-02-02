#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "wiegand.h"

extern void emulator_initialise(enum MODE);
extern bool emulator_write(uint32_t facility_code, uint32_t card);
