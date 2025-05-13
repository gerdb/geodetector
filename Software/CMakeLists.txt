# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
set (PICO_EXTRAS_PATH ${USERHOME}/.pico-sdk/pico-extras)
include(pico_extras_import.cmake)

project(geodetector C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


# Add executable. Default name is the project name, version 0.1

add_executable(geodetector
        main.c
        icons.c
        copro.c
        gps.c
        audio.c
        )

# Generate PIO header
pico_generate_pio_header(geodetector
    ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(geodetector
        pico_stdlib
        pico_audio_i2s
        hardware_i2c
        hardware_pwm
        hardware_adc
        hardware_flash
        pico_multicore
        )

target_compile_definitions(geodetector PRIVATE
        PICO_AUDIO_I2S_MONO_INPUT=1
        USE_AUDIO_I2S=1
        PICO_AUDIO_I2S_CLOCK_PINS_SWAPPED=1
        PICO_AUDIO_I2S_DATA_PIN=29
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=27
        )

pico_enable_stdio_usb(geodetector 1)
pico_enable_stdio_uart(geodetector 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(geodetector)

# add url via pico_set_program_url
