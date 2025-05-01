#ifndef LED_EMBUTIDO_H
#define LED_EMBUTIDO_H

#include "pico/cyw43_arch.h"

void led_embutido_init(void);
void led_embutido_set(bool estado);

#endif