#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f_util.h"
#include "hardware/rtc.h"

#include "TPIC6B595.h"
#include "acl.h"
#include "buzzer.h"
#include "common.h"
#include "led.h"
#include "relays.h"
#include "sdcard.h"
#include "sys.h"
#include "tcpd.h"
#include "uart.h"

typedef void (*handler)(uint32_t, uint32_t);

void help(txrx f, void *context);
void query(txrx f, void *context);
void reboot();

void on_card_command(char *cmd, handler fn);
void on_door_unlock();

void grant(uint32_t, uint32_t);
void revoke(uint32_t, uint32_t);
void list_acl();
void read_acl();
void write_acl();

void mount();
void unmount();
void format();

void serial(void *context, const char *msg) {
    puts(msg);
}

/* Executes a command from the TTY terminal.
 *
 */
void exec(char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        if (strncasecmp(cmd, "cls", 3) == 0) {
            cls();
            return; // don't invoke clearline because that puts a >> prompt in the wrong place
        } else {
            execw(cmd, serial, NULL);
        }
    }

    clearline();
}

/* Executes a command.
 *
 */
void execw(char *cmd, txrx f, void *context) {
    if (f != NULL) {
        int N = strlen(cmd);

        if (N > 0) {
            if (strncasecmp(cmd, "blink", 5) == 0) {
                led_blink(5);
            } else if (strncasecmp(cmd, "query", 5) == 0) {
                query(f, context);
            } else if (strncasecmp(cmd, "unlock", 6) == 0) {
                on_door_unlock();
            } else if (strncasecmp(cmd, "reboot", 6) == 0) {
                reboot();
            } else if (strncasecmp(cmd, "mount", 5) == 0) {
                mount();
            } else if (strncasecmp(cmd, "unmount", 7) == 0) {
                unmount();
            } else if (strncasecmp(cmd, "format", 6) == 0) {
                format();
            } else if (strncasecmp(cmd, "list acl", 8) == 0) {
                list_acl();
            } else if (strncasecmp(cmd, "read acl", 8) == 0) {
                read_acl();
            } else if (strncasecmp(cmd, "write acl", 9) == 0) {
                write_acl();
            } else if (strncasecmp(cmd, "grant ", 6) == 0) {
                on_card_command(&cmd[6], grant);
            } else if (strncasecmp(cmd, "revoke ", 7) == 0) {
                on_card_command(&cmd[7], revoke);
            } else if ((cmd[0] == 't') || (cmd[0] == 'T')) {
                sys_settime(&cmd[1]);
            } else {
                help(f, context);
            }
        }
    }
}

/* Displays the last read/write card, if any.
 *
 */
void query(txrx f, void *context) {
    char s[64];

    cardf(&last_card, s, sizeof(s));
    f(context, s);
}

/* Adds a card number to the ACL.
 *
 */
void grant(uint32_t facility_code, uint32_t card) {
    char s[64];
    char c[16];

    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    if (acl_grant(facility_code, card)) {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "GRANTED");
    } else {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "ERROR");
    }

    tx(s);
    write_acl();
}

/* Removes a card number from the ACL.
 *
 */
void revoke(uint32_t facility_code, uint32_t card) {
    char s[64];
    char c[16];

    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    if (acl_revoke(facility_code, card)) {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "REVOKED");
    } else {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "ERROR");
    }

    tx(s);
    write_acl();
}

/* Lists the ACL cards.
 *
 */
void list_acl() {
    uint32_t *cards;
    int N = acl_list(&cards);

    if (N == 0) {
        tx("ACL    NO CARDS");
    } else {
        for (int i = 0; i < N; i++) {
            char s[32];
            snprintf(s, sizeof(s), "ACL    %u", cards[i]);
            tx(s);
        }
    }

    free(cards);
}

/* Loads an ACL from the SD card
 *
 */
