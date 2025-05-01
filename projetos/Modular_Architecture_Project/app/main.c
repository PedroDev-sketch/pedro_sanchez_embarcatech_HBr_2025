#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "../include/hal_led.h"

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) {
        return -1;
    }

    hal_led_init();

    while (true) {
        hal_led_toggle();
        sleep_ms(500);
    }
}