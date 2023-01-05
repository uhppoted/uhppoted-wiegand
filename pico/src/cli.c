#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/rtc.h"

#include "../include/acl.h"
#include "../include/cli.h"
#include "../include/sys.h"
#include "../include/wiegand.h"
#include "../include/writer.h"

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
void help();

void on_card_command(char *cmd, handler fn);
void write(uint32_t, uint32_t);
void grant(uint32_t, uint32_t);
void revoke(uint32_t, uint32_t);

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
    uint32_t msg = MSG_TX | ((uint32_t)message & 0x0fffffff); // SRAM_BASE is 0x20000000
    if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
        free(message);
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
        case 'o':
        case 'O':
            if (mode == READER) {
                gpio_put(READER_LED, 0);
            }
            break;

        case 'x':
        case 'X':
            if (mode == READER) {
                gpio_put(READER_LED, 1);
            }
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

        case 'g':
        case 'G':
            on_card_command(&cmd[1], grant);
            break;

        case 'r':
        case 'R':
            on_card_command(&cmd[1], revoke);
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
    if (mode == EMULATOR) {
        writer_write(facility_code, card);
    }
}

/* Adds a card number to the ACL.
 *
 */
void grant(uint32_t facility_code, uint32_t card) {
    char s[64];
    char c[16];
    datetime_t now;

    rtc_get_datetime(&now);
    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    int N = timef(&now, s, sizeof(s));

    if (acl_grant(facility_code, card)) {
        snprintf(&s[N], sizeof(s) - N, "  CARD %-8s %s", c, "GRANTED");
    } else {
        snprintf(&s[N], sizeof(s) - N, "  CARD %-8s %s", c, "ERROR");
    }

    puts(s);
}

/* Removes a card number from the ACL.
 *
 */
void revoke(uint32_t facility_code, uint32_t card) {
    char s[64];
    char c[16];
    datetime_t now;

    rtc_get_datetime(&now);
    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    int N = timef(&now, s, sizeof(s));

    if (acl_revoke(facility_code, card)) {
        snprintf(&s[N], sizeof(s) - N, "  CARD %-8s %s", c, "REVOKED");
    } else {
        snprintf(&s[N], sizeof(s) - N, "  CARD %-8s %s", c, "ERROR");
    }

    puts(s);
}

/* Should probably display the commands?
 *
 */
void help() {
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
