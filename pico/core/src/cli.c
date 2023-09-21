#include <stdio.h>
#include <string.h>

#include <cli.h>
#include <logd.h>
#include <wiegand.h>
#include <write.h>

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
