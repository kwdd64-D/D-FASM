; File: lib/rune/rune_loader.inc
; Loads the rune packs listed in rune_pack_table from ..\assets\ into rune_pool.
;
; Code only; no section directive. Include inside the .text section that d_fasm.asm opens.

; Loads every pack in rune_pack_table from ..\assets\ (relative to the exe folder),
; checks each file is exactly count*1024 bytes, and registers its cells in bank 0.
; Returns eax = 1 on success, 0 on any failure.
proc LoadRunePacks uses rbx rsi rdi r12 r13 r14 r15
    invoke GetModuleFileName, 0, rune_pack_path, 260
    test eax, eax
    jz .failed
    cmp eax, 260
    jae .failed

    mov esi, eax
.find_separator:
    test esi, esi
    jz .failed
    dec esi
    cmp word [rune_pack_path + rsi*2], 0x005C
    jne .find_separator
    inc esi                              ; esi = index just after the last backslash

    mov eax, esi
    add eax, RUNE_ASSETS_PREFIX_CHARS + RUNE_NAME_CHARS
    cmp eax, 260
    ja .failed                           ; path would not fit

    mov r12d, esi
    lea rdi, [rune_pack_path + r12*2]
    lea rsi, [rune_assets_prefix]
    mov ecx, RUNE_ASSETS_PREFIX_CHARS
    cld
    rep movsw
    add r12d, RUNE_ASSETS_PREFIX_CHARS   ; r12d = index where each filename goes

    lea r13, [rune_pack_table]           ; current table entry
    lea r14, [rune_pool]                 ; where this pack's masks are stored
    mov ebx, (rune_pack_table_end - rune_pack_table) / RUNE_PACK_ENTRY_BYTES
.next_pack:
    lea rdi, [rune_pack_path + r12*2]
    lea rsi, [r13 + 8]
    mov ecx, RUNE_NAME_CHARS
    rep movsw                            ; filename (includes its NUL)

    mov eax, [r13 + 4]
    shl eax, 10                          ; count * 1024
    mov [rune_pack_expect], eax

    invoke CreateFile, rune_pack_path, 0x80000000, 1, 0, 3, 0x80, 0
    cmp rax, -1
    je .failed
    mov r15, rax

    invoke GetFileSize, r15, 0
    cmp eax, [rune_pack_expect]
    jne .close_failed

    mov dword [rune_pack_bytes_read], 0
    mov eax, [rune_pack_expect]
    invoke ReadFile, r15, r14, rax, rune_pack_bytes_read, 0
    test eax, eax
    jz .close_failed
    mov eax, [rune_pack_bytes_read]
    cmp eax, [rune_pack_expect]
    jne .close_failed
    invoke CloseHandle, r15

    ; Register: bank 0, rune ID (first + i) -> r14 + i*1024
    mov eax, [r13]
    mov ecx, [r13 + 4]
    lea rdx, [rune_mask_table]
    lea rdx, [rdx + rax*8]
    mov rax, r14
.register_cell:
    mov [rdx], rax
    add rdx, 8
    add rax, RUNE_CELL_BYTES
    dec ecx
    jnz .register_cell

    mov eax, [r13 + 4]
    shl eax, 10
    add r14, rax
    add r13, RUNE_PACK_ENTRY_BYTES
    dec ebx
    jnz .next_pack

    mov eax, 1
    ret

.close_failed:
    invoke CloseHandle, r15
.failed:
    xor eax, eax
    ret
endp
