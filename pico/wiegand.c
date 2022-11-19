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

typedef struct reader {
    uint32_t card;
    uint32_t bits;
    int32_t timer;
    absolute_time_t start;
    absolute_time_t delta;
} reader;

typedef struct LED {
    uint pin;
    int32_t timer;
} LED;

void setup_gpio(void);
void setup_uart(void);
int count(uint32_t);

void blink(LED *);
int64_t off(alarm_id_t, void *);
int64_t timeout(alarm_id_t, void *);

const uint GPIO_0 = 0;   // Pico 1
const uint GPIO_1 = 1;   // Pico 2
const uint GPIO_2 = 2;   // Pico 4
const uint GPIO_3 = 3;   // Pico 5
const uint GPIO_4 = 4;   // Pico 6
const uint GPIO_5 = 5;   // Pico 7
const uint GPIO_6 = 6;   // Pico 9
const uint GPIO_7 = 7;   // Pico 10
const uint GPIO_8 = 8;   // Pico 11
const uint GPIO_12 = 12; // Pico 16
const uint GPIO_13 = 13; // Pico 17
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
const uint YELLOW_LED = GPIO_14;
const uint ORANGE_LED = GPIO_13;
const uint BLUE_LED = GPIO_12;
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

const LED TIMEOUT_LED = {
    .pin = YELLOW_LED,
    .timer = -1,
};

const LED BAD_LED = {
    .pin = ORANGE_LED,
    .timer = -1,
};

const LED GOOD_LED = {
    .pin = BLUE_LED,
    .timer = -1,
};

// READER

void rxi() {
    const uint sm = 0;

    static reader rdr = {
        .card = 0,
        .bits = 0,
        .timer = -1,
        .start = 0,
    };

    if (rdr.bits == 0) {
        rdr.start = get_absolute_time();
        rdr.bits = 0;
        rdr.timer = add_alarm_in_ms(READ_TIMEOUT, timeout, (reader *)&rdr, true);
    }

    uint32_t value = reader_program_get(PIO_IN, sm);

    switch (value) {
    case 1:
        rdr.card <<= 1;
        rdr.card |= 0x00000001;
        rdr.bits++;
        break;

    case 2:
        rdr.card <<= 1;
        rdr.bits++;
        break;
    }

    if (rdr.bits >= 26) {
        if (rdr.timer != -1) {
            cancel_alarm(rdr.timer);
        }

        uint32_t v = rdr.card;
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }

        rdr.card = 0;
        rdr.bits = 0;
        rdr.timer = -1;
    }
}

int main() {
    bi_decl(bi_program_description("Pico-Wiegand interface"));
    bi_decl(bi_program_version_string(VERSION));
    bi_decl(bi_1pin_with_name(LED_PIN, "on-board LED"));
    bi_decl(bi_1pin_with_name(READER_LED, "reader LED"));

    stdio_init_all();

    setup_gpio();
    setup_uart();

    // ... initialise FIFO
    queue_init(&queue, sizeof(uint32_t), 32);

    // ... initialise timers
    alarm_pool_init_default();

    // ... initialise PIOs
    PIO pio = PIO_IN;
    uint sm = 0;
    uint offset = pio_add_program(pio, &reader_program);

    reader_program_init(pio, sm, offset, D0, D1);

    irq_set_exclusive_handler(PIO0_IRQ_0, rxi);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_set_irq0_source_enabled(pio, pis_sm0_rx_fifo_not_empty, true);

    // ... start message
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
                blink((LED *)&BAD_LED);
                snprintf(s, sizeof(s), "%d%05d INVALID", facility_code, card_number);
            } else {
                blink((LED *)&GOOD_LED);
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

void setup_gpio() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(READER_LED);
    gpio_set_dir(READER_LED, GPIO_OUT);
    gpio_pull_up(READER_LED);
    gpio_put(READER_LED, 1);

    gpio_init(YELLOW_LED);
    gpio_init(ORANGE_LED);
    gpio_init(BLUE_LED);

    gpio_set_dir(YELLOW_LED, GPIO_OUT);
    gpio_set_dir(ORANGE_LED, GPIO_OUT);
    gpio_set_dir(BLUE_LED, GPIO_OUT);

    gpio_put(YELLOW_LED, 0);
    gpio_put(ORANGE_LED, 0);
    gpio_put(BLUE_LED, 0);
}

void setup_uart() {
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
}

int64_t timeout(alarm_id_t id, void *data) {
    reader *r = (reader *)data;
    r->bits = 0;
    r->card = 0;

    blink((LED *)&TIMEOUT_LED);
    cancel_alarm(id);
}

void blink(LED *led) {
    if (led->timer != -1) {
        cancel_alarm(led->timer);
    }

    gpio_put(led->pin, 1);

    led->timer = add_alarm_in_ms(500, off, (void *)led, true);
}

int64_t off(alarm_id_t id, void *data) {
    const LED *led = data;

    gpio_put(led->pin, 0);
    cancel_alarm(id);

    return 0;
}

int count(uint32_t v) {
    int N = 0;

    while (v != 0) {
        N++;
        v &= v - 1;
    }

    return N;
}
