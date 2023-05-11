#pragma once

#include <stdbool.h>

#include "wiegand.h"

typedef void (*txrx)(void *, const char *);

extern bool tcpd_initialise(enum MODE);
extern void tcpd_terminate();
extern void tcpd_poll();

extern void execw(char *, txrx, void *);
