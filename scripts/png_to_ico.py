#!/usr/bin/env python3
"""Generate a multi-resolution Windows .ico from a single source PNG.
Usage: png_to_ico.py <input.png> <output.ico>
"""
import sys

try:
    from PIL import Image
except ImportError:
    print("error: Pillow is required (pip install pillow)", file=sys.stderr)
    sys.exit(1)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: png_to_ico.py <input.png> <output.ico>", file=sys.stderr)
        return 1

    src_path, dst_path = sys.argv[1], sys.argv[2]
    sizes = [16, 24, 32, 48, 64, 128, 256]

    image = Image.open(src_path).convert("RGBA")
    image.save(dst_path, format="ICO", sizes=[(s, s) for s in sizes])
    return 0


if __name__ == "__main__":
    sys.exit(main())