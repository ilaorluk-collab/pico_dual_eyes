# Отчёт: исправление цветного шума на дисплеях ST7789

## Дата: 6 апреля 2026

## Проблема

Оба ST7789 дисплея (левый SPI0, правый SPI1) показывали цветной шум вместо анимации Бендера. SD карта удалена, данные встроены в прошивку (anim_data.h).

## Диагностика

1. Минимальный тест (LED blink + USB printf) подтвердил что прошивка запускается корректно
2. UART логи показали что весь код выполняется без падений — `send_framebuf()` отрабатывает успешно
3. Значит проблема была в инициализации/протоколе дисплея, а не в прошивке

## Исправления (что реально починило)

### 1. Полная инициализация ST7789 (`src/display.c`)

Было всего 6 команд: SLPOUT, COLMOD, MADCTL, INVON, NORON, DISPON. Большинство модулей ST7789 требуют полную инициализацию — без настройки питания и gamma панель не принимает данные в RAM.

Добавлены команды:
- `0xB2` PORCTRL — porch timing (5 байт)
- `0xB7` GCTRL — gate control (VGH/VGL)
- `0xBB` VCOMS — VCOM voltage
- `0xC0` LCMCTRL — LCM control
- `0xC2` VDVVRHEN — enable VDV/VRH
- `0xC3` VRHS — voltage regulation
- `0xC4` VDVS — VDS setting
- `0xC6` FRCTRL2 — frame rate 60Hz
- `0xD0` PWCTRL1 — power control (AVDD, AVCL)
- `0xE0` PVGAMCTRL — positive gamma (14 байт)
- `0xE1` NVGAMCTRL — negative gamma (14 байт)

Добавлена функция `write_cmd_buf()` для отправки команды с массивом данных за один вызов.

### 2. col_offset: 80 → 0 (`src/display.c`)

`col_offset = 80` предполагает 320-колоночный адресуемый дисплей (240 видимых + 80 offset). Дисплеи 240x240 используют 240-колоночную адресацию — смещение 80 записывало данные мимо видимой области.

### 3. CS не дёргается между строками (`src/main.c`)

`send_framebuf()` поднимал CS (chip select) после каждой строки. После команды RAMWR (0x2C) дисплей находится в режиме приёма пикселей, но CS=high прерывает этот режим. Пиксели не записывались в RAM.

Фикс: CS держится low весь кадр целиком.

### 4. COLOR_YELLOW: 0xFFFF → 0xFFDE (`src/unpack.h`)

Жёлтый был определён как 0xFFFF (белый). Исправлен на 0xFFDE — корректный RGB565 для жёлтого.

### 5. SPI скорость: 10MHz → 25MHz (`src/display.c`)

Оба SPI работали на 10MHz — остаток от SD карты. Подняно до 25MHz.

## Итоговые изменённые файлы

| Файл | Изменения |
|---|---|
| `src/display.c` | Полная инициализация ST7789, col_offset=0, SPI 25MHz, write_cmd_buf() |
| `src/main.c` | CS держится весь кадр, убран дублирующий spi_set_baudrate, USB debug |
| `src/unpack.h` | COLOR_YELLOW 0xFFFF → 0xFFDE |
| `CMakeLists.txt` | pico_enable_stdio_usb(1) для USB debug |
