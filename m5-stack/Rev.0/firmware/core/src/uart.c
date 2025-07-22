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

const char *ERR_OK = "OK\n";
const char *ERR_BAD_REQUEST = "ERROR 1000 bad request\n";
const char *ERR_INVALID_CARD = "ERROR 1001 invalid card\n";
const char *ERR_WRITE = "ERROR 1002 write failed\n";

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
void uart_flush(uint8_t);
void uart_rxchar(const uart_inst_t *, uint8_t, line *);
void uart_exec(const uart_inst_t *, char *);
void uart_write(const uart_inst_t *, const char *);

void uart_swipe(const uart_inst_t *uart, char *msg);
void uart_keypad(const uart_inst_t *uart, char *msg);

void uart0_rx(buffer *);
void uart1_rx(buffer *);

struct rxh RXH0 = {
    .buffer = &UART.UART0.rx,
    .f = uart0_rx,
};

struct rxh RXH1 = {
    .buffer = &UART.UART1.rx,
    .f = uart1_rx,
};

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
        .message = MSG_RX,
        .tag = MESSAGE_RXH,
        .rxh = &RXH0,
    });
}

void on_uart1_rx() {
    while (uart_is_readable(uart1)) {
        uint8_t ch = uart_getc(uart1);

        buffer_push(&UART.UART1.rx, ch);
    }

    push((message){
        .message = MSG_RX,
        .tag = MESSAGE_RXH,
        .rxh = &RXH1,
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

void uart0_rx(buffer *b) {
    buffer_flush(b, uart0_rxchar);
}

void uart0_rxchar(uint8_t ch) {
    uart_rxchar(uart0, ch, &UART.UART0.line);
}

void uart1_rx(buffer *b) {
    buffer_flush(b, uart1_rxchar);
}

void uart1_rxchar(uint8_t ch) {
    uart_rxchar(uart1, ch, &UART.UART1.line);
}

void uart_flush(uint8_t ch) {
}

void uart_rxchar(const uart_inst_t *uart, uint8_t ch, line *line) {
    switch (ch) {
    case CR:
    case LF:
        if (line->ix > 0) {
            uart_exec(uart, line->bytes);
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

void uart_exec(const uart_inst_t *uart, char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%s", msg);
    debugf(LOGTAG, s);

    if (strncasecmp(msg, "swipe ", 6) == 0) {
        uart_swipe(uart, &msg[6]);
    } else if (strncasecmp(msg, "code ", 5) == 0) {
        uart_keypad(uart, &msg[5]);
    }
}

void uart_swipe(const uart_inst_t *uart, char *msg) {
    char *token = strtok(msg, " ,");

    if (token != NULL) {
        uint32_t u32;

        if (sscanf(msg, "%u", &u32) < 1) {
            uart_write(uart, ERR_BAD_REQUEST);
            return;
        }

        uint32_t facility_code = u32 / 100000;
        uint32_t card = u32 % 100000;
        char *code;

        if (facility_code < 1 || facility_code > 255 || card > 65535) {
            uart_write(uart, ERR_BAD_REQUEST);
            return;
        }

        if (!write_card(facility_code, card)) {
            uart_write(uart, ERR_WRITE);
            debugf(LOGTAG, "card %u%05u error", facility_code, card);
            return;
        }

        if ((code = strtok(NULL, " ,")) != NULL) {
            int N = strlen(code);
            for (int i = 0; i < N; i++) {
                if (!write_keycode(code[i])) {
                    uart_write(uart, ERR_WRITE);
                    debugf(LOGTAG, "keycode %c error", code[i]);
                    return;
                }
            }
        }

        uart_write(uart, ERR_OK);
    }
}

void uart_keypad(const uart_inst_t *uart, char *msg) {
    int N = strlen(msg);

    if (N > 0) {
        for (int i = 0; i < N; i++) {
            if (!write_keycode(msg[i])) {
                uart_write(uart, ERR_WRITE);
                debugf(LOGTAG, "keycode %c error", msg[i]);
                return;
            }
        }

        uart_write(uart, ERR_OK);
    }
}

void uart_write(const uart_inst_t *uart, const char *msg) {
    int N = strlen(msg);

    if (uart == uart0) {
        uart_write_blocking(uart0, msg, N);
    } else if (uart == uart1) {
        uart_write_blocking(uart1, msg, N);
    }
}
