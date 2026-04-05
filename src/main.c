#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "display.h"
#include "flash_anim.h"
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

#define ROW_BYTES (TFT_WIDTH * 2)

static flash_anim_t anims[MAX_ANIMS];
static int anim_count = 0;
static uint8_t frame_buf[RAW_FRAME_SIZE];

static uint32_t rng_state = 42;
static uint32_t rand32(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static bool preload_animations_to_flash(void) {
    static uint8_t prog_buf[FLASH_FRAME_SIZE];

    FIL fil;
    if (f_open(&fil, "/bender_bit/manifest.txt", FA_READ) != FR_OK) return false;

    char line[64];
    anim_count = 0;
    int total_frames = 0;

    while (f_gets(line, sizeof(line), &fil)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (anim_count >= MAX_ANIMS) break;
        flash_anim_t *a = &anims[anim_count];
        if (sscanf(line, "%15[^:]:%d:%d", a->name, &a->frames, &a->fps) == 3) {
            a->delay_ms = 1000 / a->fps;
            total_frames += a->frames;
            anim_count++;
        }
    }
    f_close(&fil);

    if (anim_count == 0) return false;

    uint32_t total_size = (uint32_t)total_frames * FLASH_FRAME_SIZE;
    uint32_t aligned_size = (total_size + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1);
    if (aligned_size > ANIM_FLASH_MAX_BYTES) return false;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    multicore_lockout_start_blocking();
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(ANIM_FLASH_OFFSET, aligned_size);
    restore_interrupts(ints);
    multicore_lockout_end_blocking();

    uint32_t cur_offset = 0;
    for (int a = 0; a < anim_count; a++) {
        flash_anim_t *anim = &anims[a];
        anim->flash_offset = cur_offset;

        for (int f = 0; f < anim->frames; f++) {
            gpio_put(LED_PIN, (a + f) & 1);

            char path[64];
            snprintf(path, sizeof(path), "/bender_bit/%s/f%02d.1bit", anim->name, f);

            FIL frame_fil;
            if (f_open(&frame_fil, path, FA_READ) != FR_OK) return false;

            memset(prog_buf, 0xFF, sizeof(prog_buf));
            UINT br;
            FRESULT fr = f_read(&frame_fil, prog_buf, BIT_FRAME_SIZE, &br);
            f_close(&frame_fil);
            if (fr != FR_OK || br != BIT_FRAME_SIZE) return false;

            multicore_lockout_start_blocking();
            ints = save_and_disable_interrupts();
            flash_range_program(ANIM_FLASH_OFFSET + cur_offset, prog_buf, FLASH_SECTOR_SIZE);
            flash_range_program(ANIM_FLASH_OFFSET + cur_offset + FLASH_SECTOR_SIZE,
                                prog_buf + FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
            restore_interrupts(ints);
            multicore_lockout_end_blocking();

            cur_offset += FLASH_FRAME_SIZE;
        }
    }

    gpio_put(LED_PIN, 0);
    return true;
}

static void clear_displays(void) {
    memset(frame_buf, 0, RAW_FRAME_SIZE);
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

static bool load_and_render(flash_anim_t *anim, int idx) {
    const uint8_t *bit_data = (const uint8_t *)(XIP_BASE + ANIM_FLASH_OFFSET +
                            anim->flash_offset + (uint32_t)idx * FLASH_FRAME_SIZE);
    unpack_1bit_to_rgb565(bit_data, frame_buf);

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

    multicore_launch_core1(multicore_lockout_victim_init);

    if (!preload_animations_to_flash()) blink_fatal(3);

    f_mount(NULL, pSD->pcName, 0);

    init_displays();
    spi_set_baudrate(spi0, 62 * 1000 * 1000);
    clear_displays();

    rng_state = (uint32_t)to_ms_since_boot(get_absolute_time());
    int cur = 0;

    while (1) {
        flash_anim_t *a = &anims[cur];
        for (int f = 0; f < a->frames; f++) {
            uint32_t t0 = to_ms_since_boot(get_absolute_time());
            load_and_render(a, f);
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
