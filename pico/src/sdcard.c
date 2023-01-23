// #include <string.h>
// #include "my_debug.h"
#include "hw_config.h"
// #include "ff.h"
#include "diskio.h"

#include "../include/sdcard.h"

void spi0_dma_isr();

// Configure SD card SPI
static spi_t spis[] = {
    {
        .hw_inst = spi0,
        .miso_gpio = 3,
        .mosi_gpio = 2,
        .sck_gpio = 18,
        .baud_rate = 12500 * 1000,
        .dma_isr = spi0_dma_isr,
    },
};

// Configure SD card  structs
static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",
        .spi = &spis[0],
        .ss_gpio = 5,
        .use_card_detect = false,
        .card_detect_gpio = 6,
        .card_detected_true = -1,
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
