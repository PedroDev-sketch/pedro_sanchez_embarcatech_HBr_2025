#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#define btna 5
#define btnb 6

const uint16_t I2C_SDA = 14;
const uint16_t I2C_SCL = 15;
typedef struct render_area render_area;

volatile bool begin = false;
volatile bool on = false;
volatile bool update = false;
volatile uint8_t counter = 0;
volatile uint8_t clickNum = 0;

volatile absolute_time_t last_click_time = 0;

render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};
uint8_t ssd[ssd1306_buffer_length];

void display()
{
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
}

void irqCallback(uint gpio, uint32_t events) {
    if(gpio==btna && !on) begin = true;
    else if(gpio==btnb && on && absolute_time_diff_us(last_click_time, get_absolute_time()) > 200000) 
    {
        clickNum++;
        update = true;

        last_click_time = get_absolute_time();
    }
}

void display_update() {
    memset(ssd, 0, sizeof(ssd));
    render_on_display(ssd, &frame_area);

    ssd1306_draw_string(ssd, 5, 0, "Tempo");
    int len = snprintf(NULL, 0, "%d", counter);
    char *result = malloc(len + 1);
    snprintf(result, len + 1, "%d", counter);  
    ssd1306_draw_string(ssd, 5, 8, result);

    ssd1306_draw_string(ssd, 5, 24, "Clicks");
    len = snprintf(NULL, 0, "%d", clickNum);
    result = malloc(len + 1);
    snprintf(result, len + 1, "%d", clickNum);  
    ssd1306_draw_string(ssd, 5, 32, result);

    render_on_display(ssd, &frame_area);
}

void countdown() {
    on = true;
    clickNum=0;
    counter=9;
    while(counter > 0) 
    {
        display_update();
        sleep_ms(1000);
        counter--;
    } on = false;
    display_update();
    sleep_ms(3000);
}

int main()
{
    stdio_init_all();

    //botão a
    {
        gpio_init(btna);
        gpio_set_dir(btna, GPIO_IN);
        gpio_pull_up(btna);
        gpio_set_irq_enabled_with_callback(btna, GPIO_IRQ_EDGE_FALL, true, &irqCallback);
    }

    //botão b
    {
        gpio_init(btnb);
        gpio_set_dir(btnb, GPIO_IN);
        gpio_pull_up(btnb);
        gpio_set_irq_enabled(btnb, GPIO_IRQ_EDGE_FALL, true);
    }

    display();

    bool on = false;
    while(true)
    {
        if(begin)
        {
            begin = false;
            countdown();
        }
        if(update)
        {
            update = false;
            display_update();
        }
    }
}
