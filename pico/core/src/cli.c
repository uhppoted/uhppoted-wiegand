#include <stdio.h>
#include <string.h>

#include <TPIC6B595.h>
#include <buzzer.h>
#include <cli.h>
#include <logd.h>
#include <wiegand.h>
#include <write.h>

/* Goes into a tight loop until the watchdog resets the processor.
 *
 */
void reboot(txrx f, void *context) {
    while (true) {
        buzzer_beep(1);

        TPIC_set(RED_LED, false);
        TPIC_set(YELLOW_LED, false);
        TPIC_set(ORANGE_LED, false);
        TPIC_set(GREEN_LED, false);
        sleep_ms(750);

        TPIC_set(RED_LED, true);
        TPIC_set(YELLOW_LED, true);
        TPIC_set(ORANGE_LED, true);
        TPIC_set(GREEN_LED, true);
        sleep_ms(750);
    }
}

/* Keypad emulation.
 *  Sends the keycode code as 4-bit burst mode Wiegand.
 *
 */
void keypad(char *cmd, txrx f, void *context) {
    int N = strlen(cmd);

    if (((mode == WRITER) || (mode == EMULATOR)) && N > 0) {
        char s[64];

        for (int i = 0; i < N; i++) {
            char key = cmd[i];

            write_keycode(key);
        }

        snprintf(s, sizeof(s), "KEYPAD OK");
        f(context, s);
        logd_log(s);
    }
}
