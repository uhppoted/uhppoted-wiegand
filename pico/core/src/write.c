#include <stdio.h>

#include <WRITE.pio.h>
#include <buzzer.h>
#include <common.h>
#include <logd.h>
#include <write.h>

typedef struct writer {
} writer;

/* Initialises the WRITER PIO.
 *
 */
void write_initialise(enum MODE mode) {
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
    uint32_t word = 0;

    switch (digit) {
    case '0':
        word = 0;
        break;
    case '1':
        word = 1;
        break;
    case '2':
        word = 2;
        break;
    case '3':
        word = 3;
        break;
    case '4':
        word = 4;
        break;
    case '5':
        word = 5;
        break;
    case '6':
        word = 6;
        break;
    case '7':
        word = 7;
        break;
    case '8':
        word = 8;
        break;
    case '9':
        word = 9;
        break;
    case '*':
        word = 10;
        break;
    case '#':
        word = 11;
        break;

    default:
        return false;
    }

    writer_program_put(PIO_WRITER, SM_WRITER, word, 4);

    return true;
}
