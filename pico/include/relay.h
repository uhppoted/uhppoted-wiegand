#pragma once

#include <stdbool.h>

#include "../include/wiegand.h"

extern bool relay_initialise(enum MODE);
extern void relay_event(uint32_t);