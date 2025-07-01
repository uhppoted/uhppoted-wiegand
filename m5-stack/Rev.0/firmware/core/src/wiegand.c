#include <string.h>

#include "wiegand.pio.h"

#include <M5.h>
#include <log.h>
#include <wiegand.h>

#define LOGTAG "WIEGAND"

extern const constants IO;

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
    int keypad;
} wiegand = {
    .pio = pio0,
    .sm = 0,
};

int bits(uint32_t);

/* Initialises the Wiegand interface PIO.
 *
 */
void wiegand_init() {
    if (strncasecmp(KEYPAD, "8-bit", 5) == 0) {
        wiegand.keypad = 8;
    } else {
        wiegand.keypad = 4;
    }

    PIO pio = wiegand.pio;
    int sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &wiegand_program);

    wiegand_program_init(pio, sm, offset, IO.DO, IO.DI);
    wiegand.sm = sm;

    infof(LOGTAG, "wiegand initialised keypad:%s, sm:%d)", KEYPAD, sm);
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
    // buzzer_beep(1);

    return true;
}

/* Write keycode command.
 *  Pushes the keycode to the PIO out queue as a 4-bit Wiegand code.
 *
 */
bool write_keycode(char digit) {
    PIO pio = wiegand.pio;
    int sm = wiegand.sm;
    int mode = wiegand.keypad;

    for (int i = 0; i < KEYCODES_SIZE; i++) {
        if (KEYCODES[i].digit == digit) {
            if (mode == 8) {
                wiegand_program_put(pio, sm, KEYCODES[i].code8, 8);
                // buzzer_beep(1);
            } else {
                wiegand_program_put(pio, sm, KEYCODES[i].code4, 4);
                // buzzer_beep(1);
            }

            return true;
        }
    }

    // buzzer_beep(1);
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
