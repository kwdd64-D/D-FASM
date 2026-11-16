; File: lib/rune/rune_state.inc
; Rune runtime data: buffers, scratch values, the coverage palette, the mask table and the
; mask pool. Data only: no code. Include AFTER config/rune_packs.inc, because the pool is
; sized from the cell total that the table accumulates in rune_pool_cells.

section '.data' data readable writeable
    rune_pack_path dw 260 dup(0)
    rune_assets_prefix TCHAR '..\assets\', 0
    rune_assets_prefix_end:
    assert (rune_assets_prefix_end - rune_assets_prefix) = (RUNE_ASSETS_PREFIX_CHARS + 1) * 2
    align 4
    rune_pack_bytes_read dd 0
    rune_pack_expect     dd 0
    coverage_palette dd 256 dup(0)
    rune_origin_x dd 0
    rune_origin_y dd 0

; Uninitialized data: takes no space in the exe file. The loader supplies it
; zero-filled, which rune_mask_table relies on (0 = not loaded).
section '.bss' data readable writeable
    align 8
    rune_mask_table rb RUNE_BANK_COUNT * 256 * 8       ; [bank*256 + rune] -> mask address, 0 = not loaded
    rune_pool       rb rune_pool_cells * RUNE_CELL_BYTES ; all loaded masks, pack after pack
