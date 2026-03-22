#include <stdio.h>
#include <stdint.h>
#include <cmath>
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

// Левый глаз
#define LEFT_CS   20
#define LEFT_DC   16  
#define LEFT_RST  18
#define LEFT_MOSI 7
#define LEFT_SCLK 6

// Правый глаз  
#define RIGHT_CS   21
#define RIGHT_DC   17
#define RIGHT_RST  19
#define RIGHT_MOSI 11
#define RIGHT_SCLK 10

#define TFT_BLK 22
#define TFT_WIDTH 240
#define TFT_HEIGHT 240
#define BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT)

// Буфер для синхронной отрисовки
uint16_t frame_buffer[BUFFER_SIZE];

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

void init_display(st7789_config_t *config) {
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
    write_data(spi, 0x00, cs, dc);
    
    write_cmd(spi, 0xB2, cs, dc);
    write_data(spi, 0x0C, cs, dc);
    write_data(spi, 0x0C, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0x33, cs, dc);
    write_data(spi, 0x33, cs, dc);
    
    write_cmd(spi, 0xB7, cs, dc);
    write_data(spi, 0x35, cs, dc);
    
    write_cmd(spi, 0xBB, cs, dc);
    write_data(spi, 0x19, cs, dc);
    
    write_cmd(spi, 0xC0, cs, dc);
    write_data(spi, 0x2C, cs, dc);
    
    write_cmd(spi, 0xC2, cs, dc);
    write_data(spi, 0x01, cs, dc);
    
    write_cmd(spi, 0xC3, cs, dc);
    write_data(spi, 0x12, cs, dc);
    
    write_cmd(spi, 0xC4, cs, dc);
    write_data(spi, 0x20, cs, dc);
    
    write_cmd(spi, 0xC6, cs, dc);
    write_data(spi, 0x0F, cs, dc);
    
    write_cmd(spi, 0xD0, cs, dc);
    write_data(spi, 0xA4, cs, dc);
    write_data(spi, 0xA1, cs, dc);
    
    write_cmd(spi, 0xE0, cs, dc);
    write_data(spi, 0xD0, cs, dc);
    write_data(spi, 0x04, cs, dc);
    write_data(spi, 0x0D, cs, dc);
    write_data(spi, 0x11, cs, dc);
    write_data(spi, 0x13, cs, dc);
    write_data(spi, 0x2B, cs, dc);
    write_data(spi, 0x3F, cs, dc);
    write_data(spi, 0x54, cs, dc);
    write_data(spi, 0x4C, cs, dc);
    write_data(spi, 0x18, cs, dc);
    write_data(spi, 0x0D, cs, dc);
    write_data(spi, 0x0B, cs, dc);
    write_data(spi, 0x1F, cs, dc);
    write_data(spi, 0x23, cs, dc);
    
    write_cmd(spi, 0xE1, cs, dc);
    write_data(spi, 0xD0, cs, dc);
    write_data(spi, 0x04, cs, dc);
    write_data(spi, 0x0C, cs, dc);
    write_data(spi, 0x11, cs, dc);
    write_data(spi, 0x13, cs, dc);
    write_data(spi, 0x2C, cs, dc);
    write_data(spi, 0x3F, cs, dc);
    write_data(spi, 0x44, cs, dc);
    write_data(spi, 0x51, cs, dc);
    write_data(spi, 0x2F, cs, dc);
    write_data(spi, 0x1F, cs, dc);
    write_data(spi, 0x1F, cs, dc);
    write_data(spi, 0x20, cs, dc);
    write_data(spi, 0x23, cs, dc);
    
    write_cmd(spi, 0x21, cs, dc);
    
    write_cmd(spi, 0x29, cs, dc);
    sleep_ms(50);
}

void set_window(spi_inst_t *spi, uint cs, uint dc) {
    write_cmd(spi, 0x2A, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0xEF, cs, dc);
    
    write_cmd(spi, 0x2B, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0x00, cs, dc);
    write_data(spi, 0xEF, cs, dc);
    
    write_cmd(spi, 0x2C, cs, dc);
}

