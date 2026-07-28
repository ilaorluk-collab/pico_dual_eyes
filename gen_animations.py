#!/usr/bin/env python3
"""
Bender Eye Animation Generator
Генерирует все анимации программно в формате .1bit (7200 байт/кадр, 240x240, MSB first).

Анимации:
  0: music    — зрачок прыгает в ритм
  1: happy    — зрачок сплющивается горизонтально
  2: wakeup   — глаз открывается снизу
  3: headbob  — зрачок качается вверх-вниз
  4: orbit    — зрачок ходит по орбите
  5: blink    — полное моргание
  6: angry    — злой прищур сверху
  7: scan     — зрачок сканирует влево-вправо
  8: wink     — хитрый прищур

Настройка зрачка:
  DEFAULT_PX > CX → зрачок правее центра → оба глаза смотрят вперёд
  (правый дисплей зеркалит горизонталь через MADCTL MX=1)
"""

import math
from pathlib import Path
from PIL import Image, ImageDraw

OUTPUT_DIR = Path.home() / "Downloads" / "bender_bit"
OUTPUT_DIR.mkdir(exist_ok=True)

W = H = 240
CX, CY = 120, 120
EYE_R  = 113    # радиус глаза — почти весь экран

PUPIL_W = 30    # ширина зрачка (квадрат как у Бендера)
PUPIL_H = 30    # высота зрачка

# Зрачок по умолчанию: чуть правее и чуть выше центра
# +10 по X → оба глаза смотрят вперёд (не врозь)
DEFAULT_PX = CX + 10
DEFAULT_PY = CY - 5


# ──────────────────────────────────────────────
#  Утилиты
# ──────────────────────────────────────────────

def ease_inout(t: float) -> float:
    """Плавное начало и конец."""
    return t * t * (3 - 2 * t)

def ease_out(t: float) -> float:
    return 1 - (1 - t) ** 2

def lerp(a, b, t):
    return a + (b - a) * t

def to_1bit(img: Image.Image) -> bytes:
    """Конвертировать grayscale PIL-изображение в packed 1-bit MSB-first (7200 байт)."""
    pixels = list(img.getdata())
    result = bytearray(7200)
    for i, px in enumerate(pixels):
        if px > 127:
            result[i >> 3] |= (0x80 >> (i & 7))
    return bytes(result)


def make_frame(
    px: int = None, py: int = None,
    pw: int = PUPIL_W, ph: int = PUPIL_H,
    top_lid: int = 0, bot_lid: int = 0,
    draw_pupil: bool = True,
) -> Image.Image:
    """
    Нарисовать один кадр глаза Бендера.

    px, py      — центр зрачка
    pw, ph      — размер зрачка (ширина/высота)
    top_lid     — глубина верхнего века (пикселей от верха круга)
    bot_lid     — глубина нижнего века (пикселей от низа круга)
    draw_pupil  — рисовать ли зрачок
    """
    if px is None: px = DEFAULT_PX
    if py is None: py = DEFAULT_PY

    img  = Image.new("L", (W, H), 0)
    draw = ImageDraw.Draw(img)

    # Жёлтый (белый в 1-bit) круг — глаз
    draw.ellipse([CX - EYE_R, CY - EYE_R, CX + EYE_R, CY + EYE_R], fill=255)

    # Верхнее веко (чёрный прямоугольник сверху)
    if top_lid > 0:
        lid_y = CY - EYE_R + top_lid
        draw.rectangle([0, 0, W, int(lid_y)], fill=0)

    # Нижнее веко (чёрный прямоугольник снизу)
    if bot_lid > 0:
        lid_y = CY + EYE_R - bot_lid
        draw.rectangle([0, int(lid_y), W, H], fill=0)

    # Чёрный зрачок
    if draw_pupil:
        x0 = int(px - pw / 2)
        y0 = int(py - ph / 2)
        x1 = int(px + pw / 2)
        y1 = int(py + ph / 2)
        draw.rectangle([x0, y0, x1, y1], fill=0)

    return img


def save_anim(name: str, frames: list, fps: int):
    anim_dir = OUTPUT_DIR / name
    anim_dir.mkdir(exist_ok=True)
    for i, data in enumerate(frames):
        (anim_dir / f"f{i:02d}.1bit").write_bytes(data)
    total_kb = len(frames) * 7200 / 1024
    print(f"  {name:10s}: {len(frames):3d} кадров @ {fps:2d} fps  ({total_kb:.0f} KB)")


# ──────────────────────────────────────────────
#  Генераторы анимаций
# ──────────────────────────────────────────────

def gen_orbit(n=16, fps=10):
    """Зрачок плавно ходит по кругу."""
    frames = []
    R = 32  # радиус орбиты
    for i in range(n):
        angle = 2 * math.pi * i / n - math.pi / 2  # начать сверху
        px = DEFAULT_PX + int(R * math.cos(angle))
        py = DEFAULT_PY + int(R * math.sin(angle))
        frames.append(to_1bit(make_frame(px=px, py=py)))
    save_anim("orbit", frames, fps)


