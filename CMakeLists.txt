cmake_minimum_required(VERSION 3.13)

# Имя проекта
set(PROJECT_NAME pico_double_display)
project(${PROJECT_NAME} C CXX ASM)

# Инициализация Pico SDK
set(PICO_SDK_PATH /Users/ilya/pico/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

pico_sdk_init()

# Создание исполняемого файла
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# Подключение необходимых библиотек
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_dma
    hardware_pio
    hardware_irq
    hardware_timer
)

# Настройки компиляции
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wno-format
    -O3
)

# Создание бинарных файлов вручную
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -Oihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-objcopy -Obinary ${PROJECT_NAME} ${PROJECT_NAME}.bin
    COMMENT "Creating binary files"
)