#pragma once

#include <stdbool.h>

#include "../include/wiegand.h"

extern bool relay_initialise(enum MODE);
extern void relay_event(uint32_t);
extern void relay_open(uint32_t);
extern void relay_door_contact(bool);
extern void door_event(uint32_t);