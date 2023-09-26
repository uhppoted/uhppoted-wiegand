#include <stdio.h>
#include <stdlib.h>

#include <acl.h>
#include <led.h>
#include <logd.h>
#include <read.h>
#include <relays.h>
#include <wiegand.h>

void dispatch(uint32_t v) {
    if ((v & MSG) == MSG_CODE) {
        char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
        char s[64];

        if (acl_passcode(b)) {
            snprintf(s, sizeof(s), "CODE   OK");
            led_blink(8);
            door_unlock(5000);
        } else {
            snprintf(s, sizeof(s), "CODE   INVALID");
            led_blink(3);
        }

        logd_log(s);
        free(b);
    }

    if ((v & MSG) == MSG_KEYPAD_DIGIT) {
        on_keypad_digit(v & 0x0fffffff);
    }
}
