#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "tusb.h"

#include "arm_math.h"

#define SAMPLE_RATE 16000
#define FFT_SIZE    256
#define FREQUENCY   440

q15_t input_q15[FFT_SIZE];
q15_t window_q15[FFT_SIZE];
q15_t windowed_input_q15[FFT_SIZE];

arm_rfft_instance_q15 S_q15;

q15_t fft_q15[FFT_SIZE * 2];
q15_t fft_mag_q15[FFT_SIZE / 2];

void input_init_q15();
void hanning_window_init_q15(q15_t* window, size_t size);

int main() {
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }

    printf("pico CMSIS DSP FFT test q15\n");


    input_init_q15();
    hanning_window_init_q15(window_q15, FFT_SIZE);
    arm_rfft_init_q15(&S_q15, FFT_SIZE, 0, 1);
    
    uint64_t dsp_start_us = to_us_since_boot(get_absolute_time());

    arm_mult_q15(window_q15, input_q15, windowed_input_q15, FFT_SIZE);
    arm_rfft_q15(&S_q15, windowed_input_q15, fft_q15);
    arm_cmplx_mag_q15(fft_q15, fft_mag_q15, FFT_SIZE / 2);

    uint64_t dsp_end_us = to_us_since_boot(get_absolute_time());

    printf("dsp time = %llu us\n", dsp_end_us - dsp_start_us);

    while (1) { tight_loop_contents(); }

    return 0;
}

void input_init_q15() {
    for (int i = 0; i < FFT_SIZE; i++) {
        float32_t f = sin((2 * PI * FREQUENCY) / SAMPLE_RATE * i);

        arm_float_to_q15(&f, &input_q15[i], 1);
    }
}

void hanning_window_init_q15(q15_t* window, size_t size) {
    for (size_t i = 0; i < size; i++) {
       float32_t f = 0.5 * (1.0 - arm_cos_f32(2 * PI * i / FFT_SIZE ));

       arm_float_to_q15(&f, &window_q15[i], 1);
    }
}