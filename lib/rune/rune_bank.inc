; File: lib/rune/rune_bank.inc
; Which bank of runes is active. Policy lives here so it can change in one place
; (for example, giving Scroll Lock a different job).
;
; Code only; no section directive. Include after rune_loader.inc, which opens .text.

; Returns eax = current bank from the lock keys: 0 none, 1 Caps Lock, 2 Scroll Lock
; (both on: Scroll Lock wins, a temporary rule). Reads the live state every call.
proc ReadBank uses rbx
    xor ebx, ebx
    invoke GetKeyState, RUNE_VK_CAPITAL
    test al, 1
    jz .caps_off
    mov ebx, 1
.caps_off:
    invoke GetKeyState, RUNE_VK_SCROLL
    test al, 1
    jz .scroll_off
    mov ebx, 2
.scroll_off:
    mov eax, ebx
    ret
endp
