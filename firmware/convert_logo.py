#!/usr/bin/env python3
# Usage:
#   Single logo:   python convert_logo.py logo.png                      → prints icon.c content
#   Animation:     python convert_logo.py --anim f0.png f1.png ...      → prints anim.c content
#   Threshold:     add --threshold 0-255 (default 128; lower = more white, higher = more black)
#   Invert:        add --invert to swap black/white
# Requires: pip install pillow

import sys
from PIL import Image

def png_to_buf(path, x_offset=96, threshold=128, invert=False):
    img = Image.open(path).convert("L")  # grayscale first for threshold control
    w, h = img.size
    if (w, h) != (32, 32):
        print(f"# Warning: expected 32x32, got {w}x{h}. Resizing.", file=sys.stderr)
        img = img.resize((32, 32), Image.NEAREST)

    buf = bytearray(512)
    for page in range(4):
        for col in range(32):
            byte = 0
            for bit in range(8):
                row = page * 8 + bit
                pixel = img.getpixel((col, row))
                lit = (pixel >= threshold) if not invert else (pixel < threshold)
                if lit:
                    byte |= (1 << bit)
            buf[page * 128 + x_offset + col] = byte
    return buf

def fmt_bytes(buf, indent="        "):
    lines = []
    for i in range(0, 512, 16):
        row = ", ".join(f"0x{b:02x}" for b in buf[i:i+16])
        lines.append(f"{indent}{row},")
    return "\n".join(lines)

def single(path, threshold, invert):
    buf = png_to_buf(path, threshold=threshold, invert=invert)
    print("const char my_logo[] PROGMEM = {")
    print(fmt_bytes(buf, indent="\t"))
    print("};")

def animation(paths, threshold, invert):
    n = len(paths)
    print(f"// Paste this into bmp/anim.c and set ANIM_FRAME_COUNT {n} in bmp/anim.h")
    print(f"const char anim_frames[{n}][512] PROGMEM = {{")
    for i, path in enumerate(paths):
        buf = png_to_buf(path, threshold=threshold, invert=invert)
        print(f"    // frame {i}: {path}")
        print("    {")
        print(fmt_bytes(buf, indent="        "))
        print("    }," if i < n - 1 else "    },")
    print("};")

if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        print("Usage:")
        print("  python convert_logo.py [--threshold N] [--invert] logo.png")
        print("  python convert_logo.py [--threshold N] [--invert] --anim frame0.png frame1.png ...")
        print("  --threshold 0-255  default 128; lower keeps more white, higher keeps more black")
        print("  --invert           swap which pixels are lit")
        sys.exit(1)

    threshold = 128
    invert = False
    while args and args[0].startswith("--") and args[0] not in ("--anim",):
        flag = args.pop(0)
        if flag == "--threshold":
            threshold = int(args.pop(0))
        elif flag == "--invert":
            invert = True

    if not args:
        print("No input file specified.")
        sys.exit(1)

    if args[0] == "--anim":
        if len(args) < 2:
            print("Provide at least one frame PNG after --anim")
            sys.exit(1)
        animation(args[1:], threshold, invert)
    else:
        single(args[0], threshold, invert)
