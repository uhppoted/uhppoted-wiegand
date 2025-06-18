#pragma once

#include <stdbool.h>
#include <stdint.h>

extern bool sys_init();
extern void sys_tick();

extern bool sysinit();
extern void stdout_connected(bool);
extern void dispatch(uint32_t);
extern void print(const char *);
