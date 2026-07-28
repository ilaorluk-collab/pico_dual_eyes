#include "display.h"
#include "unpack.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"

st7789_config_t left_cfg, right_cfg;

/* ── DMA state ─────────────────────────────────────────────── */
static int dma_chan_left  = -1;
static int dma_chan_right = -1;
static dma_channel_config dma_cfg_left, dma_cfg_right;

/* ── Backlight ─────────────────────────────────────────────── */
void init_backlight(void) {
    gpio_init(TFT_BLK);
    gpio_set_dir(TFT_BLK, GPIO_OUT);
    gpio_put(TFT_BLK, 1);
}

/* ── SPI + GPIO init (пины не меняем, скорость 25→50 MHz) ─── */
void init_spi_pins(void) {
    spi_init(spi0, 25 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(LEFT_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(LEFT_MOSI, GPIO_FUNC_SPI);

    spi_init(spi1, 25 * 1000 * 1000);
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(RIGHT_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(RIGHT_MOSI, GPIO_FUNC_SPI);

    gpio_init(LEFT_CS);  gpio_set_dir(LEFT_CS,  GPIO_OUT); gpio_put(LEFT_CS,  1);
    gpio_init(LEFT_DC);  gpio_set_dir(LEFT_DC,  GPIO_OUT);
    gpio_init(LEFT_RST); gpio_set_dir(LEFT_RST, GPIO_OUT); gpio_put(LEFT_RST, 1);

    gpio_init(RIGHT_CS);  gpio_set_dir(RIGHT_CS,  GPIO_OUT); gpio_put(RIGHT_CS,  1);
    gpio_init(RIGHT_DC);  gpio_set_dir(RIGHT_DC,  GPIO_OUT);
    gpio_init(RIGHT_RST); gpio_set_dir(RIGHT_RST, GPIO_OUT); gpio_put(RIGHT_RST, 1);

    left_cfg.spi = spi0; left_cfg.cs = LEFT_CS; left_cfg.dc = LEFT_DC; left_cfg.rst = LEFT_RST;
    left_cfg.col_offset = 80;
    left_cfg.row_offset = 0;

    right_cfg.spi = spi1; right_cfg.cs = RIGHT_CS; right_cfg.dc = RIGHT_DC; right_cfg.rst = RIGHT_RST;
    right_cfg.col_offset = 80;
    right_cfg.row_offset = 0;
}

/* ── Helpers ───────────────────────────────────────────────── */
static void write_cmd_buf(spi_inst_t *spi, uint8_t cmd, const uint8_t *data, int len, uint cs, uint dc) {
    write_cmd(spi, cmd, cs, dc);
    if (len > 0) {
        gpio_put(dc, 1);
        gpio_put(cs, 0);
        spi_write_blocking(spi, data, len);
        gpio_put(cs, 1);
    }
}

/* ── Display init ──────────────────────────────────────────── */
static void init_display_with_orientation(st7789_config_t *config, uint8_t madctl) {
    spi_inst_t *spi = config->spi;
    uint cs = config->cs, dc = config->dc, rst = config->rst;

    gpio_put(rst, 0); sleep_ms(15);
    gpio_put(rst, 1); sleep_ms(120);

    write_cmd(spi, 0x11, cs, dc);
    sleep_ms(120);

    write_cmd_buf(spi, 0x3A, (const uint8_t[]){0x55}, 1, cs, dc);
    write_cmd_buf(spi, 0xB2, (const uint8_t[]){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5, cs, dc);
    write_cmd_buf(spi, 0xB7, (const uint8_t[]){0x35}, 1, cs, dc);
    write_cmd_buf(spi, 0xBB, (const uint8_t[]){0x19}, 1, cs, dc);
    write_cmd_buf(spi, 0xC0, (const uint8_t[]){0x2C}, 1, cs, dc);
    write_cmd_buf(spi, 0xC2, (const uint8_t[]){0x01}, 1, cs, dc);
    write_cmd_buf(spi, 0xC3, (const uint8_t[]){0x12}, 1, cs, dc);
    write_cmd_buf(spi, 0xC4, (const uint8_t[]){0x20}, 1, cs, dc);
    write_cmd_buf(spi, 0xC6, (const uint8_t[]){0x0F}, 1, cs, dc);
    write_cmd_buf(spi, 0xD0, (const uint8_t[]){0xA4, 0xA1}, 2, cs, dc);
    write_cmd_buf(spi, 0xE0, (const uint8_t[]){0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 14, cs, dc);
    write_cmd_buf(spi, 0xE1, (const uint8_t[]){0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 14, cs, dc);
    write_cmd_buf(spi, 0x36, (const uint8_t[]){madctl}, 1, cs, dc);

    write_cmd(spi, 0x21, cs, dc);
    write_cmd(spi, 0x13, cs, dc);
    write_cmd(spi, 0x29, cs, dc);
    sleep_ms(50);
}

void init_displays(void) {
    init_display_with_orientation(&left_cfg,  0xA0);
    init_display_with_orientation(&right_cfg, 0xE0);
}

/* ── Low-level SPI primitives ──────────────────────────────── */
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

/* ── set_window (оригинал, 9 CS-переключений) ─────────────── */
void set_window(st7789_config_t *cfg, uint x0, uint y0, uint x1, uint y1) {
    spi_inst_t *spi = cfg->spi;
    uint cs = cfg->cs, dc = cfg->dc;

    x0 += cfg->col_offset; x1 += cfg->col_offset;
    y0 += cfg->row_offset; y1 += cfg->row_offset;

    write_cmd(spi, 0x2A, cs, dc);
    write_data(spi, (x0 >> 8) & 0xFF, cs, dc);
    write_data(spi, x0 & 0xFF,        cs, dc);
    write_data(spi, (x1 >> 8) & 0xFF, cs, dc);
    write_data(spi, x1 & 0xFF,        cs, dc);
    write_cmd(spi, 0x2B, cs, dc);
    write_data(spi, (y0 >> 8) & 0xFF, cs, dc);
    write_data(spi, y0 & 0xFF,        cs, dc);
    write_data(spi, (y1 >> 8) & 0xFF, cs, dc);
    write_data(spi, y1 & 0xFF,        cs, dc);
    write_cmd(spi, 0x2C, cs, dc);
}

/* ── set_window_fast (3 CS-переключения вместо 9) ─────────── */
void set_window_fast(st7789_config_t *cfg, uint x0, uint y0, uint x1, uint y1) {
    spi_inst_t *spi = cfg->spi;
    uint cs = cfg->cs, dc = cfg->dc;

    x0 += cfg->col_offset; x1 += cfg->col_offset;
    y0 += cfg->row_offset; y1 += cfg->row_offset;

    uint8_t caset[4] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
    uint8_t raset[4] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
    uint8_t cmd;

    /* CASET: command + 4 bytes data за один CS */
    cmd = 0x2A;
    gpio_put(dc, 0); gpio_put(cs, 0);
    spi_write_blocking(spi, &cmd, 1);
    gpio_put(dc, 1);
    spi_write_blocking(spi, caset, 4);
    gpio_put(cs, 1);

    /* RASET */
    cmd = 0x2B;
    gpio_put(dc, 0); gpio_put(cs, 0);
    spi_write_blocking(spi, &cmd, 1);
    gpio_put(dc, 1);
    spi_write_blocking(spi, raset, 4);
    gpio_put(cs, 1);

    /* RAMWR — только команда; данные пойдут через DMA */
    cmd = 0x2C;
    gpio_put(dc, 0); gpio_put(cs, 0);
    spi_write_blocking(spi, &cmd, 1);
    gpio_put(cs, 1);
}

/* ── DMA init ──────────────────────────────────────────────── */
void init_dma(void) {
    dma_chan_left  = dma_claim_unused_channel(true);
    dma_chan_right = dma_claim_unused_channel(true);

    /* Left — SPI0 TX */
    dma_cfg_left = dma_channel_get_default_config(dma_chan_left);
    channel_config_set_transfer_data_size(&dma_cfg_left, DMA_SIZE_8);
    channel_config_set_dreq(&dma_cfg_left, spi_get_dreq(spi0, true));
    channel_config_set_read_increment(&dma_cfg_left, true);
    channel_config_set_write_increment(&dma_cfg_left, false);

    /* Right — SPI1 TX */
    dma_cfg_right = dma_channel_get_default_config(dma_chan_right);
    channel_config_set_transfer_data_size(&dma_cfg_right, DMA_SIZE_8);
    channel_config_set_dreq(&dma_cfg_right, spi_get_dreq(spi1, true));
    channel_config_set_read_increment(&dma_cfg_right, true);
    channel_config_set_write_increment(&dma_cfg_right, false);
}

/* ── DMA: запустить передачу (неблокирующий) ──────────────── */
void start_framebuf_dma(const uint8_t *buf) {
    /* Выставляем окна — используем проверенный set_window */
    set_window(&left_cfg,  0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    set_window(&right_cfg, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);

    /* CS=0, DC=1 — режим данных на обоих */
    gpio_put(LEFT_DC,  1); gpio_put(LEFT_CS,  0);
    gpio_put(RIGHT_DC, 1); gpio_put(RIGHT_CS, 0);

    /* Конфигурируем каналы с новым src-адресом, не запускаем */
    dma_channel_configure(dma_chan_left,  &dma_cfg_left,
        &spi_get_hw(spi0)->dr, buf, RAW_FRAME_SIZE, false);
    dma_channel_configure(dma_chan_right, &dma_cfg_right,
        &spi_get_hw(spi1)->dr, buf, RAW_FRAME_SIZE, false);

    /* Запускаем оба канала одновременно */
    dma_start_channel_mask((1u << dma_chan_left) | (1u << dma_chan_right));
}

/* ── DMA: ждать завершения + SPI flush, снять CS ──────────── */
void wait_framebuf_dma(void) {
    dma_channel_wait_for_finish_blocking(dma_chan_left);
    dma_channel_wait_for_finish_blocking(dma_chan_right);
    /* Ждём пока SPI TX FIFO полностью опустеет */
    while (spi_get_hw(spi0)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
    while (spi_get_hw(spi1)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
    gpio_put(LEFT_CS,  1);
    gpio_put(RIGHT_CS, 1);
}