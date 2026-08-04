#!/usr/bin/env python3
"""Render 128x64 Flipper-style mockups of Bastion's screens for the README.

Faithful on purpose: every coordinate below is copied from the layout constants
in views/scan_view.c and views/result_view.c, and text is positioned by BASELINE
(PIL anchor "ls"/"ms"/"rs") because canvas_draw_str() takes y as the baseline.
Drawing the screens the way the firmware does is what catches a collision before
it ships, so these are worth keeping in sync when a row moves.
"""
from PIL import Image, ImageDraw, ImageFont
import os

OUT = os.path.join(os.path.dirname(__file__), "images")
os.makedirs(OUT, exist_ok=True)

ORANGE = (255, 159, 12)
INK = (26, 18, 2)
BEZEL = (18, 18, 22)
BEZEL_HI = (44, 44, 52)

SCALE = 7
W, H = 128, 64

FB = "/System/Library/Fonts/Supplemental/Arial Bold.ttf"
FR = "/System/Library/Fonts/Supplemental/Arial.ttf"
FBLK = "/System/Library/Fonts/Supplemental/Arial Black.ttf"

PRIM = ImageFont.truetype(FB, 9)  # FontPrimary
SEC = ImageFont.truetype(FR, 8)  # FontSecondary
BIG = ImageFont.truetype(FBLK, 19)  # FontBigNumbers


def screen():
    return Image.new("RGB", (W, H), ORANGE)


def base(d, x, y, s, font, fill=INK, anchor="ls"):
    """Draw `s` with its BASELINE on y, like canvas_draw_str()."""
    d.text((x, y), s, font=font, fill=fill, anchor=anchor)


def width(d, s, font):
    return d.textlength(s, font=font)


def finish(img, name):
    up = img.resize((W * SCALE, H * SCALE), Image.NEAREST)
    pad = 20
    canvas = Image.new("RGB", (W * SCALE + pad * 2, H * SCALE + pad * 2), BEZEL)
    d = ImageDraw.Draw(canvas)
    d.rounded_rectangle(
        [6, 6, canvas.width - 6, canvas.height - 6], radius=16, outline=BEZEL_HI, width=3
    )
    canvas.paste(up, (pad, pad))
    path = os.path.join(OUT, name)
    canvas.save(path)
    print("wrote", path)
    return canvas


# ------------------------------------------------------------------- 1. menu
def m_menu():
    img = screen()
    d = ImageDraw.Draw(img)
    base(d, 64, 9, "Bastion", PRIM, anchor="ms")
    d.line([(0, 11), (127, 11)], fill=INK)
    d.rounded_rectangle([2, 14, 125, 26], radius=3, fill=INK)
    base(d, 7, 23, "Grade a Badge", SEC, fill=ORANGE)
    base(d, 7, 36, "Badge Log", SEC)
    base(d, 7, 48, "Settings", SEC)
    base(d, 7, 60, "About", SEC)
    return finish(img, "screen_menu.png")


