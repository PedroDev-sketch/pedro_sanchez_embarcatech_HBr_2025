#include "../include/led_embutido.h"

void led_embutido_init(void) {
    // Inicialização é feita pela cyw43_arch_init() no main
}

void led_embutido_set(bool estado) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, estado);
}