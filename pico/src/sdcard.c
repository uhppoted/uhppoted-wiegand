#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "hw_config.h"
// #include "ff.h"
#include "diskio.h"

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
    },
    {
        .pcName = "1:",
        .spi = &spis[0],
        .ss_gpio = CS,
        .use_card_detect = true,
        .card_detect_gpio = DET,
        .card_detected_true = 1,
        .m_Status = STA_NOINIT,
    },
};

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
        sd_card_t *sdcard = sd_get_by_num(0);

        if (sdcard->use_card_detect) {
            gpio_set_irq_enabled_with_callback(
                sdcard->card_detect_gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &card_detect_callback);
        }
    } else {
        tx("SDCARD ERROR");
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
    FRESULT fr = f_unmount(sdcard->pcName);

    return fr == FR_OK ? 0 : fr;
}

/* Format SD card.
 *
 */
int sdcard_format() {
    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr = f_mkfs(sdcard->pcName, 0, 0, FF_MAX_SS * 2);

    return fr == FR_OK ? 0 : fr;
}

/* Lists files on the SD card.
 *
 */

int sdcard_ls() {
    FIL file;
    FRESULT fr;

    if ((fr = f_open(&file, "qwerty.txt", FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK) {
        return fr;
    }

    if (f_printf(&file, "yo, dawg... \n") < 0) {
        printf("f_printf failed\n");
    }

    return f_close(&file);
}

// int sdcard_ls() {
//     // char cwdbuf[FF_LFN_BUF] = {0};
//     char const *p_dir = "/";
//
//     DIR dj;
//     FILINFO fno;
//
//     memset(&dj, 0, sizeof dj);
//     memset(&fno, 0, sizeof fno);
//
//     FRESULT fr = f_findfirst(&dj, &fno, p_dir, "*");
//     if (FR_OK != fr) {
//         return fr;
//     }
//
//     int count = 0;
//     while (fr == FR_OK && fno.fname[0]) {
//         // const char *pcWritableFile = "writable file",
//         //            *pcReadOnlyFile = "read only file",
//         //            *pcDirectory = "directory";
//         // const char *pcAttrib;
//         // /* Point pcAttrib to a string that describes the file. */
//         // if (fno.fattrib & AM_DIR) {
//         //     pcAttrib = pcDirectory;
//         // } else if (fno.fattrib & AM_RDO) {
//         //     pcAttrib = pcReadOnlyFile;
//         // } else {
//         //     pcAttrib = pcWritableFile;
//         // }
//         // /* Create a string that includes the file name, the file size and the
//         //  attributes string. */
//
//         // char s[64];
//         // snprintf(s, sizeof(s), "%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);
//         // tx(s);
//
//         count++;
//         fr = f_findnext(&dj, &fno); /* Search for next item */
//     }
//
//     f_closedir(&dj);
//
//     return count;
// }

static void card_detect_callback(uint gpio, uint32_t events) {
    static bool busy;

    if (!busy) {
        busy = true;

        for (size_t i = 0; i < sd_get_num(); ++i) {
            sd_card_t *sdcard = sd_get_by_num(i);

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
        }

        busy = false;
    }
}
