#pragma once

#include <stdbool.h>

#include "wiegand.h"

extern bool tcpd_initialise(enum MODE);
extern void tcpd_terminate();
extern void tcpd_poll();
