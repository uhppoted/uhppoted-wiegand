#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <TPIC6B595.h>
#include <acl.h>
#include <buzzer.h>
#include <cli.h>
#include <led.h>
#include <logd.h>
#include <relays.h>
#include <sys.h>
#include <wiegand.h>
#include <write.h>

/* Goes into a tight loop until the watchdog resets the processor.
 *
 */
void cli_reboot(txrx f, void *context) {
    while (true) {
        buzzer_beep(1);

        TPIC_set(RED_LED, false);
        TPIC_set(YELLOW_LED, false);
        TPIC_set(ORANGE_LED, false);
        TPIC_set(GREEN_LED, false);
        sleep_ms(750);

        TPIC_set(RED_LED, true);
        TPIC_set(YELLOW_LED, true);
        TPIC_set(ORANGE_LED, true);
        TPIC_set(GREEN_LED, true);
        sleep_ms(750);
    }
}

/* Sets the system time.
 *
 */
void cli_set_time(char *cmd, txrx f, void *context) {
    sys_settime(cmd);

    f(context, ">> SET TIME OK");
}

/* Blinks the blue LED.
 *
 */
void cli_blink(txrx f, void *context) {
    led_blink(5);
}

/* Unlocks door lock for 5 seconds.
 *
 */
void cli_unlock_door(txrx f, void *context) {
    if (mode == CONTROLLER) {
        door_unlock(5000);
    }
}

/* Door contact emulation command handler.
 *  Opens/closes the door contact emulation relay (in reader mode only).
 *
 */
void cli_on_door_open(txrx f, void *context) {
    relay_door_contact(false);
    f(context, ">> OPENED");
}

void cli_on_door_close(txrx f, void *context) {
    relay_door_contact(true);
    f(context, ">> CLOSED");
}

/* Lists the ACL cards.
 *
 */
void cli_acl_list(txrx f, void *context) {
    uint32_t *cards;
    int N = acl_list(&cards);

    if (N == 0) {
        f(context, "ACL    NO CARDS");
        logd_log("ACL    NO CARDS");
    } else {
        for (int i = 0; i < N; i++) {
            char s[32];
            snprintf(s, sizeof(s), "ACL    %u", cards[i]);
            f(context, s);
        }
    }

    free(cards);
}

/* Removes all cards from the ACL.
 *
 */
void cli_acl_clear(txrx f, void *context) {
    char s[64];

    bool ok = acl_clear();

    if (ok) {
        snprintf(s, sizeof(s), "ACL    CLEARED");
    } else {
        snprintf(s, sizeof(s), "ACL    CLEAR ERROR");
    }

    f(context, s);
    logd_log(s);

    if (ok) {
        cli_acl_write(f, context);
    }
}

/* Writes the ACL to flash/SD card.
 *
 */
void cli_acl_write(txrx f, void *context) {
    char s[64];

    if (!gpio_get(SD_DET)) {
        f(context, "DISK NO SDCARD");
        logd_log("DISK NO SDCARD");
    }

    int rc = acl_save();

    if (rc < 0) {
        snprintf(s, sizeof(s), "DISK   ACL WRITE ERROR (%d)", rc);
    } else {
        snprintf(s, sizeof(s), "DISK   ACL WRITE OK (%d)", rc);
    }

    f(context, s);
}

/* Grants card permissions.
 *  Extract the facility code and card number and (optional) PIN and updates
 *  the ACL.
 *
 */
void cli_acl_grant(char *cmd, txrx f, void *context) {
    // ... extract facility code, card and (optional) PIN code
    uint32_t facility_code = FACILITY_CODE;
    uint32_t card = 0;
    char *PIN = "";

    // ... facility code and card
    char *p = strtok(cmd, " ");

    if (p != NULL) {
        int N = strlen(p);
        int rc;

        if (N < 5) {
            if ((rc = sscanf(p, "%0u", &card)) < 1) {
                return;
            }
        } else {
            if ((rc = sscanf(&p[N - 5], "%05u", &card)) < 1) {
                return;
            }

            if (N == 6 && ((rc = sscanf(p, "%01u", &facility_code)) < 1)) {
                return;
            } else if (N == 7 && ((rc = sscanf(p, "%02u", &facility_code)) < 1)) {
                return;
            } else if (N == 8 && ((rc = sscanf(p, "%03u", &facility_code)) < 1)) {
                return;
            } else if (N > 8) {
                return;
            }
        }

        // ... optional PIN code
        if ((p = strtok(NULL, " ")) != NULL) {
            PIN = p;
        }

        // ... grant access
        char s[64];
        bool ok = acl_grant(facility_code, card, PIN);

        if (ok) {
            snprintf(s, sizeof(s), "CARD   %u%05u %s", facility_code, card, "GRANTED");
        } else {
            snprintf(s, sizeof(s), "CARD   %u%05u %s", facility_code, card, "ERROR");
        }

        f(context, s);
        logd_log(s);

        if (ok) {
            cli_acl_write(f, context);
        }
    }
}

