#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f_util.h"
#include "hardware/rtc.h"

#include "TPIC6B595.h"
#include "acl.h"
#include "buzzer.h"
#include "common.h"
#include "logd.h"
#include "relays.h"
#include "sdcard.h"
#include "sys.h"
#include "tcpd.h"
#include "uart.h"
#include "write.h"

typedef void (*handler)(uint32_t, uint32_t, txrx, void *);

void help(txrx, void *);
void query(txrx, void *);

void on_card_command(char *, handler, txrx, void *);

void on_press_button(txrx, void *);
void on_release_button(txrx, void *);

void revoke(uint32_t, uint32_t, txrx, void *);
void read_acl(txrx, void *);

void mount(txrx, void *);
void unmount(txrx, void *);
void format(txrx, void *);

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
            if (strncasecmp(cmd, "reboot", 6) == 0) {
                cli_reboot(f, context);
            } else if (strncasecmp(cmd, "time ", 5) == 0) {
                cli_set_time(&cmd[5], f, context);
            } else if (strncasecmp(cmd, "blink", 5) == 0) {
                cli_blink(f, context);
            } else if (strncasecmp(cmd, "unlock", 6) == 0) {
                cli_unlock_door(f, context);
            } else if (strncasecmp(cmd, "query", 5) == 0) {
                query(f, context);
            } else if (strncasecmp(cmd, "open", 4) == 0) {
                cli_on_door_open(f, context);
            } else if (strncasecmp(cmd, "close", 5) == 0) {
                cli_on_door_close(f, context);
            } else if (strncasecmp(cmd, "press", 5) == 0) {
                on_press_button(f, context);
            } else if (strncasecmp(cmd, "release", 7) == 0) {
                on_release_button(f, context);
            } else if (strncasecmp(cmd, "list acl", 8) == 0) {
                cli_acl_list(f, context);
            } else if (strncasecmp(cmd, "clear acl", 9) == 0) {
                cli_acl_clear(f, context);
            } else if (strncasecmp(cmd, "grant ", 6) == 0) {
                cli_acl_grant(&cmd[6], f, context);
            } else if (strncasecmp(cmd, "revoke ", 7) == 0) {
                on_card_command(&cmd[7], revoke, f, context);
            } else if (strncasecmp(cmd, "read acl", 8) == 0) {
                read_acl(f, context);
            } else if (strncasecmp(cmd, "write acl", 9) == 0) {
                cli_acl_write(f, context);
            } else if (strncasecmp(cmd, "mount", 5) == 0) {
                mount(f, context);
            } else if (strncasecmp(cmd, "unmount", 7) == 0) {
                unmount(f, context);
            } else if (strncasecmp(cmd, "format", 6) == 0) {
                format(f, context);
            } else if (strncasecmp(cmd, "card ", 5) == 0) {
                cli_swipe(&cmd[5], f, context);
            } else if (strncasecmp(cmd, "code ", 5) == 0) {
                keypad(&cmd[1], f, context);
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

    cardf(&last_card, s, sizeof(s), true);
    f(context, s);
}

/* Removes a card number from the ACL.
 *
 */
void revoke(uint32_t facility_code, uint32_t card, txrx f, void *context) {
    char s[64];
    char c[16];

    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    if (acl_revoke(facility_code, card)) {
        snprintf(s, sizeof(s), "CARD    %-8s %s", c, "REVOKED");
    } else {
        snprintf(s, sizeof(s), "CARD    %-8s %s", c, "ERROR");
    }

    f(context, s);
    logd_log(s);

    cli_acl_write(f, context);
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
        snprintf(s, sizeof(s), "DISK  MOUNT ERROR (%d) %s", mounted, FRESULT_str(mounted));
    } else {
        snprintf(s, sizeof(s), "DISK  MOUNTED");
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
        snprintf(s, sizeof(s), "DISK UNMOUNT ERROR (%d) %s", unmounted, FRESULT_str(unmounted));
    } else {
        snprintf(s, sizeof(s), "DISK UNMOUNTED");
    }

    f(context, s);
    logd_log(s);
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

/* Displays a list of the supported commands.
 *
 */
void help(txrx f, void *context) {
    f(context, "-----");
    f(context, "Commands:");
    f(context, "TIME YYYY-MM-DD HH:mm:ss  Set date/time");
    f(context, "");
    f(context, "CARD nnnnnn dddddd        Write card + (optional) keycode to the Wiegand interface");
    f(context, "CODE dddddd               Enter keypad digits");
    f(context, "OPEN                      Opens door contact relay");
    f(context, "CLOSE                     Closes door contact relay");
    f(context, "PRESS                     Press pushbutton");
    f(context, "RELEASE                   Release pushbutton");
    f(context, "");
    f(context, "GRANT nnnnnn              Grant card access rights");
    f(context, "REVOKE nnnnnn             Revoke card access rights");
    f(context, "LIST ACL                  Lists the cards in the ACL");
    f(context, "CLEAR ACL                 Revoke all cards in ACL");
    f(context, "READ ACL                  Read ACL from SD card");
    f(context, "WRITE ACL                 Write ACL to SD card");
    f(context, "QUERY                     Display last card read/write");
    f(context, "");
    f(context, "MOUNT                     Mount SD card");
    f(context, "UNMOUNT                   Unmount SD card");
    f(context, "FORMAT                    Format SD card");
    f(context, "");
    f(context, "UNLOCK                    Unlocks door");
    f(context, "BLINK                     Blinks reader LED 5 times");
    f(context, "CLS                       Resets the terminal");
    f(context, "REBOOT                    Reboot");
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

/* Pushbutton emulation command handler.
 *  Opens/closes the pushbutton emulation relay (in reader mode only).
 *
 */
void on_press_button(txrx f, void *context) {
    relay_pushbutton(true);
}

void on_release_button(txrx f, void *context) {
    relay_pushbutton(false);
}