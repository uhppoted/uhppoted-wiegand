#pragma once

#include "wiegand.h"

void read_initialise(enum MODE);
void on_card_read(uint32_t);
void on_keypad_digit(uint32_t);
