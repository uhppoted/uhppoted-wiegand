#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <f_util.h>
#include <hardware/rtc.h>

#include "acl.h"
#include "common.h"
#include "led.h"
#include "logd.h"
#include "relays.h"
#include "sdcard.h"
#include "sys.h"
#include "tcpd.h"
#include <TPIC6B595.h>
#include <buzzer.h>
#include <cli.h>
#include <uart.h>

typedef void (*handler)(uint32_t, uint32_t, txrx, void *);

void debug(txrx, void *);
void help(txrx, void *);
void query(txrx, void *);

void on_card_command(char *cmd, handler fn, txrx, void *);

void on_door_unlock(txrx, void *);

void grant(uint32_t, uint32_t, txrx, void *);
void revoke(uint32_t, uint32_t, txrx, void *);
void list_acl(txrx, void *);
void read_acl(txrx, void *);
void write_acl(txrx, void *);

void mount(txrx, void *);
void unmount(txrx, void *);
void format(txrx, void *);

void serial(void *context, const char *msg) {
    char s[100];
    snprintf(s, sizeof(s), ">  %s", msg);
    puts(s);
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
            if (strncasecmp(cmd, "time ", 5) == 0) {
                cli_set_time(&cmd[5], f, context);
            } else if (strncasecmp(cmd, "blink", 5) == 0) {
                led_blink(5);
            } else if (strncasecmp(cmd, "query", 5) == 0) {
                query(f, context);
            } else if (strncasecmp(cmd, "unlock", 6) == 0) {
                on_door_unlock(f, context);
            } else if (strncasecmp(cmd, "reboot", 6) == 0) {
                reboot(f, context);
            } else if (strncasecmp(cmd, "mount", 5) == 0) {
                mount(f, context);
            } else if (strncasecmp(cmd, "unmount", 7) == 0) {
                unmount(f, context);
            } else if (strncasecmp(cmd, "format", 6) == 0) {
                format(f, context);
            } else if (strncasecmp(cmd, "list acl", 8) == 0) {
                list_acl(f, context);
            } else if (strncasecmp(cmd, "read acl", 8) == 0) {
                read_acl(f, context);
            } else if (strncasecmp(cmd, "write acl", 9) == 0) {
                write_acl(f, context);
            } else if (strncasecmp(cmd, "grant ", 6) == 0) {
                on_card_command(&cmd[6], grant, f, context);
            } else if (strncasecmp(cmd, "revoke ", 7) == 0) {
                on_card_command(&cmd[7], revoke, f, context);
            } else if (strncasecmp(cmd, "debug", 5) == 0) {
                debug(f, context);
            } else {
                help(f, context);
            }
        }
    }
}

/* -- DEBUG --
 *
 */
void debug(txrx f, void *context) {
    int N = 6;
    char *s;

    if ((s = calloc(N, 1)) != NULL) {
        snprintf(s, N, "%s", "12345");
        uint32_t msg = MSG_CODE | ((uint32_t)s & 0x0fffffff); // SRAM_BASE is 0x20000000
        if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
            free(s);
        }
    }

    f(context, ">> DEBUG OK");
}

/* Displays the last read/write card, if any.
 *
 */
void query(txrx f, void *context) {
    char s[64];

    cardf(&last_card, s, sizeof(s), true);
    f(context, s);
}

/* Adds a card number to the ACL.
 *
 */
void grant(uint32_t facility_code, uint32_t card, txrx f, void *context) {
    char s[64];
    char c[16];

    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    if (acl_grant(facility_code, card)) {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "GRANTED");
    } else {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "ERROR");
    }

    f(context, s);
    logd_log(s);

    write_acl(f, context);
}

/* Removes a card number from the ACL.
 *
 */
void revoke(uint32_t facility_code, uint32_t card, txrx f, void *context) {
    char s[64];
    char c[16];

    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    if (acl_revoke(facility_code, card)) {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "REVOKED");
    } else {
        snprintf(s, sizeof(s), "CARD   %-8s %s", c, "ERROR");
    }

    f(context, s);
    logd_log(s);
    write_acl(f, context);
}

/* Lists the ACL cards.
 *
 */
void list_acl(txrx f, void *context) {
    uint32_t *cards;
    int N = acl_list(&cards);

    if (N == 0) {
        f(context, "ACL    NO CARDS");
        logd_log("ACL   NO CARDS");
    } else {
        for (int i = 0; i < N; i++) {
            char s[32];
            snprintf(s, sizeof(s), "ACL    %u", cards[i]);
            f(context, s);
        }
    }

    free(cards);
}

