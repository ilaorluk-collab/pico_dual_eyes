#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "display.h"
#include "eye_animation.h"
#include "hw_config.h"
#include "ff.h"
#include "f_util.h"
#include "my_debug.h"

int current_anim = 0;
int animation_count = 0;
static int current_frame = 0;
static uint32_t last_frame_time = 0;
static uint32_t rng_state = 42;

static uint32_t simple_rand(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void unpack_binary_row(const uint8_t *row_bytes, uint8_t *pixels) {
    for (int x = 0; x < TFT_WIDTH; x++) {
        int byte_idx = x / 8;
        int bit_idx = 7 - (x % 8);
        bool is_black = (row_bytes[byte_idx] >> bit_idx) & 1;
        uint16_t color = is_black ? COLOR_BLACK : COLOR_YELLOW;
        int px = x * 2;
        pixels[px] = (color >> 8) & 0xFF;
        pixels[px + 1] = color & 0xFF;
    }
}

static bool sd_load_frame(const char *anim_name, int frame_idx) {
    char path[64];
    snprintf(path, sizeof(path), "/bender/%s/f%02d.bin", anim_name, frame_idx);

    gpio_put(SD_CS, 1);
    gpio_put(LEFT_CS, 1);

    FIL fil;
    FRESULT fr = f_open(&fil, path, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Cannot open %s (%d)\n", path, fr);
        return false;
    }

    UINT bytes_read;
    fr = f_read(&fil, frame_buffer, FRAME_SIZE, &bytes_read);
    f_close(&fil);

    if (fr != FR_OK || bytes_read != FRAME_SIZE) {
        printf("ERROR: Read %s failed (%u/%d bytes, fr=%d)\n",
               path, (unsigned)bytes_read, FRAME_SIZE, fr);
        return false;
    }
    return true;
}

static void render_frame(void) {
    for (int y = 0; y < TFT_HEIGHT; y++) {
        const uint8_t *row_bytes = frame_buffer + (y * FRAME_BYTES_PER_ROW);
        unpack_binary_row(row_bytes, row_buffer);

        set_window(spi0, LEFT_CS, LEFT_DC, 0, y, TFT_WIDTH - 1, y);
        set_window(spi1, RIGHT_CS, RIGHT_DC,
                   RIGHT_SHIFT_X, y + RIGHT_SHIFT_Y,
                   TFT_WIDTH - 1 + RIGHT_SHIFT_X, y + RIGHT_SHIFT_Y);

        gpio_put(LEFT_DC, 1);
        gpio_put(RIGHT_DC, 1);
        gpio_put(LEFT_CS, 0);
        gpio_put(RIGHT_CS, 0);

        spi_write_blocking(spi0, row_buffer, TFT_WIDTH * 2);
        spi_write_blocking(spi1, row_buffer, TFT_WIDTH * 2);

        gpio_put(LEFT_CS, 1);
        gpio_put(RIGHT_CS, 1);
    }
}

bool parse_manifest(void) {
    gpio_put(SD_CS, 1);
    gpio_put(LEFT_CS, 1);

    FIL fil;
    FRESULT fr = f_open(&fil, "/bender/manifest.txt", FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Cannot open manifest (%d)\n", fr);
        return false;
    }

    char line[64];
    animation_count = 0;

    while (f_gets(line, sizeof(line), &fil)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#')
            continue;

        char name[MAX_ANIM_NAME];
        int frames, fps;
        if (sscanf(line, "%15[^:]:%d:%d", name, &frames, &fps) == 3) {
            if (animation_count >= MAX_ANIMATIONS) break;
            animation_info_t *a = &animations[animation_count];
            strncpy(a->name, name, MAX_ANIM_NAME - 1);
            a->name[MAX_ANIM_NAME - 1] = '\0';
            a->frame_count = frames;
            a->fps = fps;
            a->frame_delay_ms = 1000 / fps;
            printf("  Animation '%s': %d frames, %d FPS (%lu ms/frame)\n",
                   a->name, a->frame_count, a->fps, (unsigned long)a->frame_delay_ms);
            animation_count++;
        }
    }

    f_close(&fil);
    printf("Loaded %d animations\n", animation_count);
    return animation_count > 0;
}

static void next_random_animation(void) {
    if (animation_count <= 1) {
        current_frame = 0;
        return;
    }
    int next;
    do {
        next = simple_rand() % animation_count;
    } while (next == current_anim);
    current_anim = next;
    current_frame = 0;
    printf("Switching to '%s' (%d frames, %d FPS)\n",
           animations[current_anim].name,
           animations[current_anim].frame_count,
           animations[current_anim].fps);
}

void start_eye_animation(void) {
    rng_state = (uint32_t)to_ms_since_boot(get_absolute_time());

    printf("\n=== BENDER EYE ANIMATION - SD CARD MODE ===\n");
    printf("Playing: '%s'\n", animations[current_anim].name);

    if (sd_load_frame(animations[current_anim].name, 0)) {
        render_frame();
    }
    last_frame_time = to_ms_since_boot(get_absolute_time());

    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        uint32_t delay = animations[current_anim].frame_delay_ms;

        if (now - last_frame_time >= delay) {
            current_frame++;
            if (current_frame >= animations[current_anim].frame_count) {
                next_random_animation();
            }

            if (sd_load_frame(animations[current_anim].name, current_frame)) {
                render_frame();
            }
            last_frame_time = now;
        }

        sleep_ms(2);
    }
}
