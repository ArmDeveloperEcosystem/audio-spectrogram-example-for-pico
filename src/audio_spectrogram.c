#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/pdm_microphone.h"
#include "tusb.h"

#include "arm_math.h"

#define SAMPLE_RATE 16000
#define FFT_SIZE    256
#define FREQUENCY   440

// configuration
const struct pdm_microphone_config config = {
    // GPIO pin for the PDM DAT signal
    .gpio_data = 2,

    // GPIO pin for the PDM CLK signal
    .gpio_clk = 3,

    // PIO instance to use
    .pio = pio0,

    // PIO State Machine instance to use
    .pio_sm = 0,

    // sample rate in Hz
    .sample_rate = 8000,

    // number of samples to buffer
    .sample_buffer_size = FFT_SIZE / 2,
};

q15_t capture_buffer_q15[FFT_SIZE / 2];
volatile int new_samples_captured = 0;

q15_t input_q15[FFT_SIZE];
q15_t window_q15[FFT_SIZE];
q15_t windowed_input_q15[FFT_SIZE];

arm_rfft_instance_q15 S_q15;

q15_t fft_q15[FFT_SIZE * 2];
q15_t fft_mag_q15[FFT_SIZE / 2];

void input_init_q15();
void hanning_window_init_q15(q15_t* window, size_t size);
void on_pdm_samples_ready();

int main() {
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }

    printf("pico audio spectrogram\n");

    hanning_window_init_q15(window_q15, FFT_SIZE);
    arm_rfft_init_q15(&S_q15, FFT_SIZE, 0, 1);

    // initialize the PDM microphone
    if (pdm_microphone_init(&config) < 0) {
        printf("PDM microphone initialization failed!\n");
        while (1) { tight_loop_contents(); }
    }

    // set callback that is called when all the samples in the library
    // internal sample buffer are ready for reading
    pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
    
     // start capturing data from the PDM microphone
    if (pdm_microphone_start() < 0) {
        printf("PDM microphone start failed!\n");
        while (1) { tight_loop_contents(); }
    }

    while (1) {
        while (new_samples_captured == 0) {
            tight_loop_contents();
        }

        new_samples_captured = 0;

        uint64_t dsp_start_us = to_us_since_boot(get_absolute_time());

        // shift input buffer over FFT_SIZE / 2
        memcpy(input_q15, input_q15 + (FFT_SIZE / 2), sizeof(input_q15[0]) * (FFT_SIZE / 2));

        // copy new samples to second half of the input buffer
        memcpy(input_q15 + (FFT_SIZE / 2), capture_buffer_q15, sizeof(input_q15[0]) * (FFT_SIZE / 2));
    
        // apply the DSP pipeline: Hanning Window + FFT
        arm_mult_q15(window_q15, input_q15, windowed_input_q15, FFT_SIZE);
        arm_rfft_q15(&S_q15, windowed_input_q15, fft_q15);
        arm_cmplx_mag_q15(fft_q15, fft_mag_q15, FFT_SIZE / 2);

        uint64_t dsp_end_us = to_us_since_boot(get_absolute_time());

        printf("dsp time = %llu us\n", dsp_end_us - dsp_start_us);

        break;
    }

    while (1) {
        tight_loop_contents();
    }

    return 0;
}

void hanning_window_init_q15(q15_t* window, size_t size) {
    for (size_t i = 0; i < size; i++) {
       float32_t f = 0.5 * (1.0 - arm_cos_f32(2 * PI * i / FFT_SIZE ));

       arm_float_to_q15(&f, &window_q15[i], 1);
    }
}

void on_pdm_samples_ready()
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading 
    new_samples_captured = pdm_microphone_read(capture_buffer_q15, FFT_SIZE / 2);
}
