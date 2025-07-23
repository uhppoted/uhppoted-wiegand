#include <M5.h>

#define LOGTAG "SYS"

extern queue_t queue;

const char *ERR_OK = "OK\r\n";
const char *ERR_BAD_REQUEST = "ERROR 1000 bad request\r\n";
const char *ERR_INVALID_CARD = "ERROR 1001 invalid card\r\n";
const char *ERR_WRITE = "ERROR 1002 write failed\r\n";

// NTS: SRAM_BASE is 0x20000000
bool push(message msg) {
    uint32_t m = msg.message;

    switch (msg.tag) {
    case MESSAGE_NONE:
        m |= msg.none & 0x0fffffff;
        break;

    case MESSAGE_BOOLEAN:
        m |= msg.boolean ? 0x00000001 : 0x00000000;
        break;

    case MESSAGE_UINT32:
        m |= msg.u32 & 0x0fffffff;
        break;

    case MESSAGE_BUFFER:
        m |= ((uint32_t)msg.buffer & 0x0fffffff);
        break;

    case MESSAGE_RXH:
        m |= ((uint32_t)msg.rxh & 0x0fffffff);
        break;
    }

    if (queue_is_full(&queue) || !queue_try_add(&queue, &m)) {
        return false;
    }

    return true;
}
