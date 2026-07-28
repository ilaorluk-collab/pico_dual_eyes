#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "display.h"
#include "anim_data.h"
#include "unpack.h"

#define LED_PIN 25

/*
 * Double buffer — пока DMA отправляет send_buf,
 * CPU распаковывает следующий кадр в prep_buf.
 */
static uint8_t frame_buf_a[RAW_FRAME_SIZE];
static uint8_t frame_buf_b[RAW_FRAME_SIZE];

/* ── ESP32 → Pico USB serial команды ──────────────────────────
 *
 * ESP32 отправляет один байт через USB serial:
 *   '0'–'8'  →  запустить анимацию с этим индексом однократно
 *   'R'      →  вернуться в рандомный режим (по умолчанию)
 *   'L0'–'L8' →  зациклить анимацию до следующей команды (TODO)
 *
 * Индексы анимаций:
 *   0 music | 1 happy | 2 wakeup | 3 headbob | 4 orbit
 *   5 blink | 6 angry  | 7 scan   | 8 wink
 * ────────────────────────────────────────────────────────────*/
static volatile int esp32_cmd = -1;   /* -1 = нет команды */

static void check_esp32(void) {
    int c = getchar_timeout_us(0);     /* неблокирующий read */
    if (c == PICO_ERROR_TIMEOUT) return;
    if (c >= '0' && c <= '8') {
        esp32_cmd = c - '0';
    } else if (c == 'R' || c == 'r') {
        esp32_cmd = -1;                /* сброс в рандом */
    }
}

/*
 * play_anim — конвейерный пайплайн с проверкой ESP32 между кадрами:
 *   1. Распаковать кадр[0] → send_buf
 *   2. Запустить DMA (неблокирующий)
 *   3. Пока DMA работает — распаковать кадр[f+1] + проверить ESP32
 *   4. Дождаться DMA, swap буферов, тайминг
 */
static void play_anim(const anim_t *a) {
    if (a->frames == 0) return;

    uint8_t *send_buf = frame_buf_a;
    uint8_t *prep_buf = frame_buf_b;

    unpack_1bit_to_rgb565(a->frames_data[0], send_buf);

    for (int f = 0; f < a->frames; f++) {
        uint32_t t0 = to_ms_since_boot(get_absolute_time());

        start_framebuf_dma(send_buf);

        /* Пока DMA работает: распаковка + опрос ESP32 */
        if (f + 1 < a->frames) {
            unpack_1bit_to_rgb565(a->frames_data[f + 1], prep_buf);
        }
        check_esp32();

        wait_framebuf_dma();

        gpio_put(LED_PIN, f & 1);

        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - t0;
        if (elapsed < a->delay_ms) sleep_ms(a->delay_ms - elapsed);

        uint8_t *tmp = send_buf;
        send_buf = prep_buf;
        prep_buf = tmp;
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(3000);
    printf("BOOT\n"); fflush(stdout);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    init_backlight();
    init_spi_pins();
    init_displays();
    init_dma();

    printf("anim start\n"); fflush(stdout);

    /* Чёрный экран при старте */
    memset(frame_buf_a, 0, RAW_FRAME_SIZE);
    start_framebuf_dma(frame_buf_a);
    wait_framebuf_dma();

    /* ── Индексы анимаций ─── */
    #define WAKEUP_IDX 2
    #define ORBIT_IDX  4

    /* Пул случайных анимаций (без wakeup) */
    static const int random_pool[] = {0, 1, 3, 4, 5, 6, 7, 8};
    #define RANDOM_POOL_SIZE 8

    /* Xorshift RNG */
    uint32_t rng = (uint32_t)to_ms_since_boot(get_absolute_time()) | 1;

    /* Оригинальный wakeup при включении */
    play_anim(&anims[WAKEUP_IDX]);

    /* ── Основной цикл ───────────────────────────────────────
     * ESP32 может в любой момент прислать '0'-'8' по USB,
     * Пика подхватит команду между кадрами и выполнит её.
     * Без команды: orbit → случайная → orbit → ...
     * ────────────────────────────────────────────────────── */
    while (1) {
        check_esp32();

        /* Команда от ESP32? */
        if (esp32_cmd >= 0 && esp32_cmd < ANIM_COUNT) {
            play_anim(&anims[esp32_cmd]);
            esp32_cmd = -1;   /* сброс после выполнения */
            continue;
        }

        /* Стандартный рандомный режим */
        play_anim(&anims[ORBIT_IDX]);

        /* Music x3 — любимая анимация как переход */
        for (int m = 0; m < 3; m++) play_anim(&anims[0]);

        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        int idx = random_pool[rng % RANDOM_POOL_SIZE];
        play_anim(&anims[idx]);
    }
}
