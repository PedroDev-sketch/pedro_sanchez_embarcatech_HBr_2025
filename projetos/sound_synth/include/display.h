#include <pico/stdlib.h>
#include <hardware/i2c.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

#define WIDTH 128
#define HEIGHT 64

#define BAR_WIDTH 4
#define BAR_SPACING 1
#define NUM_BARS (WIDTH / (BAR_WIDTH + BAR_SPACING))

extern ssd1306_t display;

void driver_i2c_init();

void display_init(); 

void show_live_audio();