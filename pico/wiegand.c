#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include <stdio.h>

#include <IN.pio.h>

#define VERSION "v0.0.0"

#define UART0 uart0
#define UART1 uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

#define PIO_IN pio0
#define PIO_OUT pio1

int count(uint32_t);

const uint GPIO_0 = 0;   // Pico 1
const uint GPIO_1 = 1;   // Pico 2
const uint GPIO_2 = 2;   // Pico 4
const uint GPIO_3 = 3;   // Pico 5
const uint GPIO_4 = 4;   // Pico 6
const uint GPIO_5 = 5;   // Pico 7
const uint GPIO_6 = 6;   // Pico 9
const uint GPIO_7 = 7;   // Pico 10
const uint GPIO_8 = 8;   // Pico 11
const uint GPIO_14 = 14; // Pico 19
const uint GPIO_15 = 15; // Pico 20
const uint GPIO_16 = 16; // Pico 21
const uint GPIO_17 = 17; // Pico 22
const uint GPIO_20 = 20; // Pico 26
const uint GPIO_21 = 21; // Pico 27
const uint GPIO_25 = 25; // Pico LED

const uint UART0_TX = GPIO_0;  // Pico 1
const uint UART0_RX = GPIO_1;  // Pico 2
const uint UART1_TX = GPIO_20; // Pico 26
const uint UART1_RX = GPIO_21; // Pico 27

const uint LED_PIN = GPIO_25;
const uint READER_LED = GPIO_15;
const uint DEBUG_LED = GPIO_14;
const uint D0 = GPIO_16;
const uint D1 = GPIO_17;
const uint32_t READ_TIMEOUT = 100;

bool on = false;
queue_t queue;

// UART
void on_uart0_rx() {
    while (uart_is_readable(UART0)) {
        uint8_t ch = uart_getc(UART0);
        switch (ch) {
        case 'o':
        case 'O':
            on = true;
            gpio_put(READER_LED, 0);
            break;

        case 'x':
        case 'X':
            on = false;
            gpio_put(READER_LED, 1);
            break;
        }
    }
}

// READER
void rxi() {
    const uint sm = 0;
    static uint32_t card = 0;
    static uint32_t bits = 0;
    static absolute_time_t start;

    absolute_time_t now = get_absolute_time();
    uint32_t delta = absolute_time_diff_us(start, now) / 1000;

    if (bits == 0 || delta > READ_TIMEOUT) {
        start = now;
        bits = 0;
    }

    uint32_t value = reader_program_get(PIO_IN, sm);

    switch (value) {
    case 1:
        card <<= 1;
        card |= 0x00000001;
        bits++;
        break;

    case 2:
        card <<= 1;
        bits++;
        break;
    }

    if (bits >= 26) {
        uint32_t v = card;
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }

        card = 0;
        bits = 0;
    }
}

int main() {
    bi_decl(bi_program_description("Pico-Wiegand interface"));
    bi_decl(bi_program_version_string(VERSION));
    bi_decl(bi_1pin_with_name(LED_PIN, "on-board LED"));
    bi_decl(bi_1pin_with_name(READER_LED, "reader LED"));

    stdio_init_all();

    // ... initialise GPIO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(READER_LED);
    gpio_set_dir(READER_LED, GPIO_OUT);
    gpio_pull_up(READER_LED);
    gpio_put(READER_LED, 1);

    gpio_init(DEBUG_LED);
    gpio_set_dir(DEBUG_LED, GPIO_OUT);
    gpio_put(DEBUG_LED, 0);

    // ... initialise FIFO
    queue_init(&queue, sizeof(uint32_t), 32);

    // ... initialise UARTs
    uart_init(UART0, 2400);
    gpio_set_function(UART0_TX, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX, GPIO_FUNC_UART);
    uart_set_baudrate(UART0, BAUD_RATE);
    uart_set_hw_flow(UART0, false, false);
    uart_set_format(UART0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART0, false);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART0, true, false);

    // ... initialise PIOs
    PIO pio = PIO_IN;
    uint sm = 0;
    uint offset = pio_add_program(pio, &reader_program);

    reader_program_init(pio, sm, offset, DEBUG_LED, D0, D1);

    irq_set_exclusive_handler(PIO0_IRQ_0, rxi);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_set_irq0_source_enabled(pio, pis_sm0_rx_fifo_not_empty, true);
    // pio0->inte0 |= PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS;

    { // 125MHz
        uint32_t hz = clock_get_hz(clk_sys);
        char s[64];
        snprintf(s, sizeof(s), "CLOCK %d", hz);
        puts(s);
    }

    while (1) {
        gpio_put(LED_PIN, 0);
        sleep_ms(250);

        gpio_put(LED_PIN, 1);

        uint32_t v;
        while (queue_try_remove(&queue, &v)) {
            int even = count(v & 0x03ffe000) % 2;
            int odd = count(v & 0x00001fff) % 2;
            uint32_t card = (v >> 1) & 0x00ffffff;
            uint32_t facility_code = (card >> 16) & 0x000000ff;
            uint32_t card_number = card & 0x0000ffff;

            char s[64];
            if (even != 0 || odd != 1) {
                snprintf(s, sizeof(s), "%d%05d INVALID", facility_code, card_number);
            } else {
                snprintf(s, sizeof(s), "CARD %d%05d OK", facility_code, card_number);
            }

            puts(s);
        }

        if (on) {
            puts("Pico/Wiegand ON");
        } else {
            puts("Pico/Wiegand OFF");
        }

        sleep_ms(1000);
    }

    // ... cleanup

    queue_free(&queue);
}

int count(uint32_t v) {
    int N = 0;

    while (v != 0) {
        N++;
        v &= v - 1;
    }

    return N;
}