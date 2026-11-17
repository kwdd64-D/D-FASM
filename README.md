# D (d_fasm)

A Windows x64 program written in FASM assembly. Current stage: a borderless
window covers the primary monitor (the **DisplaySurface**), and pressing a
key shows that key's **rune** (a 32x32 coverage mask, drawn enlarged) in the
middle of it. No text is ever printed; everything on screen is a rune.

This file is the map of what exists and works today. Details for each part
live in the README beside it (links below). Keep this file short and
current: when a stage changes, change it here first.

## Status

| Area | State |
|------|-------|
| Primary-monitor DisplaySurface (`ds_0`), frame buffer, presentation | Working |
| Rune packs: 7 packs, 110 runes, loaded from `assets\` at startup | Working |
| Key to rune mapping (English/US layout, Shift and Ctrl layers) | Working |
| Bank indicator (digit at top center), Caps Lock = bank 1, Scroll Lock = bank 2 | Working |
| Banks 1 and 2 contents | Empty: keys in those banks draw the fallback rune |
| Pack generator you can run yourself (`utils\make_runes.bat`) | Working |
| Runtime mirror/rotate of runes | Hook exists, not active (planned) |
| `arrows.bin` | Generated, not loaded by the program yet |
| `config\config.ini` | Present, not read by the program |
| Esc to exit | Temporary exit path until a region provides a close control |

Executable size is about 10 KB (the rune pool lives in an uninitialized
`.bss` section instead of the file).

## Quick start

1. **Build:** run `build.bat`. It needs FASM on PATH; edit the one
   `FASM_INCLUDE` line at the top for your machine. Output: `build\d_fasm.exe`.
2. **Run:** launch `build\d_fasm.exe` (the exe must stay in `build\` because
   packs are found at `..\assets\`). Press a key; Esc exits.
3. **Regenerate runes (optional):** `utils\make_runes.bat`, see
   `utils\README.md`. First time only: `utils\make_runes.bat setup`.

## Folder map

```text
d_fasm\
  build.bat          builds src\d_fasm.asm into build\d_fasm.exe
  assets\            rune pack .bin files (read at runtime); preview images
  build\             d_fasm.exe
  config\            layout_en_us.inc, rune_packs.inc (both GENERATED), config.ini (unused)
  docs\              d_fasm_filelist.md
  lib\               spatial.inc, draw2d.inc, keys.inc, win64_host.inc, win64_host_data.inc
    rune\          all rune code, data declarations, constants, and formats
  src\               d_fasm.asm (entry point), app_events.inc (what keys do)
  utils\             make_runes.bat, FileList.bat
    python\          gen_bank0.py, gen_sheets.py, fonts\ (rune generator)
  version\
