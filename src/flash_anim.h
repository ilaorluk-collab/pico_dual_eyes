#ifndef FLASH_ANIM_H
#define FLASH_ANIM_H

#include <stdint.h>
#include "hardware/flash.h"

#define ANIM_FLASH_MAX_BYTES     (1536u * 1024u)
#define ANIM_FLASH_OFFSET        (PICO_FLASH_SIZE_BYTES - ANIM_FLASH_MAX_BYTES)
#define BIT_FRAME_SIZE           7200
#define RAW_FRAME_SIZE           (240 * 240 * 2)
#define FLASH_FRAME_SIZE         8192
#define MAX_ANIMS                8
#define MAX_NAME                 16

#define COLOR_YELLOW  0xFFDE
#define COLOR_BLACK   0x0000

typedef struct {
    char name[MAX_NAME];
    int frames;
    int fps;
    uint32_t delay_ms;
    uint32_t flash_offset;
} flash_anim_t;

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