/* Revokes card permissions.
 *  Extract the facility code and card number and updates the ACL.
 *
 */
void cli_acl_revoke(char *cmd, txrx f, void *context) {
    uint32_t facility_code = FACILITY_CODE;
    uint32_t card = 0;
    int N = strlen(cmd);
    int rc;

    if (N < 5) {
        if ((rc = sscanf(cmd, "%0u", &card)) < 1) {
            return;
        }
    } else {
        if ((rc = sscanf(&cmd[N - 5], "%05u", &card)) < 1) {
            return;
        }

        if (N == 6 && ((rc = sscanf(cmd, "%01u", &facility_code)) < 1)) {
            return;
        } else if (N == 7 && ((rc = sscanf(cmd, "%02u", &facility_code)) < 1)) {
            return;
        } else if (N == 8 && ((rc = sscanf(cmd, "%03u", &facility_code)) < 1)) {
            return;
        } else if (N > 8) {
            return;
        }
    }

    char s[64];
    bool ok = acl_revoke(facility_code, card);

    if (ok) {
        snprintf(s, sizeof(s), "CARD   %u%05u %s", facility_code, card, "REVOKED");
    } else {
        snprintf(s, sizeof(s), "CARD   %u%05u %s", facility_code, card, "ERROR");
    }

    f(context, s);
    logd_log(s);

    if (ok) {
        cli_acl_write(f, context);
    }
}

/* Sets the override passcodes.
 *
 */
void cli_set_passcodes(char *cmd, txrx f, void *context) {
    uint32_t passcodes[] = {0, 0, 0, 0};
    int ix = 0;
    char *token = strtok(cmd, " ,");

    while (token != NULL && ix < 4) {
        char *end = NULL;
        unsigned long v = strtoul(token, &end, 10);

        if (v > 0 && v < 1000000) {
            passcodes[ix] = v;
        }

        token = strtok(NULL, " ,");
        ix++;
    }

    if (acl_set_passcodes(passcodes[0], passcodes[1], passcodes[2], passcodes[3])) {
        f(context, "ACL    SET PASSCODES OK");
    } else {
        f(context, "ACL    SET PASSCODES ERROR");
    }
}

/* Card swipe + (optional) keycode emulation.
 *  Extract the facility code, card number and (optionally) the keycode from
 *  the command and writes the card and keycode (if present) to the Wiegand
 *  interface.
 */
void cli_swipe(char *cmd, txrx f, void *context) {
    uint32_t facility_code = FACILITY_CODE;
    uint32_t card = 0;
    char *token = strtok(cmd, " ,");
    char *code;

    if (token != NULL) {
        int N = strlen(token);

        if (N < 5) {
            if (sscanf(cmd, "%0u", &card) < 1) {
                return;
            }
        } else {
            if (sscanf(&token[N - 5], "%05u", &card) < 1) {
                return;
            }

            if (N == 6 && sscanf(token, "%01u", &facility_code) < 1) {
                return;
            } else if (N == 7 && sscanf(token, "%02u", &facility_code) < 1) {
                return;
            } else if (N == 8 && sscanf(token, "%03u", &facility_code) < 1) {
                return;
            } else if (N > 8) {
                return;
            }
        }

        if (mode == EMULATOR) {
            if ((code = strtok(NULL, " ,")) == NULL) {
                write_card(facility_code, card);
                f(context, "CARD   WRITE OK");
                logd_log("CARD   WRITE OK");
            } else {
                write_card(facility_code, card);
                keypad(code, f, context);

                f(context, "CARD+  WRITE OK");
                logd_log("CARD+  WRITE OK");
            }
        }
    }
}

/* Keypad emulation.
 *  Sends the keycode code as 4-bit burst mode Wiegand.
 *
 */
void keypad(char *cmd, txrx f, void *context) {
    int N = strlen(cmd);

    if ((mode == EMULATOR) && N > 0) {
        char s[64];

        for (int i = 0; i < N; i++) {
            char key = cmd[i];

            write_keycode(key);
        }

        snprintf(s, sizeof(s), "KEYPAD OK");
        f(context, s);
        logd_log(s);
    }
}
