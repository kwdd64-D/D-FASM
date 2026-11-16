"""Build the rune packs and the key map for D.

Run it through utils\\make_runes.bat (see utils\\README.md). What it does, in order:

  1. Draws every glyph into a 32x32 cell (font glyphs, a few built from shared parts, hand-drawn arrows).
  2. Writes the pack files (*.bin), arrows.bin, a contact sheet picture, layout_en_us.inc and
     rune_packs.inc into utils\\python\\output\\ . Nothing in your project is touched by this step.
  3. Checks that the packs fit the ID space (no overlaps, filenames short enough for the table).
  4. Compares each new file with the one currently in assets\\ or config\\ and says whether they match.
  5. Only if you ask for it ("install"), copies the packs to assets\\ and the two .inc files
     to config\\. Rebuild the exe afterwards: both .inc files are compiled into it.

rune_packs.inc is the pack table the assembly reads, so the table and the .bin files can no longer
disagree: they come from the same run.

Every rune ID has its own stored mask (no runtime mirror yet). Mirrored pairs such as ( and )
are drawn once and the closing form is made by flipping, so the two always match exactly.
"""
import shutil
import string
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageChops

import gen_sheets as G          # font, glyph lists, size/baseline choice, arrow cells

T = Image.Transpose
CELL = 32

# --- Where things are ---------------------------------------------------------
HERE = Path(__file__).resolve().parent          # ...\d_fasm\utils\python
ROOT = HERE.parents[1]                          # ...\d_fasm
OUT = HERE / 'output'                           # safe scratch folder for generated files
ASSETS = ROOT / 'assets'
CONFIG = ROOT / 'config'

# --- Pack layout --------------------------------------------------------------
# First rune ID of each pack, in the order the packs are loaded. The pack table the program
# reads (config\\rune_packs.inc) is generated from this and SHEETS, so this is the one place to edit.
# A pack file name is '<sheet name>.bin' and must be at most RUNE_NAME_MAX characters.
RUNE_NAME_MAX = 15          # RUNE_NAME_CHARS - 1 in lib\\rune\\pack_format.inc
START = {'space': 0x00, 'control': 0x10, 'digits': 0x30, 'math': 0x40,
         'punctuation': 0x50, 'uppercase': 0x80, 'lowercase': 0xA0}

# (sheet, left/opening form, right/closing form): the closing form is the flipped opening form.
MIRROR_PAIRS = [('punctuation', '(', ')'), ('punctuation', '[', ']'), ('punctuation', '{', '}'),
                ('math', '<', '>'), ('math', '\u2264', '\u2265')]


def render(font, ch):
    """Draw one glyph into a 32x32 cell, lifting it if it would spill past the top or bottom."""
    e = G.extent(font, ch)
    dy = 0
    if e:
        if G.base + e[1] > 31:
            dy = 31 - (G.base + e[1])
        if G.base + e[0] + dy < 0:
            dy = -(G.base + e[0])
    big = Image.new('L', (96, 96), 0)
    ImageDraw.Draw(big).text((48, 32 + G.base + dy), ch, font=font, fill=255, anchor='ms')
    return big.crop((32, 32, 64, 64))


def rows(im):
    """(first, last) rows that contain ink, or None for an empty cell."""
    bb = im.getbbox()
    return None if bb is None else (bb[1], bb[3] - 1)


def shift_up(im, n):
    out = Image.new('L', (CELL, CELL), 0)
    out.paste(im.crop((0, n, CELL, CELL)), (0, 0))
    return out


def build_cells():
    """Return {sheet name: {glyph: 32x32 image}} with the special-case glyphs applied."""
    G.SHEETS['space'] = [' ', '`', '\ufffd']     # cell 2 = ID 0x02: the single 'unknown rune' fallback
    font = ImageFont.truetype(G.P, G.size)
    cells = {name: {ch: render(font, ch) for ch in glyphs} for name, glyphs in G.SHEETS.items()}
    punc = cells['punctuation']

    # Atoms: the '.' dot and the ',' tail are reused so every dot is identical.
    dot = punc['.']
    delta = rows(dot)[0] - rows(render(font, ':'))[0]      # how far above the period the font puts the upper dot
    upper_dot = shift_up(dot, delta)
    punc[':'] = ImageChops.lighter(dot, upper_dot)
    punc[';'] = ImageChops.lighter(punc[','], upper_dot)
    bang = render(font, '!')
    top_of_dot = rows(dot)[0]
    bar = bang.copy()
    ImageDraw.Draw(bar).rectangle([0, top_of_dot - 2, CELL, CELL], fill=0)   # keep only the bar, leave a gap row
    punc['!'] = ImageChops.lighter(bar, dot)

    # Mirrored pairs: the closing form is the flipped opening form.
    for sheet, opening, closing in MIRROR_PAIRS:
        cells[sheet][closing] = cells[sheet][opening].transpose(T.FLIP_LEFT_RIGHT)
    return cells


