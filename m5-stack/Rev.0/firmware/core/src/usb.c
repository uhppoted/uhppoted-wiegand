#include <tusb.h>

#include <M5.h>
#include <buffer.h>
#include <log.h>
#include <sys.h>
#include <usb.h>
#include <wiegand.h>

#define LOGTAG "USB"
#define CR '\n'
#define LF '\r'

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

    struct {
        char bytes[64];
        int ix;
    } line;
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

    .line = {
        .bytes = {0},
        .ix = 0,
    },
};

void usb_rx(buffer *);
void usb_rxchar(uint8_t);
void usb_exec(char *);
void usb_keypad(char *);
void usb_swipe(char *);
bool usb_write(const char *);

struct rxh RXH = {
    .buffer = &USB.usb1.buffer,
    .f = usb_rx,
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
        for (int i = 0; i < count; i++) {
            buffer_push(&USB.usb1.buffer, buf[i]);
        }

        push((message){
            .message = MSG_RX,
            .tag = MESSAGE_RXH,
            .rxh = &RXH,
        });
    }
}

void usb_rx(buffer *b) {
    buffer_flush(b, usb_rxchar);
}

void usb_rxchar(uint8_t ch) {
    switch (ch) {
    case CR:
    case LF:
        if (USB.line.ix > 0) {
            usb_exec(USB.line.bytes);
        }

        memset(USB.line.bytes, 0, sizeof(USB.line.bytes));
        USB.line.ix = 0;
        break;

    default:
        if (USB.line.ix < (sizeof(USB.line.bytes) - 1)) {
            USB.line.bytes[USB.line.ix++] = ch;
            USB.line.bytes[USB.line.ix] = 0;
        }
    }
}

void usb_exec(char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%s", msg);
    debugf(LOGTAG, s);

    if (strncasecmp(msg, "swipe ", 6) == 0) {
        usb_swipe(&msg[6]);
    } else if (strncasecmp(msg, "code ", 5) == 0) {
        usb_keypad(&msg[5]);
    }
}

void usb_swipe(char *msg) {
    char *token = strtok(msg, " ,");

    if (token != NULL) {
        uint32_t u32;

        if (sscanf(msg, "%u", &u32) < 1) {
            usb_write(ERR_BAD_REQUEST);
            return;
        }

        uint32_t facility_code = u32 / 100000;
        uint32_t card = u32 % 100000;
        char *code;

        if (facility_code < 1 || facility_code > 255 || card > 65535) {
            usb_write(ERR_BAD_REQUEST);
            return;
        }

        if (!write_card(facility_code, card)) {
            usb_write(ERR_WRITE);
            debugf(LOGTAG, "card %u%05u error", facility_code, card);
            return;
        }

        if ((code = strtok(NULL, " ,")) != NULL) {
            int N = strlen(code);
            for (int i = 0; i < N; i++) {
                if (!write_keycode(code[i])) {
                    usb_write(ERR_WRITE);
                    debugf(LOGTAG, "keycode %c error", code[i]);
                    return;
                }
            }
        }

        usb_write(ERR_OK);
    }
}

void usb_keypad(char *msg) {
    int N = strlen(msg);

    if (N > 0) {
        for (int i = 0; i < N; i++) {
            if (!write_keycode(msg[i])) {
                usb_write(ERR_WRITE);
                debugf(LOGTAG, "keycode %c error", msg[i]);
                return;
            }
        }

        usb_write(ERR_OK);
    }
}

bool usb_write(const char *msg) {
    int len = strlen(msg);
    bool ok = true;

    mutex_enter_blocking(&USB.usb1.guard);

    if (USB.usb1.connected) {
        for (int ix = 0; ix < len;) {
            uint32_t N = tud_cdc_n_write(CDC1, &msg[ix], len - ix);

            if (N == 0) {
                warnf(LOGTAG, "*** write error %u of %d", ix, len);
                ok = false;
                break;
            } else {
                debugf(LOGTAG, "write %u of %d", N, len - ix);
                ix += N;
            }
        }

        tud_cdc_n_write_flush(CDC1);

    } else {
        ok = false;
        warnf(LOGTAG, "write error: USB not connected");
    }

    mutex_exit(&USB.usb1.guard);

    return ok;
}
