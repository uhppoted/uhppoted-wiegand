#pragma once

#include <stdbool.h>

#include "cli.h"
#include "wiegand.h"

typedef void (*tcpd_handler)(void *);

extern bool tcpd_initialise(enum MODE);
extern void tcpd_terminate();
extern void tcpd_poll();
extern void tcpd_cli(void *);
extern void tcpd_log(const char *);

extern void execw(char *, txrx, void *);
