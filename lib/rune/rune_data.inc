; File: lib/rune/rune_data.inc
; Rune data that needs initial values. Data only: no code, and no section directive.
; Include inside the .data section that d_fasm.asm opens.

rune_assets_prefix TCHAR '..\assets\', 0
rune_assets_prefix_end:
assert (rune_assets_prefix_end - rune_assets_prefix) = (RUNE_ASSETS_PREFIX_CHARS + 1) * 2
