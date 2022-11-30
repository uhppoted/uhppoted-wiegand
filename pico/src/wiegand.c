#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/rtc.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "../include/cli.h"
#include "../include/reader.h"
#include "../include/sys.h"
#include "../include/wiegand.h"
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

void setup_gpio(void);
void setup_uart(void);

int64_t off(alarm_id_t, void *);
int64_t timeout(alarm_id_t, void *);
bool watchdog(repeating_timer_t *);
bool syscheck(repeating_timer_t *);

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
const uint MODE_READER = GPIO_2;
const uint MODE_EMULATOR = GPIO_3;

const uint32_t MSG = 0xf0000000;
const uint32_t MSG_WATCHDOG = 0x00000000;
const uint32_t MSG_SYSCHECK = 0x10000000;
const uint32_t MSG_CARD_READ = 0x20000000;
const uint32_t MSG_ECHO = 0xe0000000;
const uint32_t MSG_CMD = 0xf0000000;

enum MODE mode = UNKNOWN;
queue_t queue;

card last_card = {
    .facility_code = 0,
    .card_number = 0,
    .ok = false,
};

// UART
void on_uart0_rx() {
    static char buffer[64];
    static int ix = 0;
    static absolute_time_t t = 0;

    absolute_time_t now = get_absolute_time();
    int64_t dt = absolute_time_diff_us(t, now);

    if (t == 0 || dt > 10 * 1000 * 1000) {
        memset(buffer, 0, sizeof(buffer));
        ix = 0;
    }

    t = now;

    while (uart_is_readable(UART0)) {
        uint8_t ch = uart_getc(UART0);

        if (ch == '\n' || ch == '\r') {
            if (ix > 0) {
                char *cmd = calloc(64, 1);
                if (cmd != NULL) {
                    snprintf(cmd, 64, "%s", buffer);
                    uint32_t msg = MSG_CMD | ((uint32_t)cmd & 0x0fffffff); // SRAM_BASE is 0x20000000
                    if (!queue_is_full(&queue)) {
                        queue_try_add(&queue, &msg);
                    }
                }
            }

            memset(buffer, 0, sizeof(buffer));
            ix = 0;

        } else if (ix < sizeof(buffer) - 1) {
            buffer[ix++] = ch;
            buffer[ix] = 0;
            uint32_t msg = MSG_ECHO | ((uint32_t)buffer & 0x0fffffff); // SRAM_BASE is 0x20000000
            if (!queue_is_full(&queue)) {
                queue_try_add(&queue, &msg);
            }
        }
    }
}

const LED SYS_LED = {
    .pin = LED_PIN,
    .timer = -1,
};

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

int main() {
    static repeating_timer_t watchdog_rt;
    static repeating_timer_t syscheck_rt;

    bi_decl(bi_program_description("Pico-Wiegand interface"));
    bi_decl(bi_program_version_string(VERSION));
    bi_decl(bi_1pin_with_name(LED_PIN, "on-board LED"));
    bi_decl(bi_1pin_with_name(READER_LED, "reader LED"));

    stdio_init_all();
    setup_gpio();

    // ... initialise RTC
    rtc_init();
    sleep_us(64);

    // ... initialise FIFO, UART and timers
    queue_init(&queue, sizeof(uint32_t), 32);
    setup_uart();
    alarm_pool_init_default();

    // ... initialise reader/emulator

    if (!gpio_get(MODE_READER) && gpio_get(MODE_EMULATOR)) {
        mode = READER;
    } else if (gpio_get(MODE_READER) && !gpio_get(MODE_EMULATOR)) {
        mode = EMULATOR;
    } else {
        mode = UNKNOWN;
    }

    reader_initialise();

    // ... setup sys stuff
    add_repeating_timer_ms(2500, watchdog, NULL, &watchdog_rt);
    add_repeating_timer_ms(5000, syscheck, NULL, &syscheck_rt);

    char *dt = calloc(32, 1);
    if (dt != NULL) {
        snprintf(dt, 32, "%s %s", SYSDATE, SYSTIME);
        sys_settime(dt);
        free(dt);
    }

    rtc_get_datetime(&last_card.timestamp);
    sys_ok();
    VT100();

    while (true) {
        uint32_t v;
        queue_remove_blocking(&queue, &v);

        if ((v & MSG) == MSG_WATCHDOG) {
            blink((LED *)&SYS_LED);
        }

        if ((v & MSG) == MSG_SYSCHECK) {
            sys_ok();
        }

        if ((v & MSG) == MSG_ECHO) {
            const char *cmd = (const char *)(SRAM_BASE | (v & 0x0fffffff));
            echo(cmd);
        }

        if ((v & MSG) == MSG_CMD) {
            char *cmd = (char *)(SRAM_BASE | (v & 0x0fffffff));
            exec(cmd);
            free(cmd);
        }

        if ((v & MSG) == MSG_CARD_READ) {
            char s[64];

            on_card_read(v & 0x0f000000);
            cardf(&last_card, s, sizeof(s));
            puts(s);
            blink(last_card.ok ? (LED *)&GOOD_LED : (LED *)&BAD_LED);
        }
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

    gpio_init(MODE_READER);
    gpio_set_dir(MODE_READER, GPIO_IN);
    gpio_pull_up(MODE_READER);

    gpio_init(MODE_EMULATOR);
    gpio_set_dir(MODE_EMULATOR, GPIO_IN);
    gpio_pull_up(MODE_EMULATOR);
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

bool watchdog(repeating_timer_t *rt) {
    uint32_t v = MSG_WATCHDOG | 0x0000000;

    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }

    return true;
}

bool syscheck(repeating_timer_t *rt) {
    uint32_t v = MSG_SYSCHECK | 0x0000000;

    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }

    return true;
}

void blink(LED *led) {
    if (led->timer != -1) {
        cancel_alarm(led->timer);
    }

    gpio_put(led->pin, 1);

    led->timer = add_alarm_in_ms(250, off, (void *)led, true);
}

int64_t off(alarm_id_t id, void *data) {
    const LED *led = data;

    gpio_put(led->pin, 0);
    cancel_alarm(id);

    return 0;
}

// Ref. https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
// Ref. https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
int bits(uint32_t v) {
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0f0f0f0f;

    return (v * 0x01010101) >> 24;
}

void cardf(const card *c, char *s, int N) {
    if (c->card_number == 0) {
        snprintf(s, N, "CARD  ---");
    } else {
        snprintf(s, N, "CARD  %04d-%02d-%02d %02d:%02d:%02d  %d%05d %s",
                 c->timestamp.year,
                 c->timestamp.month,
                 c->timestamp.day,
                 c->timestamp.hour,
                 c->timestamp.min,
                 c->timestamp.sec,
                 c->facility_code,
                 c->card_number,
                 c->ok ? "OK" : "INVALID");
    }
}
