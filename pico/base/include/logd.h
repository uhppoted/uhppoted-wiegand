#pragma once

#include <stdbool.h>

#include "wiegand.h"

extern bool logd_initialise(enum MODE);
extern void logd_terminate();
extern void logd_log(const char *);
