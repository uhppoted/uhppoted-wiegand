#include <stdio.h>

#include <uart.h>
#include <wiegand.h>

#include "logd.h"

/* Internal state for the logging daemon.
 *
 */
typedef struct LOGD {
} LOGD;

/* Initialises the logging daemon.
 *
 */
bool logd_initialise(enum MODE mode) {
    return true;
}

void logd_terminate() {
}

void logd_log(char *msg) {
    char s[72];

    snprintf(s, sizeof(s), "....   %s", msg);
    tx(s);
}
