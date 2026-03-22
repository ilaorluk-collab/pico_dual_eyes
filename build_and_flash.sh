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

echo "=== Converting to UF2 ==="
picotool uf2 convert -t elf "$BUILD_DIR/$PROJECT_NAME" "$BUILD_DIR/$PROJECT_NAME.uf2"

echo "=== Loading to Pico ==="
picotool load "$BUILD_DIR/$PROJECT_NAME.uf2"

echo "=== Rebooting Pico ==="
picotool reboot

echo "=== Done! ==="
