#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"

#include "../include/cli.h"
#include "../include/sys.h"
#include "../include/wiegand.h"

const char ESC = 27;
uint8_t height = 25;

void query();
void cpr(char *);
void help();
void clear_cmd();

/* Clears screen and requests terminal window size
 *
 */
void VT100() {
    char s[24];
    snprintf(s, sizeof(s), "\033[2J\033[999;999H\033[6n\033[H");
    fputs(s, stdout);
    fflush(stdout);
}

/* Saves the cursor position, displays the current command buffer and then restores
 *  the cursor position
 *
 */
void echo(const char *cmd) {
    char s[64];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> %s\0338", height, cmd);
    fputs(s, stdout);
    fflush(stdout);
}

/* Saves the cursor position, clears the command line, redisplays the prompt and then
 * restores the cursor position.
 *
 */
void clear_cmd() {
    char s[24];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> \033[0K\0338", height);
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
        case 27:
            cpr(&cmd[1]);
            break;

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

    clear_cmd();
}

/* Displays the last read card, if any.
 *
 */
void query() {
    char s[64];
    cardf(&last_card, s, sizeof(s));
    puts(s);
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

/* Should probably display the commands?
 *
 */
void help() {
}