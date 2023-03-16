#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f_util.h"
#include "hardware/rtc.h"

#include "../include/TPIC6B595.h"
#include "../include/acl.h"
#include "../include/buzzer.h"
#include "../include/common.h"
#include "../include/led.h"
#include "../include/relays.h"
#include "../include/sdcard.h"
#include "../include/sys.h"
#include "../include/uart.h"
#include "../include/write.h"

typedef void (*handler)(uint32_t, uint32_t);

void query();
void reboot();
void help();

void on_card_command(char *cmd, handler fn);
void on_door_unlock();
void on_door_open();
void on_door_close();
void on_press_button();
void on_release_button();
void write(uint32_t, uint32_t);
void grant(uint32_t, uint32_t);
void revoke(uint32_t, uint32_t);
void list_acl();

void mount();
void unmount();
void format();
void read_acl();
void write_acl();

/* Executes a command.
 *
 */
void exec(char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        if (strncasecmp(cmd, "reboot", 6) == 0) {
            reboot();
        } else if (strncasecmp(cmd, "cls", 5) == 0) {
            cls();
            return; // don't invoke clearline because that puts a >> prompt in the wrong place
        } else if (strncasecmp(cmd, "blink", 5) == 0) {
            led_blink(5);
        } else if (strncasecmp(cmd, "unlock", 6) == 0) {
            on_door_unlock();
        } else if (strncasecmp(cmd, "query", 5) == 0) {
            query();
        } else if (strncasecmp(cmd, "open", 4) == 0) {
            on_door_open();
        } else if (strncasecmp(cmd, "close", 5) == 0) {
            on_door_close();
        } else if (strncasecmp(cmd, "press", 5) == 0) {
            on_press_button();
        } else if (strncasecmp(cmd, "release", 7) == 0) {
            on_release_button();
        } else if (strncasecmp(cmd, "grant ", 6) == 0) {
            on_card_command(&cmd[6], grant);
        } else if (strncasecmp(cmd, "revoke ", 7) == 0) {
            on_card_command(&cmd[7], revoke);
        } else if (strncasecmp(cmd, "read acl", 8) == 0) {
            read_acl();
        } else if (strncasecmp(cmd, "write acl", 9) == 0) {
            write_acl();
        } else if (strncasecmp(cmd, "list acl", 8) == 0) {
            list_acl();
        } else if (strncasecmp(cmd, "mount", 5) == 0) {
            mount();
        } else if (strncasecmp(cmd, "unmount", 7) == 0) {
            unmount();
        } else if (strncasecmp(cmd, "format", 6) == 0) {
            format();
        } else if ((cmd[0] == 'w') || (cmd[0] == 'W')) {
            on_card_command(&cmd[1], write);
        } else if ((cmd[0] == 't') || (cmd[0] == 'T')) {
            sys_settime(&cmd[1]);
        } else {
            help();
        }
    }

    clearline();
}

/* Displays the last read/write card, if any.
 *
 */
void query() {
    char s[64];
    cardf(&last_card, s, sizeof(s));
    puts(s);
}

/* Write card command.
 *  Extract the facility code and card number pushes it to the emulator queue.
 *
 */
void write(uint32_t facility_code, uint32_t card) {
    if ((mode == WRITER) || (mode == EMULATOR)) {
        write_card(facility_code, card);
    }
}

/* Adds a card number to the ACL.
 *
 */
void grant(uint32_t facility_code, uint32_t card) {
    char s[64];
    char c[16];

    snprintf(c, sizeof(c), "%u%05u", facility_code, card);

    if (acl_grant(facility_code, card)) {
        snprintf(s, sizeof(s), "CARD  %-8s %s", c, "GRANTED");
    } else {
        snprintf(s, sizeof(s), "CARD  %-8s %s", c, "ERROR");
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
        snprintf(s, sizeof(s), "CARD  %-8s %s", c, "REVOKED");
    } else {
        snprintf(s, sizeof(s), "CARD  %-8s %s", c, "ERROR");
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
        tx("ACL   NO CARDS");
    } else {
        for (int i = 0; i < N; i++) {
            char s[32];
            snprintf(s, sizeof(s), "ACL   %u", cards[i]);
            tx(s);
        }
    }

    free(cards);
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
        snprintf(s, sizeof(s), "DISK  MOUNT ERROR (%d) %s", mounted, FRESULT_str(mounted));
    } else {
        snprintf(s, sizeof(s), "DISK  MOUNTED");
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
        snprintf(s, sizeof(s), "DISK UNMOUNT ERROR (%d) %s", unmounted, FRESULT_str(unmounted));
    } else {
        snprintf(s, sizeof(s), "DISK UNMOUNTED");
    }

    tx(s);
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
        snprintf(s, sizeof(s), "DISK READ ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    } else {
        snprintf(s, sizeof(s), "DISK READ ACL OK (%d)", N);

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
        snprintf(s, sizeof(s), "DISK  WRITE ACL ERROR (%d) %s", rc, FRESULT_str(rc));
    } else {
        snprintf(s, sizeof(s), "DISK  WRITE ACL OK");
    }

    tx(s);
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
    tx("TYYYY-MM-DD HH:mm:ss  Set date/time");
    tx("");
    tx("Wnnnnnn               Write card to Wiegand-26 interface");
    tx("OPEN                  Opens door contact relay");
    tx("CLOSE                 Closes door contact relay");
    tx("PRESS                 Press pushbutton");
    tx("RELEASE               Release pushbutton");
    tx("");
    tx("GRANT nnnnnn          Grant card access rights");
    tx("REVOKE nnnnnn         Revoke card access rights");
    tx("READ ACL              Read ACL from SD card");
    tx("WRITE ACL             Write ACL to SD card");
    tx("LIST ACL              Lists the cards in the ACL");
    tx("QUERY                 Display last card read/write");
    tx("");
    tx("MOUNT                 Mount SD card");
    tx("UNMOUNT               Unmount SD card");
    tx("FORMAT                Format SD card");
    tx("");
    tx("UNLOCK                Unlocks door");
    tx("BLINK                 Blinks reader LED 5 times");
    tx("CLS                   Resets the terminal");
    tx("REBOOT                Reboot");
    tx("?                     Display list of commands");
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

/* Unlocks door lock for 5 seconds (READER/CONTROLLER mode only).
 *
 */
void on_door_unlock() {
    if ((mode == READER) || (mode == CONTROLLER)) {
        door_unlock(5000);
    }
}

/* Door contact emulation command handler.
 *  Opens/closes the door contact emulation relay (in reader mode only).
 *
 */
void on_door_open() {
    relay_door_contact(false);
}

void on_door_close(char *cmd) {
    relay_door_contact(true);
}

/* Pushbutton emulation command handler.
 *  Opens/closes the pushbutton emulation relay (in reader mode only).
 *
 */
void on_press_button() {
    relay_pushbutton(true);
}

void on_release_button() {
    relay_pushbutton(false);
}
