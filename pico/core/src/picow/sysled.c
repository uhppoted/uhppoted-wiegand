#include <stdio.h>
#include <pico/cyw43_arch.h>

void init_sysled() {
}

void set_sysled(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,on);        
}