# ------------------------------------------------------------------- 2. scan
def m_scan(name="screen_scan.png", stage="Demodulating ASK", energised=True, phase=3):
    """Mirrors views/scan_view.c: SV_* constants and the sv_sin table."""
    img = screen()
    d = ImageDraw.Draw(img)

    base(d, 2, 9, "Read a Badge", PRIM)
    base(d, 126, 9, "AUTO", SEC, anchor="rs")
    d.line([(0, 11), (127, 11)], fill=INK)

    # coil: sv_draw_coil(canvas, 12, 28)
    cx, cy = 12, 28
    d.rounded_rectangle([cx - 8, cy - 8, cx + 8, cy + 8], radius=5, outline=INK)
    d.rounded_rectangle([cx - 5, cy - 5, cx + 5, cy + 5], radius=3, outline=INK)
    d.rectangle([cx - 1, cy - 1, cx + 1, cy + 1], fill=INK)

    # carrier: same integer sine and taper the firmware uses
    sin8 = [0, 49, 90, 118, 127, 118, 90, 49, 0, -49, -90, -118, -127, -118, -90, -49]
    X0, X1, AMP = 24, 96, 8
    span = X1 - X0
    prev = None
    for x in range(X0, X1 + 1, 2):
        dx = x - X0
        edge = min(dx, span - dx)
        amp = min(2 + (AMP - 2) * edge // (span // 4), AMP)
        y = cy - (sin8[((dx // 2) + phase) % 16] * amp) // 127
        if prev is not None:
            d.line([prev, (x, y)], fill=INK)
        prev = (x, y)

    # badge: sv_draw_badge(canvas, 102, 20, energised)
    bx, by = 102, cy - 8
    if energised:
        d.rounded_rectangle([bx, by, bx + 21, by + 15], radius=3, fill=INK)
        d.rectangle([bx + 4, by + 4, bx + 17, by + 6], fill=ORANGE)
        d.rectangle([bx + 4, by + 9, bx + 12, by + 11], fill=ORANGE)
    else:
        d.rounded_rectangle([bx, by, bx + 21, by + 15], radius=3, outline=INK)
        d.line([(bx + 4, by + 5), (bx + 17, by + 5)], fill=INK)
        d.line([(bx + 4, by + 10), (bx + 12, by + 10)], fill=INK)

    base(d, 64, 53, stage, SEC, anchor="ms")
    base(d, 64, 63, "Hold badge flat to the back", SEC, anchor="ms")
    return finish(img, name)


# ----------------------------------------------------------------- 3. result
def m_result(fname, name, letter, band, score, clone_time, clone_label, data):
    """Mirrors views/result_view.c: RV_* constants and rv_draw_bits()."""
    img = screen()
    d = ImageDraw.Draw(img)

    base(d, 2, 9, name, PRIM)
    d.line([(0, 11), (127, 11)], fill=INK)

    # band bar, inverted
    d.rounded_rectangle([0, 13, 127, 24], radius=2, fill=INK)
    base(d, 4, 23, letter, PRIM, fill=ORANGE)
    d.text((76, 19), band, font=SEC, fill=ORANGE, anchor="mm")

    # score
    if score is None:
        base(d, 3, 44, "--", BIG)
    else:
        s = str(score)
        base(d, 3, 44, s, BIG)
        base(d, 3 + width(d, s, BIG) + 2, 43, "/100", SEC)

    if clone_time is None:  # nothing decoded - point at the report instead
        base(d, 52, 33, "Nothing read", SEC)
        base(d, 52, 43, "OK for help", SEC)
    else:
        base(d, 52, 33, f"CLONE  {clone_time}", SEC)
        base(d, 52, 43, clone_label, SEC)

    # the credential, as bars
    d.rectangle([2, 46, 125, 53], outline=INK)
    ix, iy, iw, ih = 4, 48, 120, 4
    if data:
        bits = min(len(data) * 8, 60)
        step = max(iw // bits, 1)
        bw = step - 1 if step > 2 else 1
        for i in range(bits):
            if (data[i // 8] >> (7 - (i % 8))) & 1:
                d.rectangle([ix + i * step, iy, ix + i * step + bw - 1, iy + ih - 1], fill=INK)
    else:
        for x in range(ix, ix + iw, 4):
            d.line([(x, iy + ih // 2), (x + 1, iy + ih // 2)], fill=INK)

    # footer
    d.rectangle([0, 55, 127, 63], fill=INK)
    base(d, 3, 62, "OK Report", SEC, fill=ORANGE)
    base(d, 125, 62, "Rescan >", SEC, fill=ORANGE, anchor="rs")
    return finish(img, fname)


# ----------------------------------------------------------- 4. scrolled text
def m_scroll(fname, lines):
    """The widget text-scroll element: bold headers, a scrollbar on the right."""
    img = screen()
    d = ImageDraw.Draw(img)
    y = 8
    for text, bold in lines:
        base(d, 2, y, text, PRIM if bold else SEC)
        y += 9 if bold else 8
        if y > 64:
            break
    # scrollbar
    d.line([(126, 0), (126, 63)], fill=INK)
    d.rectangle([124, 0, 127, 22], fill=INK)
    return finish(img, fname)


def strip(images, name, cols=None):
    cols = cols or len(images)
    gap = 14
    cw, ch = images[0].width, images[0].height
    rows = (len(images) + cols - 1) // cols
    sheet = Image.new(
        "RGB", (cols * cw + (cols - 1) * gap, rows * ch + (rows - 1) * gap), (12, 12, 16)
    )
    for i, im in enumerate(images):
        sheet.paste(im, ((i % cols) * (cw + gap), (i // cols) * (ch + gap)))
    path = os.path.join(OUT, name)
    sheet.save(path)
    print("wrote", path)


if __name__ == "__main__":
    menu = m_menu()
    scan = m_scan()
    m_scan("screen_scan_idle.png", stage="Sensing...", energised=False, phase=0)

    em = m_result(
        "screen_grade_em4100.png",
        "EM4100 / EM4102",
        "F",
        "BROADCAST",
        13,
        "~2 s",
        "Cheap cloner",
        [0x12, 0x00, 0x34, 0x56, 0x78],
    )
    hid = m_result(
        "screen_grade_hid.png",
        "HID Prox H10301 26-bit",
        "F",
        "BROADCAST",
        9,
        "~5 s",
        "FSK/PSK tool",
        [0x2D, 0x30, 0x39],
    )
    gal = m_result(
        "screen_grade_gallagher.png",
        "Gallagher / Cardax",
        "D",
        "OBSCURED",
        38,
        "~30 s",
        "Format decoder",
        [0x9C, 0x41, 0x7A, 0x0E, 0x53, 0xB6, 0x28, 0xD1],
    )
    fdx = m_result(
        "screen_grade_fdxb.png",
        "FDX-B animal tag",
        "-",
        "NOT A KEY",
        None,
        "n/a",
        "Not a key",
        [0x3E, 0x1C, 0x00, 0x00, 0x4B, 0x2A, 0x7F, 0x01],
    )

    unread = m_result(
        "screen_unread.png",
        "Unknown 125 kHz tag",
        "-",
        "UNREAD",
        None,
        None,
        None,
        [],
    )

    report = m_scroll(
        "screen_report.png",
        [
            ("HID Prox H10301 26-bit", True),
            ("Grade F   9/100   BROADCAST", False),
            ("26 bits, and only 16 of them", False),
            ("are yours", False),
            ("", False),
            ("Findings", True),
            ("[x] No authentication: any", False),
            ("    reader gets the same answer", False),
            ("[!] FSK/PSK stops bargain", False),
            ("    cloners, not a Flipper", False),
        ],
    )
    score = m_scroll(
        "screen_score.png",
        [
            ("Score", True),
            ("Authentication  0/45", False),
            ("Integrity       2/15", False),
            ("Obfuscation     3/25", False),
            ("Key space       4/15", False),
            ("Total           9/100", False),
            ("", False),
            ("No 125 kHz credential scores", False),
            ("on authentication. That is the", False),
            ("45 points nothing here can win.", False),
        ],
    )

    strip([menu, scan, em, gal], "screens.png", cols=4)
    strip([hid, report, score, fdx], "screens_report.png", cols=4)
    strip([unread], "screen_unread_solo.png", cols=1)
