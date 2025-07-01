#include <stddef.h>

#include <buffer.h>

bool buffer_push(buffer *b, uint8_t ch) {
    int head = b->head;
    int next = (head + 1) % sizeof(b->bytes);

    if (next == b->tail) {
        return false;
    } else {
        b->bytes[head] = ch;
        b->head = next;
    }

    return true;
}

bool buffer_pop(buffer *b, uint8_t *ch) {
    int tail = b->tail;

    if (tail == b->head) {
        return false;
    }

    *ch = b->bytes[tail++];
    b->tail = tail % sizeof(b->bytes);

    return true;
}

bool buffer_empty(buffer *b) {
    return b->tail == b->head;
}

void buffer_flush(buffer *b, void (*f)(uint8_t)) {
    int tail = b->tail;
    uint8_t ch;

    while (tail != b->head) {
        ch = b->bytes[tail++];
        tail %= sizeof(b->bytes);

        if (f != NULL) {
            f(ch);
        }
    }

    b->tail = tail;
}
