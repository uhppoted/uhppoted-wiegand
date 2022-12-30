#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void writer_initialise();
extern bool writer_write(uint32_t facility_code, uint32_t card);
