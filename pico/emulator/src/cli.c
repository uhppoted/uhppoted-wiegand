#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/rtc.h"

#include "../include/TPIC6B595.h"
#include "../include/buzzer.h"
#include "../include/cli.h"
#include "../include/emulator.h"
#include "../include/led.h"
#include "../include/relays.h"
#include "../include/sys.h"
#include "../include/write.h"

typedef struct CLI {
    int ix;
    int32_t timer;
    char buffer[64];
} CLI;

typedef void (*handler)(uint32_t, uint32_t);

const char ESC = 27;
uint8_t height = 25;
const uint32_t CLI_TIMEOUT = 10000;

int64_t cli_timeout(alarm_id_t, void *);
void echo(const char *);
void clearline();

void exec(char *);
void cpr(char *);
void query();
void reboot();
void help();

void on_card_command(char *cmd, handler fn);
void on_door_sensor(char *cmd);
void on_pushbutton(char *cmd);
void write(uint32_t, uint32_t);

/* Clears the screen
 *
 */
void clear_screen() {
    char s[24];
    snprintf(s, sizeof(s), "\033[2J");
    fputs(s, stdout);
    fflush(stdout);
}

/* Requests terminal window size - the scroll area is set from the response.
 *
 */
void set_scroll_area() {
    char s[24];
    snprintf(s, sizeof(s), "\0337\033[999;999H\033[6n\0338");
    fputs(s, stdout);
    fflush(stdout);
}

/* Saves the cursor position, clears the command line, redisplays the prompt and then
 * restores the cursor position.
 *
 */
void clearline() {
    char s[24];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> \033[0K\0338", height);
    fputs(s, stdout);
    fflush(stdout);
}

/** Processes received UART characters.
 *
 */
void rx(char *received) {
    static CLI cli = {
        .ix = 0,
        .timer = -1,
        .buffer = {0},
    };

    if (cli.timer > 0) {
        cancel_alarm(cli.timer);
        cli.timer = 0;
    }

    int N = strlen(received);
    for (int i = 0; i < N; i++) {
        uint8_t ch = received[i];

        // CRLF ?
        if (ch == '\n' || ch == '\r') {
            if (cli.ix > 0) {
                exec(cli.buffer);
            }

            memset(cli.buffer, 0, sizeof(cli.buffer));
            cli.ix = 0;
            continue;
        }

        // VT100 escape code?
        if (cli.buffer[0] == 27 && ch == 'R' && (cli.ix < sizeof(cli.buffer) - 1)) { // VT100 cursor position report
            cli.buffer[cli.ix++] = ch;
            cli.buffer[cli.ix] = 0;

            cpr(&cli.buffer[1]);
            memset(cli.buffer, 0, sizeof(cli.buffer));
            cli.ix = 0;
            continue;
        }

        // backspace?
        if (ch == 8) {
            if (cli.ix > 0) {
                cli.buffer[--cli.ix] = 0;
                echo(cli.buffer);
            }

            if (cli.ix > 0) {
                cli.timer = add_alarm_in_ms(CLI_TIMEOUT, cli_timeout, (CLI *)&cli, true);
            }

            continue;
        }

        // Add character to buffer
        if (cli.ix < sizeof(cli.buffer) - 1) {
            cli.buffer[cli.ix++] = ch;
            cli.buffer[cli.ix] = 0;

            // ... echo if normal commnad and not a VT100 code
            if (cli.buffer[0] != 27) {
                echo(cli.buffer);
            }

            cli.timer = add_alarm_in_ms(CLI_TIMEOUT, cli_timeout, (CLI *)&cli, true);
            continue;
        }
    }
}

/* Adds a message to the queue.
 *
 */
void tx(char *message) {
    static int size = 128;
    char *s;

    if ((s = calloc(size, 1)) != NULL) {
        datetime_t now;
        int N;
        uint32_t msg;

        rtc_get_datetime(&now);

        N = timef(now, s, size);

        snprintf(&s[N], size - N, "  %s", message);

        msg = MSG_TX | ((uint32_t)s & 0x0fffffff); // SRAM_BASE is 0x20000000
        if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
            free(message);
        }
    }
}

/* Timeout handler. Clears the current command and command line.
 *
 */
int64_t cli_timeout(alarm_id_t id, void *data) {
    CLI *cli = (CLI *)data;
    memset(cli->buffer, 0, sizeof(cli->buffer));
    cli->ix = 0;
    cli->timer = 0;

    clearline();

    return 0;
}

/* Saves the cursor position, displays the current command buffer and then restores
 *  the cursor position
 *
 */
void echo(const char *cmd) {
    char s[64];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> %s\033[0K\0338", height, cmd);
    fputs(s, stdout);
    fflush(stdout);
}

/* Executes a command.
 *
 */
void exec(char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        switch (cmd[0]) {
        case 'x':
        case 'X':
            led_blink(5);
            break;

        case 'q':
        case 'Q':
            query();
            break;

        case 't':
        case 'T':
            sys_settime(&cmd[1]);
            break;

        case 'w':
        case 'W':
            on_card_command(&cmd[1], write);
            break;

        case 'r':
        case 'R':
            on_door_sensor(cmd);
            break;

        case 'p':
        case 'P':
            on_pushbutton(cmd);
            break;

        case 'z':
        case 'Z':
            reboot();
            break;

        case '?':
            help();
            break;
        }
    }

    clearline();
}

/* Cursor position report.
 *  Sets the scrolling window with space at the bottom for the command echo.
 *
 */
void cpr(char *cmd) {
    int rows;
    int cols;
    int rc = sscanf(cmd, "[%d;%dR", &rows, &cols);

    if (rc > 0) {
        height = rows - 1;

        char s[24];
        snprintf(s, sizeof(s), "\0337\033[0;%dr\0338", height - 2);
        fputs(s, stdout);
        fflush(stdout);
    }
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
    tx("T         Set date/time (YYYY-MM-DD HH:mm:ss)");
    tx("Wnnnnnn   Write card to Wiegand-26 interface");
    tx("Q         Display last card read/write");
    tx("X         Blinks reader LED 5 times");
    tx("LOCK      Locks door");
    tx("CO        Opens door contact relay");
    tx("CC        Closes door contact relay");
    tx("PP        Press pushbutton");
    tx("PR        Release pushbutton");
    tx("Z         reboot");
    tx("?         Display list of commands");
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
void on_door_sensor(char *cmd) {
    if (strncasecmp(cmd, "ro", 2) == 0) {
        relay_door_contact(false);
    }

    if (strncasecmp(cmd, "rc", 2) == 0) {
        relay_door_contact(true);
    }
}

/* Pushbutton emulation command handler.
 *  Opens/closes the pushbutton emulation relay (in reader mode only).
 *
 */
void on_pushbutton(char *cmd) {
    if (strncasecmp(cmd, "pp", 2) == 0) {
        relay_pushbutton(true);
    }

    if (strncasecmp(cmd, "pr", 2) == 0) {
        relay_pushbutton(false);
    }
}
