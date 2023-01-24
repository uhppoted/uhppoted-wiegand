// #include <string.h>
// #include "my_debug.h"
#include "hw_config.h"
// #include "ff.h"
#include "diskio.h"

#include "../include/sdcard.h"
#include "../include/wiegand.h"

void spi0_dma_isr();

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
}

/* Format SD card.
 *
 */
int sdcard_format() {
    sd_card_t *sdcard = sd_get_by_num(0);
    FRESULT fr = f_mkfs(sdcard->pcName, 0, 0, FF_MAX_SS * 2);

    return fr == FR_OK ? 0 : fr;
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