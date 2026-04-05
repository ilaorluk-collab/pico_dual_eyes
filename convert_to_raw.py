#!/usr/bin/env python3
"""Convert BMP animation frames to RAW RGB565 for Pico display.

Input:  ./bender_frames/{anim}/f00.bmp ...
Output: ./bender/{anim}/f00.raw ...  +  ./bender/manifest.txt

Copy bender_frames/ from SD to this directory, then run this script.
Copy resulting bender/ folder to SD card root.

Requires: pip install Pillow
"""

from PIL import Image
import os
import struct

ANIMS = ['wakeup', 'happy', 'music', 'headbob', 'orbit']
SIZE = 240
FPS = 12

def convert_bmp_to_raw(src_dir, dst_dir):
    frames = sorted(f for f in os.listdir(src_dir) if f.endswith('.bmp'))
    if not frames:
        return 0

    os.makedirs(dst_dir, exist_ok=True)
    count = 0

    for bmp_name in frames:
        img = Image.open(os.path.join(src_dir, bmp_name)).convert('RGB')
        img = img.resize((SIZE, SIZE), Image.NEAREST)
        img = img.rotate(90, expand=True)
        img = img.transpose(Image.FLIP_LEFT_RIGHT)

        raw_name = bmp_name.replace('.bmp', '.raw')
        raw_path = os.path.join(dst_dir, raw_name)

        with open(raw_path, 'wb') as f:
            pixels = img.load()
            for y in range(SIZE):
                for x in range(SIZE):
                    r, g, b = pixels[x, y]
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    f.write(struct.pack('>H', rgb565))

        count += 1
        print(f"  {bmp_name} -> {raw_name} ({count}/{len(frames)})")

    return count

def main():
    base = os.path.dirname(os.path.abspath(__file__))
    src_base = os.path.join(base, 'bender_frames')
    dst_base = os.path.join(base, 'bender')

    if not os.path.isdir(src_base):
        print(f"ERROR: {src_base} not found")
        print("Copy bender_frames/ from your SD card to this directory first.")
        return

    manifest_lines = []

    for anim in ANIMS:
        src_dir = os.path.join(src_base, anim)
        if not os.path.isdir(src_dir):
            continue

        print(f"\nConverting {anim}...")
        dst_dir = os.path.join(dst_base, anim)
        count = convert_bmp_to_raw(src_dir, dst_dir)

        if count > 0:
            manifest_lines.append(f"{anim}:{count}:{FPS}")
            print(f"  Done: {count} frames")

    manifest_path = os.path.join(dst_base, 'manifest.txt')
    with open(manifest_path, 'w') as f:
        f.write('\n'.join(manifest_lines) + '\n')

    print(f"\nManifest: {manifest_path}")
    print("Copy the 'bender/' folder to your SD card root.")

if __name__ == '__main__':
    main()
