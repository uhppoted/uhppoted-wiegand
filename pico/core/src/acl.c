#include <stdio.h>
#include <stdlib.h>

#include <f_util.h>
#include <hardware/rtc.h>

#include "../include/acl.h"
#include "../include/flash.h"
#include "../include/sdcard.h"
#include "../include/wiegand.h"

CARD ACL[32];

#define ACL_SIZE (sizeof(ACL) / sizeof(CARD))

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
void acl_load(char *s, int len) {
    // ... load ACL from flash
    {
        int N = ACL_SIZE;

        flash_read_acl(ACL, &N);
        snprintf(s, len, "FLASH  CARDS %d", N);
    }

    // // ... override with ACL from SDCARD
    // uint32_t cards[N];
    // int N = ACL_SIZE;
    // int rc = sdcard_read_acl(cards, &N);
    //
    // if (rc != 0) {
    //     snprintf(s, len, "DISK   READ ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    // } else {
    //     snprintf(s, len, "DISK   READ ACL OK (%d)", N);
    //
    //     for (int i = 0; i < size; i++) {
    //         ACL[i] = 0xffffffff;
    //     }
    //
    //     for (int i = 0; i < N && i < size; i++) {
    //         ACL[i] = cards[i];
    //     }
    // }
}

/* Persists the in-memory ACL to flash/SDCARD
 *
 */
void acl_save(char *s, int len) {
    // ... save to flash
    flash_write_acl(ACL, ACL_SIZE);

    // // ... save to SDCARD
    // int rc = sdcard_write_acl(acl, N);
    //
    // if (rc != 0) {
    //     snprintf(s, len, "DISK   WRITE ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    // } else {
    //     snprintf(s, len, "DISK   WRITE ACL OK");
    // }
}

/* Adds a card to the ACL.
 *
 */
bool acl_grant(uint32_t facility_code, uint32_t card) {
    uint32_t v = (facility_code * 100000) + (card % 100000);
    datetime_t now;
    datetime_t start;
    datetime_t end;

    rtc_get_datetime(&now);

    start.year = now.year;
    start.month = 1;
    start.day = 1;
    start.dotw = 0;
    start.hour = 0;
    start.min = 0;
    start.sec = 0;

    end.year = now.year + 1;
    end.month = 12;
    end.day = 31;
    end.dotw = 0;
    end.hour = 23;
    end.min = 59;
    end.sec = 59;

    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number == v) {
            ACL[i].start = start;
            ACL[i].end = end;
            ACL[i].allowed = true;
            ACL[i].name = "----";
            return true;
        }
    }

    for (int i = 0; i < ACL_SIZE; i++) {
        if (ACL[i].card_number == 0xffffffff) {
            ACL[i].card_number = v;
            ACL[i].start = start;
            ACL[i].end = end;
            ACL[i].allowed = true;
            ACL[i].name = "----";

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
