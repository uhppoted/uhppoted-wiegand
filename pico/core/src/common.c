#include <stdio.h>

#include "../include/common.h"
#include "../include/wiegand.h"

// Ref. https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
// Ref. https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
int bits(uint32_t v) {
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0f0f0f0f;

    return (v * 0x01010101) >> 24;
}

void cardf(const card *c, char *s, int N, bool timestamped) {
    char t[24];
    char n[16];
    char g[16];

    timef(c->timestamp, t, sizeof(t));

    if (c->card_number == 0) {
        snprintf(n, sizeof(n), "---");
        snprintf(g, sizeof(g), "");
    } else {
        snprintf(n, sizeof(n), "%u%05u", c->facility_code, c->card_number);

        if (!c->ok) {
            snprintf(g, sizeof(g), "INVALID");
        } else {
            switch (c->granted) {
            case GRANTED:
                snprintf(g, sizeof(g), "ACCESS GRANTED");
                break;

            case DENIED:
                snprintf(g, sizeof(g), "ACCESS DENIED");
                break;

            default:
                snprintf(g, sizeof(g), "-");
            }
        }
    }

    if (timestamped) {
        snprintf(s, N, "%-19s  CARD   %-8s %s", t, n, g);
    } else {
        snprintf(s, N, "CARD   %-8s %s", n, g);
    }
}

int timef(const datetime_t timestamp, char *s, int N) {
    if (is_valid(timestamp)) {
        snprintf(s, N, "%04d-%02d-%02d %02d:%02d:%02d",
                 timestamp.year,
                 timestamp.month,
                 timestamp.day,
                 timestamp.hour,
                 timestamp.min,
                 timestamp.sec);
    } else {
        snprintf(s, N, "---- -- -- --:--:--");
    }
}

bool is_valid(const datetime_t t) {
    if (t.year < 2000) {
        return false;
    }

    if (t.month < 1 || t.month > 12) {
        return false;
    }

    if (t.day < 1 || t.day > 31) {
        return false;
    }

    if (t.hour < 0 || t.hour > 23) {
        return false;
    }

    if (t.min < 0 || t.min > 59) {
        return false;
    }

    if (t.sec < 0 || t.sec > 59) {
        return false;
    }

    return true;
}
