; File: src/app_events.inc
; What the application does with events the host passes along. The host (lib/win64_host.inc)
; calls AppOnKey on every key press and acts on the HOST_KEY_* value it returns.
;
; Code only; no section directive. Include after lib/rune/rune_loader.inc, which opens .text.

; Esc closes the window. TEMPORARY: a full-surface window has no close control. Remove once a
; region provides one. (Alt+F4 also works.) Every other key goes to the rune code, which returns
; 1 (a rune or lock key: redraw) or 0 (not ours): the same values as HOST_KEY_REDRAW and
; HOST_KEY_IGNORED.
proc AppOnKey virtual_key
    cmp ecx, KEY_ESCAPE
    jne .rune_key
    call HostClose
    mov eax, HOST_KEY_DONE
    ret
.rune_key:
    fastcall HandleRuneKey, rcx
    ret
endp
