cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pico_audio_spectogram)

# initialize the Pico SDK
pico_sdk_init()

# rest of your project
add_executable(fft_test_f32
    ${CMAKE_CURRENT_LIST_DIR}/src/fft_test_f32.c
)

target_link_libraries(fft_test_f32 pico_stdlib pico_cmsis_dsp)

# enable usb output, disable uart output
pico_enable_stdio_usb(fft_test_f32 1)
pico_enable_stdio_uart(fft_test_f32 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(fft_test_f32)

# rest of your project
add_executable(fft_test_q15
    ${CMAKE_CURRENT_LIST_DIR}/src/fft_test_q15.c
)

target_link_libraries(fft_test_q15 pico_stdlib pico_cmsis_dsp)

# enable usb output, disable uart output
pico_enable_stdio_usb(fft_test_q15 1)
pico_enable_stdio_uart(fft_test_q15 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(fft_test_q15)

# rest of your project
add_executable(audio_spectrogram
    ${CMAKE_CURRENT_LIST_DIR}/src/audio_spectrogram.c
)

target_link_libraries(audio_spectrogram pico_stdlib pico_pdm_microphone pico_cmsis_dsp)

# enable usb output, disable uart output
pico_enable_stdio_usb(audio_spectrogram 1)
pico_enable_stdio_uart(audio_spectrogram 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(audio_spectrogram)

add_subdirectory("lib/pico-CMSIS-DSP" EXCLUDE_FROM_ALL)
add_subdirectory("lib/pico-microphone" EXCLUDE_FROM_ALL)
