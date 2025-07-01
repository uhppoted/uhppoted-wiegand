#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void wiegand_init();
extern bool write_card(uint32_t, uint32_t);
extern bool write_keycode(char);
