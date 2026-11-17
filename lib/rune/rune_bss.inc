; File: lib/rune/rune_bss.inc
; Rune runtime state that starts out empty: the path buffer, scratch values, the coverage
; palette, the mask table, and the mask pool. Reserved space only (rb/rw/rd): none of it takes
; space in the exe file, and the loader supplies it zero-filled, which rune_mask_table relies
; on (0 = not loaded). No code and no section directive.
;
; Include inside the .bss section that d_fasm.asm opens, AFTER config/rune_packs.inc, because the
; pool is sized from the cell total the pack table accumulates in rune_pool_cells.

align 4
rune_pack_path       rw 260                        ; exe folder + ..\assets\ + pack file name
rune_pack_bytes_read rd 1
rune_pack_expect     rd 1
coverage_palette     rd 256                        ; coverage 0..255 -> 0xAARRGGBB, built at startup
rune_origin_x        rd 1
rune_origin_y        rd 1

align 8
rune_mask_table      rb RUNE_BANK_COUNT * 256 * 8         ; [bank*256 + rune] -> mask address, 0 = not loaded
rune_pool            rb rune_pool_cells * RUNE_CELL_BYTES ; all loaded masks, pack after pack
