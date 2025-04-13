#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main()
{
    stdio_init_all();
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

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
        uint16_t adc_value = adc_read();
        
        const float conversion = 3.3f / (1 << 12);
        float voltage = adc_value * conversion;
        float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

        uint8_t ssd[ssd1306_buffer_length];
        memset(ssd, 0, ssd1306_buffer_length);

        char xstring[16];
        sprintf(xstring, "%.2f Celsius\n", temperature);
        
        int index = 0;
        ssd1306_draw_string(ssd, 5, 16, xstring);

        render_on_display(ssd, &frame_area);

        sleep_ms(1000);
    }
}
