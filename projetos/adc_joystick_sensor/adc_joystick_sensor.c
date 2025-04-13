#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"

#define vrx 27
#define vry 26
#define sw 22

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main()
{
    stdio_init_all();
    adc_init();

    adc_gpio_init(vrx);
    adc_gpio_init(vry);

    gpio_init(sw);
    gpio_set_dir(sw, GPIO_IN);
    gpio_pull_up(sw);

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    while (true) {
        adc_select_input(0);
        uint16_t vry_val = adc_read();

        adc_select_input(1);
        uint16_t vrx_val = adc_read();
        
        sleep_ms(200);

        uint8_t ssd[ssd1306_buffer_length];
        memset(ssd, 0, ssd1306_buffer_length);

        char xstring[16];
        sprintf(xstring, "x %d  y %d\n", vrx_val, vry_val);
        ssd1306_draw_string(ssd, 5, 16, xstring);

        render_on_display(ssd, &frame_area);

        sleep_ms(500);
    }
}
