# utils

## make_runes.bat

Builds the rune packs (`assets\*.bin`) and the key map (`config\layout_en_us.inc`).

| Command                   | What it does |
|---------------------------|--------------|
| `make_runes.bat setup`    | One time only. Installs Pillow, the Python picture library. |
| `make_runes.bat`          | Generates everything into `utils\python\output\`, checks it, and compares it with your current files. Changes nothing else. |
| `make_runes.bat install`  | Same, then copies the seven packs into `assets\`, and the key map and pack table (`layout_en_us.inc`, `rune_packs.inc`) into `config\`. Then run `build.bat`: both `.inc` files are compiled into the exe. |

Edit the `PYTHON` line at the top of the bat file if Python lives somewhere else.

## python\

- `gen_bank0.py`: the script you run (through the bat file). Draws the runes,
  writes the files (including the pack table the assembly reads), and checks that packs do not overlap.
- `gen_sheets.py`: a library `gen_bank0.py` imports (glyph lists, font size, arrows). Do not run it.
- `fonts\`: DejaVu Sans Mono, the font the runes are drawn from, with its license.
- `output\`: created on first run. Safe to delete at any time.

## Changing a rune

Pack contents and order are defined in `gen_sheets.py` (`SHEETS`), and the first
ID of each pack in `gen_bank0.py` (`START`). Those two are the only places to
edit: the generator writes the pack table (`config\rune_packs.inc`) itself, so
nothing in the assembly needs updating by hand. The script refuses packs that
overlap or whose file names are too long for the table. After `install`, run
`build.bat`.

## ---

cd /d D:\projects\D\d_fasm\utils

make_runes.bat setup

This installs Pillow, the Python library that draws the pictures. Python is just the engine, and Pillow is a toolbox it borrows. You only do this once.

Each time you generate

Run make_runes.bat  

It creates utils\python\output\ and writes everything there. 
Your project is untouched. 
At the bottom it compares each new file to what’s in assets\ and config\, and each line should say “identical to the current file”. 
If a pack says “differs”, a different Pillow version can anti-alias a few edge pixels slightly differently. 

Check output\contact.png (a picture of every rune) against your preview PNGs, and keep your backup.

Run make_runes.bat install only when you’re happy. 
It copies the seven packs to assets\ and the layout to config\. 
If the script’s pack table ever disagrees with rune_display.inc, it refuses to install.

Run build.bat as usual.

## How the script works

A cell is a 32x32 picture with one byte per pixel (0 is empty, 255 is solid ink).
A pack is cells stored back to back, so cell N of a file is rune ID (first ID + N).
gen_sheets.py holds the glyph lists (SHEETS) and picks one font size that fits everything. It also draws the arrows.
gen_bank0.py does the rest:
render draws each glyph.
build_cells handles the special cases: : ; ! are built from the . dot, and ) ] } > ≥ are flipped copies of their opening forms.
write_layout holds the keyboard table, one line per key.
main writes the files and runs the checks.