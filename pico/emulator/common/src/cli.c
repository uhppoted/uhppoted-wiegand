#include <stdio.h>
#include <string.h>

#include <TPIC6B595.h>
#include <buzzer.h>
#include <common.h>
#include <logd.h>
#include <relays.h>
#include <sys.h>
#include <tcpd.h>
#include <uart.h>
#include <write.h>

typedef void (*handler)(uint32_t, uint32_t, txrx, void *);

void help(txrx, void *);
void cli_set_time(char *, txrx, void *);
void query(txrx, void *);
void reboot();

void on_card_command(char *cmd, handler fn, txrx, void *);

void on_door_open(txrx, void *);
void on_door_close(txrx, void *);
void on_press_button(txrx, void *);
void on_release_button(txrx, void *);
void write(uint32_t, uint32_t, txrx, void *);

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
            if (strncasecmp(cmd, "query", 5) == 0) {
                query(f, context);
            } else if (strncasecmp(cmd, "open", 4) == 0) {
                on_door_open(f, context);
            } else if (strncasecmp(cmd, "close", 5) == 0) {
                on_door_close(f, context);
            } else if (strncasecmp(cmd, "press", 5) == 0) {
                on_press_button(f, context);
            } else if (strncasecmp(cmd, "release", 7) == 0) {
                on_release_button(f, context);
            } else if (strncasecmp(cmd, "reboot", 6) == 0) {
                reboot(f, context);
            } else if ((cmd[0] == 't') || (cmd[0] == 'T')) {
                cli_set_time(&cmd[1], f, context);
            } else if ((cmd[0] == 'w') || (cmd[0] == 'W')) {
                on_card_command(&cmd[1], write, f, context);
            } else {
                help(f, context);
            }
        }
    }
}

/* Sets the system time.
 *
 */
void cli_set_time(char *cmd, txrx f, void *context) {
    sys_settime(cmd);

    f(context, ">> SET TIME OK");
}

/* Displays the last read/write card, if any.
 *
 */
void query(txrx f, void *context) {
    char s[64];

    cardf(&last_card, s, sizeof(s));
    f(context, s);
}

/* Write card command.
 *  Extract the facility code and card number pushes it to the emulator queue.
 *
 */
void write(uint32_t facility_code, uint32_t card, txrx f, void *context) {
    if ((mode == WRITER) || (mode == EMULATOR)) {
        write_card(facility_code, card);
    }

    f(context, "CARD   WRITE OK");
    logd_log("CARD   WRITE OK");
}

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

/* Displays a list of the supported commands.
 *
 */
/* Displays a list of the supported commands.
 *
 */
void help(txrx f, void *context) {
    f(context, "-----");
    f(context, "Commands:");
    f(context, "TYYYY-MM-DD HH:mm:ss  Set date/time");
    f(context, "Wnnnnnn               Write card to Wiegand-26 interface");
    f(context, "QUERY                 Display last card read/write");
    f(context, "OPEN                  Opens door contact relay");
    f(context, "CLOSE                 Closes door contact relay");
    f(context, "PRESS                 Press pushbutton");
    f(context, "RELEASE               Release pushbutton");
    f(context, "CLS                   Resets the terminal");
    f(context, "REBOOT                Reboot");
    f(context, "?                     Display list of commands");
    f(context, "-----");
}

/* Card command handler.
 *  Extract the facility code and card number and invokes the handler function.
 *
 */
void on_card_command(char *cmd, handler fn, txrx f, void *context) {
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

    fn(facility_code, card, f, context);
}

/* Door contact emulation command handler.
 *  Opens/closes the door contact emulation relay (in reader mode only).
 *
 */
void on_door_open(txrx f, void *context) {
    relay_door_contact(false);
    f(context, ">> OPENED");
}

void on_door_close(txrx f, void *context) {
    relay_door_contact(true);
    f(context, ">> CLOSED");
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
