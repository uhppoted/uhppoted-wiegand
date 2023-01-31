#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f_util.h"
#include "ff.h"

#include "diskio.h"
#include "hw_config.h"

#include "../include/cli.h"
#include "../include/sdcard.h"
#include "../include/wiegand.h"

void spi0_dma_isr();
static void card_detect_callback(uint, uint32_t);

#define CLK 2
#define SI 3
#define SO 4
#define CS 5
#define DET 6

// Configure SD card SPI
static spi_t spis[] = {
    {
        .hw_inst = spi0,
        .miso_gpio = SO,
        .mosi_gpio = SI,
        .sck_gpio = CLK,
        .baud_rate = 12500 * 1000, // 12500 * 1000,
        .dma_isr = spi0_dma_isr,
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
void sdcard_initialise(enum MODE mode) {
    if (sd_init_driver()) {
        int rc;
        char s[64];

        if ((rc = sdcard_mount()) != 0) {
            snprintf(s, sizeof(s), "DISK MOUNT ERROR (%d) %s", rc, FRESULT_str(rc));
            tx(s);
        }

        sd_card_t *sdcard = sd_get_by_num(0);

        if (sdcard->use_card_detect) {
            gpio_set_irq_enabled_with_callback(
                sdcard->card_detect_gpio,
                GPIO_IRQ_EDGE_FALL,
                true,
                &card_detect_callback);
        }
    } else {
        tx("DISK INIT ERROR");
    }
}

/* Mounts SD card.
 *
 */
int sdcard_mount() {
    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr = f_mount(&sdcard->fatfs, sdcard->pcName, 1);

    return fr == FR_OK ? 0 : fr;
}

/* Unmounts SD card.
 *
 */
int sdcard_unmount() {
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
    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr = f_mkfs(sdcard->pcName, 0, 0, FF_MAX_SS * 2);

    return fr == FR_OK ? 0 : fr;
}

/* Reads the ACL file from the SD card.
 *
 */
int sdcard_read_acl(uint32_t cards[], int *N) {
    FIL file;
    FRESULT fr;

    if ((fr = f_open(&file, "ACL", FA_READ | FA_OPEN_EXISTING)) != FR_OK) {
        return fr;
    }

    char buffer[256];
    int count = 0;
    uint32_t card;
    while (f_gets(buffer, sizeof(buffer), &file) && count < *N) {
        buffer[strcspn(buffer, "\n")] = 0;
        if ((card = strtol(buffer, NULL, 10)) != 0) {
            cards[count++] = card;
        }
    }

    *N = count;

    return f_close(&file);
}

/* Writes a list of cards to the SD card ACL file.
 *
 */
int sdcard_write_acl(CARD cards[], int N) {
    FIL file;
    FRESULT fr;

    if ((fr = f_open(&file, "ACL.tmp", FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK) {
        return fr;
    }

    for (int i = 0; i < N; i++) {
        CARD card = cards[i];
        char record[64];

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
    }

    if ((fr = f_close(&file)) != FR_OK) {
        return fr;
    }

    // ... rename ACL.tmp to ACL file
    f_unlink("ACL.bak");

    if ((fr = f_rename("ACL", "ACL.bak")) != FR_OK) {
        return fr;
    }

    return f_rename("ACL.tmp", "ACL");
}

static void card_detect_callback(uint gpio, uint32_t events) {
    static bool busy;

    if (!busy && !gpio_get(SD_DET)) {
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
