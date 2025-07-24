#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <M5.h>
#include <log.h>
#include <wiegand.h>

#define LOGTAG "API"

const char *swipe(char *msg) {
    char *token = strtok(msg, " ,");

    if (token != NULL) {
        uint32_t u32;

        if (sscanf(msg, "%u", &u32) < 1) {
            return ERR_BAD_REQUEST;
        }

        uint32_t facility_code = u32 / 100000;
        uint32_t card = u32 % 100000;
        char *code;

        if (facility_code < 1 || facility_code > 255 || card > 65535) {
            return ERR_BAD_REQUEST;
        }

        if (!write_card(facility_code, card)) {
            debugf(LOGTAG, "card %u%05u error", facility_code, card);
            return ERR_WRITE;
        }

        if ((code = strtok(NULL, " ,")) != NULL) {
            int N = strlen(code);
            for (int i = 0; i < N; i++) {
                if (!write_keycode(code[i])) {
                    debugf(LOGTAG, "keycode %c error", code[i]);
                    return ERR_WRITE;
                }
            }
        }

        return ERR_OK;
    }

    return "";
}

const char *keycode(char *msg) {
    int N = strlen(msg);

    if (N > 0) {
        for (int i = 0; i < N; i++) {
            if (!write_keycode(msg[i])) {
                debugf(LOGTAG, "keycode %c error", msg[i]);
                return ERR_WRITE;
            }
        }

        return ERR_OK;
    }

    return "";
}
