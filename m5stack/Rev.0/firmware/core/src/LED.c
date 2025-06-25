#include <pico/stdlib.h>
#include <pico/time.h>

#include <LED.h>
#include <log.h>
#include <wiegand.h>

#define LOGTAG "LED"

extern const constants IO;

typedef struct IIR {
    float x₁;
    float y₁;
} IIR;

typedef struct LPF {
    float a₀;
    float a₁;
    float b₀;
    float b₁;
} LPF;

struct {
    bool state;
    IIR lpf;
    repeating_timer_t timer;
} LED = {
    .state = false,
    .lpf = {.x₁ = 0.0, .y₁ = 0.0},
};

// 25Hz 1 pole Butterworth LPF (approx. 15ms debounce delay) for input
const struct LPF LPF₁ = {
    .a₀ = 1.0,
    .a₁ = -0.85408069,
    .b₀ = 0.07295966,
    .b₁ = 0.07295966,
};

const int32_t LED_TICK = 1; // ms

bool LED_on_update(repeating_timer_t *);
float lpf₁(IIR *iir, float in);

void LED_init() {
    debugf(LOGTAG, "init");

    gpio_init(IO.LED);
    gpio_set_dir(IO.LED, GPIO_IN);
    gpio_set_input_hysteresis_enabled(IO.LED, true);
    gpio_pull_down(IO.LED);

    infof(LOGTAG, "initialised - LED %s", LED.state ? "on" : "off");
}

void LED_start() {
    infof(LOGTAG, "start");

    add_repeating_timer_us(1000 * LED_TICK, LED_on_update, NULL, &LED.timer);
}

bool LED_on_update(repeating_timer_t *rt) {
    bool v = gpio_get(IO.LED);

    if (v != LED.state) {
        LED.state = v;

        push((message){
            .message = MSG_LED,
            .tag = MESSAGE_BOOLEAN,
            .boolean = LED.state,
        });
    }

    // uint8_t inputs = U3x.inputs.state;
    // uint8_t stable = U3x.stable.state;
    // uint8_t mask = 0x01;
    //
    // for (int i = 0; i < 8; i++) {
    //     // ... input filter
    //     float u = (data & U3_MASKS[i]) != 0x00 ? 1.0 : 0.0;
    //     float v = lpf₁(&U3x.inputs.lpf[i], u);
    //
    //     if (v > 0.9) {
    //         inputs |= mask;
    //     } else if (v < 0.1) {
    //         inputs &= ~mask;
    //     }
    //
    //     // ... unstable input filter
    //     float p = ((v > 0.9) || (v < 0.1)) ? 1.0 : 0.0;
    //     float q = lpf₂(&U3x.stable.lpf[i], p);
    //
    //     if (q > 0.9) {
    //         stable |= mask;
    //     } else if (q < 0.1) {
    //         stable &= ~mask;
    //     }
    //
    //     mask <<= 1;
    // }
    //
    // if (inputs != U3x.inputs.state) {
    //     uint8_t old = U3x.inputs.state;
    //     U3x.inputs.state = inputs;
    //
    //     debugf(LOGTAG, "inputs   %08b (%08b)", U3x.inputs.state, U3x.stable.state);
    //
    //     static const struct {
    //         uint8_t mask;
    //         EVENT open;
    //         EVENT closed;
    //     } events[] = {
    //         {0x01, EVENT_DOOR_1_OPEN, EVENT_DOOR_1_CLOSE},
    //         {0x02, EVENT_DOOR_2_OPEN, EVENT_DOOR_2_CLOSE},
    //         {0x04, EVENT_DOOR_3_OPEN, EVENT_DOOR_3_CLOSE},
    //         {0x08, EVENT_DOOR_4_OPEN, EVENT_DOOR_4_CLOSE},
    //
    //         {0x10, EVENT_DOOR_1_RELEASED, EVENT_DOOR_1_PRESSED},
    //         {0x20, EVENT_DOOR_2_RELEASED, EVENT_DOOR_2_PRESSED},
    //         {0x40, EVENT_DOOR_3_RELEASED, EVENT_DOOR_3_PRESSED},
    //         {0x80, EVENT_DOOR_4_RELEASED, EVENT_DOOR_4_PRESSED},
    //     };
    //
    //     // ... door open/close/pressed/released
    //     int N = sizeof(events) / sizeof(events[0]);
    //
    //     for (int i = 0; i < N; i++) {
    //         uint8_t mask = events[i].mask;
    //
    //         if (((U3x.inputs.state & mask) == 0x00) && ((old & mask) == mask)) {
    //             push((message){
    //                 .message = MSG_EVENT,
    //                 .tag = MESSAGE_EVENT,
    //                 .event = events[i].open,
    //             });
    //         }
    //
    //         if (((U3x.inputs.state & mask) == mask) && ((old & mask) == 0x00)) {
    //             push((message){
    //                 .message = MSG_EVENT,
    //                 .tag = MESSAGE_EVENT,
    //                 .event = events[i].closed,
    //             });
    //         }
    //     }
    // }
    //
    // if (stable != U3x.stable.state) {
    //     U3x.stable.state = stable;
    //
    //     debugf(LOGTAG, "inputs   %08b (%08b)", U3x.inputs.state, U3x.stable.state);
    // }

    return true;
}
