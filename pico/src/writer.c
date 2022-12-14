#include "../include/writer.h"
#include "../include/wiegand.h"
#include <WRITE.pio.h>

typedef struct writer {
} writer;

/* Initialises the OUT PIO.
 *
 */
void writer_initialise() {
    uint offset = pio_add_program(PIO_EMULATOR, &writer_program);

    writer_program_init(PIO_EMULATOR, SM_EMULATOR, offset, WRITER_D0, WRITER_D1);
}

/* Write card command.
 *  Reformats the facility code and card number as Wiegand-26 and
 *  pushes it to the PIO out queue.
 *
 */
bool writer_write(uint32_t facility_code, uint32_t card) {
    uint32_t v = ((facility_code & 0x000000ff) << 16) | (card & 0x0000ffff);
    int even = bits(v & 0x00fff000);
    int odd = 1 + bits(v & 0x00000fff);
    uint32_t w = (v << 1) | (((even % 2) & 0x00000001) << 25) | ((odd % 2) & 0x00000001);

    writer_program_put(PIO_EMULATOR, SM_EMULATOR, w);

    return true;
}
