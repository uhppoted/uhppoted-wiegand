#include <tusb.h>

// #include <wiegand.h>
#include <log.h>
#include <sys.h>
// #include <types/buffer.h>
#include <usb.h>

#define LOGTAG "USB"

const uint8_t CDC1 = 1;

struct {
    repeating_timer_t usb_timer;

    // struct {
    //     circular_buffer cli;
    // } buffers;

    struct {
        bool usb0;
    } connected;

    struct {
        mutex_t usb0;
    } guard;
} USB = {
    // .buffers = {
    //     .cli = {
    //         .head = 0,
    //         .tail = 0,
    //     },
    // },

    .connected = {
        .usb0 = false,
    },

};

bool on_usb_rx(repeating_timer_t *rt);

bool usb_init() {
    USB.connected.usb0 = false;

    mutex_init(&USB.guard.usb0);

    add_repeating_timer_ms(50, on_usb_rx, NULL, &USB.usb_timer);

    infof(LOGTAG, "initialised");

    return true;
}

bool on_usb_rx(repeating_timer_t *rt) {
    if (tud_cdc_n_connected(0) && !USB.connected.usb0) {
        USB.connected.usb0 = true;

        infof(LOGTAG, "USB.0 connected");
        stdout_connected(true);
    } else if (!tud_cdc_n_connected(0) && USB.connected.usb0) {
        USB.connected.usb0 = false;

        infof(LOGTAG, "USB.0 disconnected");
        stdout_connected(false);
    }

    tud_task();

    return true;
}

// // tinusb callback for received data
// void tud_cdc_rx_cb(uint8_t itf) {
//     uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];
//
//     // | IMPORTANT: also do this for CDC0 because otherwise
//     // | you won't be able to print anymore to CDC0
//     // | next time this function is called
//     uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
//
//     // USB.0 ?
//     if (itf == 0 && count > 0) {
//         for (int i = 0; i < count; i++) {
//             buffer_push(&USB.buffers.cli, buf[i]);
//         }
//
//         message qmsg = {
//             .message = MSG_TTY,
//             .tag = MESSAGE_BUFFER,
//             .buffer = &USB.buffers.cli,
//         };
//
//         push(qmsg);
//     }
// }
