#pragma once

#include <hardware/pio.h>
#include <pico/types.h>

typedef struct constants {
    struct {
        uint gpio;
        PIO pio;
        int sm;
    } WS2812;

    struct {
        uint gpio;
        PIO pio;
        int sm;
    } SK6812;

    struct {
        uint DO;
        uint DI;
        PIO pio;
        int sm;
    } wiegand;

    uint LED;

    struct {
        uint tx;
        uint rx;
    } UART0;

    struct {
        uint tx;
        uint rx;
    } UART1;

    struct {
        uint IO6;
        uint IO7;
    } gpio;
} constants;
