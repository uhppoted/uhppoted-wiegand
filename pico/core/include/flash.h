#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "wiegand.h"

extern void flash_read_acl();
extern void flash_write_acl(CARD[], int);
