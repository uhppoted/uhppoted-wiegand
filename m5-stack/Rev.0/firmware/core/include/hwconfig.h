#pragma once

#include <hardware/pio.h>
#include <pico/types.h>

typedef struct constants {
    uint LED;

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

    struct {
        uint IO6;
        uint IO7;
    } gpio;
} constants;
