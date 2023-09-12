#include <stdio.h>

#include <logd.h>
#include <write.h>
#include <buzzer.h>
#include <common.h>
#include <WRITE.pio.h>

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
    uint32_t w = (v << 1) | (((even % 2) & 0x00000001) << 25) | ((odd % 2) & 0x00000001);

    writer_program_put(PIO_WRITER, SM_WRITER, w);
    buzzer_beep(1);

    return true;
}

/* Write keycode command.
 *  Pushes the keycode to the PIO out queue as a 4-bit Wiegand code.
 *
 */
bool write_keycode(char digit) {
    char s[64];

    switch (digit) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '*':
        case '#':
            snprintf(s,sizeof(s),">>>>> KEY:%c",digit);
            logd_debug(s);        
            break;

        default:
            return false;
    }

    return true;
}
