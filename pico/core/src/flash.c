#include <stdio.h>
#include <stdlib.h>

#include <hardware/flash.h>
#include <hardware/sync.h>

#include "../include/flash.h"
#include "../include/logd.h"

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

const uint32_t MAGIC_WORD = 0xaa5555aa;

typedef struct header {
    uint32_t magic;
    uint32_t version;
    uint32_t cards;
    uint32_t crc;
} header;

uint32_t bcd(datetime_t);

/* Reads the ACL from onboard flash.
 *
 */
void flash_read_acl() {
    uint32_t addr = XIP_BASE + FLASH_TARGET_OFFSET;
    uint32_t *p = (uint32_t *)addr;

    struct header header;
    char s[64];

    header.magic = *p++;
    header.version = *p++;
    header.cards = *p++;
    header.crc = *p++;

    snprintf(s, sizeof(s), ">>>>>>> HEADER: magic:%08x version:%lu cards:%lu crc:%lu", header.magic, header.version, header.cards, header.crc);
    logd_debug(s);

    if (header.cards <= 60) {
        p = (uint32_t *)(addr + FLASH_PAGE_SIZE);

        for (uint32_t i = 0; i < header.cards; i++) {
            uint32_t card = *(p + 0);
            uint32_t start = *(p + 1);
            uint32_t end = *(p + 2);

            snprintf(s, sizeof(s), ">>>>>>> CARD  %lu  %-10lu %08x %08x", i + 1, card, start, end);
            logd_debug(s);

            p += 64;
        }
    }
}

/* Writes the ACL to onboard flash.
 *
 */
void flash_write_acl(CARD cards[], int N) {
    uint32_t buffer[FLASH_SECTOR_SIZE / sizeof(uint32_t)];
    struct header header;

    header.magic = MAGIC_WORD;
    header.version = 1;
    header.cards = N;
    header.crc = 0x00000000;

    buffer[0] = header.magic;
    buffer[1] = header.version;
    buffer[2] = header.cards;
    buffer[3] = header.crc;

    int ix = 0;
    uint32_t *p = &buffer[64];

    while (ix < N && ix < 60) {
        CARD card = cards[ix];

        *(p + 0) = card.card_number;
        *(p + 1) = bcd(cards[ix].start);
        *(p + 2) = bcd(cards[ix].end);

        p += 64;
        ix++;
    }

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
    flash_range_program((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), (uint8_t *)buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);
}

uint32_t bcd(datetime_t dt) {
    uint32_t bcd = 0;

    uint8_t yyyymmdd[] = {
        dt.year / 100,
        dt.year % 100,
        dt.month,
        dt.day};

    for (int i = 0; i < 4; i++) {
        bcd <<= 4;
        bcd |= (yyyymmdd[i] / 10) & 0x0f;
        bcd <<= 4;
        bcd |= (yyyymmdd[i] % 10) & 0x0f;
    }

    return bcd;
}
