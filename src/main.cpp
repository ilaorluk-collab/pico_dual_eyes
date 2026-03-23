#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

typedef struct {
    spi_inst_t *spi;
    uint cs;
    uint dc;
    uint rst;
} st7789_config_t;

st7789_config_t left_cfg, right_cfg;

// Пины
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
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

// Цвета
#define COLOR_BLACK     0x0000
#define COLOR_YELLOW    0xFFE0

// Параметры глаза (после поворота координаты меняются)
#define EYE_RADIUS      110
#define PUPIL_SIZE      16
#define PUPIL_HALF      8
#define CENTER_X        120
#define CENTER_Y        120
#define PUPIL_MOVE_LIMIT 45

// Параметры "нахмуривания" (черный квадрат сверху после поворота)
#define SQUEEZE_HEIGHT  60     // Высота черного квадрата
#define SQUEEZE_DURATION_MS 600
#define NORMAL_DURATION_MS 4000

// Смещение для правого дисплея
#define RIGHT_SHIFT_X   0
#define RIGHT_SHIFT_Y   80

// Тайминги движения
#define MOVE_INTERVAL_MS 2000
#define STEP_DELAY_MS    2

void write_cmd(spi_inst_t *spi, uint8_t cmd, uint cs, uint dc) {
    gpio_put(dc, 0);
    gpio_put(cs, 0);
    spi_write_blocking(spi, &cmd, 1);
    gpio_put(cs, 1);
}

void write_data(spi_inst_t *spi, uint8_t data, uint cs, uint dc) {
    gpio_put(dc, 1);
    gpio_put(cs, 0);
    spi_write_blocking(spi, &data, 1);
    gpio_put(cs, 1);
}

void init_display_with_orientation(st7789_config_t *config, uint8_t madctl) {
    uint cs = config->cs;
    uint dc = config->dc;
    uint rst = config->rst;
    spi_inst_t *spi = config->spi;
    
    gpio_put(rst, 0);
    sleep_ms(15);
    gpio_put(rst, 1);
    sleep_ms(120);
    
    write_cmd(spi, 0x11, cs, dc);
    sleep_ms(120);
    
    write_cmd(spi, 0x3A, cs, dc);
    write_data(spi, 0x55, cs, dc);
    
    write_cmd(spi, 0x36, cs, dc);
    write_data(spi, madctl, cs, dc);
    
    write_cmd(spi, 0x21, cs, dc);
    
    write_cmd(spi, 0x29, cs, dc);
    sleep_ms(50);
}

void set_window(spi_inst_t *spi, uint cs, uint dc, uint x0, uint y0, uint x1, uint y1) {
    write_cmd(spi, 0x2A, cs, dc);
    write_data(spi, (x0 >> 8) & 0xFF, cs, dc);
    write_data(spi, x0 & 0xFF, cs, dc);
    write_data(spi, (x1 >> 8) & 0xFF, cs, dc);
    write_data(spi, x1 & 0xFF, cs, dc);
    
    write_cmd(spi, 0x2B, cs, dc);
    write_data(spi, (y0 >> 8) & 0xFF, cs, dc);
    write_data(spi, y0 & 0xFF, cs, dc);
    write_data(spi, (y1 >> 8) & 0xFF, cs, dc);
    write_data(spi, y1 & 0xFF, cs, dc);
    
    write_cmd(spi, 0x2C, cs, dc);
}

