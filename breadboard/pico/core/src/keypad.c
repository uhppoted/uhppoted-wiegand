#include <keypad.h>

enum KEYPADMODE keypadmode = BITS4;

const keycode KEYCODES[] = {
    {'0', 0, 0x00f0},
    {'1', 1, 0x00e1},
    {'2', 2, 0x00d2},
    {'3', 3, 0x00c3},
    {'4', 4, 0x00b4},
    {'5', 5, 0x00a5},
    {'6', 6, 0x0096},
    {'7', 7, 0x0087},
    {'8', 8, 0x0078},
    {'9', 9, 0x0069},
    {'*', 10, 0x005a},
    {'#', 11, 0x004b},
};

const int KEYCODES_SIZE = sizeof(KEYCODES) / sizeof(keycode);
