#!/usr/bin/env python3
"""Generate the 1-bit 10x10 Flipper app icon for Bastion from an ASCII bitmap.

'#' = foreground (black / on), anything else = background (white / off).
fbt thresholds PNGs to 1-bit, where dark pixels become 'on'.

    python3 tools_gen_icons.py            # write icons/bastion_10px.png
    python3 tools_gen_icons.py --preview  # also write a 20x upscale to inspect
"""
from PIL import Image
import os
import sys

OUT = os.path.join(os.path.dirname(__file__), "icons")
os.makedirs(OUT, exist_ok=True)

GLYPHS = {
    # A bastion gatehouse: crenellations, a window and an open gate. Solid
    # silhouettes read at 10px where line art turns to mush - and the open gate
    # is the whole point of the app.
    "bastion_10px": [
        "#.#.#.#.#.",
        "##########",
        "##########",
        "##########",
        "###....###",
        "###....###",
        "##########",
        "####..####",
        "####..####",
        "####..####",
    ],
}


def render(name, rows):
    img = Image.new("1", (10, 10), 1)  # 1 = white background
    for y, row in enumerate(rows):
        for x, ch in enumerate(row[:10]):
            if ch == "#":
                img.putpixel((x, y), 0)  # 0 = black foreground
    path = os.path.join(OUT, name + ".png")
    img.save(path)
    return path, img


if __name__ == "__main__":
    preview = "--preview" in sys.argv
    for name, rows in GLYPHS.items():
        assert len(rows) == 10, f"{name} must have 10 rows"
        for r in rows:
            assert len(r) == 10, f"{name} row is not 10 wide: {r!r}"
        path, img = render(name, rows)
        print("wrote", path)
        if preview:
            big = img.convert("L").resize((200, 200), Image.NEAREST)
            p = os.path.join(OUT, name + "_preview.png")
            big.save(p)
            print("wrote", p)
