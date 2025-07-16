#pragma once

#include <stdbool.h>

struct buffer;

extern void UART_init();
extern void UART_start();
extern void UART_rx(struct buffer *);
