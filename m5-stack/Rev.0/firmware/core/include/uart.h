#pragma once

#include <hardware/uart.h>
#include <stdbool.h>

struct buffer;

extern void UART_init();
extern void UART_start();
extern void UART_rx(uart_inst_t *, struct buffer *);
