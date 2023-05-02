#include <stdio.h>
#include <string.h>

#include "TPIC6B595.h"
#include "buzzer.h"
#include "common.h"
#include "relays.h"
#include "sys.h"
#include "uart.h"
#include "write.h"

#include "../include/cli.h"
#include "../include/emulator.h"
#include "./tcpd/tcpd.h"

typedef void (*handler)(uint32_t, uint32_t);

void query();
void reboot();
void help();

void on_card_command(char *cmd, handler fn);
void on_door_open();
void on_door_close();
void on_press_button();
void on_release_button();
void write(uint32_t, uint32_t);

/* Executes a command.
 *
 */
void exec(char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        if (strncasecmp(cmd, "query", 5) == 0) {
            query();
        } else if (strncasecmp(cmd, "open", 4) == 0) {
            on_door_open();
        } else if (strncasecmp(cmd, "close", 5) == 0) {
            on_door_close();
        } else if (strncasecmp(cmd, "press", 5) == 0) {
            on_press_button();
        } else if (strncasecmp(cmd, "release", 7) == 0) {
            on_release_button();
        } else if (strncasecmp(cmd, "reboot", 6) == 0) {
            reboot();
        } else if (strncasecmp(cmd, "cls", 5) == 0) {
            cls();
            return; // don't invoke clearline because that puts a >> prompt in the wrong place
        } else if ((cmd[0] == 't') || (cmd[0] == 'T')) {
            sys_settime(&cmd[1]);
        } else if ((cmd[0] == 'w') || (cmd[0] == 'W')) {
            on_card_command(&cmd[1], write);
        } else {
            help();
        }
    }

    clearline();
}

/* Displays the last read/write card, if any.
 *
 */
void query() {
    char s[64];
    cardf(&last_card, s, sizeof(s));
    puts(s);
}

/* Write card command.
 *  Extract the facility code and card number pushes it to the emulator queue.
 *
 */
void write(uint32_t facility_code, uint32_t card) {
    if ((mode == WRITER) || (mode == EMULATOR)) {
        write_card(facility_code, card);
    }
}

/* Goes into a tight loop until the watchdog resets the processor.
 *
 */
void reboot() {
    tcpd_terminate();

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
void help() {
    static uint8_t x = 0x00;

    TPIC_set(RED_LED, (x & 0x01) == 0x01);
    TPIC_set(ORANGE_LED, (x & 0x02) == 0x02);
    TPIC_set(YELLOW_LED, (x & 0x04) == 0x04);
    TPIC_set(GREEN_LED, (x & 0x08) == 0x08);

    x++;

    tx("-----");
    tx("Commands:");
    tx("TYYYY-MM-DD HH:mm:ss  Set date/time");
    tx("Wnnnnnn               Write card to Wiegand-26 interface");
    tx("QUERY                 Display last card read/write");
    tx("OPEN                  Opens door contact relay");
    tx("CLOSE                 Closes door contact relay");
    tx("PRESS                 Press pushbutton");
    tx("RELEASE               Release pushbutton");
    tx("CLS                   Resets the terminal");
    tx("REBOOT                Reboot");
    tx("?                     Display list of commands");
    tx("-----");
}

/* Card command handler.
 *  Extract the facility code and card number and invokes the handler function.
 *
 */
void on_card_command(char *cmd, handler fn) {
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

    fn(facility_code, card);
}

/* Door contact emulation command handler.
 *  Opens/closes the door contact emulation relay (in reader mode only).
 *
 */
void on_door_open() {
    relay_door_contact(false);
}

void on_door_close(char *cmd) {
    relay_door_contact(true);
}

/* Pushbutton emulation command handler.
 *  Opens/closes the pushbutton emulation relay (in reader mode only).
 *
 */
void on_press_button() {
    relay_pushbutton(true);
}

void on_release_button() {
    relay_pushbutton(false);
}
