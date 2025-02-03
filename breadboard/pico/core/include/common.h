#pragma once

#include <stdbool.h>

#include "pico/util/datetime.h"

#include "wiegand.h"

extern void cardf(const card *, char *, int, bool);
extern int timef(const datetime_t, char *, int);
extern int bits(uint32_t);
extern bool is_valid(const datetime_t);
