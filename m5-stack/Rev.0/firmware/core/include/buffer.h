#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct buffer {
    volatile int head;
    volatile int tail;
    uint8_t bytes[256];
} buffer;

bool buffer_push(buffer *, uint8_t);
bool buffer_pop(buffer *, uint8_t *);
bool buffer_empty(buffer *);
void buffer_flush(buffer *, void (*)(uint8_t));
