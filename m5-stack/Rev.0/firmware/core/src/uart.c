#include <pico/stdlib.h>
#include <pico/sync.h>
#include <pico/time.h>

#include <hardware/uart.h>
#include <tusb.h>

#include <M5.h>
#include <buffer.h>
#include <log.h>
#include <sys.h>
#include <uart.h>
#include <wiegand.h>

#define LOGTAG "UART"
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define CR '\n'
#define LF '\r'

extern const constants HW;

typedef struct line {
    char bytes[64];
    int ix;
} line;

struct {
    repeating_timer_t timer;

    struct {
        buffer rx;
        line line;
    } UART0;

    struct {
        buffer rx;
        line line;
    } UART1;

    struct {
        bool connected;
    } usb0;
} UART = {
    .UART0 = {
        .rx = {
            .head = 0,
            .tail = 0,
        },
        .line = {
            .bytes = {0},
            .ix = 0,
        },
    },

    .UART1 = {
        .rx = {
            .head = 0,
            .tail = 0,
        },
        .line = {
            .bytes = {0},
            .ix = 0,
        },
    },

    .usb0 = {
        .connected = false,
    },

};

void on_uart0_rx();
void on_uart1_rx();
bool on_uart_monitor(repeating_timer_t *);
void uart0_rxchar(uint8_t);
void uart1_rxchar(uint8_t);
void uart_rxchar(uint8_t, line *);
void uart_exec(char *);

void UART_init() {
    // ... uart0
    gpio_pull_up(HW.UART0.tx);
    gpio_pull_up(HW.UART0.rx);

    gpio_set_function(HW.UART0.tx, GPIO_FUNC_UART);
    gpio_set_function(HW.UART0.rx, GPIO_FUNC_UART);

    uart_init(uart0, BAUD_RATE);
    uart_set_baudrate(uart0, BAUD_RATE);
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_hw_flow(uart0, false, false);
    uart_set_fifo_enabled(uart0, true);
    uart_set_translate_crlf(uart0, false);

    // ... uart1
    gpio_pull_up(HW.UART1.tx);
    gpio_pull_up(HW.UART1.rx);

    gpio_set_function(HW.UART1.tx, GPIO_FUNC_UART);
    gpio_set_function(HW.UART1.rx, GPIO_FUNC_UART);

    uart_init(uart1, BAUD_RATE);
    uart_set_baudrate(uart1, BAUD_RATE);
    uart_set_format(uart1, DATA_BITS, STOP_BITS, PARITY);
    uart_set_hw_flow(uart1, false, false);
    uart_set_fifo_enabled(uart1, true);
    uart_set_translate_crlf(uart1, false);

    // ... 'k, done'
    infof(LOGTAG, "initialised");
}

void UART_start() {
    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    uart_set_irq_enables(uart0, true, false);
    irq_set_enabled(UART0_IRQ, true);

    irq_set_exclusive_handler(UART1_IRQ, on_uart1_rx);
    uart_set_irq_enables(uart1, true, false);
    irq_set_enabled(UART1_IRQ, true);

    add_repeating_timer_ms(50, on_uart_monitor, NULL, &UART.timer);
}

void on_uart0_rx() {
    while (uart_is_readable(uart0)) {
        uint8_t ch = uart_getc(uart0);

        buffer_push(&UART.UART0.rx, ch);
    }

    push((message){
        .message = MSG_RX0,
        .tag = MESSAGE_BUFFER,
        .buffer = &UART.UART0.rx,
    });
}

void on_uart1_rx() {
    while (uart_is_readable(uart1)) {
        uint8_t ch = uart_getc(uart1);

        buffer_push(&UART.UART1.rx, ch);
    }

    push((message){
        .message = MSG_RX1,
        .tag = MESSAGE_BUFFER,
        .buffer = &UART.UART1.rx,
    });
}

bool on_uart_monitor(repeating_timer_t *rt) {
    if (tud_cdc_n_connected(0) && !UART.usb0.connected) {
        UART.usb0.connected = true;

        stdout_connected(true);
        infof(LOGTAG, "USB.0 connected");
    } else if (!tud_cdc_n_connected(0) && UART.usb0.connected) {
        UART.usb0.connected = false;

        stdout_connected(false);
        infof(LOGTAG, "USB.0 disconnected");
    }

    return true;
}

void UART_rx(uart_inst_t *uart, buffer *b) {
    buffer_flush(b, uart0_rxchar);
}

void uart0_rxchar(uint8_t ch) {
    uart_rxchar(ch, &UART.UART0.line);
}

void uart1_rxchar(uint8_t ch) {
    uart_rxchar(ch, &UART.UART1.line);
}

void uart_rxchar(uint8_t ch, line *line) {
    switch (ch) {
    case CR:
    case LF:
        if (line->ix > 0) {
            uart_exec(line->bytes);
        }

        memset(line->bytes, 0, sizeof(line->bytes));
        line->ix = 0;
        break;

    default:
        if (line->ix < (sizeof(line->bytes) - 1)) {
            line->bytes[line->ix++] = ch;
            line->bytes[line->ix] = 0;
        }
    }
}

void uart_exec(char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%s", msg);
    debugf(LOGTAG, s);

    uint32_t card = 0;
    char *command = strtok(msg, " ");

    if (command != NULL) {
        if (strcmp(command, "CARD") == 0) {
            char *arg = strtok(NULL, " ");
            uint32_t card = 0;

            if (arg != NULL) {
                if (sscanf(arg, "%0u", &card) == 1) {
                    uint32_t facility_code = card / 100000;
                    uint32_t card_number = card % 100000;

                    if (facility_code > 0 && facility_code < 256 && card_number > 0 && card_number < 65536) {
                        infof(LOGTAG, "card %u%05u", facility_code, card_number);
                        if (!write_card(facility_code, card_number)) {
                            errorf(LOGTAG, "card %u%05u error", facility_code, card);
                        }
                    } else {
                        errorf(LOGTAG, "invalid card %lu", card);
                    }
                }
            }
        }
    }
}

// bool uart0_write(const uint8_t *bytes, int N) {
//     uart_write_blocking(uart0, bytes, N);
//
//     return true;
// }
