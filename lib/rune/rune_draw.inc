; File: lib/rune/rune_draw.inc
; Everything that puts runes on the DisplaySurface: the coverage palette, scaled mask
; drawing, the bank indicator, and drawing the selected rune.
;
; Code only; no section directive. Include after rune_loader.inc, which opens .text.
;
; PLANNED - mirror/rotate: with only 256 IDs per bank, mirrored and rotated variants
; (paired brackets, arrows) will eventually need to share one stored mask. DrawRuneValue
; keeps a transform-flags register (RUNE_XFORM_*) for that. Today it is always
; RUNE_XFORM_NONE. See "Planned: mirror and rotate" in src/README.md before removing it.

; Draws one 32x32 mask at (dest_x, dest_y) in pixel_buffer, each mask pixel
; enlarged to scale_px x scale_px. The caller keeps the whole block inside the buffer.
; rcx = mask address, edx = x, r8d = y, r9d = scale
proc DrawMaskScaled uses rbx rsi rdi r12 r13 r14 r15, mask_ptr, dest_x, dest_y, scale_px
    mov r12, rcx
    mov r13d, edx
    mov r14d, r8d
    mov r15d, r9d
    mov esi, [ds_0 + AREA_W]
    cld
    xor ebx, ebx                ; source y
.row_loop:
    xor r10d, r10d              ; source x
.column_loop:
    mov eax, ebx
    shl eax, 5
    add eax, r10d
    movzx eax, byte [r12 + rax]
    test eax, eax
    jz .next_pixel
    mov r9d, dword [coverage_palette + rax*4]

    mov eax, ebx
    imul eax, r15d
    add eax, r14d               ; pixel y
    mov edx, esi
    imul rax, rdx               ; y * width
    mov edx, r10d
    imul edx, r15d
    add edx, r13d               ; pixel x
    add rax, rdx
    shl rax, 2
    mov rdi, [pixel_buffer]
    add rdi, rax

    mov r8d, r15d
.scale_y:
    mov eax, r9d
    mov ecx, r15d
    rep stosd
    mov edx, esi
    sub edx, r15d
    shl edx, 2
    add rdi, rdx                ; down to the next row of this block
    dec r8d
    jnz .scale_y

.next_pixel:
    inc r10d
    cmp r10d, 32
    jb .column_loop
    inc ebx
    cmp ebx, 32
    jb .row_loop
    ret
endp

; Draws the bank digit (no clearing first). Used after the surface has been cleared.
proc DrawBankIndicator uses rbx
    cmp dword [ds_0 + AREA_H], RUNE_INDICATOR_MIN_H
    jb .skip
    call ReadBank
    add eax, RUNE_DIGIT_BASE_ID
    lea rdx, [rune_mask_table]
    mov rcx, [rdx + rax*8]      ; bank 0 digit mask
    test rcx, rcx
    jz .skip
    mov eax, [ds_0 + AREA_W]
    sub eax, RUNE_INDICATOR_PX
    shr eax, 1
    mov edx, eax                ; x: centred
    mov r8d, RUNE_INDICATOR_Y
    mov r9d, RUNE_INDICATOR_SCALE
    call DrawMaskScaled
.skip:
    ret
endp

; Clears the indicator's box, then draws the current bank digit. Used at startup and
; when a lock key is pressed, so the indicator never waits for a rune key.
proc RefreshBankIndicator uses rbx rdi r14 r15
    cmp dword [ds_0 + AREA_H], RUNE_INDICATOR_MIN_H
    jb .skip
    movsxd r11, dword [ds_0 + AREA_W]
    mov eax, [ds_0 + AREA_W]
    sub eax, RUNE_INDICATOR_PX
    shr eax, 1
    mov ebx, eax
    mov edx, [ds_0 + AREA_COLOR]
    draw_rect_block pixel_buffer, r11, rbx, RUNE_INDICATOR_Y, RUNE_INDICATOR_PX, RUNE_INDICATOR_PX, edx
    call DrawBankIndicator
.skip:
    ret
endp

; Builds the 256-entry coverage palette: coverage 0 is exactly ds_0's color, coverage 255
; is white, and values in between blend each color channel linearly. The surface color is
; read from ds_0, so changing it in one place keeps rune edges clean (no halo).
; Call after ds_0 is set up.
proc BuildCoveragePalette uses rbx rsi r12 r13 r14
    mov r12d, [ds_0 + AREA_COLOR]       ; background, 0xAARRGGBB
    xor esi, esi                        ; coverage 0..255
