#include <stdio.h>
#include <string.h>

#include <WRITE.pio.h>
#include <buzzer.h>
#include <common.h>
#include <keypad.h>
#include <logd.h>
#include <write.h>

typedef struct writer {
} writer;

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
    for (int i = 0; i < KEYCODES_SIZE; i++) {
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
