#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f_util.h"
#include "ff.h"

#include "diskio.h"
#include "hw_config.h"

#include "../include/logd.h"
#include "../include/sdcard.h"
#include "../include/wiegand.h"

void spi0_dma_isr();
datetime_t string2date(const char *);

static void card_detect_callback(uint, uint32_t);

#define CLK 2
#define SI 3
#define SO 4
#define CS 5
#define DET 6

/* Internal state for the SDCARD.
 *
 */
struct {
    bool initialised;
} SDCARD = {
    .initialised = false,
};

// Configure SD card SPI
static spi_t spis[] = {
    {
        .hw_inst = spi0,
        .miso_gpio = SO,
        .mosi_gpio = SI,
        .sck_gpio = CLK,
        .baud_rate = 12500 * 1000,
        .dma_isr = spi0_dma_isr,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
    },
};

// Configure SD card  structs
static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",
        .spi = &spis[0],
        .ss_gpio = CS,
        .use_card_detect = true,
        .card_detect_gpio = DET,
        .card_detected_true = 1,
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .m_Status = STA_NOINIT,
    }};

void spi0_dma_isr() {
    spi_irq_handler(&spis[0]);
}

size_t sd_get_num() {
    return count_of(sd_cards);
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

size_t spi_get_num() {
    return count_of(spis);
}

spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

/* Initialises the SD card driver.
 *
 */
void sdcard_initialise(enum MODE mode, bool card_detect_enabled) {
    char s[64];

    if (sd_init_driver()) {
        SDCARD.initialised = true;

        sd_card_t *sdcard = sd_get_by_num(0);

        FRESULT fr = f_mount(&sdcard->fatfs, sdcard->pcName, 1);
        if (fr != FR_OK) {
            snprintf(s, sizeof(s), "DISK  MOUNT ERROR (%d) %s", fr, FRESULT_str(fr));
            logd_log(s);
        }

        // NTS: there is a weird unresolved conflict between the SD card detect interrupt
        //      USB and the CYW43 WiFi stack
        if (card_detect_enabled && sdcard->use_card_detect) {
            gpio_set_irq_enabled_with_callback(
                sdcard->card_detect_gpio,
                GPIO_IRQ_EDGE_FALL,
                true,
                &card_detect_callback);
        }
    } else {
        logd_log("DISK  INIT ERROR");
    }
}

/* Mounts SD card.
 *
 */
int sdcard_mount() {
    if (!SDCARD.initialised) {
        return -1;
    }

    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr = f_mount(&sdcard->fatfs, sdcard->pcName, 1);

    return fr == FR_OK ? 0 : fr;
}

/* Unmounts SD card.
 *
 */
int sdcard_unmount() {
    if (!SDCARD.initialised) {
        return -1;
    }

    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr;

    if ((fr = f_unmount(sdcard->pcName)) != FR_OK) {
        return fr;
    } else {
        sdcard->mounted = false;
        sdcard->m_Status |= STA_NOINIT;
        return 0;
    }
}

/* Format SD card.
 *
 */
int sdcard_format() {
    if (!SDCARD.initialised) {
        return -1;
    }

    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr = f_mkfs(sdcard->pcName, 0, 0, FF_MAX_SS * 2);

    return fr == FR_OK ? 0 : fr;
}

/* Reads the ACL file from the SD card.
 *
 */
int sdcard_read_acl(CARD cards[], int *N) {
    if (!SDCARD.initialised) {
        return -1;
    }

    FIL file;
    FRESULT fr;

    if ((fr = f_open(&file, "ACL", FA_READ | FA_OPEN_EXISTING)) != FR_OK) {
        return fr;
    }

    char buffer[256];
    int ix = 0;
    uint32_t card_number;
    while (f_gets(buffer, sizeof(buffer), &file) && ix < *N) {
        buffer[strcspn(buffer, "\n")] = 0;

        printf(">> READ %s\n", buffer);

        char *p = strtok(buffer, " ");

        if (p && ((card_number = strtol(p, NULL, 10)) != 0)) {
            cards[ix].card_number = card_number;
            cards[ix].start = string2date("2000-01-01");
            cards[ix].end = string2date("2000-12-31");
            cards[ix].allowed = false;
            snprintf(cards[ix].name, CARD_NAME_SIZE, "****");

            if ((p = strtok(NULL, " ")) != NULL) {
                cards[ix].start = string2date(p);
                if ((p = strtok(NULL, " ")) != NULL) {
                    cards[ix].end = string2date(p);
                    if ((p = strtok(NULL, " ")) != NULL) {
                        cards[ix].allowed = strncmp(p, "Y", 1) == 0 || strncmp(p, "y", 1) == 0;
                        if ((p = strtok(NULL, " ")) != NULL) {
                            snprintf(cards[ix].name, CARD_NAME_SIZE, p);
                        }
                    }
                }
            }

            ix++;
        }
    }

    *N = ix;

    return f_close(&file);
}

/* Writes a list of cards to the SD card ACL file.
 *
 */
int sdcard_write_acl(CARD cards[], int N) {
    if (!SDCARD.initialised) {
        return -1;
    }

    FIL file;
    FRESULT fr;

    if ((fr = f_open(&file, "ACL.tmp", FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK) {
        return fr;
    }

    int count = 0;
    for (int i = 0; i < 32 && count < N; i++) {
        CARD card = cards[i];
        char record[64];

        if (card.card_number != 0 && card.card_number != 0xffffffff) {
            snprintf(record,
                     sizeof(record),
                     "%-8u %04d-%02d-%02d %04d-%02d-%02d %c %s",
                     card.card_number,
                     card.start.year,
                     card.start.month,
                     card.start.day,
                     card.end.year,
                     card.end.month,
                     card.end.day,
                     card.allowed ? 'Y' : 'N',
                     card.name);

            if ((fr = f_printf(&file, "%s\n", record)) < 0) {
                f_close(&file);
                return fr;
            }

            count++;
        }
    }

    if ((fr = f_close(&file)) != FR_OK) {
        return fr;
    }

    // ... rename ACL.tmp to ACL file
    f_unlink("ACL.bak");

    fr = f_rename("ACL", "ACL.bak");
    if (fr != FR_OK && fr != FR_NO_FILE) {
        return fr;
    }

    return f_rename("ACL.tmp", "ACL");
}

static void card_detect_callback(uint gpio, uint32_t events) {
    static bool busy;

    if (SDCARD.initialised && !busy && !gpio_get(SD_DET)) {
        busy = true;

        sd_card_t *sdcard = sd_get_by_num(0);

        if (sdcard->card_detect_gpio == gpio) {
            if (sdcard->mounted) {
                FRESULT fr = f_unmount(sdcard->pcName);
                if (FR_OK == fr) {
                    sdcard->mounted = false;
                }
            }

            sdcard->m_Status |= STA_NOINIT; // in case medium is removed
            sd_card_detect(sdcard);
        }

        busy = false;
    }
}

datetime_t string2date(const char *s) {
    datetime_t dt = {
        .year = 2023,
        .month = 1,
        .day = 1,
        .dotw = 0,
        .hour = 0,
        .min = 0,
        .sec = 0,
    };

    if (s) {
        int year;
        int month;
        int day;
        int rc = sscanf(s, "%04d-%02d-%02d", &year, &month, &day);

        if (rc == 3) {
            dt.year = year;
            dt.month = month;
            dt.day = day;
        }
    }

    return dt;
}
