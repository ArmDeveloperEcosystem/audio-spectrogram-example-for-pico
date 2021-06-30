/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"

#include "pico/pdm_microphone.h"
#include "pico/st7789.h"

#include "arm_math.h"

#include "color_map.h"

// constants
#define SAMPLE_RATE       16000
#define FFT_SIZE          256
#define INPUT_BUFFER_SIZE 64
#define INPUT_SHIFT       2

#define LCD_WIDTH         240
#define LCD_HEIGHT        320

#define FFT_BINS_SKIP     5
#define FFT_MAG_MAX       2000.0

// lcd configuration
const struct st7789_config lcd_config = {
    .spi      = PICO_DEFAULT_SPI_INSTANCE,
    .gpio_din = PICO_DEFAULT_SPI_TX_PIN,
    .gpio_clk = PICO_DEFAULT_SPI_SCK_PIN,
    .gpio_cs  = PICO_DEFAULT_SPI_CSN_PIN,
    .gpio_dc  = 20,
    .gpio_rst = 21,
    .gpio_bl  = 22,
};

// microphone configuration
const struct pdm_microphone_config pdm_config = {
    // GPIO pin for the PDM DAT signal
    .gpio_data = 2,

    // GPIO pin for the PDM CLK signal
    .gpio_clk = 3,

    // PIO instance to use
    .pio = pio0,

    // PIO State Machine instance to use
    .pio_sm = 0,

    // sample rate in Hz
    .sample_rate = SAMPLE_RATE,

    // number of samples to buffer
    .sample_buffer_size = INPUT_BUFFER_SIZE,
};

q15_t capture_buffer_q15[INPUT_BUFFER_SIZE];
volatile int new_samples_captured = 0;

q15_t input_q15[FFT_SIZE];
q15_t window_q15[FFT_SIZE];
q15_t windowed_input_q15[FFT_SIZE];

arm_rfft_instance_q15 S_q15;

q15_t fft_q15[FFT_SIZE * 2];
q15_t fft_mag_q15[FFT_SIZE / 2];

uint16_t row_pixels[LCD_WIDTH];

void input_init_q15();
void hanning_window_init_q15(q15_t* window, size_t size);
void on_pdm_samples_ready();

int main() {
    // initialize stdio
    stdio_init_all();

    printf("pico audio spectrogram\n");

    // initialize the LCD and fill the screen black
    st7789_init(&lcd_config, LCD_WIDTH, LCD_HEIGHT);
    st7789_fill(0x000);

    // initialize the hanning window and RFFT instance
    hanning_window_init_q15(window_q15, FFT_SIZE);
    arm_rfft_init_q15(&S_q15, FFT_SIZE, 0, 1);

    // initialize the PDM microphone
    if (pdm_microphone_init(&pdm_config) < 0) {
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

    uint16_t row = 0;

    while (1) {
        // wait for new samples
        while (new_samples_captured == 0) {
            tight_loop_contents();
        }
        new_samples_captured = 0;

        // move input buffer values over by INPUT_BUFFER_SIZE samples
        arm_copy_q15(input_q15 + INPUT_BUFFER_SIZE, input_q15, (FFT_SIZE - INPUT_BUFFER_SIZE));

        // copy new samples to end of the input buffer with a bit shift of INPUT_SHIFT
        arm_shift_q15(capture_buffer_q15, INPUT_SHIFT, input_q15 + (FFT_SIZE - INPUT_BUFFER_SIZE), INPUT_BUFFER_SIZE);
    
        // apply the DSP pipeline: Hanning Window + FFT
        arm_mult_q15(window_q15, input_q15, windowed_input_q15, FFT_SIZE);
        arm_rfft_q15(&S_q15, windowed_input_q15, fft_q15);
        arm_cmplx_mag_q15(fft_q15, fft_mag_q15, FFT_SIZE / 2);

        // map the FFT magnitude values to pixel values
        for (int i = 0; i < (LCD_WIDTH / 2); i++) {
            // get the current FFT magnitude value
            q15_t magntitude = fft_mag_q15[i + FFT_BINS_SKIP];

            // scale it between 0 to 255 to map, so we can map it to a color based on the color map
            int color_index = (magntitude / FFT_MAG_MAX) * 255;

            if (color_index > 255) {
                color_index = 255;
            }

            // cacluate the pixel color using the color map and color index
            uint16_t pixel = COLOR_MAP[color_index];

            // set the pixel value for the next two rows
            row_pixels[LCD_WIDTH - 1 - (i * 2)] = pixel;
            row_pixels[LCD_WIDTH - 1 - (i * 2 + 1)] = pixel;
        }

        // update the cursor to the start of the current row
        st7789_set_cursor(0, row);

        // write the row value pixels
        st7789_write(row_pixels, sizeof(row_pixels));

        // scroll to the new row
        st7789_vertical_scroll(row);

        // calculate the next row to update
        row = (row + 1) % LCD_HEIGHT;
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
