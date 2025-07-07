#include <string.h>

#include "wiegand.pio.h"

#include <M5.h>
#include <SK6812.h>
#include <log.h>
#include <wiegand.h>

#define LOGTAG "WIEGAND"

extern const constants HW;

typedef struct keycode {
    char digit;
    uint32_t code4;
    uint32_t code8;
} keycode;

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

struct {
    PIO pio;
    int sm;
    uint D0;
    uint D1;
    int keypad;
} wiegand = {
    .keypad = 4,
};

int bits(uint32_t);

/* Initialises the Wiegand interface PIO.
 *
 */
void wiegand_init() {
    wiegand.pio = HW.wiegand.pio;
    wiegand.sm = HW.wiegand.sm;
    wiegand.D0 = HW.wiegand.DO;
    wiegand.D1 = HW.wiegand.DI;

    if (strncasecmp(KEYPAD, "8-bit", 5) == 0) {
        wiegand.keypad = 8;
    }

    pio_sm_claim(wiegand.pio, wiegand.sm);
    uint offset = pio_add_program(wiegand.pio, &wiegand_program);

    wiegand_program_init(wiegand.pio, wiegand.sm, offset, wiegand.D0, wiegand.D1);

    infof(LOGTAG, "wiegand initialised (%s keypad)", KEYPAD);
}

/* Write card command.
 *  Reformats the facility code and card number as Wiegand-26 and
 *  pushes it to the PIO out queue.
 *
 */
bool write_card(uint32_t facility_code, uint32_t card) {
    uint32_t v = ((facility_code & 0x000000ff) << 16) | (card & 0x0000ffff);
    int even = bits(v & 0x00fff000);
    int odd = 1 + bits(v & 0x00000fff);
    uint32_t word = (v << 1) | (((even % 2) & 0x00000001) << 25) | ((odd % 2) & 0x00000001);

    PIO pio = wiegand.pio;
    int sm = wiegand.sm;

    wiegand_program_put(pio, sm, word, 26);
    SK6812_blink(136 / 2, 6 / 2, 106 / 2, 1); // french violet

    infof(LOGTAG, "card %u%05u", facility_code, card);

    return true;
}

/* Write keycode command.
 *  Pushes the keycode to the PIO out queue as a 4-bit Wiegand code.
 *
 */
bool write_keycode(char digit) {
    PIO pio = wiegand.pio;
    int sm = wiegand.sm;
    uint mode = wiegand.keypad;

    for (int i = 0; i < KEYCODES_SIZE; i++) {
        if (KEYCODES[i].digit == digit) {
            if (mode == 8) {
                wiegand_program_put(pio, sm, KEYCODES[i].code8, 8);
                SK6812_blink(255 / 5, 191 / 5, 0 / 5, 1); // amber
            } else {
                wiegand_program_put(pio, sm, KEYCODES[i].code4, 4);
                SK6812_blink(255 / 5, 191 / 5, 0 / 5, 1); // amber
            }

            infof(LOGTAG, "code %c", digit);

            return true;
        }
    }

    return false;
}

/* Sets the keypad mode to either 4 or 8-bit..
 *
 */
bool keypad_mode(uint8_t mode) {
    if (mode == 4) {
        infof(LOGTAG, "keycode - set 4 bit mode");
        wiegand.keypad = 4;
        return true;
    }

    if (mode == 8) {
        infof(LOGTAG, "keycode - set 8 bit mode");
        wiegand.keypad = 8;
        return true;
    }

    return false;
}

// Ref. https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
// Ref. https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
int bits(uint32_t v) {
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0f0f0f0f;

    return (v * 0x01010101) >> 24;
}