```

Per-folder details: [`src\README.md`](src/README.md) (entry point, packs,
banks, planned mirror/rotate), [`lib\README.md`](lib/README.md) (each include
file), [`utils\README.md`](utils/README.md) (generator workflow).

## How it fits together

1. `d_fasm.asm` asks the host for the primary monitor's bounds
   (`HostGetPrimaryBounds`) and stores them in `ds_0`, a 32-byte `AREA`
   record (position x,y,z; size w,h,d; color; layout in `lib\spatial.inc`).
2. It allocates a 32-bit-per-pixel frame buffer of that size (`pixel_buffer`,
   owned by the surface, not the host) and fills it with the surface color.
3. `BuildCoveragePalette` derives the rune palette from `ds_0`'s color.
   `LoadRunePacks` reads each pack from `..\assets\`, checks every file is
   exactly `count * 1024` bytes, and registers each cell in
   `rune_mask_table[bank*256 + rune]`. Any failure shows an error and exits.
4. `HostCreate` (in `win64_host.inc`) registers the window class and creates
   the borderless window over `ds_0`; `HostRun` runs the message loop. The
   host presents the buffer with `StretchDIBits`. It knows nothing about
   runes or `ds_0` by name: it is handed an `AREA` and a buffer pointer. No
   other file touches an `HWND`.
5. On a key press the host calls `AppOnKey` (`src\app_events.inc`) with the
   virtual-key code. Esc closes the window (`HostClose`). Any other key goes
   to `HandleRuneKey`, which reads Shift/Ctrl, finds the key in the layout
   table, picks the bank from the lock keys, and `DrawRuneValue` clears the
   surface and draws that rune at 8x, centered. An ID with no loaded mask
   draws the fallback rune (bank 0, ID `0x02`). `AppOnKey` tells the host
   whether to repaint (`HOST_KEY_*`).

## Rune ID map (bank 0)

| First ID | Cells | Pack file         |
|----------|-------|-------------------|
| `0x00`   | 3     | `space.bin` (ID `0x02` is the fallback rune) |
| `0x10`   | 8     | `control.bin`     |
| `0x30`   | 10    | `digits.bin`      |
| `0x40`   | 11    | `math.bin`        |
| `0x50`   | 26    | `punctuation.bin` |
| `0x80`   | 26    | `uppercase.bin`   |
| `0xA0`   | 26    | `lowercase.bin`   |

A cell is 32x32 pixels, one byte per pixel (0 = empty, 255 = full ink), no
header. Cell N of a pack is rune ID (first + N).

## Where each fact is defined (single source of truth)

Change a fact in its one home, then rebuild. Do not hand-edit generated files.

| Fact | Defined in |
|------|-----------|
| What glyphs a pack holds, and their order | `utils\python\gen_sheets.py` (`SHEETS`) |
| First ID of each pack | `utils\python\gen_bank0.py` (`START`). The pack table the program loads, `config\rune_packs.inc`, is **generated** from it (do not edit by hand) |
| Pack-table entry format | `lib\rune\pack_format.inc` (used by `LoadRunePacks` and `write_packs`) |
| Pool size | Computed from the pack table; nothing to edit |
| Which key makes which rune | `write_layout` in `utils\python\gen_bank0.py`, which **generates** `config\layout_en_us.inc` (do not edit the `.inc` by hand) |
| Bank selection rule | `ReadBank` in `lib\rune\rune_bank.inc` |
| Rune size on screen (8x), indicator position, bank count | `lib\rune\rune_constants.inc` |
| Which keys do what at application level (for example Esc to exit) | `AppOnKey` in `src\app_events.inc` |
| Host API (create, run, close, key-result codes) | header of `lib\win64_host.inc` |
| Key codes (`KEY_*`) | `lib\keys.inc` |
| Section layout and include order | `src\d_fasm.asm` (the only file that declares sections) |
| Surface geometry and background color | `ds_0` in `src\d_fasm.asm` (the rune palette is derived from this color at startup, so it is the only place to change it) |
| Key-layout entry format (entry size, field offsets, Shift/Ctrl bits) | `lib\rune\layout_format.inc` (used by `HandleRuneKey` and `write_layout`) |
| FASM install location | `FASM_INCLUDE` in `build.bat` |
| Python location | `PYTHON` in `utils\make_runes.bat` |

## Common changes

- **Change how a rune looks:** edit the glyph list or drawing code in
  `utils\python`, run `make_runes.bat`, check `utils\python\output\contact.png`,
  then `make_runes.bat install`. No assembly rebuild is needed for appearance
  alone (the exe reads the packs at startup). If a pack's cell count changed,
  rebuild too: the pack table is compiled in.
- **Add a rune to a pack:** add it to `SHEETS`; give it a key in
  `write_layout` if it should be typeable; regenerate, install, rebuild. The
  pack table updates itself.
- **Add a pack:** add it to `SHEETS` and `START`; regenerate, install,
  rebuild. Pool size follows automatically.
- **Give a new key a rune:** add a line in `write_layout`, regenerate, install.
  Rebuild the exe (the layout is compiled in).

## Known limits and notes

- `draw_rect_block` does not validate or clip; callers must keep rectangles
  inside the buffer. It clobbers `rdi`, `r14`, `r15`, so any proc using it
  must list them in `uses`.
- The mirrored pairs `( )`, `[ ]`, `{ }`, `< >`, `≤ ≥` are each stored twice
  (the right-hand form is a flipped copy). Runtime mirroring would save 5 KB;
  see "Planned: mirror and rotate" in `src\README.md`.
- If both Caps and Scroll Lock are on, Scroll Lock wins (temporary rule).
- Only `src\d_fasm.asm` declares sections (`.data`, `.bss`, `.text`, `.idata`).
  Every other file is constants-only, data-only, bss-only, or code-only, and
  lands in whichever section is open where it is included, so the include
  order in `d_fasm.asm` matters.
- Only the primary monitor is used; there is one DisplaySurface.

## Planned separations (one file, one purpose)

Goal: each file has one purpose so data can be separated from code as the
project grows. Done one step at a time, each tested with a build before the
next. All rune files go in `lib\rune\`.

1. **Done:** rune files in `lib\rune\`; the rune palette derives from `ds_0`'s
   color (no hard-coded copy); layout entry format constants in
   `layout_format.inc`.
2. **Done:** the pack table is generated (`config\rune_packs.inc`);
   `rune_display.inc` split into `rune_constants`, `pack_format`,
   `rune_data`, `rune_bss`, `rune_loader`, `rune_bank`, `rune_draw`, and `rune_input`.
3. **Done:** host boundary. Window creation, the message loop, and monitor
   detection moved out of `d_fasm.asm` into the host (`HostGetPrimaryBounds`,
   `HostCreate`, `HostRun`, `HostClose`); the surface owns `pixel_buffer`; the
   host hands key presses to the application (`AppOnKey` in
   `src\app_events.inc`) instead of calling `HandleRuneKey`; Esc policy moved
   to the application; key codes live in `lib\keys.inc`.
4. **In progress:** section layout. `d_fasm.asm` owns the sections (`.data`,
   `.bss`, `.text`, `.idata`: four instead of ten) and every include is
   data-only, bss-only, or code-only. The host and the rune state were split
   into data and code files, the zero-filled buffers moved to `.bss`, and the
   generated `.inc` files no longer declare sections.
5. Later: scaled-mask drawing in `draw2d.inc` with a canvas parameter, one
   shared rune lookup, Python layout data file, arrows module.

## Not started / deferred

Banks 1 and 2 content, runtime mirror/rotate, reading `config.ini`, a real
close control to replace Esc, loading `arrows.bin`, more than one
DisplaySurface.

## Working with an AI on this project

To keep sessions focused, share only what the task touches:

| Task | Share |
|------|-------|
| Drawing and palette | this file, `lib\rune\rune_draw.inc`, `lib\rune\rune_constants.inc`, `lib\rune\rune_bss.inc` |
| Key handling and banks | this file, `lib\rune\rune_input.inc`, `lib\rune\rune_bank.inc`, `lib\rune\layout_format.inc`, `config\layout_en_us.inc` |
| Pack loading | this file, `lib\rune\rune_loader.inc`, `lib\rune\pack_format.inc`, `config\rune_packs.inc` |
| Window, presentation, raw input | this file, `lib\win64_host.inc`, `lib\win64_host_data.inc`, `lib\keys.inc`, `src\d_fasm.asm` |
| What keys do at application level | this file, `src\app_events.inc`, `lib\keys.inc` |
| Rune shapes, packs, key map | this file, `utils\python\gen_bank0.py`, `utils\python\gen_sheets.py` |
| Surface geometry, new records | this file, `lib\spatial.inc`, `src\d_fasm.asm` |

Ground rules worth stating at the start of a session:

- Treat this README's "Where each fact is defined" table as authoritative.
- Do not invent files or folders that are not in the folder map.
- Binary packs cannot be shared; describe changes to them by editing the
  generator, not the `.bin` files.
- When a change alters what is documented here, update this README and the
  README of the affected folder in the same change.

## Stage log

Add one line per stage as the project moves on.

- **Step 4 of separation (done):** `d_fasm.asm` owns the sections; data, bss, and code files are separate; exe sections 10 to 4.
- **Step 3 of separation (done):** host boundary: `HostGetPrimaryBounds`/`HostCreate`/`HostRun`/`HostClose` in the host, `AppOnKey` in `src\app_events.inc`, key codes in `lib\keys.inc`, `pixel_buffer` owned by the surface.
- **Step 2 of separation (done):** pack table generated into `config\rune_packs.inc`; `rune_display.inc` split into seven single-purpose rune files.
- **Step 1 of separation (done):** rune files in `lib\rune\`, palette derived from `ds_0`, shared layout-format constants.
- **Current:** single DisplaySurface on the primary monitor; 7 rune packs
  (110 runes); English/US key layer; bank indicator with Caps/Scroll Lock;
  self-serve pack generator; exe about 10 KB.
