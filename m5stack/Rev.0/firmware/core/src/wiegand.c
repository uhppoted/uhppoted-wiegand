#include <log.h>
#include <wiegand.h>

#define LOGTAG "WIEGAND"

bool write_card(uint8_t facility_code, uint32_t card) {
    warnf(LOGTAG, "** NOT IMPLEMENTED: write-card");
    return false;
}

bool write_keycode(char ch) {
    warnf(LOGTAG, "** NOT IMPLEMENTED: write-keycode");
    return false;
}
