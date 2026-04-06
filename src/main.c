#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "display.h"
#include "anim_data.h"
#include "unpack.h"

#define LED_PIN 25
#define ROW_BYTES (TFT_WIDTH * 2)

static uint8_t frame_buf[RAW_FRAME_SIZE];

static void fill_color(uint16_t color) {
    uint16_t *fb16 = (uint16_t *)frame_buf;
    for (int i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) fb16[i] = color;
}

static void send_framebuf(void) {
    printf("send left...\n"); fflush(stdout);
    set_window(&left_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_put(LEFT_DC, 1);
    gpio_put(LEFT_CS, 0);
    for (int y = 0; y < TFT_HEIGHT; y++) {
        spi_write_blocking(spi0, frame_buf + y * ROW_BYTES, ROW_BYTES);
    }
    gpio_put(LEFT_CS, 1);
    printf("send left done\n"); fflush(stdout);

    printf("send right...\n"); fflush(stdout);
    set_window(&right_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_put(RIGHT_DC, 1);
    gpio_put(RIGHT_CS, 0);
    for (int y = 0; y < TFT_HEIGHT; y++) {
        spi_write_blocking(spi1, frame_buf + y * ROW_BYTES, ROW_BYTES);
    }
    gpio_put(RIGHT_CS, 1);
    printf("send right done\n"); fflush(stdout);
}

int main(void) {
    stdio_init_all();
    sleep_ms(3000);
    printf("BOOT\n"); fflush(stdout);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    printf("backlight...\n"); fflush(stdout);
    init_backlight();
    printf("backlight on\n"); fflush(stdout);

    printf("spi init...\n"); fflush(stdout);
    init_spi_pins();
    printf("spi init done\n"); fflush(stdout);

    printf("display init...\n"); fflush(stdout);
    init_displays();
    printf("display init done\n"); fflush(stdout);

    printf("red fill\n"); fflush(stdout);
    fill_color(0xF800);
    send_framebuf();
    sleep_ms(2000);

    printf("green fill\n"); fflush(stdout);
    fill_color(0x07E0);
    send_framebuf();
    sleep_ms(2000);

    printf("blue fill\n"); fflush(stdout);
    fill_color(0x001F);
    send_framebuf();
    sleep_ms(2000);

    printf("yellow fill\n"); fflush(stdout);
    fill_color(COLOR_YELLOW);
    send_framebuf();
    sleep_ms(2000);

    printf("anim start\n"); fflush(stdout);

    uint32_t rng_state = (uint32_t)to_ms_since_boot(get_absolute_time());
    int cur = 0;

    while (1) {
        const anim_t *a = &anims[cur];
        printf("anim: %s\n", a->name); fflush(stdout);
        for (int f = 0; f < a->frames; f++) {
            uint32_t t0 = to_ms_since_boot(get_absolute_time());

            const uint8_t *bit_data = a->frames_data[f];
            unpack_1bit_to_rgb565(bit_data, frame_buf);
            send_framebuf();

            gpio_put(LED_PIN, f & 1);
            uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - t0;
            if (elapsed < a->delay_ms) sleep_ms(a->delay_ms - elapsed);
        }
        if (ANIM_COUNT > 1) {
            rng_state ^= rng_state << 13;
            rng_state ^= rng_state >> 17;
            rng_state ^= rng_state << 5;
            int next;
            do { next = rng_state % ANIM_COUNT; } while (next == cur);
            cur = next;
        }
    }
}