void draw_eye_with_pupil(int pupil_offset_x, int pupil_offset_y, bool squeezing) {
    int pupil_center_x = CENTER_X + pupil_offset_x;
    int pupil_center_y = CENTER_Y + pupil_offset_y;
    
    // Верхняя граница глаза (после поворота это левая граница)
    int eye_top = CENTER_Y - EYE_RADIUS;
    
    for(int y = 0; y < TFT_HEIGHT; y++) {
        // Левый дисплей - без смещения
        set_window(spi0, LEFT_CS, LEFT_DC, 0, y, TFT_WIDTH-1, y);
        
        // Правый дисплей - со смещением
        set_window(spi1, RIGHT_CS, RIGHT_DC, 
                   RIGHT_SHIFT_X, 
                   y + RIGHT_SHIFT_Y, 
                   TFT_WIDTH-1 + RIGHT_SHIFT_X, 
                   y + RIGHT_SHIFT_Y);
        
        gpio_put(LEFT_DC, 1);
        gpio_put(RIGHT_DC, 1);
        gpio_put(LEFT_CS, 0);
        gpio_put(RIGHT_CS, 0);
        
        for(int x = 0; x < TFT_WIDTH; x++) {
            // После поворота на 90°, Y становится X, а X становится Y
            // Поворачиваем координаты для правильной отрисовки
            int rotated_x = y;
            int rotated_y = TFT_WIDTH - 1 - x;
            
            int dx = rotated_x - CENTER_X;
            int dy = rotated_y - CENTER_Y;
            int distance_sq = dx*dx + dy*dy;
            
            uint16_t color;
            
            // Проверяем закрывание сверху (после поворота это проверка по rotated_y)
            bool is_squeezed = false;
            if(squeezing && rotated_y >= eye_top && rotated_y < eye_top + SQUEEZE_HEIGHT) {
                is_squeezed = true;
            }
            
            if(is_squeezed) {
                color = COLOR_BLACK;
            } else if(distance_sq <= EYE_RADIUS * EYE_RADIUS) {
                int pupil_dx = rotated_x - pupil_center_x;
                int pupil_dy = rotated_y - pupil_center_y;
                
                if(abs(pupil_dx) <= PUPIL_HALF && abs(pupil_dy) <= PUPIL_HALF) {
                    color = COLOR_BLACK;
                } else {
                    color = COLOR_YELLOW;
                }
            } else {
                color = COLOR_BLACK;
            }
            
            uint8_t bytes[2];
            bytes[0] = (color >> 8) & 0xFF;
            bytes[1] = color & 0xFF;
            
            spi_write_blocking(spi0, bytes, 2);
            spi_write_blocking(spi1, bytes, 2);
        }
        
        gpio_put(LEFT_CS, 1);
        gpio_put(RIGHT_CS, 1);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("=== BENDER EYES - ROTATED 90 DEGREES ===\n");
    
    // Подсветка
    gpio_init(TFT_BLK);
    gpio_set_dir(TFT_BLK, GPIO_OUT);
    gpio_put(TFT_BLK, 1);
    
    // SPI0
    spi_init(spi0, 20000000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(LEFT_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(LEFT_MOSI, GPIO_FUNC_SPI);
    
    // SPI1
    spi_init(spi1, 20000000);
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(RIGHT_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(RIGHT_MOSI, GPIO_FUNC_SPI);
    
    // Пины
    gpio_init(LEFT_CS); gpio_set_dir(LEFT_CS, GPIO_OUT); gpio_put(LEFT_CS, 1);
    gpio_init(LEFT_DC); gpio_set_dir(LEFT_DC, GPIO_OUT);
    gpio_init(LEFT_RST); gpio_set_dir(LEFT_RST, GPIO_OUT);
    
    gpio_init(RIGHT_CS); gpio_set_dir(RIGHT_CS, GPIO_OUT); gpio_put(RIGHT_CS, 1);
    gpio_init(RIGHT_DC); gpio_set_dir(RIGHT_DC, GPIO_OUT);
    gpio_init(RIGHT_RST); gpio_set_dir(RIGHT_RST, GPIO_OUT);
    
    gpio_put(LEFT_RST, 1);
    gpio_put(RIGHT_RST, 1);
    
    left_cfg.spi = spi0;
    left_cfg.cs = LEFT_CS;
    left_cfg.dc = LEFT_DC;
    left_cfg.rst = LEFT_RST;
    
    right_cfg.spi = spi1;
    right_cfg.cs = RIGHT_CS;
    right_cfg.dc = RIGHT_DC;
    right_cfg.rst = RIGHT_RST;
    
    printf("Init displays...\n");
    // Поворачиваем оба дисплея на 90 градусов (MV бит = 0x20)
    init_display_with_orientation(&left_cfg, 0x40);   // 90° влево
    init_display_with_orientation(&right_cfg, 0x80);

    printf("Displays rotated 90 degrees\n");
    printf("Squeeze height: %d pixels from top\n", SQUEEZE_HEIGHT);
    
    // Параметры движения
    int pupil_x = 0, pupil_y = 0;
    int dir_x = 1, dir_y = 1;
    bool squeezing = false;
    
    absolute_time_t last_direction_change = get_absolute_time();
    absolute_time_t last_squeeze_change = get_absolute_time();
    
    // Рисуем начальное положение
    draw_eye_with_pupil(pupil_x, pupil_y, squeezing);
    
    printf("\n=== BENDER IS WATCHING ===\n");
    printf("Both displays rotated 90 degrees left\n");
    printf("Eyelids should close from the TOP now!\n");
    
    while(1) {
        if(absolute_time_diff_us(last_direction_change, get_absolute_time()) > MOVE_INTERVAL_MS * 1000) {
            last_direction_change = get_absolute_time();
            
            dir_x = (rand() % 3) - 1;
            dir_y = (rand() % 3) - 1;
            
            if(dir_x == 0 && dir_y == 0) {
                dir_x = 1;
                dir_y = 1;
            }
        }
        
        if(absolute_time_diff_us(last_squeeze_change, get_absolute_time()) > 
           (squeezing ? SQUEEZE_DURATION_MS : NORMAL_DURATION_MS) * 1000) {
            last_squeeze_change = get_absolute_time();
            squeezing = !squeezing;
            printf("%s\n", squeezing ? ">:( Eyelids closing!" : ":) Eyes open");
        }
        
        int new_x = pupil_x + dir_x;
        int new_y = pupil_y + dir_y;
        
        if(abs(new_x) <= PUPIL_MOVE_LIMIT) {
            pupil_x = new_x;
        } else {
            dir_x = -dir_x;
        }
        
        if(abs(new_y) <= PUPIL_MOVE_LIMIT) {
            pupil_y = new_y;
        } else {
            dir_y = -dir_y;
        }
        
        draw_eye_with_pupil(pupil_x, pupil_y, squeezing);
        sleep_ms(STEP_DELAY_MS);
    }
}