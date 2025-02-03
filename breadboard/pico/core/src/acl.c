#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <f_util.h>
#include <hardware/rtc.h>

#include <acl.h>
#include <common.h>
#include <flash.h>
#include <led.h>
#include <logd.h>
#include <sdcard.h>
#include <wiegand.h>

typedef struct ACL_t {
    CARD cards[MAX_CARDS];
    uint32_t passcodes[4];
    alarm_id_t timeout;
} ACL_t;

ACL_t ACL = {
    .passcodes = {0, 0, 0, 0},
    .timeout = 0,
};

const uint32_t OVERRIDE = MASTER_PASSCODE;
const int CARDS = sizeof(ACL.cards) / sizeof(CARD);
const int PASSCODES = sizeof(ACL.passcodes) / sizeof(uint32_t);
const uint32_t PIN_TIMEOUT = 12500; // ms

int64_t acl_PIN_timeout(alarm_id_t id, void *);

/* Initialises the ACL.
 *
 */
void acl_initialise() {
    for (int i = 0; i < CARDS; i++) {
        ACL.cards[i].card_number = 0xffffffff;
    }

    for (int i = 0; i < PASSCODES; i++) {
        ACL.passcodes[i] = 0;
    }
}

/* Initialises the ACL from flash/SDCARD.
 *
 */
int acl_load() {
    bool ok = true;
    int count = 0;
    char s[64];

    // ... load ACL from flash
    {
        int N = CARDS;
        flash_read_acl(ACL.cards, &N, ACL.passcodes, PASSCODES);
        snprintf(s, sizeof(s), "ACL    LOADED %d CARDS FROM FLASH", N);
        logd_log(s);

        count = N;
    }

    // ... override with ACL from SDCARD
    {
        CARD cards[CARDS];
        int N = CARDS;
        int rc = sdcard_read_acl(cards, &N);

        if (rc != 0) {
            snprintf(s, sizeof(s), "DISK   READ ACL ERROR (%d) %s", rc, FRESULT_str(rc));
            logd_log(s);
            return -1;
        } else {
            for (int i = 0; i < CARDS; i++) {
                ACL.cards[i].card_number = 0xffffffff;
            }

            for (int i = 0; i < N && i < CARDS; i++) {
                ACL.cards[i].card_number = cards[i].card_number;
                ACL.cards[i].start = cards[i].start;
                ACL.cards[i].end = cards[i].end;
                ACL.cards[i].allowed = cards[i].allowed;
                snprintf(ACL.cards[i].PIN, sizeof(cards[i].PIN), cards[i].PIN);
                snprintf(ACL.cards[i].name, sizeof(cards[i].name), cards[i].name);
            }

            count = N;

            snprintf(s, sizeof(s), "DISK   LOADED %d CARDS FROM SDCARD", N);
            logd_log(s);
        }
    }

    return count;
}

/* Persists the in-memory ACL to flash/SDCARD
 *
 */
int acl_save() {
    int N = 0;
    char s[64];

    // ... discard invalid cards
    for (int i = 0; i < CARDS; i++) {
        uint32_t card = ACL.cards[i].card_number;
        uint32_t facility_code = card / 100000;
        uint32_t card_number = card % 100000;

        if (facility_code > 255 || card_number > 65535) {
            ACL.cards[i].card_number = 0xffffffff;
        }
    }

    for (int i = 0; i < CARDS; i++) {
        if (ACL.cards[i].card_number != 0xffffffff) {
            N++;
        }
    }

    // ... save to flash
    flash_write_acl(ACL.cards, CARDS, ACL.passcodes, PASSCODES);
    snprintf(s, sizeof(s), "ACL    STORED %d CARDS TO FLASH", N);
    logd_log(s);

    // ... save to SDCARD
    int rc = sdcard_write_acl(ACL.cards, N);

    if (rc != 0) {
        snprintf(s, sizeof(s), "DISK   WRITE ACL ERROR (%d) %s", rc, FRESULT_str(rc));
        logd_log(s);
    } else {
        snprintf(s, sizeof(s), "DISK   WRITE ACL OK");
        logd_log(s);
    }

    return 0;
}

/* Returns a list of valid cards
 *
 */
int acl_list(uint32_t *list[]) {
    int count = 0;
    for (int i = 0; i < CARDS; i++) {
        if (ACL.cards[i].card_number != 0xffffffff) {
            count++;
        }
    }

    if ((*list = calloc(count, sizeof(uint32_t))) != NULL) {
        int ix = 0;
        for (int i = 0; i < CARDS; i++) {
            if (ACL.cards[i].card_number != 0xffffffff) {
                (*list)[ix++] = ACL.cards[i].card_number;
            }
        }
    }

    return count;
}

/* Removes all cards from the ACL.
 *
 */
bool acl_clear() {
    for (int i = 0; i < CARDS; i++) {
        ACL.cards[i].card_number = 0xffffffff;
    }

    return true;
}

/* Adds a card to the ACL.
 *
 */
