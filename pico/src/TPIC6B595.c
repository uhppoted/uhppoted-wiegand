#include "hardware/spi.h"

#include "../include/TPIC6B595.h"

extern const uint SPI_CLK;
extern const uint SPI_TX;
extern const uint SPI_RX;
extern const uint SPI_CS;

extern spi_inst_t *SPI;

void TPIC_initialise(enum MODE mode) {
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);

    gpio_set_function(SPI_CLK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX, GPIO_FUNC_SPI);

    spi_init(SPI, 1000 * 1000);

    spi_set_format(SPI, 8, 1, 1, SPI_MSB_FIRST);
}

void TPIC_write(uint8_t v) {
    uint8_t bytes[1];

    bytes[0] = v;

    gpio_put(SPI_CS, 0);
    spi_write_blocking(SPI, bytes, 1);
    gpio_put(SPI_CS, 1);
}