.palette_loop:
    xor r13d, r13d                      ; blended color being built
    xor r14d, r14d                      ; channel bit offset: 0 = blue, 8 = green, 16 = red
.channel_loop:
    mov ecx, r14d
    mov ebx, r12d
    shr ebx, cl
    movzx ebx, bl                       ; background value of this channel
    mov eax, 255
    sub eax, ebx                        ; distance from background to white
    imul eax, esi                       ; * coverage
    add eax, 127                        ; round to nearest
    xor edx, edx
    mov ecx, 255
    div ecx
    add eax, ebx                        ; background + blended share
    mov ecx, r14d
    shl eax, cl
    or r13d, eax
    add r14d, 8
    cmp r14d, 24
    jb .channel_loop
    or r13d, 0xFF000000                 ; fully opaque
    mov dword [coverage_palette + rsi*4], r13d
    inc esi
    cmp esi, 256
    jb .palette_loop
    ret
endp

proc DrawRuneValue uses rbx rsi rdi r12 r13 r14 r15, rune_bank, rune_value
    ; rcx = bank, rdx = rune value
    mov eax, ecx
    cmp eax, RUNE_BANK_COUNT
    jae .done                   ; unknown bank: draw nothing
    shl eax, 8
    movzx ebx, dl
    add eax, ebx
    lea rsi, [rune_mask_table]
    mov r13, [rsi + rax*8]      ; mask address, or 0 if not loaded
    test r13, r13
    jnz .have_mask
    mov r13, [rsi + RUNE_UNKNOWN_ID*8]   ; not loaded: use the fallback rune (bank 0, ID 0x02)
    test r13, r13
    jz .done                    ; even the fallback is missing: draw nothing
.have_mask:

    cmp dword [ds_0 + AREA_W], RUNE_VIEW_SIZE
    jb .done
    cmp dword [ds_0 + AREA_H], RUNE_VIEW_SIZE
    jb .done

    ; Clear the surface, leaving only the selected rune visible.
    movsxd r11, dword [ds_0 + AREA_W]
    movsxd r12, dword [ds_0 + AREA_H]
    mov edx, [ds_0 + AREA_COLOR]
    draw_rect_block pixel_buffer, r11, 0, 0, r11, r12, edx

    mov r12, r13                ; mask address from the table
    mov ebx, RUNE_XFORM_NONE    ; transform flags: always none for now, every ID has its own mask
                                ; (future: load from a per-ID transform field)

    mov esi, [ds_0 + AREA_W]
    mov eax, esi
    sub eax, RUNE_VIEW_SIZE
    shr eax, 1
    mov [rune_origin_x], eax
    mov eax, [ds_0 + AREA_H]
    sub eax, RUNE_VIEW_SIZE
    shr eax, 1
    mov [rune_origin_y], eax

    cld
    xor r13d, r13d              ; source y
.row_loop:
    xor r14d, r14d              ; destination/source x
.column_loop:
    mov eax, r14d
    test ebx, RUNE_XFORM_MIRROR_X
    jz .source_x_ready          ; mirror hook: reads the row right-to-left when set
    mov edx, 31
    sub edx, r14d
    mov eax, edx
.source_x_ready:
    mov edx, r13d
    shl edx, 5
    add eax, edx
    movzx eax, byte [r12 + rax]
    test eax, eax
    jz .next_pixel

    mov eax, dword [coverage_palette + rax*4]
    mov r9d, eax

    ; Compute the top-left pixel for this 8x8 coverage block.
    mov eax, r13d
    shl eax, 3
    add eax, [rune_origin_y]
    mov edx, esi
    imul rax, rdx
    mov edx, r14d
    shl edx, 3
    add edx, [rune_origin_x]
    add rax, rdx
    shl rax, 2
    mov rdi, [pixel_buffer]
    add rdi, rax

    mov r8d, RUNE_VIEW_SCALE
.scale_y:
    mov eax, r9d
    mov ecx, RUNE_VIEW_SCALE
    rep stosd
    mov edx, esi
    shl edx, 2
    sub edx, RUNE_VIEW_SCALE * 4
    add rdi, rdx
    dec r8d
    jnz .scale_y

.next_pixel:
    inc r14d
    cmp r14d, 32
    jb .column_loop
    inc r13d
    cmp r13d, 32
    jb .row_loop

    call DrawBankIndicator      ; the surface was cleared above, so redraw the indicator

.done:
    xor eax, eax
    ret
endp
