# DisplaySurface lib support files

This folder holds the assembly components used by the DisplaySurface
implementation in `src/d_fasm.asm`.

## `spatial.inc`

Defines the 32-byte `AREA` record layout and its field offsets: position
(`x`, `y`, `z`), extent (`w`, `h`, `d`), color, and a reserved dword. The
record is padded to 32 bytes so indexing is a shift. `d` is layer extent,
not color depth. The current `ds_0` descriptor is declared in
`src/d_fasm.asm`.

## `draw2d.inc`

Defines the `draw_rect_block` macro, which fills a rectangular region of a
32-bit frame buffer.

- The caller must provide positive dimensions and bounds that fit inside
  the buffer. The macro does not validate or clip.
- It clobbers `rax`, `rcx`, `rdi`, `r8`, `r10`, `r11`, `r14`, and `r15`.
  `rdi`, `r14`, and `r15` are non-volatile in the Win64 ABI, so any proc
  that uses the macro must list them in its `uses` clause.

## `rune/`

All rune code and constants live in this folder, so `lib` itself stays small as
it grows.

The folder follows one rule: each file has one purpose. Generated data lives in
`config\`, not here. A file is one of: constants/format, initialized data,
reserved space (bss), or code, and none of them declares a section.

### Formats and constants (no code, no data)

- `layout_format.inc`: the key-layout entry size, field offsets, and the
  Shift/Ctrl bits. `HandleRuneKey` reads the layout through these names, and
  the generated `config\layout_en_us.inc` follows the same format.
- `pack_format.inc`: the pack-table entry format and the `rune_pack` macro,
  which also keeps a running cell total (`rune_pool_cells`). `LoadRunePacks`
  reads the generated `config\rune_packs.inc` through this layout.
- `rune_constants.inc`: values shared by the rune files: rune size on screen,
  bank count, the fallback rune ID, and the bank-indicator
  geometry.

### Data

Data files declare no section either: `d_fasm.asm` includes the initialized
ones under `.data` and `rune_bss.inc` under `.bss`.

- `rune_data.inc`: rune data that needs initial values (currently the
  `..\assets\` path prefix string).
- `rune_bss.inc`: everything the rune code writes to at run time and that
  starts out empty: the pack path buffer, scratch values, the coverage
  palette, the mask table, and the mask pool. Reserved space only, so none of
  it takes room in the exe file. The pool is sized from the cell total the
  pack table accumulates, so this file is included after
  `config\rune_packs.inc`.

### Code

None of the code files declares a section: `d_fasm.asm` opens `.text` and
includes them in order.

- `rune_loader.inc`: `LoadRunePacks` reads every pack in `rune_pack_table`
  from `..\assets\` (relative to the executable folder), checks each file
  is exactly `count * 1024` bytes, and registers each cell in
  `rune_mask_table[bank*256 + rune]`. All current packs register into bank 0.
- `rune_bank.inc`: `ReadBank` returns the bank from lock-key state: 0 none,
  1 Caps Lock, 2 Scroll Lock (Scroll Lock wins if both are on).
- `rune_draw.inc`: `BuildCoveragePalette` (blends coverage from the `ds_0`
  background color toward white), `DrawMaskScaled`, `DrawRuneValue` (clears
  the surface and draws one mask enlarged 8x and centered; an unloaded ID
  falls back to rune `0x02`), and the bank indicator
  (`DrawBankIndicator` / `RefreshBankIndicator`, top center at 2x, skipped on
  surfaces too short to keep it clear of the centered rune).
- `rune_input.inc`: `HandleRuneKey` reads Shift/Ctrl, finds the key in the
  generated layout table, and draws the mapped rune. Lock keys refresh the
  bank indicator.

Every rune ID currently has its own stored mask; there is no runtime
mirror. A mirror hook is intentionally left in `DrawRuneValue` for the day
mirrored and rotated variants need to share masks (see the source README,
"Planned: mirror and rotate").

## `keys.inc`

Win32 virtual-key codes used by the project (`KEY_ESCAPE`, `KEY_SHIFT`,
`KEY_CONTROL`, `KEY_CAPITAL`, `KEY_SCROLL`). Constants only; the one home for
key codes.

## `win64_host.inc`

The Windows presentation boundary, in two files: `win64_host_data.inc`
(constants and data declarations) and `win64_host.inc` (code only). It owns the OS window and everything that
needs an `HWND`, and it knows nothing about runes or `ds_0` by name: it is
handed an `AREA` (position and size) and a frame-buffer pointer.

| Proc | Purpose |
|------|---------|
| `HostGetPrimaryBounds(area)` | fills an `AREA`'s x, y, w, h from the primary monitor |
| `HostCreate(area, buffer, title)` | registers the window class and creates a borderless window covering the `AREA`, presenting `buffer` |
| `HostRun()` | shows the window and runs the message loop until it closes |
| `HostClose()` | destroys the window, ending `HostRun` |

Key presses are passed to the application, not handled here: the host calls
`AppOnKey(virtual_key)` (supplied by `src\app_events.inc`) and acts on the
`HOST_KEY_*` value it returns (`IGNORED`: default handling, `REDRAW`: repaint,
`DONE`: nothing more to do). Needs `spatial.inc` included first.

## Current scope

The implementation asks the host for the primary monitor's rectangle,
copies it into the single `ds_0` descriptor, and allocates the frame buffer
from it.
Configuration-file loading is not implemented; `config.ini` is retained for
upcoming work.


