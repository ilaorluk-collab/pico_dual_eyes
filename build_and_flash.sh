#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
PROJECT_NAME="pico_double_display"

echo "=== Cleaning build directory ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "=== Configuring CMake ==="
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER=arm-none-eabi-gcc \
    -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ \
    -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc \
    -DCMAKE_C_COMPILER_WORKS=ON \
    -DCMAKE_CXX_COMPILER_WORKS=ON \
    -DCMAKE_C_FLAGS="-mcpu=cortex-m0plus -mthumb" \
    -DCMAKE_CXX_FLAGS="-mcpu=cortex-m0plus -mthumb" \
    -DCMAKE_ASM_FLAGS="-mcpu=cortex-m0plus -mthumb"

echo "=== Building ==="
cmake --build "$BUILD_DIR" -j$(sysctl -n hw.ncpu)

# Проверяем, создался ли файл
if [ -f "$BUILD_DIR/$PROJECT_NAME" ] || [ -f "$BUILD_DIR/$PROJECT_NAME.elf" ]; then
    if [ -f "$BUILD_DIR/$PROJECT_NAME" ]; then
        ELF_FILE="$BUILD_DIR/$PROJECT_NAME"
    else
        ELF_FILE="$BUILD_DIR/$PROJECT_NAME.elf"
    fi
    echo "=== Converting to UF2 ==="
    # Используем pico-sdk的工具 для конвертации
    if command -v picotool &> /dev/null; then
        picotool uf2 convert -t elf "$ELF_FILE" "$BUILD_DIR/$PROJECT_NAME.uf2"
        echo "=== Loading to Pico ==="
        picotool load "$BUILD_DIR/$PROJECT_NAME.uf2"
        echo "=== Rebooting Pico ==="
        picotool reboot
    else
        echo "picotool not found, creating UF2 via elf2uf2..."
        # Альтернативный способ через elf2uf2
        if [ -f "$PICO_SDK_PATH/../elf2uf2/elf2uf2" ]; then
            "$PICO_SDK_PATH/../elf2uf2/elf2uf2" "$ELF_FILE" "$BUILD_DIR/$PROJECT_NAME.uf2"
        else
            echo "Please install picotool or elf2uf2 for UF2 conversion"
            exit 1
        fi
    fi
else
    echo "Error: ELF file not found!"
    exit 1
fi

echo "=== Done! ==="