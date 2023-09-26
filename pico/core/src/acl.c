#include <stdio.h>
#include <stdlib.h>

#include <f_util.h>
#include <hardware/rtc.h>

#include <acl.h>
#include <flash.h>
#include <logd.h>
#include <sdcard.h>
#include <wiegand.h>

CARD ACL[32];
uint32_t PASSCODES[4] = {0, 0, 0, 0};
const uint32_t OVERRIDE = MASTER_PASSCODE;
const int ACL_SIZE = sizeof(ACL) / sizeof(CARD);
const int PASSCODES_SIZE = sizeof(PASSCODES) / sizeof(uint32_t);

/* Initialises the ACL.
 *
 */
void acl_initialise() {
    for (int i = 0; i < ACL_SIZE; i++) {
        ACL[i].card_number = 0xffffffff;
    }
}

/* Initialises the ACL from flash/SDCARD.
 *
 */
int acl_load() {
    // ... load ACL from flash
    int N = ACL_SIZE;
    char s[64];

    flash_read_acl(ACL, &N);
    snprintf(s, sizeof(s), "ACL    LOADED %d CARDS FROM FLASH", N);
    logd_log(s);

    // ... override with ACL from SDCARD
    {
        CARD cards[ACL_SIZE];
        int N = ACL_SIZE;
        int rc = sdcard_read_acl(cards, &N);

        if (rc != 0) {
            snprintf(s, sizeof(s), "DISK   READ ACL ERROR (%d) %s", rc, FRESULT_str(rc));
            logd_log(s);
            return -1;
        } else {
            for (int i = 0; i < ACL_SIZE; i++) {
                ACL[i].card_number = 0xffffffff;
            }

            for (int i = 0; i < N && i < ACL_SIZE; i++) {
                ACL[i].card_number = cards[i].card_number;
                ACL[i].start = cards[i].start;
                ACL[i].end = cards[i].end;
                ACL[i].allowed = cards[i].allowed;
                snprintf(ACL[1].name, CARD_NAME_SIZE, cards[i].name);
            }

            snprintf(s, sizeof(s), "DISK   LOADED %d CARDS FROM SDCARD", N);
            logd_log(s);
        }
    }

    return N;
}

/* Persists the in-memory ACL to flash/SDCARD
 *
 */
int acl_save() {
    int N = 0;
    char s[64];

    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number != 0xffffffff) {
            N++;
        }
    }

    // ... save to flash
    flash_write_acl(ACL, ACL_SIZE);
    snprintf(s, sizeof(s), "ACL    STORED %d CARDS TO FLASH", N);
    logd_log(s);

    // ... save to SDCARD
    int rc = sdcard_write_acl(ACL, N);

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
    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number != 0xffffffff) {
            count++;
        }
    }

    if ((*list = calloc(count, sizeof(uint32_t))) != NULL) {
        int ix = 0;
        for (int i = 0; i < ACL_SIZE; i++) {
            if (ACL[i].card_number != 0xffffffff) {
                (*list)[ix++] = ACL[i].card_number;
            }
        }
    }

    return count;
}

/* Adds a card to the ACL.
 *
 */
bool acl_grant(uint32_t facility_code, uint32_t card) {
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

    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number == v) {
            ACL[i].start = start;
            ACL[i].end = end;
            ACL[i].allowed = true;
            snprintf(ACL[i].name, CARD_NAME_SIZE, "----");
            return true;
        }
    }

    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number == 0xffffffff) {
            ACL[i].card_number = v;
            ACL[i].start = start;
            ACL[i].end = end;
            ACL[i].allowed = true;
            snprintf(ACL[i].name, CARD_NAME_SIZE, "----");

            return true;
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

    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number == v) {
            ACL[i].card_number = 0xffffffff;
            ACL[i].allowed = false;
            revoked = true;
        }
    }

    return revoked;
}

/* Checks a card against the ACL.
 *
 */
bool acl_allowed(uint32_t facility_code, uint32_t card) {
    for (int i = 0; i < ACL_SIZE; i++) {
        const uint32_t card_number = ACL[i].card_number;
        const bool allowed = ACL[i].allowed;

        if ((card_number != 0xffffffff) && ((card_number / 100000) == facility_code) && ((card_number % 100000) == card)) {
            return allowed;
        }
    }

    return false;
}

/* Checks a keycode against the ACL passcode. Falls back to the compiled in master
 * code if all the passcodes are zero.
 *
 */
bool acl_passcode(const char *code) {
    const int passcode = atoi(code);
    char s[64];

    if (passcode != 0) {
        for (int i = 0; i < PASSCODES_SIZE; i++) {
            if (passcode == PASSCODES[i]) {
                return true;
            }
        }

        for (int i = 0; i < PASSCODES_SIZE; i++) {
            if (PASSCODES[i] != 0) {
                return false;
            }
        }

        if (passcode == OVERRIDE) {
            return true;
        }
    }

    return false;
}
