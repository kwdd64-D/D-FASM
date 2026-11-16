"""Shared rune-sheet library (imported by gen_bank0.py; you do not run this file directly).

What lives here:
  * which glyphs go in which sheet, and in what order (SHEETS)
  * choosing one font size + baseline that fits every glyph in a 32x32 cell
  * the arrow cells, drawn by hand rather than taken from the font

A "cell" is 32x32 pixels, one byte per pixel (0 = empty, 255 = fully covered), no header.
A "sheet" is cells stored back to back, so cell N of a .bin file is rune ID (first ID + N).
"""
import string
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageChops

# --- Font ---------------------------------------------------------------------
# The font ships beside this script in fonts\ so the result does not depend on what is
# installed on your PC. (Same family and version as the original bins: DejaVu Sans Mono.)
HERE = Path(__file__).resolve().parent
FONT_CANDIDATES = [
    HERE / 'fonts' / 'DejaVuSansMono.ttf',
    Path('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'),   # Linux fallback
]
P = next((str(p) for p in FONT_CANDIDATES if p.is_file()), None)
if P is None:
    raise SystemExit('Font not found. Expected: ' + str(FONT_CANDIDATES[0]))

CELL = 32

# --- Sheets -------------------------------------------------------------------
# name -> glyph list, in cell order. Order here is the order of IDs inside the pack file.
SHEETS = {
    'space':       [' ', '`'],
    'control':     [chr(c) for c in (0x21E5, 0x21B5, 0x21A4, 0x237D, 0x2205, 0x238B, 0x232B, 0x2326)],
    'digits':      list('0123456789'),
    'math':        list('+\u2212\u00d7\u00f7=\u2260<>\u2264\u2265%'),
    'punctuation': list(".,:;!?'\"()[]{}@#&|~_\\$^-*/"),
    'uppercase':   list(string.ascii_uppercase),
    'lowercase':   list(string.ascii_lowercase),
}

# This glyph sits below the baseline by design, so it is lifted on its own instead of
# making every cell smaller to fit it.
NBSP = chr(0x237D)


def extent(font, ch):
    """Top and bottom ink rows of a glyph, measured relative to the baseline row."""
    big = Image.new('L', (128, 128), 0)
    ImageDraw.Draw(big).text((64, 64), ch, font=font, fill=255, anchor='ms')
    bb = big.getbbox()
    return None if bb is None else (bb[1] - 64, bb[3] - 1 - 64)


def choose():
    """Largest font size where every glyph fits in 32 rows, plus the baseline row to use."""
    for size in range(32, 20, -1):
        f = ImageFont.truetype(P, size)
        top = 0
        bot = -99
        for glyphs in SHEETS.values():
            for ch in glyphs:
                if ch == NBSP:
                    continue
                e = extent(f, ch)
                if e:
                    top = min(top, e[0])
                    bot = max(bot, e[1])
        lo, hi = -top, 31 - bot
        if lo <= hi:
            return size, (lo + hi) // 2, lo, hi
    raise SystemExit('No font size between 21 and 32 fits every glyph in a 32x32 cell.')


size, base, lo, hi = choose()

# --- Arrows -------------------------------------------------------------------
# Two base shapes are drawn by hand, centred exactly on the cell centre. Every other
# direction is an exact 90-degree pixel permutation of a base cell (no resampling), so
# all directions match by construction. Only the diagonal base is rotated by 45 degrees.
ARROWS = ['left', 'up', 'right', 'down', 'up-left', 'up-right', 'down-right', 'down-left',
          'left-right', 'up-down']


def arrow_cells():
    S = 8                       # draw at 8x size, shrink at the end for smooth edges
    c = 16 * S
    Lh = 9 * S                  # half length
    hh = 5.5 * S                # head half-height
    hl = 7.5 * S                # head length
    sh = 1.25 * S               # shaft half-thickness

    def base(double):
        # Draw the upper half, then mirror it: symmetric about the horizontal axis by construction.
        im = Image.new('L', (CELL * S, CELL * S), 0)
        d = ImageDraw.Draw(im)
        d.polygon([(c - Lh, c), (c - Lh + hl, c - hh), (c - Lh + hl, c)], fill=255)
        d.rectangle([c - Lh + hl - S // 2, c - sh,
                     ((c + Lh - hl + S // 2) if double else (c + Lh)) - 1, c - 1], fill=255)
        if double:
            d.polygon([(c + Lh, c), (c + Lh - hl, c - hh), (c + Lh - hl, c)], fill=255)
        return ImageChops.lighter(im, im.transpose(Image.Transpose.FLIP_TOP_BOTTOM))

    def down(im):
        return im.resize((CELL, CELL), Image.BOX)

    left = down(base(False))
    nw = down(base(False).rotate(-45, resample=Image.BICUBIC, center=(c, c)))
    dbl = down(base(True))
    # Rotations below are only exact if the base is symmetric top-to-bottom:
    assert left.tobytes() == left.transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes()
    T = Image.Transpose
    return [left, left.transpose(T.ROTATE_270), left.transpose(T.ROTATE_180), left.transpose(T.ROTATE_90),
            nw, nw.transpose(T.ROTATE_270), nw.transpose(T.ROTATE_180), nw.transpose(T.ROTATE_90),
            dbl, dbl.transpose(T.ROTATE_90)]
