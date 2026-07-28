cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME pico_double_display)
project(${PROJECT_NAME} C CXX ASM)

set(PICO_SDK_PATH /Users/ilya/pico/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    src/main.c
    src/display.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_dma
    m
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wno-format
    -Os
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11 cxx_std_11)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND picotool uf2 convert $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.uf2 --family rp2040
    COMMENT "Creating binary files"
)
