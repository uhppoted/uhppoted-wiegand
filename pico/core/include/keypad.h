#pragma once

#include <stdint.h>

typedef struct keycode {
    char digit;
    uint32_t code4;
    uint32_t code8;
} keycode;

enum KEYPADMODE {
    BITS4 = 4,
    BITS8 = 8,
};

extern enum KEYPADMODE keypadmode;
extern const keycode KEYCODES[];
extern const int KEYCODES_SIZE;
