#include <stdint.h>

#include "hardware/pio.h"

// PIOs
const PIO PIO_READER = pio0;
const PIO PIO_BLINK = pio0;
const PIO PIO_WRITER = pio1;
const PIO PIO_LED = pio1;
const PIO PIO_BUZZER = pio1;

const uint SM_READER = 0;
const uint SM_BLINK = 1;
const uint SM_WRITER = 0;
const uint SM_LED = 1;
const uint SM_BUZZER = 2;

const enum pio_interrupt_source IRQ_READER = pis_sm0_rx_fifo_not_empty;
const enum pio_interrupt_source IRQ_LED = pis_sm1_rx_fifo_not_empty;

const uint PIO_READER_IRQ = PIO0_IRQ_0;
const uint PIO_LED_IRQ = PIO1_IRQ_0;

// GPIO
const uint GPIO_0 = 0;   // Pico 1
const uint GPIO_1 = 1;   // Pico 2
const uint GPIO_2 = 2;   // Pico 4
const uint GPIO_3 = 3;   // Pico 5
const uint GPIO_4 = 4;   // Pico 6
const uint GPIO_5 = 5;   // Pico 7
const uint GPIO_6 = 6;   // Pico 9
const uint GPIO_7 = 7;   // Pico 10
const uint GPIO_8 = 8;   // Pico 11
const uint GPIO_9 = 9;   // Pico 12
const uint GPIO_10 = 10; // Pico 14
// const uint GPIO_11 = 11; // Pico 15
const uint GPIO_12 = 12; // Pico 16
// const uint GPIO_13 = 13; // Pico 17
// const uint GPIO_14 = 14; // Pico 19
const uint GPIO_15 = 15; // Pico 20
const uint GPIO_16 = 16; // Pico 21
const uint GPIO_17 = 17; // Pico 22
const uint GPIO_18 = 18; // Pico 24
const uint GPIO_19 = 19; // Pico 25
const uint GPIO_20 = 20; // Pico 26
const uint GPIO_21 = 21; // Pico 27
// const uint GPIO_25 = 25; // Pico LED

const uint UART0_TX = GPIO_0; // Pico 1
const uint UART0_RX = GPIO_1; // Pico 2
const uint UART1_TX = GPIO_4; // Pico 6
const uint UART1_RX = GPIO_5; // Pico 7

const uint SD_CLK = GPIO_2;
const uint SD_SI = GPIO_3;
const uint SD_SO = GPIO_4;
const uint SD_CS = GPIO_5;
const uint SD_DET = GPIO_6;

const uint MODE_CONTROLLER = GPIO_7;
const uint MODE_EMULATOR = GPIO_8;

// const uint LED_PIN = GPIO_25;
// const uint RED_LED = GPIO_11;
const uint BUZZER = GPIO_12;
// const uint YELLOW_LED = GPIO_13;
// const uint ORANGE_LED = GPIO_14;
const uint GREEN_LED = GPIO_15;

const uint WRITER_LED = GPIO_21;
const uint READER_LED = GPIO_20;
const uint WRITER_D1 = GPIO_19;
const uint WRITER_D0 = GPIO_18;
const uint READER_D1 = GPIO_17;
const uint READER_D0 = GPIO_16;