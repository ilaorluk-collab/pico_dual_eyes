#ifndef UNPACK_H
#define UNPACK_H

#include <stdint.h>

#define BIT_FRAME_SIZE  7200
#define RAW_FRAME_SIZE  115200
#define COLOR_YELLOW    0xFFDE
#define COLOR_BLACK     0x0000

static inline void unpack_1bit_to_rgb565(const uint8_t *src, uint8_t *dst) {
    uint16_t *out = (uint16_t *)dst;
    for (int i = 0; i < BIT_FRAME_SIZE; i++) {
        uint8_t byte = src[i];
        *out++ = (byte & 0x80) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x40) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x20) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x10) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x08) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x04) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x02) ? COLOR_YELLOW : COLOR_BLACK;
        *out++ = (byte & 0x01) ? COLOR_YELLOW : COLOR_BLACK;
    }
}

#endif
