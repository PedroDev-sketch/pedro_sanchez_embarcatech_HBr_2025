#include <stdio.h>
#include "pico/stdlib.h"
#include "include/ssd1306.h"
#include "include/display.h"
#include "include/button.h"
#include "include/microphone.h"
#include "include/buzzer.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

int main()
{
    stdio_init_all();

    display_init();
    button_init();
    microphone_init_dma();

    sleep_ms(200);

    bool is_recording = false;
    bool is_playing = false;

    while (true)
    {
        if(!gpio_get(A) && !is_playing) // Assuming A is active-low
        {
            // Simple debounce for the button press
            sleep_ms(50);
            if(gpio_get(A)) continue; // If button released immediately, ignore

            is_recording = true;
            // microphone_init() is replaced by microphone_init_dma() which is done once
            // and microphone_start_recording() is called inside show_live_audio()
            
            ssd1306_clear(&display);
            ssd1306_draw_string(&display, 0, 16, 1, "Gravando...");
            ssd1306_show(&display);
            sleep_ms(500); // Show "Recording..." for a moment

            show_live_audio(); // This now records for 10 seconds with live display

            char * done = "Gravacao Feita";
            ssd1306_clear(&display);
            ssd1306_draw_string(&display, 0, 16, 1, done);
            ssd1306_show(&display);
            sleep_ms(1000); // Display message for 1 second

            is_recording = false;
        }

        if(!gpio_get(B) && !is_recording) // Assuming B is active-low
        {
            // Simple debounce for the button press
            sleep_ms(50);
            if(gpio_get(B)) continue; // If button released immediately, ignore

            is_playing = true;
            buzzer_init();
            sleep_ms(50);

            play_audio();

            is_playing = false;
        }

        sleep_ms(100); // General debounce for main loop
    }
}
