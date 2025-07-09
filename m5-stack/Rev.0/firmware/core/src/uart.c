#include <pico/sync.h>
#include <pico/time.h>

#include <tusb.h>

#include <buffer.h>
#include <log.h>
#include <sys.h>
#include <uart.h>

#define LOGTAG "UART"

struct {
    repeating_timer_t timer;

    struct {
        bool connected;
        buffer buffer;
        mutex_t guard;
    } usb0;
} UART = {
    .usb0 = {
        .connected = false,
        .buffer = {
            .head = 0,
            .tail = 0,
        },
    },
};

bool on_uart_rx(repeating_timer_t *rt);

bool uart_initialise() {
    UART.usb0.connected = false;

    mutex_init(&UART.usb0.guard);

    add_repeating_timer_ms(50, on_uart_rx, NULL, &UART.timer);

    infof(LOGTAG, "initialised");

    return true;
}

bool on_uart_rx(repeating_timer_t *rt) {
    if (tud_cdc_n_connected(0) && !UART.usb0.connected) {
        UART.usb0.connected = true;

        infof(LOGTAG, "USB.0 connected");
        stdout_connected(true);
    } else if (!tud_cdc_n_connected(0) && UART.usb0.connected) {
        UART.usb0.connected = false;

        infof(LOGTAG, "USB.0 disconnected");
        stdout_connected(false);
    }

    return true;
}
