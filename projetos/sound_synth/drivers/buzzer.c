#include "pico/stdlib.h"
#include "include/buzzer.h"
#include "include/microphone.h" // Includes TOTAL_RECORD_SAMPLES, full_audio_buffer, ADC_ADJUST, ADC_MAX
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include <math.h> // For fabs if needed, and in general for float operations.

// externs to access global variables from microphone.c
extern uint16_t full_audio_buffer[TOTAL_RECORD_SAMPLES];

// Add the PWM_WRAP definition here as a global constant or in buzzer.h
// It's better to put it in buzzer.h if other files might need to know the PWM_WRAP.
// For now, let's just make sure it's accessible within buzzer.c
const uint16_t PWM_WRAP_VALUE = 255; // Define it here, or make it accessible from buzzer.h

void buzzer_init()
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();

    // Set PWM wrap value
    pwm_config_set_wrap(&config, PWM_WRAP_VALUE); // Use the defined constant

    // Calculate clkdiv to achieve 8kHz PWM update rate
    // PWM_Freq = System_Clock / (clkdiv * (wrap + 1))
    // 8000 = 125,000,000 / (clkdiv * (255 + 1))
    // clkdiv = 125,000,000 / (8000 * 256) = 61.035
    pwm_config_set_clkdiv(&config, 61.0f); // Use float for precision, SDK handles rounding

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Start with buzzer off
}

void play_audio()
{
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);

    // Ensure the PWM is running
    pwm_set_enabled(slice_num, true);

    // Play all samples from the full recording buffer
    for (int i = 0; i < TOTAL_RECORD_SAMPLES; i++)
    {
        // Get the raw 12-bit ADC value
        uint16_t raw_adc_val = full_audio_buffer[i];

        // Convert the raw ADC value to a float voltage, remove DC bias
        float adjusted_voltage = ADC_ADJUST(raw_adc_val); // Range -1.65 to +1.65 (assuming ADC_MAX 1.65)

        // Map the adjusted_voltage to a 0-1.0 normalized range
        // (value + ADC_MAX) / (2 * ADC_MAX)
        float normalized_pwm_val = (adjusted_voltage + ADC_MAX) / (2.0f * ADC_MAX);

        // Clamp normalized_pwm_val to ensure it's within [0.0, 1.0]
        if (normalized_pwm_val < 0.0f) normalized_pwm_val = 0.0f;
        if (normalized_pwm_val > 1.0f) normalized_pwm_val = 1.0f;

        // Scale to PWM duty cycle (0 to PWM_WRAP_VALUE)
        // === FIX: Use the defined constant PWM_WRAP_VALUE directly ===
        uint16_t pwm_val = (uint16_t)(normalized_pwm_val * PWM_WRAP_VALUE);

        pwm_set_chan_level(slice_num, chan, pwm_val);

        // No sleep_us needed here, PWM should be configured to update at SAMPLE_RATE.
        // The loop will run as fast as possible, sending new samples to PWM.
    }
    pwm_set_chan_level(slice_num, chan, 0); // Turn off buzzer after playing
    pwm_set_enabled(slice_num, false); // Disable PWM slice
}