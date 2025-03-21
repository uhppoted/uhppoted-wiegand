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
    char s[64];

    if ((v & MSG) == MSG_CARD) {
        on_card_read(v & 0x0fffffff);

        if (last_card.ok && mode == CONTROLLER) {
            enum ACCESS access = acl_allowed(last_card.facility_code, last_card.card_number, "");

            switch (access) {
            case GRANTED:
                last_card.access = GRANTED;
                led_blink(1);
                door_unlock(5000);
                break;

            case NEEDS_PIN:
                last_card.access = NEEDS_PIN;
                led_blink(1);
                break;

            default:
                last_card.access = DENIED;
                led_blink(3);
            }
        }

        cardf(&last_card, s, sizeof(s), false);
        logd_log(s);
    }

    if ((v & MSG) == MSG_CODE) {
        char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
        enum ACCESS access;

        if (mode == CONTROLLER) {
            if (last_card.ok && last_card.access == NEEDS_PIN) {
                if ((access = acl_allowed(last_card.facility_code, last_card.card_number, b)) == GRANTED) {
                    last_card.access = GRANTED;
                    led_blink(1);
                    door_unlock(5000);
                } else {
                    last_card.access = DENIED;
                    led_blink(3);
                }

                cardf(&last_card, s, sizeof(s), false);

            } else if (acl_passcode(b)) {
                snprintf(s, sizeof(s), "CODE   OK");
                led_blink(8);
                door_unlock(5000);
            } else {
                snprintf(s, sizeof(s), "CODE   INVALID");
                led_blink(3);
            }

            logd_log(s);
        }

        free(b);
    }

    if ((v & MSG) == MSG_KEYPAD_DIGIT) {
        on_keypad_digit(v & 0x0fffffff);
    }
}