bool acl_grant(uint32_t facility_code, uint32_t card, const char *PIN) {
    if (facility_code < 256 && card < 65536) {
        uint32_t v = (facility_code * 100000) + (card % 100000);
        datetime_t now;

        rtc_get_datetime(&now);

        datetime_t start = {
            .year = now.year,
            .month = 1,
            .day = 1,
            .dotw = 0,
            .hour = 0,
            .min = 0,
            .sec = 0,
        };

        datetime_t end = {
            .year = now.year + 1,
            .month = 12,
            .day = 31,
            .dotw = 0,
            .hour = 23,
            .min = 59,
            .sec = 59,
        };

        for (int i = 0; i < CARDS; i++) {
            if (ACL.cards[i].card_number == v) {
                ACL.cards[i].start = start;
                ACL.cards[i].end = end;
                ACL.cards[i].allowed = true;
                snprintf(ACL.cards[i].PIN, sizeof(ACL.cards[i].PIN), PIN);
                snprintf(ACL.cards[i].name, sizeof(ACL.cards[i].name), "----");
                return true;
            }
        }

        for (int i = 0; i < CARDS; i++) {
            if (ACL.cards[i].card_number == 0xffffffff) {
                ACL.cards[i].card_number = v;
                ACL.cards[i].start = start;
                ACL.cards[i].end = end;
                ACL.cards[i].allowed = true;
                snprintf(ACL.cards[i].PIN, sizeof(ACL.cards[i].PIN), PIN);
                snprintf(ACL.cards[i].name, sizeof(ACL.cards[i].name), "----");

                return true;
            }
        }
    }

    return false;
}

/* Removes a card from the ACL.
 *
 */
bool acl_revoke(uint32_t facility_code, uint32_t card) {
    uint32_t v = (facility_code * 100000) + (card % 100000);
    bool revoked = false;

    for (int i = 0; i < CARDS; i++) {
        if (ACL.cards[i].card_number == v) {
            ACL.cards[i].card_number = 0xffffffff;
            ACL.cards[i].allowed = false;
            revoked = true;
        }
    }

    return revoked;
}

/* Checks a card against the ACL.
 *
 */
enum ACCESS acl_allowed(uint32_t facility_code, uint32_t card, const char *pin) {
    if (ACL.timeout > 0) {
        cancel_alarm(ACL.timeout);
        ACL.timeout = 0;
    }

    for (int i = 0; i < CARDS; i++) {
        const uint32_t card_number = ACL.cards[i].card_number;
        const bool allowed = ACL.cards[i].allowed;

        if ((card_number != 0xffffffff) && ((card_number / 100000) == facility_code) && ((card_number % 100000) == card)) {
            if (ACL.cards[i].allowed && strncmp(ACL.cards[i].PIN, "", CARD_PIN_SIZE) == 0) {
                return GRANTED;
            }

            if (ACL.cards[i].allowed && strncmp(ACL.cards[i].PIN, pin, CARD_PIN_SIZE) == 0) {
                return GRANTED;
            }

            if (ACL.cards[i].allowed && strncmp(ACL.cards[i].PIN, "", CARD_PIN_SIZE) != 0 && strncmp(pin, "", CARD_PIN_SIZE) == 0) {
                if ((ACL.timeout = add_alarm_in_ms(PIN_TIMEOUT, acl_PIN_timeout, NULL, true)) >= 0) {
                    return NEEDS_PIN;
                }
            }

            return DENIED;
        }
    }

    return DENIED;
}

/* Sets the override passcodes.
 *
 */
bool acl_set_passcodes(uint32_t passcode1, uint32_t passcode2, uint32_t passcode3, uint32_t passcode4) {
    // ... set in-memory passcodes
    ACL.passcodes[0] = passcode1 > 0 && passcode1 < 1000000 ? passcode1 : 0;
    ACL.passcodes[1] = passcode2 > 0 && passcode2 < 1000000 ? passcode2 : 0;
    ACL.passcodes[2] = passcode3 > 0 && passcode3 < 1000000 ? passcode3 : 0;
    ACL.passcodes[3] = passcode4 > 0 && passcode4 < 1000000 ? passcode4 : 0;

    // ... save to flash
    flash_write_acl(ACL.cards, CARDS, ACL.passcodes, PASSCODES);
    logd_log("ACL    UPDATED PASSCODES IN FLASH");

    return true;
}

/* Checks a keycode against the ACL passcode. Falls back to the compiled in master
 * code if all the passcodes are zero.
 *
 */
bool acl_passcode(const char *code) {
    const int passcode = atoi(code);
    char s[64];

    if (passcode != 0) {
        for (int i = 0; i < PASSCODES; i++) {
            if (passcode == ACL.passcodes[i]) {
                return true;
            }
        }

        for (int i = 0; i < PASSCODES; i++) {
            if (ACL.passcodes[i] != 0) {
                return false;
            }
        }

        if (passcode == OVERRIDE) {
            return true;
        }
    }

    return false;
}

int64_t acl_PIN_timeout(alarm_id_t id, void *data) {
    char s[64];

    ACL.timeout = 0;
    last_card.access = DENIED;
    led_blink(3);

    cardf(&last_card, s, sizeof(s), false);
    logd_log(s);

    return 0;
}
