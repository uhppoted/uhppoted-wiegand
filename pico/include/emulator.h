#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void emulator_initialise();
extern bool emulator_write(uint32_t facility_code, uint32_t card);
