#include <stdio.h>
#include <stdlib.h>

#include "../include/buzzer.h"
#include "../include/wiegand.h"
#include <BUZZER.pio.h>

const uint32_t BUZZER_DELAY = 100;

/* struct for buzzer state
*
 */
struct {
    bool initialised;
} BUZZER_STATE = {
    .initialised = false,
};


/* struct for communicating between led_blinks API function and blinki
 * alarm handler. Allocated and initialiesd in led_blinks and free'd
 * in blinki.
 */
typedef struct beeps {
    uint32_t count;
} beeps;

/* Alarm handler for 'end of buzzer start delay'. Queues up 'count'
 * beeps the BUZZER FIFO..
 *
 */
int64_t buzzeri(alarm_id_t id, void *data) {
    uint32_t count = ((beeps *)data)->count;

    free(data);

    while (count-- > 0) {
        buzzer_program_beep(PIO_BUZZER, SM_BUZZER);
    }

    return 0;
}

/* Initialises the PIO for the buzzer. The PIO is initialised
 * in both 'reader' and 'writer' modes.
 *
 */
void buzzer_initialise(enum MODE mode) {
    uint offset = pio_add_program(PIO_BUZZER, &buzzer_program);

    buzzer_program_init(PIO_BUZZER, SM_BUZZER, offset, BUZZER);

    BUZZER_STATE.initialised = true;
}

/* Handler for a buzzer beep.
 *
 * NOTE: because the PIO FIFO is only 8 deep, the maximum number of beeps
 *       is 8. Which seems like enough.
 */
void buzzer_beep(uint8_t count) {
    if (BUZZER_STATE.initialised) {
    struct beeps *b = malloc(sizeof(struct beeps));

    b->count = count > 8 ? 8 : count;

    add_alarm_in_ms(BUZZER_DELAY, buzzeri, (void *)b, true);        
    }
}
