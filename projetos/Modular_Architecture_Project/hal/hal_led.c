#include "../include/hal_led.h"
#include "../include/led_embutido.h"

static bool led_state = false;

void hal_led_init(void) {
    led_embutido_init();
    led_state = false;
    led_embutido_set(led_state);
}

void hal_led_toggle(void) {
    led_state = !led_state;
    led_embutido_set(led_state);
}

void hal_led_set(bool estado) {
    led_state = estado;
    led_embutido_set(led_state);
}