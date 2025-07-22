#pragma once

#include <hardware/uart.h>
#include <stdbool.h>

struct buffer;

extern void UART_init();
extern void UART_start();
