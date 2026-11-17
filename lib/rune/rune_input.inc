; File: lib/rune/rune_input.inc
; Turns a key press into a rune: reads Shift/Ctrl, finds the key in the generated
; layout table (config/layout_en_us.inc), and draws the mapped rune. Lock keys refresh
; the bank indicator. The table format is defined in layout_format.inc.
;
; Code only; no section directive. Include inside the .text section that d_fasm.asm opens.

; The generated layout must be a whole number of entries (see layout_format.inc).
assert (layout_en_us_keys_end - layout_en_us_keys) mod LAYOUT_ENTRY_SIZE = 0

proc HandleRuneKey uses rbx rsi r12 r13 r14, virtual_key
    mov ebx, ecx
    xor r12d, r12d                   ; modifier mask built from LAYOUT_MOD_* bits
    invoke GetKeyState, KEY_SHIFT
    test ax, 0x8000
    jz .shift_up
    or r12d, LAYOUT_MOD_SHIFT
.shift_up:
    invoke GetKeyState, KEY_CONTROL
    test ax, 0x8000
    jz .ctrl_up
    or r12d, LAYOUT_MOD_CTRL
.ctrl_up:

.map_key:
    ; The map is data in config/layout_en_us.inc, not in this reader.
    lea rsi, [layout_en_us_keys]
    mov r13d, (layout_en_us_keys_end - layout_en_us_keys) / LAYOUT_ENTRY_SIZE
.map_loop:
    cmp word [rsi + LAYOUT_OFF_VK], bx
    jne .next_binding
    movzx eax, byte [rsi + LAYOUT_OFF_MOD]
    cmp eax, r12d
    jne .next_binding

    movzx r14d, byte [rsi + LAYOUT_OFF_RUNE]   ; rune value
    call ReadBank
    mov ebx, eax
    fastcall DrawRuneValue, rbx, r14
    mov eax, 1
    ret
.next_binding:
    add rsi, LAYOUT_ENTRY_SIZE
    dec r13d
    jnz .map_loop
.unhandled:
    ; Lock keys are not runes, but they change the bank: refresh the indicator.
    cmp ebx, KEY_CAPITAL
    je .lock_key
    cmp ebx, KEY_SCROLL
    je .lock_key
    xor eax, eax
    ret
.lock_key:
    call RefreshBankIndicator
    mov eax, 1
    ret
endp
