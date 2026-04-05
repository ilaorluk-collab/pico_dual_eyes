cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME pico_double_display)
project(${PROJECT_NAME} C CXX ASM)

set(PICO_SDK_PATH /Users/ilya/pico/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

pico_sdk_init()

add_subdirectory(lib/fatfs/FatFs_SPI)

add_executable(${PROJECT_NAME}
    src/main.c
    src/display.c
    hw_config.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_dma
    hardware_pio
    hardware_irq
    hardware_timer
    hardware_rtc
    FatFs_SPI
    m
)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wno-format
    -Os
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11 cxx_std_11)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -Oihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-objcopy -Obinary ${PROJECT_NAME} ${PROJECT_NAME}.bin
    COMMENT "Creating binary files"
)
