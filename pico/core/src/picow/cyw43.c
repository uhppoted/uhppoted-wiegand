#include <pico/cyw43_arch.h>

void setup_cyw43() {
    char s[64];
    int err;

    if ((err = cyw43_arch_init()) != 0) {
        snprintf(s, sizeof(s), "CYW43 initialiation error (%d)", err);
        puts(s);
    } else {
        puts("CYW43 initialised");
    }
}
