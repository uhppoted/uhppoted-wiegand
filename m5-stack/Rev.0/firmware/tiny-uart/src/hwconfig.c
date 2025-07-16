#include <hwconfig.h>

// GPIO
const uint GPIO_0 = 0;
const uint GPIO_1 = 1;
const uint GPIO_2 = 2;
const uint GPIO_3 = 3;
const uint GPIO_4 = 4;
const uint GPIO_5 = 5;
const uint GPIO_6 = 6;
const uint GPIO_7 = 7;
const uint GPIO_8 = 8;
const uint GPIO_9 = 9;
const uint GPIO_10 = 10;
const uint GPIO_11 = 11;
const uint GPIO_12 = 12;
const uint GPIO_13 = 13;
const uint GPIO_14 = 14;
const uint GPIO_15 = 15;
const uint GPIO_16 = 16;
const uint GPIO_17 = 17;
const uint GPIO_18 = 18;
const uint GPIO_19 = 19;
const uint GPIO_20 = 20;
const uint GPIO_21 = 21;
const uint GPIO_22 = 22;
const uint GPIO_26 = 26;
const uint GPIO_27 = 27;
const uint GPIO_28 = 28;

const uint UART0_TX = GPIO_0;
const uint UART0_RX = GPIO_1;

const uint UART1_TX = GPIO_4;
const uint UART1_RX = GPIO_5;

const constants HW = {
    .WS2812 = {
        .gpio = GPIO_16,
        .pio = pio0,
        .sm = 0,
    },

    .SK6812 = {
        .gpio = GPIO_8,
        .pio = pio0,
        .sm = 1,
    },

    .wiegand = {
        .pio = pio0,
        .sm = 2,
        .DO = GPIO_6,
        .DI = GPIO_7,
    },

    .LED = GPIO_9,

    .UART0 = {
        .tx = GPIO_0,
        .rx = GPIO_1,
    },

    .UART1 = {
        .tx = GPIO_4,
        .rx = GPIO_5,
    },

    .gpio = {
        .IO6 = GPIO_10,
        .IO7 = GPIO_11,
    }};
