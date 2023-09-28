#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "wiegand.h"

extern void flash_read_acl(CARD[], int *, uint32_t[4]);
extern void flash_write_acl(CARD[], int, uint32_t[4]);
