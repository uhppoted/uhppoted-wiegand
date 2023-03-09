#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f_util.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"

#include "TPIC6B595.h"
#include "acl.h"
#include "buzzer.h"
#include "led.h"
#include "relays.h"
#include "sdcard.h"
#include "sys.h"

#include "../include/cli.h"
#include "../include/controller.h"

typedef struct CLI {
    int ix;
    int32_t timer;
    char buffer[64];
} CLI;

typedef void (*handler)(uint32_t, uint32_t);

const char ESC = 27;
uint8_t height = 25;
const uint32_t CLI_TIMEOUT = 10000;

int64_t cli_timeout(alarm_id_t, void *);
void echo(const char *);
void clearline();

void exec(char *);
void cpr(char *);
void query();
void reboot();
void help();

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

/* Clears the screen
 *
 */
void clear_screen() {
    char s[24];
    snprintf(s, sizeof(s), "\033[2J");
    fputs(s, stdout);
    fflush(stdout);
}

/* Requests terminal window size - the scroll area is set from the response.
 *
 */
void set_scroll_area() {
    char s[24];
    snprintf(s, sizeof(s), "\0337\033[999;999H\033[6n\0338");
    fputs(s, stdout);
    fflush(stdout);
}

/* Saves the cursor position, clears the command line, redisplays the prompt and then
 * restores the cursor position.
 *
 */
void clearline() {
    char s[24];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> \033[0K\0338", height);
    fputs(s, stdout);
    fflush(stdout);
}

/** Processes received UART characters.
 *
 */
void rx(char *received) {
    static CLI cli = {
        .ix = 0,
        .timer = -1,
        .buffer = {0},
    };

    if (cli.timer > 0) {
        cancel_alarm(cli.timer);
        cli.timer = 0;
    }

    int N = strlen(received);
    for (int i = 0; i < N; i++) {
        uint8_t ch = received[i];

        // CRLF ?
        if (ch == '\n' || ch == '\r') {
            if (cli.ix > 0) {
                exec(cli.buffer);
            }

            memset(cli.buffer, 0, sizeof(cli.buffer));
            cli.ix = 0;
            continue;
        }

        // VT100 escape code?
        if (cli.buffer[0] == 27 && ch == 'R' && (cli.ix < sizeof(cli.buffer) - 1)) { // VT100 cursor position report
            cli.buffer[cli.ix++] = ch;
            cli.buffer[cli.ix] = 0;

            cpr(&cli.buffer[1]);
            memset(cli.buffer, 0, sizeof(cli.buffer));
            cli.ix = 0;
            continue;
        }

        // backspace?
        if (ch == 8) {
            if (cli.ix > 0) {
                cli.buffer[--cli.ix] = 0;
                echo(cli.buffer);
            }

            if (cli.ix > 0) {
                cli.timer = add_alarm_in_ms(CLI_TIMEOUT, cli_timeout, (CLI *)&cli, true);
            }

            continue;
        }

        // Add character to buffer
        if (cli.ix < sizeof(cli.buffer) - 1) {
            cli.buffer[cli.ix++] = ch;
            cli.buffer[cli.ix] = 0;

            // ... echo if normal commnad and not a VT100 code
            if (cli.buffer[0] != 27) {
                echo(cli.buffer);
            }

            cli.timer = add_alarm_in_ms(CLI_TIMEOUT, cli_timeout, (CLI *)&cli, true);
            continue;
        }
    }
}

/* Adds a message to the queue.
 *
 */
void tx(char *message) {
    static int size = 128;
    char *s;

    if ((s = calloc(size, 1)) != NULL) {
        datetime_t now;
        int N;
        uint32_t msg;

        rtc_get_datetime(&now);

        N = timef(now, s, size);

        snprintf(&s[N], size - N, "  %s", message);

        msg = MSG_TX | ((uint32_t)s & 0x0fffffff); // SRAM_BASE is 0x20000000
        if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
            free(message);
        }
    }
}

/* Timeout handler. Clears the current command and command line.
 *
 */
int64_t cli_timeout(alarm_id_t id, void *data) {
    CLI *cli = (CLI *)data;
    memset(cli->buffer, 0, sizeof(cli->buffer));
    cli->ix = 0;
    cli->timer = 0;

    clearline();

    return 0;
}

/* Saves the cursor position, displays the current command buffer and then restores
 *  the cursor position
 *
 */
void echo(const char *cmd) {
    char s[64];
    snprintf(s, sizeof(s), "\0337\033[%d;0H>> %s\033[0K\0338", height, cmd);
    fputs(s, stdout);
    fflush(stdout);
}

/* Executes a command.
 *
 */
void exec(char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        if (strncasecmp(cmd, "blink", 5) == 0) {
            led_blink(5);
        } else if (strncasecmp(cmd, "query", 5) == 0) {
            query();
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
            help();
        }
    }

    clearline();
}

/* Cursor position report.
 *  Sets the scrolling window with space at the bottom for the command echo.
 *
 */
void cpr(char *cmd) {
    int rows;
    int cols;
    int rc = sscanf(cmd, "[%d;%dR", &rows, &cols);

    if (rc > 0) {
        height = rows - 1;

        char s[24];
        snprintf(s, sizeof(s), "\0337\033[0;%dr\0338", height - 2);
        fputs(s, stdout);
        fflush(stdout);
    }
}

/* Displays the last read/write card, if any.
 *
 */
void query() {
    char s[64];
    cardf(&last_card, s, sizeof(s));
    puts(s);
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
void help() {
    static uint8_t x = 0x00;

    TPIC_set(RED_LED, (x & 0x01) == 0x01);
    TPIC_set(ORANGE_LED, (x & 0x02) == 0x02);
    TPIC_set(YELLOW_LED, (x & 0x04) == 0x04);
    TPIC_set(GREEN_LED, (x & 0x08) == 0x08);

    x++;

    tx("-----");
    tx("Commands:");
    tx("T              Set date/time (YYYY-MM-DD HH:mm:ss)");
    tx("GRANT nnnnnn   Grant card access rights");
    tx("REVOKE nnnnnn  Revoke card access rights");
    tx("CLnnnnnn       List cards in ACL");
    tx("QUERY          Display last card read/write");
    tx("BLINK          Blinks reader LED 5 times");
    tx("UNLOCK         Unlocks door");
    tx("LOCK           Locks door");
    tx("MOUNT          Mount SD card");
    tx("UNMOUNT        Unmount SD card");
    tx("FORMAT         Format SD card");
    tx("READ ACL       Read ACL from SD card");
    tx("WRITE ACL      Write ACL to SD card");
    tx("REBOOT         Reboot");
    tx("?              Display list of commands");
    tx("-----");
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
