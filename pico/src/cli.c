#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"

#include "../include/cli.h"
#include "../include/sys.h"
#include "../include/wiegand.h"

void query();
void help();

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
}

void query() {
    char s[64];
    cardf(&last_card, s, sizeof(s));
    puts(s);
}

void help() {
}