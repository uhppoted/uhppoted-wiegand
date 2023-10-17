#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <f_util.h>
#include <hardware/rtc.h>

#include "acl.h"
#include "common.h"
#include "logd.h"
#include "sdcard.h"
#include "sys.h"
#include "tcpd.h"
#include <TPIC6B595.h>
#include <buzzer.h>
#include <cli.h>
#include <uart.h>

typedef void (*handler)(uint32_t, uint32_t, txrx, void *);

void debug(txrx, void *, int);
void help(txrx, void *);
void query(txrx, void *);

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
            if (strncasecmp(cmd, "reboot", 6) == 0) {
                cli_reboot(f, context);
            } else if (strncasecmp(cmd, "time ", 5) == 0) {
                cli_set_time(&cmd[5], f, context);
            } else if (strncasecmp(cmd, "blink", 5) == 0) {
                cli_blink(f, context);
            } else if (strncasecmp(cmd, "query", 5) == 0) {
                query(f, context);
            } else if (strncasecmp(cmd, "unlock", 6) == 0) {
                cli_unlock_door(f, context);
            } else if (strncasecmp(cmd, "mount", 5) == 0) {
                mount(f, context);
            } else if (strncasecmp(cmd, "unmount", 7) == 0) {
                unmount(f, context);
            } else if (strncasecmp(cmd, "format", 6) == 0) {
                format(f, context);
            } else if (strncasecmp(cmd, "list acl", 8) == 0) {
                cli_acl_list(f, context);
            } else if (strncasecmp(cmd, "clear acl", 9) == 0) {
                cli_acl_clear(f, context);
            } else if (strncasecmp(cmd, "grant ", 6) == 0) {
                cli_acl_grant(&cmd[6], f, context);
            } else if (strncasecmp(cmd, "revoke ", 7) == 0) {
                cli_acl_revoke(&cmd[7], f, context);
            } else if (strncasecmp(cmd, "passcodes", 9) == 0) {
                cli_set_passcodes(&cmd[9], f, context);
            } else if (strncasecmp(cmd, "debugx", 7) == 0) {
                debug(f, context, 1);
            } else if (strncasecmp(cmd, "debugy", 7) == 0) {
                debug(f, context, 2);
            } else {
                help(f, context);
            }
        }
    }
}

/* -- DEBUG --
 *
 */
void debug(txrx f, void *context, int action) {
    // ... card
    if (action == 1) {
        uint32_t v = MSG_CARD | (0x0C9C841 & 0x03ffffff); // 10058400
        if (!queue_is_full(&queue)) {
            queue_try_add(&queue, &v);
        }

        f(context, ">> DEBUG CARD");
    }

    // ... keycode
    if (action == 2) {
        int N = 6;
        char *code;

        if ((code = calloc(N, 1)) != NULL) {
            snprintf(code, N, "%s", "12345");
            uint32_t msg = MSG_CODE | ((uint32_t)code & 0x0fffffff); // SRAM_BASE is 0x20000000
            if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
                free(code);
            } else {
                f(context, ">> DEBUG CODE");
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

/* Displays a list of the supported commands.
 *
 */
void help(txrx f, void *context) {
    f(context, "-----");
    f(context, "Commands:");
    f(context, "TIME YYYY-MM-DD HH:mm:ss  Set date/time (YYYY-MM-DD HH:mm:ss)");
    f(context, "");
    f(context, "LIST ACL                  List cards in ACL");
    f(context, "CLEAR ACL                 Revoke all cards in ACL");
    f(context, "GRANT nnnnnn              Grant card access rights");
    f(context, "REVOKE nnnnnn             Revoke card access rights");
    f(context, "PASSCODES                 Sets the override passcodes");
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
