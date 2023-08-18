#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/flash.h>
#include <hardware/sync.h>

#include "../include/flash.h"
#include "../include/logd.h"

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

const uint32_t ACL_MAGIC_WORD = 0xaa5555aa;
const uint32_t ACL_ALLOWED = 1;
const uint32_t ACL_DENIED = 0;
const uint32_t ACL_VERSION = 16384;
const uint32_t HEADER_SIZE = FLASH_PAGE_SIZE;

typedef struct header {
    uint32_t magic;
    uint32_t version;
    uint32_t cards;
    uint32_t crc;
} header;

uint32_t bcd(datetime_t);
datetime_t date(uint32_t yyyymmdd);
uint32_t crc32(const char *, size_t);

/* Reads the ACL from onboard flash.
 *
 */
void flash_read_acl(CARD cards[], int *N) {
    uint32_t addr = XIP_BASE + FLASH_TARGET_OFFSET;
    uint32_t *p = (uint32_t *)addr;

    struct header header;
    char s[128];

    header.magic = *p++;
    header.version = *p++;
    header.cards = *p++;
    header.crc = *p++;

    snprintf(s, sizeof(s), ">>>>   HEADER  magic:%08X version:%lu cards:%lu crc:%08x N:%d",
             header.magic,
             header.version,
             header.cards,
             header.crc,
             *N);
    logd_debug(s);

    if (header.magic == ACL_MAGIC_WORD && header.version < ACL_VERSION && header.cards <= 60) {
        uint32_t crc = crc32((char *)(addr + HEADER_SIZE), header.cards * 64);

        if (header.crc == crc) {
            uint32_t *p = (uint32_t *)(addr + HEADER_SIZE);
            int ix = 0;

            for (uint32_t i = 0; i < header.cards && ix < *N; i++) {
                uint32_t card = *(p + 0);
                uint32_t start = *(p + 1);
                uint32_t end = *(p + 2);
                bool allowed = *(p + 3) == ACL_ALLOWED;
                char name[CARD_NAME_SIZE];

                snprintf(name, sizeof(name), "%s", (char *)(p + 4));

                snprintf(s, sizeof(s), ">>>>   CARD    %lu  %-8lu %08x   %08x   %s %s", i + 1, card, start, end, allowed ? "Y" : "N", name);
                logd_debug(s);

                p += 16;

                cards[ix].card_number = card;
                cards[ix].start = date(start);
                cards[ix].end = date(end);
                cards[ix].allowed = allowed;
                snprintf(cards[ix].name, CARD_NAME_SIZE, name);

                snprintf(s, sizeof(s), ">>>>   DEBUG      %-8lu %04d-%02d-%02d %04d-%02d-%02d %s %s",
                         cards[ix].card_number,
                         cards[ix].start.year,
                         cards[ix].start.month,
                         cards[ix].start.day,
                         cards[ix].end.year,
                         cards[ix].end.month,
                         cards[ix].end.day,
                         cards[ix].allowed ? "Y" : "N",
                         cards[ix].name);

                logd_debug(s);

                ix++;
            }

            *N = ix;
            return;
        }
    }

    *N = 0;
}

/* Writes the ACL to onboard flash.
 *
 */
void flash_write_acl(CARD cards[], int N) {
    uint32_t buffer[FLASH_SECTOR_SIZE / sizeof(uint32_t)];
    struct header header;

    // .. fill cards buffer
    uint32_t *p = (uint32_t *)(&buffer[64]);
    int ix = 0;
    int count = 0;

    while (ix < N && count < 60) {
        CARD card = cards[ix];

        if (card.card_number != 0xffffffff) {
            *(p + 0) = card.card_number;
            *(p + 1) = bcd(cards[ix].start);
            *(p + 2) = bcd(cards[ix].end);
            *(p + 3) = card.allowed ? ACL_ALLOWED : ACL_DENIED;

            memset((char *)(p + 4), 0, 48);
            snprintf((char *)(p + 4), 48, "%s", card.name);

            p += 16;
            count++;
        }

        ix++;
    }

    // ... set header
    header.magic = ACL_MAGIC_WORD;
    header.version = 1;
    header.cards = count;
    header.crc = crc32((char *)(&buffer[64]), count * 64);

    // ... copy header to buffer
    buffer[0] = header.magic;
    buffer[1] = header.version;
    buffer[2] = header.cards;
    buffer[3] = header.crc;

    // ... write to flash
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

uint32_t bin(uint32_t bcd) {
    uint32_t v = 0;

    for (int i = 0; i < 8; i++) {
        int shr = 4 * (7 - i);
        v *= 10;
        v += (bcd >> shr) & 0x0f;
    }

    return v;
}

datetime_t date(uint32_t yyyymmdd) {
    datetime_t dt;

    dt.year = bin((yyyymmdd >> 16) & 0x0000ffff);
    dt.month = bin((yyyymmdd >> 8) & 0x000000ff);
    dt.day = bin((yyyymmdd >> 0) & 0x000000ff);
    dt.dotw = 0;
    dt.hour = 0;
    dt.min = 0;
    dt.sec = 0;

    return dt;
}

// Ref. https://lxp32.github.io/docs/a-simple-example-crc32-calculation
uint32_t crc32(const char *s, size_t N) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < N; i++) {
        char ch = s[i];
        for (size_t j = 0; j < 8; j++) {
            crc = ((ch ^ crc) & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
            ch >>= 1;
        }
    }

    return ~crc;
}
