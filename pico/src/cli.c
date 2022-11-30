#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"

#include "../include/cli.h"
#include "../include/sys.h"
#include "../include/wiegand.h"

const char ESC = 27;
const uint8_t WINDOW_HEIGHT = 48;
const uint8_t WINDOW_CMDLINE = WINDOW_HEIGHT + 2;

void query();
void help();
void clear_cmd();

// Sets scrolling area to 0;48 and displays >> at command line
void VT100() {
    char s[24];
    snprintf(s, sizeof(s), "\033[0;%dr\0337\033[%d;0H>> \033[0K\0338", WINDOW_HEIGHT, WINDOW_CMDLINE);
    fputs(s, stdout);
    fflush(stdout);
}

void echo(const char *cmd) {
    char s[64];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> %s\0338", WINDOW_CMDLINE, cmd);
    fputs(s, stdout);
    fflush(stdout);
}

void clear_cmd() {
    char s[24];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> \033[0K\0338", WINDOW_CMDLINE);
    fputs(s, stdout);
    fflush(stdout);
}

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

    clear_cmd();
}

void query() {
    char s[64];
    cardf(&last_card, s, sizeof(s));
    puts(s);
}

void help() {
}