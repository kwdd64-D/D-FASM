# DisplaySurface source

`d_fasm.asm` is the assembly entry point for the DisplaySurface
implementation.

At startup it asks the host for the primary monitor's bounds and stores them
in one `ds_0` descriptor (the `AREA` layout from `lib/spatial.inc`),
allocates a 32-bit-per-pixel frame buffer of the same size (`pixel_buffer`,
owned here, not by the host), fills it with the descriptor's color, loads
the rune packs, then hands the descriptor and buffer to the host
(`lib/win64_host.inc`) to create the window and run the message loop. The
entry point makes no Windows window calls itself.

`app_events.inc` holds what the application does with events the host
passes along: `AppOnKey` handles Esc (close) and forwards every other key to
the rune code.

`config/config.ini` is retained for upcoming configuration work. The
assembly does not read it yet.

## Project layout

```text
d_fasm\
  assets\    rune pack .bin files (read at runtime)
  build\     build output (d_fasm.exe); launch from here
  config\    layout_en_us.inc (GENERATED key map), rune_packs.inc (GENERATED
             pack table), config.ini (not used yet)
  docs\
  lib\       spatial.inc, draw2d.inc, keys.inc, win64_host.inc
    rune\    all rune code, data declarations, and constants (see lib\README.md)
  src\       d_fasm.asm, app_events.inc (this folder)
  utils\     FileList.bat; python\ holds the pack generators
              (gen_bank0.py builds the packs and key map; it imports gen_sheets.py)
  version\
  build.bat  sets the FASM INCLUDE path, runs fasm, reports output size
```

`build.bat` has one machine-specific line, `FASM_INCLUDE`, which points at
the `INCLUDE` folder of your FASM install. It also expects `fasm` on PATH.

## Include order

Project includes are relative to this file. FASM's own includes are found
through the `INCLUDE` path set by `build.bat`.

```text
win64w.inc           FASM's Windows x64 definitions
spatial.inc          AREA field offsets used by ds_0
keys.inc             Win32 virtual-key codes (KEY_*)
draw2d.inc           frame-buffer drawing macro
layout_format.inc    layout entry format constants           (lib/rune/)
pack_format.inc      pack table format constants and macro   (lib/rune/)
rune_constants.inc   constants shared by the rune files      (lib/rune/)

section '.data'      (ds_0, strings and buffer pointers are declared in d_fasm.asm itself)
win64_host_data.inc  host constants and data                 (lib/)
layout_en_us.inc     GENERATED key-to-rune assignments       (config/)
rune_packs.inc       GENERATED pack table                    (config/)
rune_data.inc        rune data with initial values           (lib/rune/)

section '.bss'
rune_bss.inc         rune buffers, palette, mask table, pool (lib/rune/)

section '.text'
win64_host.inc       OS window and presentation              (lib/)
rune_loader.inc      pack loading                            (lib/rune/)
rune_bank.inc        active bank from the lock keys          (lib/rune/)
rune_draw.inc        palette, mask drawing, bank indicator   (lib/rune/)
rune_input.inc       key press to rune                       (lib/rune/)
app_events.inc       AppOnKey: Esc closes, other keys go to runes (src/)
start:               entry point (this file)
```

`d_fasm.asm` is the only file that declares sections. Every include is
constants-only, data-only, bss-only, or code-only, and lands in the section
open at that point. FASM cannot reopen a section, so all the data is included
first, then the reserved space, then the code. Within that:

- constants and formats come before anything that uses them (they sit above
  the first section);
