#include <tusb.h>

#include <M5.h>
#include <buffer.h>
#include <log.h>
#include <sys.h>
#include <usb.h>

#define LOGTAG "USB"

const uint8_t CDC1 = 1;

struct {
    repeating_timer_t timer;

    struct {
        bool connected;
        buffer buffer;
        mutex_t guard;
    } usb0;

    struct {
        bool connected;
        buffer buffer;
        mutex_t guard;
    } usb1;
} USB = {
    .usb0 = {
        .connected = false,
        .buffer = {
            .head = 0,
            .tail = 0,
        },
    },
    .usb1 = {
        .connected = false,
        .buffer = {
            .head = 0,
            .tail = 0,
        },
    },

};

bool on_usb_rx(repeating_timer_t *rt);

bool usb_init() {
    USB.usb0.connected = false;
    USB.usb1.connected = false;

    mutex_init(&USB.usb0.guard);
    mutex_init(&USB.usb1.guard);

    add_repeating_timer_ms(50, on_usb_rx, NULL, &USB.timer);

    infof(LOGTAG, "initialised");

    return true;
}

bool on_usb_rx(repeating_timer_t *rt) {
    if (tud_cdc_n_connected(0) && !USB.usb0.connected) {
        USB.usb0.connected = true;

        infof(LOGTAG, "USB.0 connected");
        stdout_connected(true);
    } else if (!tud_cdc_n_connected(0) && USB.usb0.connected) {
        USB.usb0.connected = false;

        infof(LOGTAG, "USB.0 disconnected");
        stdout_connected(false);
    }

    if (tud_cdc_n_connected(1) && !USB.usb1.connected) {
        USB.usb1.connected = true;

        infof(LOGTAG, "USB.1 connected");
    } else if (!tud_cdc_n_connected(1) && USB.usb1.connected) {
        USB.usb1.connected = false;

        infof(LOGTAG, "USB.1 disconnected");
    }

    tud_task();

    return true;
}

// tinusb callback for received data
void tud_cdc_rx_cb(uint8_t itf) {
    uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];

    // | IMPORTANT: also do this for CDC0 because otherwise
    // | you won't be able to print anymore to CDC0
    // | next time this function is called
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

    // USB.0 ?
    if (itf == 0 && count > 0) {
        for (int i = 0; i < count; i++) {
            buffer_push(&USB.usb0.buffer, buf[i]);
        }

        push((message){
            .message = MSG_TTY,
            .tag = MESSAGE_BUFFER,
            .buffer = &USB.usb0.buffer,
        });
    }

    // USB.1 ?
    if (itf == 1 && count > 0) {
    }
}
