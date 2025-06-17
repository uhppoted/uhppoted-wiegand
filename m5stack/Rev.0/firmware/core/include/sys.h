#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool sys_init();
extern void sys_tick();

extern bool sysinit();
extern void dispatch(uint32_t);
