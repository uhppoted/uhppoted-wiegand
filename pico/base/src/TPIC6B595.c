#include "hardware/spi.h"

#include "../include/TPIC6B595.h"

extern const uint SPI_CLK;
extern const uint SPI_TX;
extern const uint SPI_RX;
extern const uint SPI_CS;

extern spi_inst_t *SPI;

const uint8_t TPIC_MASK_DOOR_UNLOCK = 0x01;
const uint8_t TPIC_MASK_PUSHBUTTON = 0x02;
const uint8_t TPIC_MASK_DOOR_CONTACT = 0x04;

const uint8_t TPIC_MASK_RED_LED = 0x10;
const uint8_t TPIC_MASK_ORANGE_LED = 0x20;
const uint8_t TPIC_MASK_YELLOW_LED = 0x40;
const uint8_t TPIC_MASK_GREEN_LED = 0x80;

/* struct for TPIC state
*
 */
struct {
        uint8_t state;
    bool initialised;

} TPIC = {
        .state = 0x00,
    .initialised = false,
};


void TPIC_initialise(enum MODE mode) {
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);

    gpio_set_function(SPI_CLK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX, GPIO_FUNC_SPI);

    spi_init(SPI, 1000 * 1000);
    spi_set_format(SPI, 8, 1, 1, SPI_MSB_FIRST);

    uint8_t bytes[] = {0x00};

    gpio_put(SPI_CS, 0);
    spi_write_blocking(SPI, bytes, 1);
    gpio_put(SPI_CS, 1);

    TPIC.initialised = true;
}

void TPIC_set(enum TPICIO io, bool on) {
    if (TPIC.initialised) {
    switch (io) {
    case DOOR_UNLOCK:
        TPIC.state = (TPIC.state & ~TPIC_MASK_DOOR_UNLOCK) | (on ? TPIC_MASK_DOOR_UNLOCK : 0x00);
        break;

    case PUSHBUTTON:
        TPIC.state = (TPIC.state & ~TPIC_MASK_PUSHBUTTON) | (on ? TPIC_MASK_PUSHBUTTON : 0x00);
        break;

    case DOOR_CONTACT:
        TPIC.state = (TPIC.state & ~TPIC_MASK_DOOR_CONTACT) | (on ? TPIC_MASK_DOOR_CONTACT : 0x00);
        break;

    case RED_LED:
        TPIC.state = (TPIC.state & ~TPIC_MASK_RED_LED) | (on ? TPIC_MASK_RED_LED : 0x00);
        break;

    case ORANGE_LED:
        TPIC.state = (TPIC.state & ~TPIC_MASK_ORANGE_LED) | (on ? TPIC_MASK_ORANGE_LED : 0x00);
        break;

    case YELLOW_LED:
        TPIC.state = (TPIC.state & ~TPIC_MASK_YELLOW_LED) | (on ? TPIC_MASK_YELLOW_LED : 0x00);
        break;

    case GREEN_LED:
        TPIC.state = (TPIC.state & ~TPIC_MASK_GREEN_LED) | (on ? TPIC_MASK_GREEN_LED : 0x00);
        break;
    }

    uint8_t bytes[] = {TPIC.state};

    gpio_put(SPI_CS, 0);
    spi_write_blocking(SPI, bytes, 1);
    gpio_put(SPI_CS, 1);
    }
}
