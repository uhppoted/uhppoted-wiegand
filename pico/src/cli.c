#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"

#include "../include/cli.h"
#include "../include/sys.h"
#include "../include/wiegand.h"

const char ESC = 27;
uint8_t height = 25;

void echo(const char *);
void clearline();

void exec(char *);
void cpr(char *);
void query();
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
    static char buffer[64];
    static int ix = 0;
    // static absolute_time_t t = 0;
    //
    // // ... clear command buffer on idle for 30s
    // absolute_time_t now = get_absolute_time();
    // int64_t dt = absolute_time_diff_us(t, now);
    //
    // if (t == 0 || dt > 30 * 1000 * 1000) {
    //     memset(buffer, 0, sizeof(buffer));
    //     ix = 0;
    // }
    //
    // t = now;

    int N = strlen(received);
    for (int i = 0; i < N; i++) {
        uint8_t ch = received[i];

        // CRLF ?
        if (ch == '\n' || ch == '\r') {
            if (ix > 0) {
                exec(buffer);
            }

            memset(buffer, 0, sizeof(buffer));
            ix = 0;
            continue;
        }

        // VT100 escape code?
        if (buffer[0] == 27 && ch == 'R' && (ix < sizeof(buffer) - 1)) { // VT100 cursor position report
            buffer[ix++] = ch;
            buffer[ix] = 0;

            cpr(&buffer[1]);
            memset(buffer, 0, sizeof(buffer));
            ix = 0;
            continue;
        }

        // backspace?
        if (ch == 8) {
            if (ix > 0) {
                buffer[--ix] = 0;
                echo(buffer);
            }
            continue;
        }

        // Add character to buffer
        if (ix < sizeof(buffer) - 1) {
            buffer[ix++] = ch;
            buffer[ix] = 0;

            // ... echo if normal commnad and not a VT100 code
            if (buffer[0] != 27) {
                echo(buffer);
            }
        }
    }
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

/* Should probably display the commands?
 *
 */
void help() {
}