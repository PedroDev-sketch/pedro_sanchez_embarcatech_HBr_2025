#include "pico/stdlib.h"
#include "include/ssd1306.h"
#include "include/display.h"
#include "include/microphone.h"
#include "hardware/i2c.h"
#include <math.h>

ssd1306_t display;
extern volatile uint32_t current_buffer_idx;
extern volatile bool new_data_ready;
extern uint16_t full_audio_buffer[TOTAL_RECORD_SAMPLES];

void driver_i2c_init() 
{
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void display_init() 
{
    driver_i2c_init();

    display.external_vcc=false;
    ssd1306_init(&display, WIDTH, HEIGHT, 0x3C, I2C_PORT);
    ssd1306_clear(&display);
}

void show_live_audio()
{
    const uint32_t RECORD_DURATION_MS = 10 * 1000; // 10 seconds
    absolute_time_t start_time = get_absolute_time();
    absolute_time_t end_time = delayed_by_ms(start_time, RECORD_DURATION_MS);

    microphone_start_recording(); // Start the continuous DMA recording

    while (get_absolute_time() < end_time)
    {
        // Wait for a new chunk of data to be ready from DMA
        // This makes the display updates asynchronous with DMA,
        // and only updates when new data is available.
        if (new_data_ready) 
        {
            new_data_ready = false;

            uint32_t display_chunk_start_idx;
            if (current_buffer_idx == 0) {
                // If DMA wrapped to 0, the last full chunk is at the end of the buffer
                display_chunk_start_idx = TOTAL_RECORD_SAMPLES - DMA_BUFFER_SIZE;
            } else {
                display_chunk_start_idx = current_buffer_idx - DMA_BUFFER_SIZE;
            }
            // In display.c, inside show_live_audio(), replacing the histogram drawing loop:

            ssd1306_clear(&display);

            // --- Waveform drawing ---
            // Determine how many samples per pixel horizontally.
            // If WIDTH is 128 pixels, and DMA_BUFFER_SIZE is 256 samples,
            // we have 256 / 128 = 2 samples per pixel.
            // For a 128-pixel wide display, we can plot 128 points.
            // We'll average 'samples_per_pixel' to get each point.

            // Calculate samples to skip or average per pixel
            int samples_per_pixel = DMA_BUFFER_SIZE / WIDTH; // How many samples condense into one pixel column
            if (samples_per_pixel == 0) samples_per_pixel = 1; // At least one sample per pixel

            // Loop through each pixel column on the display
            for (int x = 0; x < WIDTH; x++) {
                float avg_sample_value = 0.0f;
                int count = 0;

                // Average relevant samples for this pixel column
                for (int s = 0; s < samples_per_pixel; s++) {
                    int sample_index_in_chunk = (x * samples_per_pixel) + s;

                    // Ensure we don't go out of bounds of the current chunk
                    if (sample_index_in_chunk < DMA_BUFFER_SIZE) {
                        // Access the sample from the full_audio_buffer
                        // Apply ADC_ADJUST to get voltage, it will be centered around 0.
                        avg_sample_value += ADC_ADJUST(full_audio_buffer[display_chunk_start_idx + sample_index_in_chunk]);
                        count++;
                    }
                }

                if (count > 0) {
                    avg_sample_value /= count; // Calculate average for this pixel column
                }

                // Scale the adjusted sample value to display height
                // The adjusted range is -ADC_MAX to +ADC_MAX (i.e., -1.65V to +1.65V)
                // We want to map this to 0 to HEIGHT (0 to 63 pixels).
                // A value of +ADC_MAX should be HEIGHT, -ADC_MAX should be 0.
                // Or, more commonly, 0V maps to center (HEIGHT/2).
                // Let's map -ADC_MAX to 0, 0V to HEIGHT/2, +ADC_MAX to HEIGHT.

                // Scale: (value + ADC_MAX) * (HEIGHT / (2 * ADC_MAX))
                int y_pos = (int)((avg_sample_value + ADC_MAX) * (HEIGHT / (2.0f * ADC_MAX)));

                // Clamp y_pos to ensure it's within display bounds [0, HEIGHT-1]
                if (y_pos < 0) y_pos = 0;
                if (y_pos >= HEIGHT) y_pos = HEIGHT - 1;

                // Draw a pixel at (x, y_pos)
                ssd1306_draw_pixel(&display, x, y_pos);
            }
            ssd1306_show(&display);
            // --- End Waveform drawing ---
            // No sleep_ms here, we just wait for the next data ready flag
        } else {
            // Short sleep if no new data yet, to avoid busy-waiting too much
            sleep_us(100);
        }
    }

    microphone_stop_recording(); // Stop DMA and ADC after 10 seconds
}