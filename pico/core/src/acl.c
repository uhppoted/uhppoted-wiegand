#include <stdio.h>
#include <stdlib.h>

#include <f_util.h>
#include <hardware/rtc.h>

#include "../include/acl.h"
#include "../include/sdcard.h"
#include "../include/wiegand.h"

uint32_t ACL[32] = {};

/* Initialises the ACL.
 *
 */
void acl_initialise() {
    int size = sizeof(ACL) / sizeof(uint32_t);

    for (int i = 0; i < size; i++) {
        ACL[i] = 0xffffffff;
    }
}

/* Initialises the ACL from flash/SDCARD.
 *
 */
void acl_load(char *s, int len) {
    int size = sizeof(ACL) / sizeof(uint32_t);
    int N = size;
    uint32_t cards[N];
    int rc = sdcard_read_acl(cards, &N);

    if (rc != 0) {
        snprintf(s, len, "DISK   READ ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    } else {
        snprintf(s, len, "DISK   READ ACL OK (%d)", N);

        for (int i = 0; i < size; i++) {
            ACL[i] = 0xffffffff;
        }

        for (int i = 0; i < N && i < size; i++) {
            ACL[i] = cards[i];
        }
    }
}

/* Persists the in-memory ACL to flash/SDCARD
 *
 */
void acl_save(char *s, int len) {
    uint32_t *cards;
    int N = acl_list(&cards);
    CARD acl[N];

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

    for (int i = 0; i < N; i++) {
        acl[i].card_number = cards[i];
        acl[i].start = start;
        acl[i].end = end;
        acl[i].allowed = true;
        acl[i].name = "----";
    }

    free(cards);

    int rc = sdcard_write_acl(acl, N);

    if (rc != 0) {
        snprintf(s, len, "DISK   WRITE ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    } else {
        snprintf(s, len, "DISK   WRITE ACL OK");
    }
}

/* Adds a card to the ACL.
 *
 */
bool acl_grant(uint32_t facility_code, uint32_t card) {
    static int N = sizeof(ACL) / sizeof(uint32_t);

    uint32_t v = (facility_code * 100000) + (card % 100000);

    for (int i = 0; i < N; i++) {
        if (ACL[i] == v) {
            return true;
        }
    }

    for (int i = 0; i < N; i++) {
        if (ACL[i] == 0xffffffff) {
            ACL[i] = v;
            return true;
        }
    }

    return false;
}

/* Removes a card from the ACL.
 *
 */
bool acl_revoke(uint32_t facility_code, uint32_t card) {
    static int N = sizeof(ACL) / sizeof(uint32_t);

    uint32_t v = (facility_code * 100000) + (card % 100000);
    bool revoked = false;

    for (int i = 0; i < N; i++) {
        if (ACL[i] == v) {
            ACL[i] = 0xffffffff;
            revoked = true;
        }
    }

    return revoked;
}

/* Checks a card against the ACL.
 *
 */
bool acl_allowed(uint32_t facility_code, uint32_t card) {
    static int N = sizeof(ACL) / sizeof(uint32_t);

    for (int i = 0; i < N; i++) {
        const uint32_t c = ACL[i];

        if ((c != 0xffffffff) && ((c / 100000) == facility_code) && ((c % 100000) == card)) {
            return true;
        }
    }

    return false;
}

/* Returns a list of valid cards
 *
 */
int acl_list(uint32_t *list[]) {
    static int N = sizeof(ACL) / sizeof(uint32_t);

    int count = 0;
    for (int i = 0; i < N; i++) {
        if (ACL[i] != 0xffffffff) {
            count++;
        }
    }

    if ((*list = calloc(count, sizeof(uint32_t))) != NULL) {
        int ix = 0;
        for (int i = 0; i < N; i++) {
            if (ACL[i] != 0xffffffff) {
                (*list)[ix++] = ACL[i];
            }
        }
    }

    return count;
}