def write_layout(path, ident):
    """US keyboard -> rune ID, written as FASM data. Entry = vk (word), modifier mask (byte), rune ID (byte)."""
    L = []   # (virtual key, modifier mask, character). Mask: 1 = Shift, 2 = Ctrl.
    for c in string.ascii_lowercase:
        L += [(ord(c.upper()), 0, c), (ord(c.upper()), 1, c.upper())]
    for d, s in zip('0123456789', ')!@#$%^&*('):
        L += [(ord(d), 0, d), (ord(d), 1, s)]
    L += [(0x20, 0, ' ')]
    for vk, base, shifted in [(0xC0, '`', '~'), (0xBD, '-', '_'), (0xBB, '=', '+'), (0xDB, '[', '{'),
                              (0xDD, ']', '}'), (0xDC, '\\', '|'), (0xBA, ';', ':'), (0xDE, "'", '"'),
                              (0xBC, ',', '<'), (0xBE, '.', '>'), (0xBF, '/', '?')]:
        L += [(vk, 0, base), (vk, 1, shifted)]
    # Control keys (mask 0) and the Ctrl layer for math.
    L += [(0x09, 0, '\u21e5'), (0x0D, 0, '\u21b5'), (0x08, 0, '\u232b'), (0x2E, 0, '\u2326')]
    L += [(0x0D, 1, '\u21a4'), (0x20, 1, '\u237d')]
    L += [(0xBB, 2, '\u2260'), (0xBC, 2, '\u2264'), (0xBE, 2, '\u2265'),
          (0xBD, 2, '\u2212'), (0x38, 2, '\u00d7'), (0xBF, 2, '\u00f7')]
    assert len({(v, s) for v, s, _ in L}) == len(L), 'two layout entries share the same key + modifier'
    missing = [ch for _, _, ch in L if ch not in ident]
    assert not missing, f'layout uses glyphs that are in no pack: {missing!r}'

    prefix = {0: '', 1: 'Shift+', 2: 'Ctrl+'}
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('; Data-only English/US virtual-key map for bank 0 (generated by gen_bank0.py).\n')
        f.write('; Each 4-byte entry is: Win32 virtual key (word), modifier mask (byte: 1 = Shift, 2 = Ctrl), rune ID (byte).\n\n')
        f.write("section '.data' data readable writeable\nlayout_en_us_keys:\n")
        for vk, sh, ch in L:
            f.write(f'    dw 0x{vk:02X}\n    db {sh}, 0x{ident[ch]:02X}                 ; {prefix[sh]}{ch!r}\n')
        f.write('layout_en_us_keys_end:\n')
    return len(L)


def write_packs(path, manifest):
    """The pack table as FASM data: one rune_pack line per pack. Entry = first ID, cell count, file name."""
    end_prev = 0
    for name, first, count in sorted(manifest, key=lambda m: m[1]):
        assert len(name + '.bin') <= RUNE_NAME_MAX, f'{name}.bin is longer than {RUNE_NAME_MAX} characters'
        assert first >= end_prev, f'pack {name} overlaps the pack before it'
        end_prev = first + count
    assert end_prev <= 256, 'packs run past rune ID 0xFF'
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('; Data-only rune pack table (generated by gen_bank0.py). Do not edit by hand.\n')
        f.write('; One line per pack: first rune ID, number of cells, file name inside ..\\assets\\.\n')
        f.write('; Cell N of a file is rune ID (first + N) in bank 0. Format: lib/rune/pack_format.inc,\n')
        f.write('; which must be included first (it defines the rune_pack macro).\n\n')
        f.write("section '.data' data readable writeable\n    align 4\n    rune_pack_table:\n")
        for name, first, count in manifest:
            f.write(f"        rune_pack 0x{first:02X}, {count:2d}, '{name}.bin'\n")
        f.write('    rune_pack_table_end:\n')
        f.write(f'    assert (rune_pack_table_end - rune_pack_table) = {len(manifest)} * RUNE_PACK_ENTRY_BYTES\n')


