#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/gpio.h>
#include <hardware/rtc.h>
#include <hardware/watchdog.h>

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/datetime.h>

#include <TPIC6B595.h>
#include <buzzer.h>
#include <common.h>
#include <led.h>
#include <logd.h>
#include <picow.h>
#include <read.h>
#include <relays.h>
#include <sys.h>
#include <uart.h>
#include <write.h>

#include "../include/emulator.h"

#define VERSION "v0.8.5"

// GPIO
const uint32_t MSG = 0xf0000000;
const uint32_t MSG_WATCHDOG = 0x00000000;
const uint32_t MSG_SYSCHECK = 0x10000000;
const uint32_t MSG_RX = 0x20000000;
const uint32_t MSG_TX = 0x30000000;
const uint32_t MSG_LED = 0x70000000;
const uint32_t MSG_RELAY = 0x80000000;
const uint32_t MSG_DOOR = 0x90000000;
const uint32_t MSG_PUSHBUTTON = 0xa0000000;
const uint32_t MSG_LOG = 0xd0000000;
const uint32_t MSG_SYSINIT = 0xe0000000;
const uint32_t MSG_DEBUG = 0xf0000000;

// FUNCTION PROTOTYPES

void setup_gpio(void);
void sysinit();

// GLOBALS
enum MODE mode = UNKNOWN;
queue_t queue;

card last_card = {
    .facility_code = 0,
    .card_number = 0,
    .ok = false,
};

int main() {
    bi_decl(bi_program_description("Wiegand Emulator (PicoW)"));
    bi_decl(bi_program_version_string(VERSION));

    stdio_init_all();
    setup_gpio();
    watchdog_enable(5000, true);

    // ... initialise RTC
    rtc_init();
    sleep_us(64);

    // ... initialise FIFO, UART and timers
    queue_init(&queue, sizeof(uint32_t), 64);
    setup_uart();
    alarm_pool_init_default();

    // ... initialise CYW43
    setup_cyw43();

    // ... initialise reader/emulator
    add_alarm_in_ms(250, startup, NULL, true);
    clear_screen();

    while (true) {
        uint32_t v;
        queue_remove_blocking(&queue, &v);
        dispatch(v);

        if ((v & MSG) == MSG_SYSINIT) {
            sysinit();
        }

        if ((v & MSG) == MSG_WATCHDOG) {
            watchdog_update();
            blink(SYS_LED);
        }

        if ((v & MSG) == MSG_RX) {
            char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
            rx(b);
            free(b);
        }

        if ((v & MSG) == MSG_TX) {
            char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
            puts(b);
            free(b);
        }

        // if ((v & MSG) == MSG_CARD) {
        //     on_card_read(v & 0x0fffffff);
        //
        //     char s[64];
        //     cardf(&last_card, s, sizeof(s), false);
        //     logd_log(s);
        // }
        //
        // if ((v & MSG) == MSG_CODE) {
        //     char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
        //     char s[64];
        //
        //     snprintf(s, sizeof(s), "CODE   %s", b);
        //     logd_log(s);
        //     free(b);
        // }
        //
        // if ((v & MSG) == MSG_KEYPAD_DIGIT) {
        //     on_keypad_digit(v & 0x0fffffff);
        // }

        if ((v & MSG) == MSG_LED) {
            led_event(v & 0x0fffffff);
        }

        if ((v & MSG) == MSG_RELAY) {
            relay_event(v & 0x0fffffff);
        }

        if ((v & MSG) == MSG_DOOR) {
            door_event(v & 0x0fffffff);
        }

        if ((v & MSG) == MSG_PUSHBUTTON) {
            pushbutton_event(v & 0x0fffffff);
        }

        if ((v & MSG) == MSG_LOG) {
            char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
            printf("%s", b);
            free(b);
        }

        if ((v & MSG) == MSG_DEBUG) {
            char s[64];
            snprintf(s, sizeof(s), "DEBUG %d", v & 0x0fffffff);
            puts(s);
        }
    }

    // ... cleanup

    queue_free(&queue);
}

void setup_gpio() {
    gpio_init(RELAY_NO);
    gpio_set_dir(RELAY_NO, GPIO_IN);
    gpio_pull_up(RELAY_NO);

    gpio_init(RELAY_NC);
    gpio_set_dir(RELAY_NC, GPIO_IN);
    gpio_pull_up(RELAY_NC);

    gpio_init(DOOR_SENSOR);
    gpio_set_dir(DOOR_SENSOR, GPIO_IN);
    gpio_pull_up(DOOR_SENSOR);

    gpio_init(PUSH_BUTTON);
    gpio_set_dir(PUSH_BUTTON, GPIO_IN);
    gpio_pull_up(PUSH_BUTTON);

    gpio_init(JUMPER_READ);
    gpio_set_dir(JUMPER_READ, GPIO_IN);
    gpio_pull_up(JUMPER_READ);

    gpio_init(JUMPER_WRITE);
    gpio_set_dir(JUMPER_WRITE, GPIO_IN);
    gpio_pull_up(JUMPER_WRITE);

    gpio_init(SD_DET);
    gpio_set_dir(SD_DET, GPIO_IN);
    gpio_pull_down(SD_DET);
}

void sysinit() {
    static bool initialised = false;
    static repeating_timer_t watchdog_rt;
    static repeating_timer_t syscheck_rt;

    if (!initialised) {
        puts("                     *** WIEGAND EMULATOR (PicoW)");

        if (gpio_get(JUMPER_READ) && !gpio_get(JUMPER_WRITE)) {
            mode = EMULATOR;
        } else {
            mode = UNKNOWN;
        }

        logd_initialise(mode);
        read_initialise(mode);
        write_initialise(mode);
        led_initialise(mode);
        buzzer_initialise(mode);
        TPIC_initialise(mode);

        if (!relay_initialise(mode)) {
            logd_log("failed to initialise relay monitor");
        }

        // ... set door contact to 'CLOSED'
        relay_door_contact(true);

        // ... setup sys stuff
        add_repeating_timer_ms(1250, watchdog, NULL, &watchdog_rt);
        add_repeating_timer_ms(5000, syscheck, NULL, &syscheck_rt);

        char dt[32];
        snprintf(dt, sizeof(dt), "%s %s", SYSDATE, SYSTIME);
        sys_settime(dt);

        rtc_get_datetime(&last_card.timestamp);
        sys_ok();
        set_scroll_area();

        // ... 'k, done
        buzzer_beep(1);

        initialised = true;
    }
}
