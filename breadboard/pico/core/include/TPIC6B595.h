#pragma once

#include "wiegand.h"

enum TPICIO {
    RED_LED = 1,
    ORANGE_LED,
    YELLOW_LED,
    GREEN_LED,
    DOOR_UNLOCK,
    DOOR_CONTACT,
    PUSHBUTTON
};

extern void TPIC_initialise(enum MODE);
extern void TPIC_set(enum TPICIO, bool);
