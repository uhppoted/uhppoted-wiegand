#pragma once

#include <stdint.h>

extern void writer_initialise();
extern void writer_write(uint32_t facility_code, uint32_t card);
