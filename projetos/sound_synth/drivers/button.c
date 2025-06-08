#include "pico/stdlib.h"
#include "include/button.h"

void button_init()
{
    gpio_init(A);
    gpio_set_dir(GPIO_IN, A);
    gpio_pull_up(A);

    gpio_init(B);
    gpio_set_dir(GPIO_IN, B);
    gpio_pull_up(B);
}