def gen_wakeup(n=24, fps=12):
    """
    Глаз открывается снизу: верхнее веко поднимается.
    Первые кадры = тонкая полоска снизу, последние = полный круг.
    Зрачок стандартный (как во всех остальных анимациях).
    """
    frames = []
    max_lid = int(EYE_R * 1.97)  # почти полностью закрыт сверху
    for i in range(n):
        # Плавное замедление в конце (ease_out = быстро открывается, медленно финиширует)
        t = 1.0 - (1.0 - i / (n - 1)) ** 2
        top_lid = int(max_lid * (1.0 - t))
        frames.append(to_1bit(make_frame(top_lid=top_lid)))
    save_anim("wakeup", frames, fps)


def gen_happy(n=16, fps=8):
    """Зрачок сплющивается горизонтально (доволен) и возвращается."""
    frames = []
    for i in range(n):
        t = math.sin(math.pi * i / (n - 1))
        pw = int(PUPIL_W + 26 * t)
        ph = max(int(PUPIL_H - 20 * t), 5)
        frames.append(to_1bit(make_frame(pw=pw, ph=ph)))
    save_anim("happy", frames, fps)


def gen_headbob(n=12, fps=10):
    """Зрачок плавно качается вверх-вниз."""
    frames = []
    amp = 18
    for i in range(n):
        t = math.sin(2 * math.pi * i / n)
        py = DEFAULT_PY + int(amp * t)
        frames.append(to_1bit(make_frame(py=py)))
    save_anim("headbob", frames, fps)


def gen_music(n=24, fps=10):
    """Зрачок прыгает в ритм (4 удара на цикл)."""
    frames = []
    for i in range(n):
        beat_t = (i % 6) / 6.0           # 4 удара за 24 кадра
        bounce = ease_out(math.sin(math.pi * beat_t))
        py = DEFAULT_PY - int(30 * bounce)
        # На сильную долю зрачок чуть сплющивается от «удара»
        accent = 1 if i % 6 == 0 else 0
        pw = PUPIL_W + 8 * accent
        ph = max(PUPIL_H - 6 * accent, 6)
        frames.append(to_1bit(make_frame(py=py, pw=pw, ph=ph)))
    save_anim("music", frames, fps)


def gen_blink(n=14, fps=14):
    """Полное моргание: верхнее веко опускается и поднимается."""
    frames = []
    half = n // 2
    for i in range(n):
        t = i / half if i < half else 1.0 - (i - half) / half
        top_lid = int(EYE_R * 2.0 * ease_inout(t))
        frames.append(to_1bit(make_frame(top_lid=top_lid)))
    save_anim("blink", frames, fps)


def gen_angry(n=16, fps=8):
    """
    Злой взгляд: верхнее веко опускается + зрачок сужается горизонтально.
    Держит злое выражение, потом возвращается.
    """
    frames  = []
    hold    = 4    # кадров «злого» выражения
    ramp    = (n - hold) // 2

    for i in range(n):
        if i < ramp:
            t = ease_inout(i / ramp)
        elif i < ramp + hold:
            t = 1.0
        else:
            t = ease_inout(1.0 - (i - ramp - hold) / ramp)

        top_lid = int(60 * t)
        pw = int(PUPIL_W + 14 * t)
        ph = max(int(PUPIL_H - 16 * t), 6)
        # Зрачок чуть вниз при злом взгляде
        py = DEFAULT_PY + int(10 * t)
        frames.append(to_1bit(make_frame(pw=pw, ph=ph, py=py, top_lid=top_lid)))
    save_anim("angry", frames, fps)


def gen_scan(n=16, fps=8):
    """Зрачок сканирует слева направо и обратно."""
    frames = []
    sweep  = 48
    for i in range(n):
        t  = math.sin(2 * math.pi * i / n)
        px = DEFAULT_PX + int(sweep * t)
        frames.append(to_1bit(make_frame(px=px)))
    save_anim("scan", frames, fps)


def gen_wink(n=12, fps=10):
    """Хитрый прищур: оба века сходятся к щёлке, потом открываются."""
    frames = []
    half   = n // 2
    for i in range(n):
        t       = ease_inout(i / half if i < half else 1.0 - (i - half) / half)
        top_lid = int(75 * t)
        bot_lid = int(55 * t)
        frames.append(to_1bit(make_frame(top_lid=top_lid, bot_lid=bot_lid)))
    save_anim("wink", frames, fps)


# ──────────────────────────────────────────────
#  Main
# ──────────────────────────────────────────────

ANIM_TABLE = [
    # (func, n_frames, fps)
    (gen_music,   24, 10),
    (gen_happy,   16,  8),
    (gen_wakeup,  24, 12),
    (gen_headbob, 12, 10),
    (gen_orbit,   16, 10),
    (gen_blink,   14, 14),
    (gen_angry,   16,  8),
    (gen_scan,    16,  8),
    (gen_wink,    12, 10),
]

if __name__ == "__main__":
    print(f"Генерирую анимации → {OUTPUT_DIR}\n")
    total_frames = 0
    for func, n, fps in ANIM_TABLE:
        func(n=n, fps=fps)
        total_frames += n
    print(f"\nИтого: {total_frames} кадров, {total_frames * 7200 / 1024:.0f} KB в флеше")
    print("Готово! Запусти generate_anim_data.py чтобы собрать anim_data.h")
