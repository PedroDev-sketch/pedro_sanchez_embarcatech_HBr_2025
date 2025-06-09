#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "include/ssd1306.h"

#define WIDTH 128
#define HEIGHT 64
#define I2C_PORT i2c1
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define SSD1306_I2C_ADDR 0x3C
ssd1306_t display;

#define MIC_PIN 28           
#define MIC_CHANNEL 2        
#define SAMPLE_RATE 8000     

#define BUZZER_PIN 21        
#define BUTTON_A_PIN 5       
#define BUTTON_B_PIN 6       

#define ADC_VREF 3.3f
#define ADC_BITS 12
#define ADC_MAX_VAL (1 << ADC_BITS) 
#define MIC_DC_BIAS_VOLTAGE 1.65f 
#define ADC_MAX 1.55f             

#define ADC_CLOCK_DIV 15624

#define TOTAL_RECORD_SAMPLES (SAMPLE_RATE * 4) 

uint16_t audio_buffer[TOTAL_RECORD_SAMPLES];
volatile bool recording_complete = false;

void init_display() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    display.external_vcc = false;
    ssd1306_init(&display, WIDTH, HEIGHT, SSD1306_I2C_ADDR, I2C_PORT);
    ssd1306_clear(&display);
}

void init_buttons() {
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
}

void init_microphone() {
    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);

    adc_fifo_setup(
        true,   
        false,  
        1,      
        false,  
        false   
    );
    adc_set_clkdiv(ADC_CLOCK_DIV); 
}

void init_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void record_audio() {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Recording...");
    ssd1306_show(&display);

    adc_fifo_drain(); 
    adc_run(false);   
    adc_run(true);    

    for (int i = 0; i < 100; i++) { 
        while (adc_fifo_is_empty()) {
            tight_loop_contents(); 
        }
        adc_fifo_get(); 
    }

    for(int i = 0; i < TOTAL_RECORD_SAMPLES; i++) {
        while (adc_fifo_is_empty()) {
            tight_loop_contents(); 
        }
        audio_buffer[i] = adc_fifo_get(); 
    }
    
    adc_run(false); 
    recording_complete = true;
    
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Recording Complete!");
    ssd1306_show(&display);
    sleep_ms(500); 
}

void play_audio() {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Playing...");
    ssd1306_show(&display);

    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);
    
    pwm_set_enabled(slice_num, true); 

    for(int i = 0; i < TOTAL_RECORD_SAMPLES; i++) {
        uint16_t raw_adc_val = audio_buffer[i];

        float adjusted_voltage = ((float)raw_adc_val * (ADC_VREF / ADC_MAX_VAL)) - MIC_DC_BIAS_VOLTAGE;
        float normalized_pwm_val = (adjusted_voltage + ADC_MAX) / (2.0f * ADC_MAX);

        if (normalized_pwm_val < 0.0f) normalized_pwm_val = 0.0f;
        if (normalized_pwm_val > 1.0f) normalized_pwm_val = 1.0f;

        uint16_t pwm_val = (uint16_t)(normalized_pwm_val * 255); 

        pwm_set_chan_level(slice_num, chan, pwm_val);
        
        sleep_us(180); 
    }
    
    pwm_set_gpio_level(BUZZER_PIN, 0); 
    pwm_set_enabled(slice_num, false); 
    
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Playback Complete!");
    ssd1306_show(&display);
    sleep_ms(500); 
}

void show_waveform() {
    ssd1306_clear(&display);
    
    int visual_baseline_y = (HEIGHT * 3) / 4; 

    float display_amplitude_scale = (HEIGHT / 2.0f) / ADC_MAX; 

    for (int x = 0; x < WIDTH; x++) {
        int sample_idx = (x * TOTAL_RECORD_SAMPLES) / WIDTH;
        if (sample_idx >= TOTAL_RECORD_SAMPLES) sample_idx = TOTAL_RECORD_SAMPLES - 1;

        uint16_t raw_adc_val = audio_buffer[sample_idx];
        float adjusted_voltage = ((float)raw_adc_val * (ADC_VREF / ADC_MAX_VAL)) - MIC_DC_BIAS_VOLTAGE;
        
        int pixel_offset = (int)(adjusted_voltage * display_amplitude_scale);

        int y_start_bar = visual_baseline_y;
        int y_end_bar = visual_baseline_y - pixel_offset; 

        int clamped_y0 = (int)fmin((float)y_start_bar, (float)y_end_bar);
        int clamped_y1 = (int)fmax((float)y_start_bar, (float)y_end_bar);

        clamped_y0 = (clamped_y0 < 0) ? 0 : clamped_y0;
        clamped_y0 = (clamped_y0 >= HEIGHT) ? HEIGHT - 1 : clamped_y0;
        clamped_y1 = (clamped_y1 < 0) ? 0 : clamped_y1;
        clamped_y1 = (clamped_y1 >= HEIGHT) ? HEIGHT - 1 : clamped_y1;
        
        ssd1306_draw_line(&display, x, clamped_y0, x, clamped_y1);
    }
    
    ssd1306_draw_string(&display, 0, 0, 1, "Audio Graph");
    ssd1306_draw_string(&display, 0, 16, 1, recording_complete ? "Ready to Play" : "No Data Yet");
    ssd1306_show(&display);
}

int main() {
    stdio_init_all(); 
    
    init_display();
    init_buttons();
    init_microphone();
    init_buzzer();
    
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Pico Sound Synth");
    ssd1306_draw_string(&display, 0, 16, 1, "Loading...");
    ssd1306_show(&display);
    sleep_ms(1000); 

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Pico Sound Synth");
    ssd1306_draw_string(&display, 0, 16, 1, "A: Record");
    ssd1306_draw_string(&display, 0, 32, 1, "B: Play");
    ssd1306_show(&display);
    
    while(true) {
        if(!gpio_get(BUTTON_A_PIN)) {
            sleep_ms(50);
            if(!gpio_get(BUTTON_A_PIN)) {
                record_audio();
                show_waveform();
                sleep_ms(300); 
            }
        }
        
        if(!gpio_get(BUTTON_B_PIN) && recording_complete) {
            sleep_ms(50); 
            if(!gpio_get(BUTTON_B_PIN)) {
                play_audio();
                show_waveform(); 
                sleep_ms(300);
            }
        }
        
        tight_loop_contents();
    }
    
    return 0;
}