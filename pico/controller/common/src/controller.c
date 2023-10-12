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

void dispatch(uint32_t v) {
    if ((v & MSG) == MSG_CARD) {
        on_card_read(v & 0x0fffffff);

        if (last_card.ok && mode == CONTROLLER) {
            if (acl_allowed(last_card.facility_code, last_card.card_number)) {
                last_card.granted = GRANTED;
                led_blink(1);
                door_unlock(5000);
            } else {
                last_card.granted = DENIED;
                led_blink(3);
            }
        }

        char s[64];
        cardf(&last_card, s, sizeof(s), false);
        logd_log(s);
    }

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
