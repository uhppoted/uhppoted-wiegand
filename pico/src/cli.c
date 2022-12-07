#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"

#include "../include/cli.h"
#include "../include/sys.h"
#include "../include/wiegand.h"

typedef struct CLI {
    int ix;
    int32_t timer;
    char buffer[64];
} CLI;

const char ESC = 27;
uint8_t height = 25;
const uint32_t CLI_TIMEOUT = 10000;

int64_t cli_timeout(alarm_id_t, void *);
void echo(const char *);
void clearline();

void exec(char *);
void cpr(char *);
void query();
void write(char *);
void help();

/* Clears screen and requests terminal window size
 *
 */
void VT100() {
    char s[24];
    snprintf(s, sizeof(s), "\033[2J\033[999;999H\033[6n\033[H");
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

    cancel_alarm(cli.timer);

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

/* Timeout handler. Clears the current command and command line.
 *
 */
int64_t cli_timeout(alarm_id_t id, void *data) {
    cancel_alarm(id);

    CLI *cli = (CLI *)data;
    memset(cli->buffer, 0, sizeof(cli->buffer));
    cli->ix = 0;

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
            gpio_put(READER_LED, 0);
            break;

        case 'x':
        case 'X':
            gpio_put(READER_LED, 1);
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
            write(&cmd[1]);
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
        snprintf(s, sizeof(s), "\033[0;%dr", height - 2);
        fputs(s, stdout);
        fflush(stdout);
    }
}

/* Displays the last read card, if any.
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
void write(char *cmd) {
    uint32_t facility_code = 0;
    uint32_t card = 0;
    int rc = sscanf(cmd, "%02u%06u", &facility_code, &card);

    if (rc > 0) {
        uint32_t v = ((facility_code & 0x000000ff) << 16) | (card & 0x0000ffff);
        int even = bits(v & 0x00fff000);
        int odd = bits(v & 0x00000fff);
        uint32_t w = (v << 1) | (((even % 2) & 0x00000001) << 25) | (((odd + 1) % 2) & 0x00000001);

        char s[64];
        snprintf(s, sizeof(s), "W %02u%06u  %06x E:%d  O:%d  %08x", facility_code, card, v, even, odd, w);
        puts(s);
    }
}

/* Should probably display the commands?
 *
 */
void help() {
}