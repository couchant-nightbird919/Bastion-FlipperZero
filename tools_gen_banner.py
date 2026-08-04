#!/usr/bin/env python3
"""Render the Bastion GitHub banner + social-preview card.

Theme: 125 kHz is the warm, old, copper end of RFID - a coil of wire and a
number shouted in the clear - so the palette is copper on deep slate, against
Warden's violet at 13.56 MHz. The motifs are the app's own: a crenellated wall
with the gate standing open, and the credential drawn as the barcode it is.
Supersampled 2x and downscaled for clean edges.

    python3 tools_gen_banner.py
"""
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont
import os

OUT = os.path.join(os.path.dirname(__file__), "images")
os.makedirs(OUT, exist_ok=True)

BOLD = "/System/Library/Fonts/Supplemental/Arial Bold.ttf"
BLACK_F = "/System/Library/Fonts/Supplemental/Arial Black.ttf"
MONO = "/System/Library/Fonts/Supplemental/Andale Mono.ttf"
REG = "/System/Library/Fonts/Supplemental/Arial.ttf"

BG_TOP = (9, 12, 20)
BG_BOT = (19, 24, 40)
COPPER = (232, 131, 58)
COPPER_HI = (247, 178, 106)
EMBER = (255, 96, 48)
WHITE = (240, 244, 252)
GRAY = (150, 160, 182)
DIM = (108, 118, 144)
STONE = (26, 33, 52)

SS = 2  # supersample

# A plausible 40-bit EM4100 payload, reused as the bar pattern everywhere.
PATTERN = [int(b) for b in f"{0x1200345678:040b}"]


def font(path, px):
    try:
        return ImageFont.truetype(path, px)
    except OSError:
        return ImageFont.truetype(BOLD, px)


def vgradient(w, h):
    img = Image.new("RGB", (w, h), BG_TOP)
    d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        d.line(
            [(0, y), (w, y)],
            fill=tuple(int(BG_TOP[i] + (BG_BOT[i] - BG_TOP[i]) * t) for i in range(3)),
        )
    return img


