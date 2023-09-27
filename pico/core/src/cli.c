#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <acl.h>
#include <buzzer.h>
#include <cli.h>
#include <logd.h>
#include <sys.h>
#include <TPIC6B595.h>
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

/* Sets the system time.
 *
 */
void cli_set_time(char *cmd, txrx f, void *context) {
    sys_settime(cmd);

    f(context, ">> SET TIME OK");
}

/* Sets the override passcodes.
 *
 */
void set_passcodes(char *cmd, txrx f, void *context) {
    uint32_t passcodes[] = {0,0,0,0};
    int ix = 0;
    char *token = strtok(cmd," ,");
    
    while (token != NULL && ix < 4) {
        printf(">>>>>>> %s\n",token);
        char *end = NULL;
        unsigned long v = strtoul(token, &end, 10);

        if (v > 0 && v < 1000000) {
            passcodes[ix] = v;            
        }

        token = strtok (NULL, " ,");
        ix++;
    }

    if (acl_set_passcodes(passcodes[0],passcodes[1],passcodes[2],passcodes[3])) {
        f(context, "ACL    SET PASSCODES OK");
    } else {
        f(context, "ACL    SET PASSCODES ERROR");
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
