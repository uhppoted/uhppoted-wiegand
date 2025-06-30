#include <M5.h>

#define LOGTAG "SYS"

extern queue_t queue;

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
    }

    if (queue_is_full(&queue) || !queue_try_add(&queue, &m)) {
        return false;
    }

    return true;
}
