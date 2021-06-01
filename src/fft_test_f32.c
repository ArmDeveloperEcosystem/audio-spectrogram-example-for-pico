#include <stdio.h>

#include "pico/stdlib.h"
#include "tusb.h"

#include "arm_math.h"

#include <stdio.h>
#include <math.h>

#include <arm_math.h>

#define SAMPLE_RATE 16000
#define FFT_SIZE    256
#define FREQUENCY   440

float32_t input_f32[FFT_SIZE];
float32_t window_f32[FFT_SIZE];
float32_t windowed_input_f32[FFT_SIZE];

arm_rfft_fast_instance_f32 S_f32;

float32_t fft_f32[FFT_SIZE * 2];
float32_t fft_mag_f32[FFT_SIZE / 2];

void input_init_f32();
void hanning_window_init_f32(float32_t* window, size_t size);

int main() {
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }

    printf("pico CMSIS DSP FFT test f32\n");

    input_init_f32();
    hanning_window_init_f32(window_f32, FFT_SIZE);
    arm_rfft_fast_init_f32(&S_f32, FFT_SIZE);

    uint64_t dsp_start_us = to_us_since_boot(get_absolute_time());
    
    arm_mult_f32(window_f32, input_f32, windowed_input_f32, FFT_SIZE);
    arm_rfft_fast_f32(&S_f32, windowed_input_f32, fft_f32, 0);
    arm_cmplx_mag_f32(fft_f32, fft_mag_f32, FFT_SIZE / 2);

    uint64_t dsp_end_us = to_us_since_boot(get_absolute_time());

    printf("dsp time = %llu us\n", dsp_end_us - dsp_start_us);

    while (1) { tight_loop_contents(); }

    return 0;
}

void input_init_f32() {
    for (int i = 0; i < FFT_SIZE; i++) {
        input_f32[i] = sin((2 * PI * FREQUENCY) / SAMPLE_RATE * i);
    }
}

void hanning_window_init_f32(float32_t* window, size_t size) {
    for (size_t i = 0; i < size; i++) {
       window[i] = 0.5 * (1.0 - arm_cos_f32(2 * PI * i / FFT_SIZE ));
    }
}
