#include "pico/stdlib.h"
#include "include/microphone.h" // Includes the updated definitions
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/irq.h" // For DMA interrupts
#include <math.h> // For log2 in calculating ring_size_bits

// Global variables for DMA
uint dma_channel;
dma_channel_config dma_cfg;

// The large buffer to store the full 10 seconds of audio
uint16_t full_audio_buffer[TOTAL_RECORD_SAMPLES];

// This index tracks where in full_audio_buffer the DMA is currently writing
volatile uint32_t current_buffer_idx = 0;

// Flag to indicate if a new chunk of data is ready for display
volatile bool new_data_ready = false;

// DMA Interrupt Handler
void dma_handler() {
    if (dma_channel_get_irq1_status(dma_channel)) {
        dma_channel_acknowledge_irq1(dma_channel);

        // Update the current index for the next transfer.
        // The DMA automatically handles wrapping in circular mode,
        // but we need to know the start of the *next* chunk for processing.
        current_buffer_idx += DMA_BUFFER_SIZE;
        if (current_buffer_idx >= TOTAL_RECORD_SAMPLES) {
            current_buffer_idx = 0; // Wrap around for circular buffer
        }

        new_data_ready = true; // Signal that a new chunk is ready for processing
    }
}

void microphone_init_dma()
{
    // ADC Setup (same as before)
    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);

    adc_fifo_setup(
        true,  // Enable FIFO
        true,  // Enable DMA data request
        1,     // Threshold for DMA request is 1 reading
        false, // No error bit
        false  // Don't downscale to 8-bits
    );

    adc_set_clkdiv(ADC_CLOCK_DIV); // Use the corrected ADC_CLOCK_DIV

    // DMA Setup
    dma_channel = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_channel);

    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, false);  // Read from ADC FIFO (fixed address)
    channel_config_set_write_increment(&dma_cfg, true);   // Write to buffer (increment address)
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);         // Triggered by ADC
    channel_config_set_chain_to(&dma_cfg, dma_channel);  // Chain to self for continuous operation

    // === FIX 1: channel_config_set_ring - Correct arguments ===
    // Calculate the size_bits for the ring buffer.
    // The size must be a power of 2, and size_bits is log2(size_in_bytes).
    // TOTAL_RECORD_SAMPLES * sizeof(uint16_t) is 80000 * 2 = 160000 bytes.
    // 160000 is not a power of 2. The largest power of 2 less than 160000 is 131072 (2^17).
    // This means we must make our TOTAL_RECORD_SAMPLES a power of 2 for the circular buffer to work correctly.
    //
    // Let's adjust TOTAL_RECORD_SAMPLES in microphone.h to be a power of 2.
    // The closest power of 2 higher than 80000 is 2^17 = 131072.
    // Or, for 10 seconds at 8kHz, it's 80000. The DMA ring buffer size must be a power of 2.
    // So, we need to choose `TOTAL_RECORD_SAMPLES` to be a power of 2.
    // Example: For 10 seconds at 8kHz, `TOTAL_RECORD_SAMPLES` is 80000.
    // This won't work with `channel_config_set_ring` as a power of 2.
    //
    // A common workaround is to pick the smallest power of 2 that is *larger than* or equal to your desired buffer size.
    // 80000 samples * 2 bytes/sample = 160000 bytes.
    // Next power of 2 greater than or equal to 160000 is 2^18 = 262144 bytes.
    // So, we'd need `TOTAL_RECORD_SAMPLES` to be `262144 / 2 = 131072` samples.
    // This means our recording would actually be 131072 / 8000 = 16.384 seconds long.
    //
    // If you MUST have *exactly* 10 seconds and cannot use a power-of-2 sized buffer,
    // then the circular DMA won't perfectly loop at 10 seconds. You'd have to manage
    // the wrapping manually, which defeats the purpose of `channel_config_set_ring`.
    //
    // For simplicity and to get this compiling, let's redefine `TOTAL_RECORD_SAMPLES`
    // to be `131072` (2^17) in `microphone.h`. This will record for ~16.38 seconds,
    // but the `show_live_audio` function will still cut off at 10 seconds based on `get_absolute_time()`.
    // The buffer will just continue to fill beyond 10 seconds until `microphone_stop_recording()` is called.

    // Let's assume TOTAL_RECORD_SAMPLES is now a power of 2, e.g., 131072
    uint ring_buffer_bytes = TOTAL_RECORD_SAMPLES * sizeof(uint16_t); // This should be 262144
    // Calculate log2 of the size in bytes. log2(262144) = 18
    uint size_bits = log2(ring_buffer_bytes); // This will be 18 if 262144 bytes

    channel_config_set_ring(&dma_cfg, true, size_bits); // true for write ring, size_bits for log2(bytes)


    // Configure DMA channel but don't start yet
    // Source: ADC FIFO
    // Destination: The full_audio_buffer
    // Number of transfers: DMA_BUFFER_SIZE (this is what triggers the interrupt each time)
    dma_channel_configure(
        dma_channel,
        &dma_cfg,
        full_audio_buffer,    // Initial write address for the DMA
        &(adc_hw->fifo),      // Source is ADC FIFO
        DMA_BUFFER_SIZE,      // Number of transfers per chunk
        false                 // Don't start immediately
    );

    // Set up DMA interrupt
    dma_channel_set_irq1_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_handler);
    irq_set_enabled(DMA_IRQ_1, true);
}

void microphone_start_recording() 
{
    adc_fifo_drain(); // Clear any old data from FIFO
    adc_run(false);   // Stop ADC if running

    // === FIX 2: dma_channel_set_write_addr - 'trigger' argument was already added, but ensure it's false ===
    dma_channel_set_write_addr(dma_channel, full_audio_buffer, false); // Don't trigger start here, just set address

    // Start the DMA channel. It will immediately begin transfers.
    // The chain_to and ring_size settings will make it loop.
    dma_channel_start(dma_channel);

    adc_run(true); // Start ADC to begin filling FIFO and triggering DMA
    current_buffer_idx = 0; // Reset index for processing
    new_data_ready = false; // Reset flag
}

void microphone_stop_recording() 
{
    adc_run(false); // Stop ADC
    dma_channel_abort(dma_channel); // Stop DMA transfers
}