- `rune_bss.inc` comes after `config/rune_packs.inc` (the pool is sized from
  the table's cell total);
- `spatial.inc` comes before `ds_0` and the host.

## Rune packs

At startup the program loads seven packs from `..\assets\` (relative to the
executable's folder, so the exe is expected in `build\`). Each file must be
exactly `count * 1024` bytes: one 32x32, one-byte-per-pixel coverage mask
per cell, 110 cells in total. If any file is missing or the wrong size, the
app shows "Rune pack missing or invalid." and exits.

| First ID | Cells | File              | Contents      |
|----------|-------|-------------------|---------------|
| `0x00`   | 3     | `space.bin`       | space, ` , fallback rune (`0x02`) |
| `0x10`   | 8     | `control.bin`     | tab, enter, backspace, delete, etc. |
| `0x30`   | 10    | `digits.bin`      | 0-9           |
| `0x40`   | 11    | `math.bin`        | operators and comparisons |
| `0x50`   | 26    | `punctuation.bin` | punctuation and symbols |
| `0x80`   | 26    | `uppercase.bin`   | A-Z           |
| `0xA0`   | 26    | `lowercase.bin`   | a-z           |

Cell N of a file is rune ID (first + N). Any ID with no loaded mask draws
the fallback rune, ID `0x02`.

The table that drives loading (`rune_pack_table`) is generated into
`config/rune_packs.inc` by `utils\python\gen_bank0.py`, in the same run
that writes the `.bin` files, so the table and the files cannot disagree.
The key-to-ID assignments are generated into `config/layout_en_us.inc`, so
layouts can change without touching the rune code. After regenerating,
rebuild the exe: both `.inc` files are compiled in.

## Running

Launch `build\d_fasm.exe` from any working directory. Pressing a mapped
key replaces the DisplaySurface contents with that one rune, centered and
enlarged 8x. Nothing is printed as text. Esc exits (temporary exit path).

## Banks

Rune lookup is `rune_mask_table[bank*256 + rune]`. There are three banks,
selected by lock-key state and shown as a digit at the top center of the
surface:

| Bank | Selected by               | Status |
|------|---------------------------|--------|
| 0    | no lock key active        | all 110 masks loaded |
| 1    | Caps Lock                 | no masks yet (draws the fallback rune) |
| 2    | Scroll Lock               | no masks yet (draws the fallback rune) |

If both locks are on, Scroll Lock wins (a temporary rule). Scroll Lock is
used here partly because it is otherwise a dead key; its meaning is meant
to stay open for future use.

## Planned: mirror and rotate

Each bank has only 256 IDs, so stored masks will eventually run short.
Mirrored and rotated variants (for example paired brackets) should then
share a single stored mask with a transform applied at draw time.

An earlier prototype did exactly this: the low bit of the rune ID meant
"mirrored". It was removed while every ID had its own mask, but the hook is
deliberately left in `DrawRuneValue` (a flag register that is currently
always 0; nonzero reads each row right-to-left).

The first candidates are the five mirrored pairs that `gen_bank0.py` already
bakes as separate masks: `( )`, `[ ]`, `{ }`, `< >`, and `≤ ≥`. Each right-hand
form is a flipped copy of the left, so a runtime mirror would save 5 KB of
pool. When storage pressure arrives, the likely steps are:

1. Add a per-ID transform field (mirror-x, mirror-y, rotate) next to the
   mask pointer or in the layout data.
2. Replace the always-zero flag with that field.
3. Remove the now-redundant stored masks.

## Open items

- Done: pool size is computed from `rune_pack_table` (a running counter in
  the `rune_pack` macro), so adding or resizing a pack needs no other edit.
- Done: the rune pool and mask table live in an uninitialized `.bss`
  section. They were previously ~116 KB of zeros stored inside the exe; the
  exe went from about 126 KB to about 10 KB with no change in behavior. The
  loader zero-fills `.bss`, which the mask table relies on (0 = not loaded).
- Done: the pack table is generated (`config/rune_packs.inc`) from `START`
  and `SHEETS` in `utils\python\gen_bank0.py`, so it is no longer kept in
  sync by hand. The generator also refuses overlapping packs and file names
  too long for the table.
- Done: `d_fasm.asm` owns the section layout (`.data`, `.bss`, `.text`, `.idata`).
  Previously each file opened its own sections (ten in all), and FASM does not
  merge same-named ones; the files are now split into data-only, bss-only,
  and code-only so there are four.
- Config-file loading is not implemented.
