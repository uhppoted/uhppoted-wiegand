#pragma once

#include "wiegand.h"

void controller_initialise(enum MODE);
void on_card_rxi(uint32_t);
void on_card_read(uint32_t);
