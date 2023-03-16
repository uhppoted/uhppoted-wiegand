#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/clocks.h>
#include <hardware/rtc.h>
#include <hardware/uart.h>

#include <pico/time.h>
#include <pico/util/datetime.h>

#include "../include/sys.h"
#include "../include/uart.h"
#include "../include/wiegand.h"

#define UART0 uart0
#define UART1 uart1

#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

const char *MODES[] = {
    "UNKNOWN",
    "READER",
    "WRITE",
    "EMULATOR",
    "CONTROLLER",
};

void sys_start() {
    char s[64];
    uint32_t hz = clock_get_hz(clk_sys);

    snprintf(s, sizeof(s), "%6s %d", "CLOCK", hz);
    tx(s);
}

void sys_ok() {
    char s[64];

    if (mode <= CONTROLLER) {
        snprintf(s, sizeof(s), "%-6s %-8s %s", "SYS", MODES[mode], "OK");
    } else {
        snprintf(s, sizeof(s), "%-6s %-8s %s", "SYS", "???", "OK");
    }

    tx(s);
}

// NTS: seems like this needs to be a 'long lived struct' for RTC
datetime_t NOW = {
    .year = 2022,
    .month = 11,
    .day = 24,
    .hour = 21,
    .min = 44,
    .sec = 13,
};

void sys_settime(char *t) {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int rc = sscanf(t, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);

    switch (rc) {
    case 6:
        NOW.sec = second;
    case 5:
        NOW.min = minute;
    case 4:
        NOW.hour = hour;
    case 3:
        NOW.day = day;
    case 2:
        NOW.month = month;
    case 1:
        NOW.year = year;
    }

    bool ok = rtc_set_datetime(&NOW);
    sleep_us(64);

    char s[64];
    snprintf(s, sizeof(s), "SYS    SET TIME %s", ok ? "OK" : "ERROR");
    tx(s);
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

int64_t startup(alarm_id_t id, void *data) {
    uint32_t v = MSG_SYSINIT | 0x0000000;

    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }

    return 0;
}

// UART
void on_uart0_rx() {
    char buffer[32];
    int ix = 0;

    while (uart_is_readable(UART0) && ix < sizeof(buffer)) {
        buffer[ix++] = uart_getc(UART0);
    }

    if (ix > 0) {
        char *b;

        if ((b = calloc(ix + 1, 1)) != NULL) {
            memmove(b, buffer, ix);
            uint32_t msg = MSG_RX | ((uint32_t)b & 0x0fffffff); // SRAM_BASE is 0x20000000
            if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
                free(b);
            }
        }

        ix = 0;
    }
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
