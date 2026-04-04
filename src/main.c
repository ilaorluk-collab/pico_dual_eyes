#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "display.h"
#include "hw_config.h"
#include "ff.h"
#include "f_util.h"
#include "rtc.h"
#include "my_debug.h"

static void fill_solid(spi_inst_t *spi, uint cs, uint dc, uint16_t color) {
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;
    set_window(spi, cs, dc, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_put(dc, 1);
    gpio_put(cs, 0);
    for (int i = 0; i < TFT_WIDTH * 2; i++)
        row_buffer[i] = (i & 1) ? lo : hi;
    for (int y = 0; y < TFT_HEIGHT; y++)
        spi_write_blocking(spi, row_buffer, TFT_WIDTH * 2);
    gpio_put(cs, 1);
}

static void fill_both(uint16_t color) {
    gpio_put(SD_CS, 1);
    fill_solid(spi0, LEFT_CS, LEFT_DC, color);
    fill_solid(spi1, RIGHT_CS, RIGHT_DC, color);
}

static bool draw_bmp(const char *filename) {
    printf("BMP: %s\n", filename);
    gpio_put(SD_CS, 1);
    gpio_put(LEFT_CS, 1);
    gpio_put(RIGHT_CS, 1);

    FIL fil;
    FRESULT fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) { printf("open fail: %d\n", fr); return false; }

    uint8_t hdr[54];
    UINT br;
    fr = f_read(&fil, hdr, 54, &br);
    if (fr != FR_OK || br < 54) { printf("hdr fail\n"); f_close(&fil); return false; }
    if (hdr[0] != 'B' || hdr[1] != 'M') { printf("not BM\n"); f_close(&fil); return false; }

    uint32_t comp = hdr[30] | (hdr[31] << 8) | (hdr[32] << 16) | (hdr[33] << 24);
    if (comp != 0) { printf("compressed\n"); f_close(&fil); return false; }

    uint32_t data_off = hdr[10] | (hdr[11] << 8) | (hdr[12] << 16) | (hdr[13] << 24);
    int32_t w = hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24);
    int32_t h = hdr[22] | (hdr[23] << 8) | (hdr[24] << 16) | (hdr[25] << 24);
    uint16_t bpp = hdr[28] | (hdr[29] << 8);

    printf("BMP: %ldx%ld %dbpp\n", (long)w, (long)h, bpp);
    if (bpp != 24) { printf("need 24bpp\n"); f_close(&fil); return false; }

    bool top_down = h < 0;
    if (top_down) h = -h;
    int dw = w > TFT_WIDTH ? TFT_WIDTH : (int)w;
    int dh = h > TFT_HEIGHT ? TFT_HEIGHT : (int)h;
    int rb = ((w * 3 + 3) & ~3);
    int ox = (TFT_WIDTH - dw) / 2 + LEFT_OFFSET_X;
    int oy = (TFT_HEIGHT - dh) / 2 + LEFT_OFFSET_Y;
    printf("draw %dx%d offset (%d,%d)\n", dw, dh, ox, oy);

    uint8_t bmp_row[720];
    for (int y = 0; y < dh; y++) {
        int sy = top_down ? y : (h - 1 - y);
        gpio_put(SD_CS, 1);
        gpio_put(LEFT_CS, 1);
        gpio_put(RIGHT_CS, 1);
        f_lseek(&fil, data_off + (uint32_t)sy * rb);
        fr = f_read(&fil, bmp_row, rb, &br);
        if (fr != FR_OK || (int)br != rb) break;

        for (int x = 0; x < dw; x++) {
            uint8_t b = bmp_row[x * 3];
            uint8_t g = bmp_row[x * 3 + 1];
            uint8_t r = bmp_row[x * 3 + 2];
            uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            row_buffer[x * 2]     = (c >> 8) & 0xFF;
            row_buffer[x * 2 + 1] = c & 0xFF;
        }

        gpio_put(SD_CS, 1);
        set_window(spi0, LEFT_CS, LEFT_DC, ox, oy + y, ox + dw - 1, oy + y);
        set_window(spi1, RIGHT_CS, RIGHT_DC,
                   ox + RIGHT_SHIFT_X, oy + y + RIGHT_SHIFT_Y,
                   ox + dw - 1 + RIGHT_SHIFT_X, oy + y + RIGHT_SHIFT_Y);
        gpio_put(LEFT_DC, 1);
        gpio_put(RIGHT_DC, 1);
        gpio_put(LEFT_CS, 0);
        gpio_put(RIGHT_CS, 0);
        spi_write_blocking(spi0, row_buffer, dw * 2);
        spi_write_blocking(spi1, row_buffer, dw * 2);
        gpio_put(LEFT_CS, 1);
        gpio_put(RIGHT_CS, 1);
    }

    f_close(&fil);
    printf("BMP done!\n");
    return true;
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("\n=== BMP DISPLAY TEST ===\n");

    init_backlight();
    printf("BL OK\n");

    init_spi_pins();
    init_displays();
    printf("DISP OK\n");

    fill_both(0x07E0);
    printf("GREEN OK\n");
    sleep_ms(500);

    time_init();
    sd_card_t *pSD = sd_get_by_num(0);
    if (!pSD) { printf("FATAL: no SD\n"); while(1) sleep_ms(1000); }

    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (fr != FR_OK) { printf("FATAL: mount %d\n", fr); while(1) sleep_ms(1000); }
    printf("SD OK\n");

    if (!draw_bmp("/mario.bmp")) {
        printf("BMP FAIL\n");
        fill_both(0x001F);
    }

    printf("ALL DONE\n");
    while (1) sleep_ms(1000);
}
