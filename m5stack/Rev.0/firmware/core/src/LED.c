#include <pico/stdlib.h>
#include <pico/time.h>

#include <LED.h>
#include <M5.h>
#include <log.h>

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
    float u = gpio_get(IO.LED) ? 1.0 : 0.0;
    float v = lpf₁(&LED.lpf, u);
    bool state = LED.state;

    if (v > 0.9) {
        state = true;
    } else if (v < 0.1) {
        state = false;
    }

    if (state != LED.state) {
        LED.state = state;

        push((message){
            .message = MSG_LED,
            .tag = MESSAGE_BOOLEAN,
            .boolean = LED.state,
        });
    }

    return true;
}

// 25Hz LPF for input
// y₀ = (b₀x₀ + b₁x₁ - a₁y₁)/a₀
float lpf₁(IIR *iir, float x₀) {
    // clang-format off
    float y₀ = LPF₁.b₀ * x₀;
          y₀ += LPF₁.b₁ * iir->x₁;
          y₀ -= LPF₁.a₁ * iir->y₁;
    // clang-format on

    iir->x₁ = x₀;
    iir->y₁ = y₀;

    return y₀;
}