/* Loads an ACL from the SD card
 *
 */
void read_acl(txrx f, void *context) {
    int detected = gpio_get(SD_DET);
    char s[64];

    if (!detected) {
        f(context, "DISK   NO SDCARD");
        logd_log("DISK   NO SDCARD");
    }

    int rc = acl_load();
    if (rc < 0) {
        snprintf(s, sizeof(s), "ACL    READ ERROR (%d)", rc);
    } else {
        snprintf(s, sizeof(s), "ACL    READ OK (%d CARDS)", rc);
    }

    f(context, s);
}

/* Writes the ACL to the SD card
 *
 */
void write_acl(txrx f, void *context) {
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

/* Unlocks door lock for 5 seconds.
 *
 */
void on_door_unlock(txrx f, void *context) {
    if (mode == CONTROLLER) {
        door_unlock(5000);
    }
}

/* Displays a list of the supported commands.
 *
 */
void help(txrx f, void *context) {
    f(context, "-----");
    f(context, "Commands:");
    f(context, "TIME YYYY-MM-DD HH:mm:ss  Set date/time (YYYY-MM-DD HH:mm:ss)");
    f(context, "GRANT nnnnnn              Grant card access rights");
    f(context, "REVOKE nnnnnn             Revoke card access rights");
    f(context, "LIST ACL                  List cards in ACL");
    f(context, "READ ACL                  Read ACL from SD card");
    f(context, "WRITE ACL                 Write ACL to SD card");
    f(context, "QUERY                     Display last card read/write");
    f(context, "");
    f(context, "MOUNT                     Mount SD card");
    f(context, "UNMOUNT                   Unmount SD card");
    f(context, "FORMAT                    Format SD card");
    f(context, "");
    f(context, "UNLOCK                    Unlocks door");
    f(context, "LOCK                      Locks door");
    f(context, "BLINK                     Blinks reader LED 5 times");
    f(context, "CLS                       Resets the terminal");
    f(context, "REBOOT                    Reboot");
    f(context, "");
    f(context, "?                         Display list of commands");
    f(context, "-----");
}

/* Card command handler.
 *  Extract the facility code and card number and invokes the handler function.
 *
 */
void on_card_command(char *cmd, handler fn, txrx f, void *context) {
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

    fn(facility_code, card, f, context);
}

/* Mounts the SD card.
 *
 */
void mount(txrx f, void *context) {
    bool detected = gpio_get(SD_DET) == 1;
    int mounted = sdcard_mount();
    char s[64];

    if (!detected) {
        f(context, "DISK   NO SDCARD");
        logd_log("DISK   NO SDCARD");
    }

    if (mounted != 0) {
        snprintf(s, sizeof(s), "DISK   MOUNT ERROR (%d) %s", mounted, FRESULT_str(mounted));
    } else {
        snprintf(s, sizeof(s), "DISK   MOUNTED");
    }

    f(context, s);
    logd_log(s);
}

/* Unmounts the SD card.
 *
 */
void unmount(txrx f, void *context) {
    int unmounted = sdcard_unmount();
    int detected = gpio_get(SD_DET);
    char s[32];

    if (!detected) {
        f(context, "DISK   NO SDCARD");
        logd_log("DISK   NO SDCARD");
    }

    if (unmounted != 0) {
        snprintf(s, sizeof(s), "DISK   UNMOUNT ERROR (%d) %s", unmounted, FRESULT_str(unmounted));
    } else {
        snprintf(s, sizeof(s), "DISK   UNMOUNTED");
    }

    f(context, s);
    logd_log(s);
}

/* Formats the SD card.
 *
 */
void format(txrx f, void *context) {
    bool detected = gpio_get(SD_DET) == 1;
    int formatted = sdcard_format();
    char s[64];

    if (!detected) {
        f(context, "DISK   NO SDCARD");
        logd_log("DISK   NO SDCARD");
    }

    if (formatted != 0) {
        snprintf(s, sizeof(s), "DISK   FORMAT ERROR (%d) %s", formatted, FRESULT_str(formatted));
    } else {
        snprintf(s, sizeof(s), "DISK   FORMATTED");
    }

    f(context, s);
    logd_log(s);
}
