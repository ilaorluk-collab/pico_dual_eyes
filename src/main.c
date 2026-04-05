#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "display.h"
#include "hw_config.h"
#include "ff.h"
#include "f_util.h"
#include "rtc.h"
#include "my_debug.h"

#define LED_PIN 25

static void blink_fatal(int count) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (1) {
        for (int i = 0; i < count; i++) {
            gpio_put(LED_PIN, 1); sleep_ms(150);
            gpio_put(LED_PIN, 0); sleep_ms(150);
        }
        sleep_ms(1000);
    }
}

#define MAX_ANIMS 8
#define MAX_NAME 16
#define ROW_BYTES (TFT_WIDTH * 2)

typedef struct {
    char name[MAX_NAME];
    int frames;
    int fps;
    uint32_t delay_ms;
} anim_t;

static anim_t anims[MAX_ANIMS];
static int anim_count = 0;
static uint8_t frame_buf[RAW_SIZE];

static uint32_t rng_state = 42;
static uint32_t rand32(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static bool parse_manifest(void) {
    FIL fil;
    if (f_open(&fil, "/bender/manifest.txt", FA_READ) != FR_OK) return false;
    char line[64];
    anim_count = 0;
    while (f_gets(line, sizeof(line), &fil)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (anim_count >= MAX_ANIMS) break;
        anim_t *a = &anims[anim_count];
        if (sscanf(line, "%15[^:]:%d:%d", a->name, &a->frames, &a->fps) == 3) {
            a->delay_ms = 1000 / a->fps;
            anim_count++;
        }
    }
    f_close(&fil);
    return anim_count > 0;
}

static void clear_displays(void) {
    memset(frame_buf, 0, RAW_SIZE);
    gpio_put(SD_CS, 1);
    set_window(&left_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    set_window(&right_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_put(LEFT_DC, 1);
    gpio_put(RIGHT_DC, 1);
    for (int y = 0; y < TFT_HEIGHT; y++) {
        uint8_t *row = frame_buf + y * ROW_BYTES;
        gpio_put(LEFT_CS, 0);
        spi_write_blocking(spi0, row, ROW_BYTES);
        gpio_put(LEFT_CS, 1);
        gpio_put(RIGHT_CS, 0);
        spi_write_blocking(spi1, row, ROW_BYTES);
        gpio_put(RIGHT_CS, 1);
    }
}

static bool load_and_render(const char *anim, int idx) {
    char path[64];
    snprintf(path, sizeof(path), "/bender/%s/f%02d.raw", anim, idx);

    FIL fil;
    if (f_open(&fil, path, FA_READ) != FR_OK) { blink_fatal(4); return false; }

    UINT br;
    FRESULT fr = f_read(&fil, frame_buf, RAW_SIZE, &br);
    f_close(&fil);
    if (fr != FR_OK || br != RAW_SIZE) blink_fatal(5);

    gpio_put(SD_CS, 1);
    set_window(&left_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    set_window(&right_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_put(LEFT_DC, 1);
    gpio_put(RIGHT_DC, 1);

    for (int y = 0; y < TFT_HEIGHT; y++) {
        uint8_t *row = frame_buf + y * ROW_BYTES;
        gpio_put(LEFT_CS, 0);
        spi_write_blocking(spi0, row, ROW_BYTES);
        gpio_put(LEFT_CS, 1);
        gpio_put(RIGHT_CS, 0);
        spi_write_blocking(spi1, row, ROW_BYTES);
        gpio_put(RIGHT_CS, 1);
    }
    return true;
}
int main() {
    stdio_init_all();
    sleep_ms(500);

    init_backlight();
    init_spi_pins();

    time_init();
    sd_card_t *pSD = sd_get_by_num(0);
    if (!pSD) blink_fatal(1);
    if (f_mount(&pSD->fatfs, pSD->pcName, 1) != FR_OK) blink_fatal(2);

    init_displays();
    clear_displays();

    if (!parse_manifest()) blink_fatal(3);

    rng_state = (uint32_t)to_ms_since_boot(get_absolute_time());
    int cur = 0;

    while (1) {
        anim_t *a = &anims[cur];
        for (int f = 0; f < a->frames; f++) {
            uint32_t t0 = to_ms_since_boot(get_absolute_time());
            load_and_render(a->name, f);
            uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - t0;
            if (elapsed < a->delay_ms) sleep_ms(a->delay_ms - elapsed);
        }
        if (anim_count > 1) {
            int next;
            do { next = rand32() % anim_count; } while (next == cur);
            cur = next;
        }
    }
}