#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/flash.h>
#include <hardware/sync.h>

#include "../include/flash.h"
#include "../include/logd.h"

const uint32_t OFFSETS[2] = {
    PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE,
    PICO_FLASH_SIZE_BYTES - 2 * FLASH_SECTOR_SIZE,
};

const int PAGES = sizeof(OFFSETS) / sizeof(uint32_t);

const uint32_t ACL_MAGIC_WORD = 0xaa5555aa;
const uint32_t ACL_ALLOWED = 1;
const uint32_t ACL_DENIED = 0;
const uint32_t ACL_VERSION = 16384;
const uint32_t DATA_OFFSET = 128;
const uint32_t DATA_PREAMBLE = 128;
const uint32_t PASSCODES_OFFSET = 128;
const uint32_t CARDS_OFFSET = 256; // FLASH_PAGE_SIZE

typedef struct header {
    uint32_t magic;
    uint32_t version;
    uint32_t cards;
    uint32_t crc;
} header;

int flash_get_current_page();
uint32_t flash_get_version(int);
uint32_t bcd(datetime_t);
datetime_t bin2date(uint32_t yyyymmdd);
uint32_t crc32(const char *, size_t);

/* Reads the ACL from onboard flash.
 *
 */
void flash_read_acl(CARD cards[], int *N, uint32_t passcodes[4]) {
    uint32_t page = flash_get_current_page();

    if (page != -1 && page < PAGES) {
        uint32_t addr = XIP_BASE + OFFSETS[page];
        uint32_t size = *(((uint32_t *)addr) + 2);
        uint32_t *p = (uint32_t *)(addr + CARDS_OFFSET);
        uint32_t *q = (uint32_t *)(addr + PASSCODES_OFFSET);
        int ix = 0;

        uint32_t *r = (uint32_t *)(addr);

        // ... get passcodes
        passcodes[0] = *q++;
        passcodes[1] = *q++;
        passcodes[2] = *q++;
        passcodes[3] = *q++;

        // ... get cards
        for (uint32_t i = 0; i < size && ix < *N; i++) {
            uint32_t card = *(p + 0);
            uint32_t start = *(p + 1);
            uint32_t end = *(p + 2);
            bool allowed = *(p + 3) == ACL_ALLOWED;
            char *name = (char *)(p + 4);

            p += 16;

            cards[ix].card_number = card;
            cards[ix].start = bin2date(start);
            cards[ix].end = bin2date(end);
            cards[ix].allowed = allowed;
            snprintf(cards[ix].name, CARD_NAME_SIZE, name);

            ix++;
        }

        *N = ix;

        return;
    }

    logd_log("FLASH  NO VALID ACL");

    *N = 0;
}

/* Writes the ACL to onboard flash.
 *
 */
void flash_write_acl(CARD cards[], int N, uint32_t passcodes[4]) {
    uint32_t page = flash_get_current_page();
    uint32_t version = flash_get_version(page);

    if (page != -1 && page < PAGES) {
        page = (page + 1) % PAGES;
        version = (version + 1) % ACL_VERSION;
    } else {
        page = 0;
        version = 1;
    }

    uint32_t offset = OFFSETS[page];
    uint32_t buffer[FLASH_SECTOR_SIZE / sizeof(uint32_t)];
    struct header header;

    memset(buffer, 0, sizeof(buffer));

    // ... set passcodes
    buffer[32] = passcodes[0];
    buffer[32 + 1] = passcodes[1];
    buffer[32 + 2] = passcodes[2];
    buffer[32 + 3] = passcodes[3];

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
    header.version = version;
    header.cards = count;
    header.crc = crc32((char *)(&buffer[32]), DATA_PREAMBLE + count * 64); // passcodes + cards

    // ... copy header to buffer
    buffer[0] = header.magic;
    buffer[1] = header.version;
    buffer[2] = header.cards;
    buffer[3] = header.crc;

    // ... write to flash
    uint32_t interrupts = save_and_disable_interrupts();

    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    flash_range_program(offset, (uint8_t *)buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);
}

int flash_get_current_page() {
    uint32_t version1 = *((uint32_t *)(XIP_BASE + OFFSETS[0]) + 1) % ACL_VERSION;
    uint32_t version2 = *((uint32_t *)(XIP_BASE + OFFSETS[1]) + 1) % ACL_VERSION;
    int index = (version2 == 0 ? ACL_VERSION : version2) > (version1 == 0 ? ACL_VERSION : version1) ? 1 : 0;

    for (int ix = 0; ix < PAGES; ix++) {
        uint32_t addr = XIP_BASE + OFFSETS[index % PAGES];
        uint32_t *p = (uint32_t *)addr;
        struct header header;

        header.magic = *p++;
        header.version = *p++;
        header.cards = *p++;
        header.crc = *p++;

        if (header.magic == ACL_MAGIC_WORD && header.version < ACL_VERSION && header.cards <= 60) {
            uint32_t crc = crc32((char *)(addr + DATA_OFFSET), DATA_PREAMBLE + header.cards * 64); // passcodes + cards

            if (header.crc == crc) {
                return index % PAGES;
            }
        }

        index++;
    }

    return -1;
}

uint32_t flash_get_version(int page) {
    if (page != -1 && page < PAGES) {
        uint32_t addr = XIP_BASE + OFFSETS[page];
        uint32_t version = *(((uint32_t *)addr) + 1);

        return version;
    }

    return 0;
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

datetime_t bin2date(uint32_t yyyymmdd) {
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
    uint32_t crc = 0xffffffff;

    for (size_t i = 0; i < N; i++) {
        char ch = s[i];
        for (size_t j = 0; j < 8; j++) {
            crc = ((ch ^ crc) & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
            ch >>= 1;
        }
    }

    return ~crc;
}