void read_acl() {
    uint32_t cards[16];
    int N = 16;
    int rc = sdcard_read_acl(cards, &N);
    int detected = gpio_get(SD_DET);
    char s[32];

    if (!detected) {
        tx("DISK NO SDCARD");
    }

    if (rc != 0) {
        snprintf(s, sizeof(s), "DISK   READ ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    } else {
        snprintf(s, sizeof(s), "DISK   READ ACL OK (%d)", N);

        acl_initialise(cards, N);
    }

    tx(s);
}

/* Writes the ACL to the SD card
 *
 */
void write_acl() {
    uint32_t *cards;
    int N = acl_list(&cards);
    char s[32];
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

    if (!gpio_get(SD_DET)) {
        tx("DISK NO SDCARD");
    }

    if (rc != 0) {
        snprintf(s, sizeof(s), "DISK   WRITE ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    } else {
        snprintf(s, sizeof(s), "DISK   WRITE ACL OK");
    }

    tx(s);
}

/* Unlocks door lock for 5 seconds.
 *
 */
void on_door_unlock() {
    if (mode == CONTROLLER) {
        door_unlock(5000);
    }
}

/* Goes into a tight loop until the watchdog resets the processor.
 *
 */
void reboot() {
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

/* Displays a list of the supported commands.
 *
 */
void help(txrx f, void *context) {
    f(context, "-----");
    f(context, "Commands:");
    f(context, "TYYYY-MM-DD HH:mm:ss  Set date/time (YYYY-MM-DD HH:mm:ss)");
    f(context, "GRANT nnnnnn          Grant card access rights");
    f(context, "REVOKE nnnnnn         Revoke card access rights");
    f(context, "LIST ACL              List cards in ACL");
    f(context, "READ ACL              Read ACL from SD card");
    f(context, "WRITE ACL             Write ACL to SD card");
    f(context, "QUERY                 Display last card read/write");
    f(context, "");
    f(context, "MOUNT                 Mount SD card");
    f(context, "UNMOUNT               Unmount SD card");
    f(context, "FORMAT                Format SD card");
    f(context, "");
    f(context, "UNLOCK                Unlocks door");
    f(context, "LOCK                  Locks door");
    f(context, "BLINK                 Blinks reader LED 5 times");
    f(context, "CLS                   Resets the terminal");
    f(context, "REBOOT                Reboot");
    f(context, "");
    f(context, "?                     Display list of commands");
    f(context, "-----");
}

/* Card command handler.
 *  Extract the facility code and card number and invokes the handler function.
 *
 */
void on_card_command(char *cmd, handler fn) {
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

    fn(facility_code, card);
}

/* Mounts the SD card.
 *
 */
void mount() {
    bool detected = gpio_get(SD_DET) == 1;
    int mounted = sdcard_mount();
    char s[64];

    if (!detected) {
        tx("DISK  NO SDCARD");
    }

    if (mounted != 0) {
        snprintf(s, sizeof(s), "DISK   MOUNT ERROR (%d) %s", mounted, FRESULT_str(mounted));
    } else {
        snprintf(s, sizeof(s), "DISK   MOUNTED");
    }

    tx(s);
}

/* Unmounts the SD card.
 *
 */
void unmount() {
    int unmounted = sdcard_unmount();
    int detected = gpio_get(SD_DET);
    char s[32];

    if (!detected) {
        tx("DISK NO SDCARD");
    }

    if (unmounted != 0) {
        snprintf(s, sizeof(s), "DISK  UNMOUNT ERROR (%d) %s", unmounted, FRESULT_str(unmounted));
    } else {
        snprintf(s, sizeof(s), "DISK  UNMOUNTED");
    }

    tx(s);
}

/* Formats the SD card.
 *
 */
void format() {
    bool detected = gpio_get(SD_DET) == 1;
    int formatted = sdcard_format();
    char s[64];

    if (!detected) {
        tx("DISK  NO SDCARD");
    }

    if (formatted != 0) {
        snprintf(s, sizeof(s), "DISK  FORMAT ERROR (%d) %s", formatted, FRESULT_str(formatted));
    } else {
        snprintf(s, sizeof(s), "DISK  FORMATTED");
    }

    tx(s);
}
