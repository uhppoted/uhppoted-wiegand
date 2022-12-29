#include "../include/acl.h"
#include "../include/wiegand.h"

uint32_t ACL[32] = {};

/* Initialises the ACL.
 *
 */
void acl_initialise() {
    static int N = sizeof(ACL) / sizeof(uint32_t);

    for (int i = 0; i < N; i++) {
        ACL[i] = 0xffffffff;
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
