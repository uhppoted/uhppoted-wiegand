#include <stdio.h>
#include <string.h>

#include <TPIC6B595.h>
#include <buzzer.h>
#include <cli.h>
#include <common.h>
#include <logd.h>
#include <relays.h>
#include <sys.h>
#include <tcpd.h>
#include <uart.h>
#include <write.h>

typedef void (*handler)(uint32_t, uint32_t, txrx, void *);

void help(txrx, void *);
void query(txrx, void *);
void swipe(char *cmd, txrx, void *);
void swipex(char *cmd, txrx, void *);

void on_press_button(txrx, void *);
void on_release_button(txrx, void *);

void serial(void *context, const char *msg) {
    puts(msg);
}

/* Executes a command from the TTY terminal.
 *
 */
void exec(char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        if (strncasecmp(cmd, "cls", 3) == 0) {
            cls();
            return; // don't invoke clearline because that puts a >> prompt in the wrong place
        } else {
            execw(cmd, serial, NULL);
        }
    }

    clearline();
}

void execw(char *cmd, txrx f, void *context) {
    if (f != NULL) {
        int N = strlen(cmd);

        if (N > 0) {
            if (strncasecmp(cmd, "reboot", 6) == 0) {
                cli_reboot(f, context);
            } else if (strncasecmp(cmd, "time ", 5) == 0) {
                cli_set_time(&cmd[5], f, context);
            } else if (strncasecmp(cmd, "blink", 5) == 0) {
                cli_blink(f, context);
            } else if (strncasecmp(cmd, "open", 4) == 0) {
                cli_on_door_open(f, context);
            } else if (strncasecmp(cmd, "close", 5) == 0) {
                cli_on_door_close(f, context);
            } else if (strncasecmp(cmd, "press", 5) == 0) {
                on_press_button(f, context);
            } else if (strncasecmp(cmd, "release", 7) == 0) {
                on_release_button(f, context);
            } else if (strncasecmp(cmd, "card ", 5) == 0) {
                swipe(&cmd[5], f, context);
            } else if (strncasecmp(cmd, "card+ ", 6) == 0) {
                swipex(&cmd[6], f, context);
            } else if (strncasecmp(cmd, "code ", 5) == 0) {
                keypad(&cmd[1], f, context);
            } else if (strncasecmp(cmd, "query", 5) == 0) {
                query(f, context);
            } else {
                help(f, context);
            }
        }
    }
}

/* Displays the last read/write card, if any.
 *
 */
void query(txrx f, void *context) {
    char s[64];

    cardf(&last_card, s, sizeof(s), true);
    f(context, s);
}

/* Displays a list of the supported commands.
 *
 */
void help(txrx f, void *context) {
    f(context, "-----");
    f(context, "Commands:");
    f(context, "TIME yyyy-mm-dd hh:mm:ss  Set date/time");
    f(context, "CARD nnnnnn               Write card to Wiegand-26 interface");
    f(context, "CARD+ nnnnnn dddddd       Write card+keycode to Wiegand-26 interface");
    f(context, "CODE dddddd               Enter keypad digits");
    f(context, "QUERY                     Display last card read/write");
    f(context, "OPEN                      Opens door contact relay");
    f(context, "CLOSE                     Closes door contact relay");
    f(context, "PRESS                     Press pushbutton");
    f(context, "RELEASE                   Release pushbutton");
    f(context, "CLS                       Resets the terminal");
    f(context, "REBOOT                    Reboot");
    f(context, "?                         Display list of commands");
    f(context, "-----");
}

/* Card swipe emulation.
 *  Extract the facility code and card number and invokes the handler function.
 *
 */
void swipe(char *cmd, txrx f, void *context) {
    uint32_t facility_code = FACILITY_CODE;
    uint32_t card = 0;
    int N = strlen(cmd);
    int rc;

    if (N < 5) {
        if ((rc = sscanf(cmd, "%0u", &card)) < 1) {
            return;
        }
    } else {
        if ((rc = sscanf(&cmd[N - 5], "%05u", &card)) < 1) {
            return;
        }

        if (N == 6 && ((rc = sscanf(cmd, "%01u", &facility_code)) < 1)) {
            return;
        } else if (N == 7 && ((rc = sscanf(cmd, "%02u", &facility_code)) < 1)) {
            return;
        } else if (N == 8 && ((rc = sscanf(cmd, "%03u", &facility_code)) < 1)) {
            return;
        } else if (N > 8) {
            return;
        }
    }

    if ((mode == WRITER) || (mode == EMULATOR)) {
        write_card(facility_code, card);
    }

    f(context, "CARD   WRITE OK");
    logd_log("CARD   WRITE OK");
}

/* Card swipe + keycode emulation.
 *  Extract the facility code, card number keycode and invokes the handler function.
 *
 */
void swipex(char *cmd, txrx f, void *context) {
    // uint32_t facility_code = FACILITY_CODE;
    // uint32_t card = 0;
    // int N = strlen(cmd);
    // int rc;

    // if (N < 5) {
    //     if ((rc = sscanf(cmd, "%0u", &card)) < 1) {
    //         return;
    //     }
    // } else {
    //     if ((rc = sscanf(&cmd[N - 5], "%05u", &card)) < 1) {
    //         return;
    //     }

    //     if (N == 6 && ((rc = sscanf(cmd, "%01u", &facility_code)) < 1)) {
    //         return;
    //     } else if (N == 7 && ((rc = sscanf(cmd, "%02u", &facility_code)) < 1)) {
    //         return;
    //     } else if (N == 8 && ((rc = sscanf(cmd, "%03u", &facility_code)) < 1)) {
    //         return;
    //     } else if (N > 8) {
    //         return;
    //     }
    // }

    if ((mode == WRITER) || (mode == EMULATOR)) {
        write_card(100, 58401);
        keypad("7531#", f, context);
    }

    f(context, "CARD+  WRITE OK");
    logd_log("CARD+  WRITE OK");
}

/* Pushbutton emulation command handler.
 *  Opens/closes the pushbutton emulation relay (in reader mode only).
 *
 */
void on_press_button(txrx f, void *context) {
    relay_pushbutton(true);
    f(context, ">> PRESSED");
}

void on_release_button(txrx f, void *context) {
    relay_pushbutton(false);
    f(context, ">> RELEASED");
}
