#include <stdio.h>
#include <stdlib.h>

#include <acl.h>
#include <common.h>
#include <led.h>
#include <logd.h>
#include <read.h>
#include <relays.h>
#include <wiegand.h>

const uint32_t MSG_CARD = 0x40000000;
const uint32_t MSG_CODE = 0x50000000;
const uint32_t MSG_KEYPAD_DIGIT = 0x60000000;

void dispatch(uint32_t v) {
    char s[64];

    if ((v & MSG) == MSG_CARD) {
        on_card_read(v & 0x0fffffff);

        cardf(&last_card, s, sizeof(s), false);
        logd_log(s);
    }

    if ((v & MSG) == MSG_CODE) {
        char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));

        snprintf(s, sizeof(s), "CODE   %s", b);
        logd_log(s);
        free(b);
    }

    if ((v & MSG) == MSG_KEYPAD_DIGIT) {
        on_keypad_digit(v & 0x0fffffff);
    }
}
