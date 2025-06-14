#include "pico/stdlib.h"
#include "led.h"
#include "buzzer.h"
#include "button.h"
#include "hardware.h"

void rgb_led_init()
{
    gpio_init(GREEN);
    gpio_set_dir(GREEN, GPIO_OUT);
    gpio_put(GREEN, false);

    gpio_init(RED);
    gpio_set_dir(RED, GPIO_OUT);
    gpio_put(RED, false);

    gpio_init(BLUE);
    gpio_set_dir(BLUE, GPIO_OUT);
    gpio_put(BLUE, false);
}

void buzzer_init()
{
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

void button_init()
{
    gpio_init(A);
    gpio_set_dir(A, GPIO_IN);
    gpio_pull_up(A);

    gpio_init(B);
    gpio_set_dir(B, GPIO_IN);
    gpio_pull_up(B);
}

void boot_hardware()
{
    rgb_led_init();
    buzzer_init();
    button_init();
}