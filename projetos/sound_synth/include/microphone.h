#include "pico/stdlib.h"

#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

#define SAMPLE_RATE 8000
#define ADC_CLOCK_DIV (48000000.f / SAMPLE_RATE / 96.f)

#define TOTAL_RECORD_SAMPLES (SAMPLE_RATE * 10)

#define DMA_BUFFER_SIZE 256 // Número de amostras que serão feitas do ADC.
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 1.65f
#define ADC_STEP (3.3f/5.f) // Intervalos de volume do microfone.

extern uint16_t full_audio_buffer[TOTAL_RECORD_SAMPLES];

void microphone_init_dma();

void microphone_start_recording();
void microphone_stop_recording();