def add_glow(img, centre, radius, colour, strength):
    """Composite a soft radial wash additively, so it brightens rather than greys."""
    layer = Image.new("RGB", img.size, (0, 0, 0))
    d = ImageDraw.Draw(layer)
    cx, cy = centre
    d.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], fill=colour)
    layer = layer.filter(ImageFilter.GaussianBlur(radius * 0.55))
    layer = Image.eval(layer, lambda v: v * strength // 255)
    return ImageChops.add(img, layer)


def spaced(d, xy, text, fnt, fill, tracking):
    """Draw letterspaced text."""
    x, y = xy
    for ch in text:
        d.text((x, y), ch, font=fnt, fill=fill)
        x += d.textlength(ch, font=fnt) + tracking


def spaced_width(d, text, fnt, tracking):
    return sum(d.textlength(c, font=fnt) for c in text) + tracking * max(0, len(text) - 1)


def wall(d, x, y, w, h, merlon_w, gap_w, fill, gate=None, gate_fill=BG_BOT):
    """A crenellated wall. `gate` = (offset, width) punches an arch through it."""
    body_top = y + h // 4
    d.rectangle([x, body_top, x + w, y + h], fill=fill)
    cx = x
    while cx < x + w:
        d.rectangle([cx, y, min(cx + merlon_w, x + w), body_top], fill=fill)
        cx += merlon_w + gap_w
    if gate:
        gx, gw = gate
        gh = int(h * 0.52)
        d.rectangle([x + gx, y + h - gh, x + gx + gw, y + h], fill=gate_fill)
        d.ellipse(
            [x + gx, y + h - gh - gw // 2, x + gx + gw, y + h - gh + gw // 2], fill=gate_fill
        )


def wall_faded(img, x, y, w, h, merlon_w, gap_w, fill, gate):
    """The wall, dissolving into the background at its base.

    Drawn on its own RGBA layer so the gate punches a real hole rather than a
    background-coloured patch - otherwise the fade would reveal the fill.
    """
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ld = ImageDraw.Draw(layer)
    wall(ld, x, y, w, h, merlon_w, gap_w, fill + (255,), gate=gate, gate_fill=(0, 0, 0, 0))

    fade_from = y + int(h * 0.45)
    fade_to = y + h
    grad = Image.new("L", img.size, 255)
    gd = ImageDraw.Draw(grad)
    for yy in range(fade_from, fade_to):
        gd.line([(0, yy), (img.width, yy)], fill=255 - int(255 * (yy - fade_from) / (fade_to - fade_from)))
    gd.rectangle([0, fade_to, img.width, img.height], fill=0)

    layer.putalpha(ImageChops.multiply(layer.split()[3], grad))
    return Image.alpha_composite(img.convert("RGBA"), layer).convert("RGB")


def bits(d, x, y, w, h, pattern, on, off):
    """The credential, drawn as bars - the app's signature motif."""
    step = w / len(pattern)
    bw = max(2, int(step * 0.62))
    for i, b in enumerate(pattern):
        left = int(x + i * step)
        d.rectangle([left, y, left + bw, y + h], fill=on if b else off)


def grade_stamp(d, cx, cy, s, letter, sub, ring=COPPER):
    """A rounded-square grade badge with corner ticks - the app's verdict card."""
    half = s // 2
    d.rounded_rectangle(
        [cx - half, cy - half, cx + half, cy + half],
        radius=s // 7,
        fill=(13, 16, 26),
        outline=ring,
        width=5 * SS,
    )
    for dx, dy in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
        px, py = cx + dx * (half - 14 * SS), cy + dy * (half - 14 * SS)
        d.line([(px - dx * 10 * SS, py), (px, py)], fill=ring, width=3 * SS)
        d.line([(px, py - dy * 10 * SS), (px, py)], fill=ring, width=3 * SS)

    f = font(BLACK_F, int(s * 0.50))
    bbox = f.getbbox(letter)
    d.text(
        (cx - d.textlength(letter, font=f) / 2, cy - (bbox[3] + bbox[1]) / 2 - s * 0.07),
        letter,
        font=f,
        fill=WHITE,
    )

    fs = font(BOLD, int(s * 0.092))
    tw = spaced_width(d, sub, fs, 2 * SS)
    spaced(d, (cx - tw / 2, cy + s * 0.27), sub, fs, ring, 2 * SS)


def banner(w=1600, h=460, name="banner.png"):
    W, H = w * SS, h * SS
    img = vgradient(W, H)
    img = add_glow(img, (int(W * 0.22), int(H * 0.55)), int(H * 0.85), COPPER, 46)
    img = add_glow(img, (int(W * 0.83), int(H * 0.42)), int(H * 0.55), EMBER, 30)
    d = ImageDraw.Draw(img)

    # No wall here: its merlons read as vertical bars and fight the barcode
    # motif. The fortification lives in the icon and on the social card.
    pad = int(W * 0.055)

    spaced(d, (pad, int(H * 0.145)), "125 kHz BADGE GRADER", font(BOLD, 21 * SS), COPPER, 5 * SS)

    d.text((pad - 5 * SS, int(H * 0.215)), "BASTION", font=font(BLACK_F, 104 * SS), fill=WHITE)

    d.text(
        (pad, int(H * 0.555)), "Your badge is a barcode.", font=font(BOLD, 28 * SS), fill=COPPER_HI
    )
    d.text(
        (pad, int(H * 0.655)),
        "Find out how loudly it shouts - and what a copy would cost.",
        font=font(REG, 23 * SS),
        fill=GRAY,
    )

    bits(d, pad, int(H * 0.775), int(W * 0.46), int(H * 0.058), PATTERN, COPPER, (40, 48, 70))
    d.text(
        (pad, int(H * 0.865)),
        "40 bits, in the clear, to any reader in range",
        font=font(MONO, 17 * SS),
        fill=DIM,
    )

    grade_stamp(d, int(W * 0.815), int(H * 0.42), int(H * 0.46), "F", "BROADCAST")
    f_note = font(BOLD, 19 * SS)
    note = "NO 125 kHz FORMAT PASSES"
    spaced(
        d,
        (int(W * 0.815) - spaced_width(d, note, f_note, 3 * SS) / 2, int(H * 0.755)),
        note,
        f_note,
        GRAY,
        3 * SS,
    )

    d.rectangle([0, H - 5 * SS, W, H], fill=COPPER)

    path = os.path.join(OUT, name)
    img.resize((w, h), Image.LANCZOS).save(path)
    print("wrote", path)


def social(w=1280, h=640, name="social-preview.png"):
    W, H = w * SS, h * SS
    img = vgradient(W, H)
    img = add_glow(img, (int(W * 0.5), int(H * 0.46)), int(H * 0.95), COPPER, 40)
    img = add_glow(img, (int(W * 0.5), int(H * 1.02)), int(H * 0.45), EMBER, 24)
    # A gatehouse, wide and low so it reads as a wall rather than a floating
    # box, with the gate standing open - which is the whole point. Its base
    # dissolves into the background instead of ending on a hard edge.
    img = wall_faded(
        img,
        int(W * 0.28),
        int(H * 0.045),
        int(W * 0.44),
        int(H * 0.255),
        merlon_w=int(W * 0.048),
        gap_w=int(W * 0.030),
        fill=STONE,
        gate=(int(W * 0.185), int(W * 0.07)),
    )
    d = ImageDraw.Draw(img)

    f_eyebrow = font(BOLD, 23 * SS)
    eb = "125 kHz BADGE GRADER   FLIPPER ZERO"
    spaced(
        d,
        ((W - spaced_width(d, eb, f_eyebrow, 5 * SS)) / 2, int(H * 0.345)),
        eb,
        f_eyebrow,
        COPPER,
        5 * SS,
    )

    f_title = font(BLACK_F, 118 * SS)
    d.text(
        ((W - d.textlength("BASTION", font=f_title)) / 2, int(H * 0.415)),
        "BASTION",
        font=f_title,
        fill=WHITE,
    )

    f_tag = font(BOLD, 29 * SS)
    tag = "Your badge is a barcode."
    d.text(((W - d.textlength(tag, font=f_tag)) / 2, int(H * 0.685)), tag, font=f_tag, fill=COPPER_HI)

    f_sub = font(REG, 22 * SS)
    sub = "EM4100    HID Prox    Indala    AWID    ioProx    Gallagher    Nexwatch"
    d.text(((W - d.textlength(sub, font=f_sub)) / 2, int(H * 0.775)), sub, font=f_sub, fill=GRAY)

    bw = int(W * 0.62)
    bits(d, (W - bw) // 2, int(H * 0.865), bw, int(H * 0.05), PATTERN, COPPER, (40, 48, 70))

    d.rectangle([0, H - 6 * SS, W, H], fill=COPPER)

    path = os.path.join(OUT, name)
    img.resize((w, h), Image.LANCZOS).save(path)
    print("wrote", path)


if __name__ == "__main__":
    banner()
    social()
