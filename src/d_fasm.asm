; File: src/d_fasm.asm
; Step 1 of the DisplaySurface: one hard-set DisplaySurface (ds_0),
; except Esc.
 
format PE64 GUI 5.0
entry start
 
; Machine-specific paths live in build.bat (it sets the INCLUDE variable
; that FASM searches). Project files are found relative to this file.
include 'win64w.inc'
include '../lib/spatial.inc'
include '../lib/keys.inc'
 
section '.data' data readable writeable
    _title        TCHAR 'D', 0
    _error        TCHAR 'Host startup failed.', 0
    _message_error TCHAR 'Message retrieval failed.', 0
    _rune_pack_error TCHAR 'Rune pack missing or invalid.', 0

    ; --- DisplaySurface 0 ----------------------------------------------------
    ; Geometry is filled from the primary monitor at startup. The color is also the
    ; background the rune coverage palette blends from.
    align 32
    ds_0:
        dd 0, 0, 0              ; x, y, z
        dd 0, 0, 1              ; w, h, d
        dd 0xFF1E1E2E           ; color (dark)
        dd 0                    ; spare

    align 8
    buffer_bytes  dq 0          ; w * h * 4
    pixel_buffer  dq 0          ; the frame buffer: owned by this surface, only presented by the host

include '../lib/win64_host.inc'
include '../lib/draw2d.inc'
include '../lib/rune/layout_format.inc'
include '../lib/rune/pack_format.inc'
include '../lib/rune/rune_constants.inc'
include '../config/layout_en_us.inc'
include '../config/rune_packs.inc'
include '../lib/rune/rune_state.inc'
include '../lib/rune/rune_loader.inc'
include '../lib/rune/rune_bank.inc'
include '../lib/rune/rune_draw.inc'
include '../lib/rune/rune_input.inc'
include 'app_events.inc'

section '.text' code readable executable
start:
    sub rsp, 40                 ; shadow space + 16-byte alignment

    ; 1. Ask the host for the primary monitor's current bounds; they become ds_0's geometry.
    fastcall HostGetPrimaryBounds, ds_0
    test eax, eax
    jz .failed

    ; 2. Frame buffer size from ds_0
    mov eax, [ds_0 + AREA_W]
    imul eax, [ds_0 + AREA_H]
    shl rax, 2                  ; 4 bytes per pixel
    mov [buffer_bytes], rax

    ; 3. Frame buffer (zeroed by HEAP_ZERO_MEMORY = 8)
    invoke GetProcessHeap
    invoke HeapAlloc, rax, 8, [buffer_bytes]
    mov [pixel_buffer], rax
    test rax, rax
    jz .failed

    ; 4. Prepare the rune palette and load the rune packs.
    call BuildCoveragePalette
    call LoadRunePacks
    test eax, eax
    jz .rune_pack_failed

    ; 5. Fill ds_0 with its own color. Buffer coordinates start at 0,0;
    ;    ds_0's x,y place the host on the virtual desktop, not in the buffer.
    movsxd r11, dword [ds_0 + AREA_W]
    movsxd r12, dword [ds_0 + AREA_H]
    mov edx, [ds_0 + AREA_COLOR]
    draw_rect_block pixel_buffer, r11, 0, 0, r11, r12, edx
    call RefreshBankIndicator

    ; 6. Host window: borderless, exactly covering ds_0, presenting the frame buffer.
    fastcall HostCreate, ds_0, [pixel_buffer], _title
    test eax, eax
    jz .failed

    ; 7. Run until the window closes.
    fastcall HostRun
    test eax, eax
    jnz .message_error
    jmp .exit

.message_error:
    invoke MessageBox, 0, _message_error, _title, MB_OK or MB_ICONERROR
    invoke ExitProcess, 1

.rune_pack_failed:
    invoke MessageBox, 0, _rune_pack_error, _title, MB_OK or MB_ICONERROR
    invoke ExitProcess, 1

.failed:
    invoke MessageBox, 0, _error, _title, MB_OK or MB_ICONERROR
.exit:
    invoke ExitProcess, 0

section '.idata' import data readable
    library kernel32, 'KERNEL32.DLL', \
            user32,   'USER32.DLL', \
            gdi32,    'GDI32.DLL'

    include 'API\KERNEL32.INC'
    include 'API\USER32.INC'
    include 'API\GDI32.INC'