def compare(new_path, old_path):
    """One line saying how a freshly generated file relates to the one already in the project."""
    if not old_path.is_file():
        return 'no existing file to compare'
    new = new_path.read_bytes().replace(b'\r\n', b'\n')
    old = old_path.read_bytes().replace(b'\r\n', b'\n')
    if new == old:
        return 'identical to the current file'
    if len(new) != len(old):
        return f'DIFFERENT SIZE (new {len(new)}, current {len(old)})'
    n = sum(1 for a, b in zip(new, old) if a != b)
    return f'differs in {n} of {len(new)} bytes'


def main():
    install = 'install' in sys.argv[1:]
    OUT.mkdir(exist_ok=True)
    print(f'Font: {G.P}\nSize {G.size}, baseline row {G.base} (allowed {G.lo}-{G.hi})\nOutput: {OUT}\n')

    cells = build_cells()

    print('Packs:')
    manifest = []
    for name, glyphs in G.SHEETS.items():
        buf = b''.join(cells[name][ch].tobytes() for ch in glyphs)
        assert len(buf) == len(glyphs) * 1024
        (OUT / f'{name}.bin').write_bytes(buf)
        manifest.append((name, START[name], len(glyphs)))
        print(f'  {name}.bin  {len(buf)} bytes, {len(glyphs)} cells, IDs 0x{START[name]:02X}-0x{START[name] + len(glyphs) - 1:02X}')

    abuf = b''.join(c.tobytes() for c in G.arrow_cells())
    (OUT / 'arrows.bin').write_bytes(abuf)
    print(f'  arrows.bin  {len(abuf)} bytes (not loaded by the program yet; ID range still under review)')

    # Contact sheet for eyeballing: every rune in pack order on a grey background, enlarged 2x.
    order = [(n, ch) for n in G.SHEETS for ch in G.SHEETS[n]]
    cols = 16
    rws = (len(order) + cols - 1) // cols
    sheet = Image.new('L', (cols * 36, rws * 36), 60)
    for i, (n, ch) in enumerate(order):
        sheet.paste(cells[n][ch], ((i % cols) * 36 + 2, (i // cols) * 36 + 2))
    sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
    sheet.save(OUT / 'contact.png')

    ident = {}      # glyph -> rune ID
    for name, glyphs in G.SHEETS.items():
        for i, ch in enumerate(glyphs):
            ident[ch] = START[name] + i
    count = write_layout(OUT / 'layout_en_us.inc', ident)
    print(f'\nLayout: {count} key entries -> layout_en_us.inc')

    write_packs(OUT / 'rune_packs.inc', manifest)
    print(f'Pack table: {len(manifest)} packs, {sum(c for _, _, c in manifest)} cells -> rune_packs.inc')

    print('\nComparison with the files currently in your project:')
    for name, _, _ in manifest:
        print(f'  assets\\{name}.bin: {compare(OUT / f"{name}.bin", ASSETS / f"{name}.bin")}')
    print(f'  config\\layout_en_us.inc: {compare(OUT / "layout_en_us.inc", CONFIG / "layout_en_us.inc")}')
    print(f'  config\\rune_packs.inc: {compare(OUT / "rune_packs.inc", CONFIG / "rune_packs.inc")}')

    if not install:
        print('\nNothing in your project was changed. Run "make_runes.bat install" to copy these files in.')
        return 0
    for name, _, _ in manifest:
        shutil.copyfile(OUT / f'{name}.bin', ASSETS / f'{name}.bin')
    shutil.copyfile(OUT / 'layout_en_us.inc', CONFIG / 'layout_en_us.inc')
    shutil.copyfile(OUT / 'rune_packs.inc', CONFIG / 'rune_packs.inc')
    print(f'\nInstalled {len(manifest)} packs into {ASSETS}, and layout_en_us.inc + rune_packs.inc into {CONFIG}.')
    print('Now run build.bat: both .inc files are compiled into the exe.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