void prepare_eye_buffer(bool open) {
    if(open) {
        // Глаз открыт
        for(int y = 0; y < TFT_HEIGHT; y++) {
            for(int x = 0; x < TFT_WIDTH; x++) {
                int dx = x - 120;
                int dy = y - 120;
                int dist_sq = dx*dx + dy*dy;
                
                if(dist_sq < 8100) {  // 90^2 - белый склер
                    if(dist_sq < 2025) {  // 45^2 - зеленый зрачок
                        if(dist_sq < 144) {  // 12^2 - белый блик
                            frame_buffer[y * TFT_WIDTH + x] = 0xFFFF;
                        } else {
                            frame_buffer[y * TFT_WIDTH + x] = 0x07E0;
                        }
                    } else {
                        frame_buffer[y * TFT_WIDTH + x] = 0xFFFF;
                    }
                } else {
                    frame_buffer[y * TFT_WIDTH + x] = 0x0000;
                }
            }
        }
        
        // Красные круги
        for(int radius = 48; radius <= 50; radius++) {
            for(int angle = 0; angle < 360; angle++) {
                int x = 120 + radius * cos(angle * 3.14159 / 180);
                int y = 120 + radius * sin(angle * 3.14159 / 180);
                if(x >= 0 && x < TFT_WIDTH && y >= 0 && y < TFT_HEIGHT) {
                    frame_buffer[y * TFT_WIDTH + x] = 0xF800;
                }
            }
        }
    } else {
        // Глаз закрыт
        for(int i = 0; i < BUFFER_SIZE; i++) {
            frame_buffer[i] = 0x0000;
        }
        
        for(int y = 70; y < 170; y++) {
            for(int x = 30; x < 210; x++) {
                frame_buffer[y * TFT_WIDTH + x] = 0xFFFF;
            }
        }
    }
}

void sync_update_both_displays() {
    // Устанавливаем окна для обоих дисплеев
    set_window(spi0, LEFT_CS, LEFT_DC);
    set_window(spi1, RIGHT_CS, RIGHT_DC);
    
    // Переводим оба дисплея в режим данных
    gpio_put(LEFT_DC, 1);
    gpio_put(RIGHT_DC, 1);
    
    // Опускаем CS для обоих одновременно
    gpio_put(LEFT_CS, 0);
    gpio_put(RIGHT_CS, 0);
    
    // Синхронная отправка данных
    for(int i = 0; i < BUFFER_SIZE; i++) {
        uint8_t high = frame_buffer[i] >> 8;
        uint8_t low = frame_buffer[i] & 0xFF;
        
        // Отправляем старший байт на оба дисплея
        spi_write_blocking(spi0, &high, 1);
        spi_write_blocking(spi1, &high, 1);
        
        // Отправляем младший байт на оба дисплея
        spi_write_blocking(spi0, &low, 1);
        spi_write_blocking(spi1, &low, 1);
    }
    
    // Поднимаем CS для обоих
    gpio_put(LEFT_CS, 1);
    gpio_put(RIGHT_CS, 1);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("START\n");
    
    // Подсветка
    gpio_init(TFT_BLK);
    gpio_set_dir(TFT_BLK, GPIO_OUT);
    gpio_put(TFT_BLK, 1);
    
    // SPI0 - левый
    spi_init(spi0, 20000000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(LEFT_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(LEFT_MOSI, GPIO_FUNC_SPI);
    
    // SPI1 - правый
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
    
    printf("Init displays\n");
    init_display(&left_cfg);
    init_display(&right_cfg);
    
    printf("Drawing eyes\n");
    prepare_eye_buffer(true);
    sync_update_both_displays();
    
    printf("Ready! Blinking...\n");
    
    // Моргание
    bool eyes_open = true;
    absolute_time_t last_blink = get_absolute_time();
    
    while(1) {
        if(absolute_time_diff_us(last_blink, get_absolute_time()) > 3000000) {
            last_blink = get_absolute_time();
            
            if(eyes_open) {
                prepare_eye_buffer(false);
                eyes_open = false;
            } else {
                prepare_eye_buffer(true);
                eyes_open = true;
            }
            
            sync_update_both_displays();
        }
        sleep_ms(10);
    }
}