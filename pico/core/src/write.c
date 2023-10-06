#include <stdio.h>
#include <string.h>

#include <WRITE.pio.h>
#include <buzzer.h>
#include <common.h>
#include <logd.h>
#include <write.h>

typedef struct writer {
} writer;

typedef struct keycode {
    char digit;
    uint32_t code4;
    uint32_t code8;
} keycode;

enum KEYPADMODE {
    BITS4 = 4,
    BITS8 = 8,
};

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

/* Initialises the WRITER PIO.
 *
 */
void write_initialise(enum MODE mode) {
    char s[64];

    snprintf(s, sizeof(s), "KEYPAD %s", KEYPAD);
    logd_log(s);

    if (strncasecmp(KEYPAD, "8-bit", 5) == 0) {
        keypadmode = BITS8;
    }

    uint offset = pio_add_program(PIO_WRITER, &writer_program);

    writer_program_init(PIO_WRITER, SM_WRITER, offset, WRITER_D0, WRITER_D1);
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

    writer_program_put(PIO_WRITER, SM_WRITER, word, 26);
    buzzer_beep(1);

    return true;
}

/* Write keycode command.
 *  Pushes the keycode to the PIO out queue as a 4-bit Wiegand code.
 *
 */
bool write_keycode(char digit) {
    int N = sizeof(KEYCODES) / sizeof(keycode);

    for (int i = 0; i < N; i++) {
        if (KEYCODES[i].digit == digit) {
            if (keypadmode == BITS8) {
                writer_program_put(PIO_WRITER, SM_WRITER, KEYCODES[i].code8, 8);
            } else {
                writer_program_put(PIO_WRITER, SM_WRITER, KEYCODES[i].code4, 4);
            }
            return true;
        }
    }

    return false;
}
