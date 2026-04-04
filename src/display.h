#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/spi.h"

#define LEFT_CS   20
#define LEFT_DC   16
#define LEFT_RST  18
#define LEFT_MOSI 7
#define LEFT_SCLK 6

#define RIGHT_CS   21
#define RIGHT_DC   17
#define RIGHT_RST  19
#define RIGHT_MOSI 11
#define RIGHT_SCLK 10

#define TFT_BLK 22

#define SD_CS   5
#define SD_MOSI 7
#define SD_SCLK 6
#define SD_MISO 4

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define LEFT_OFFSET_X  0
#define LEFT_OFFSET_Y  0

#define RIGHT_SHIFT_X 0
#define RIGHT_SHIFT_Y 80

#define COLOR_BLACK  0x0000
#define COLOR_YELLOW 0xFFE0

#define FRAME_BYTES_PER_ROW 30
#define FRAME_SIZE (TFT_HEIGHT * FRAME_BYTES_PER_ROW)

#define MAX_ANIMATIONS 16
#define MAX_ANIM_NAME  16

typedef struct {
    spi_inst_t *spi;
    uint cs;
    uint dc;
    uint rst;
} st7789_config_t;

typedef struct {
    char name[MAX_ANIM_NAME];
    int frame_count;
    int fps;
    uint32_t frame_delay_ms;
} animation_info_t;

extern st7789_config_t left_cfg, right_cfg;
extern uint8_t frame_buffer[FRAME_SIZE];
extern uint8_t row_buffer[TFT_WIDTH * 2];
extern animation_info_t animations[MAX_ANIMATIONS];

void init_backlight(void);
void init_spi_pins(void);
void init_displays(void);
void write_cmd(spi_inst_t *spi, uint8_t cmd, uint cs, uint dc);
void write_data(spi_inst_t *spi, uint8_t data, uint cs, uint dc);
void set_window(spi_inst_t *spi, uint cs, uint dc, uint x0, uint y0, uint x1, uint y1);

#endif
