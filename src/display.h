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

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define RIGHT_SHIFT_X 0
#define RIGHT_SHIFT_Y 80

typedef struct {
    spi_inst_t *spi;
    uint cs;
    uint dc;
    uint rst;
    uint col_offset;
    uint row_offset;
} st7789_config_t;

extern st7789_config_t left_cfg, right_cfg;

void init_backlight(void);
void init_spi_pins(void);
void init_displays(void);
void write_cmd(spi_inst_t *spi, uint8_t cmd, uint cs, uint dc);
void write_data(spi_inst_t *spi, uint8_t data, uint cs, uint dc);
void set_window(st7789_config_t *cfg, uint x0, uint y0, uint x1, uint y1);

#endif
