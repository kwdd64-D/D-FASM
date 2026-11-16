; File: lib/win64_host.inc
; Windows x64 host: the one OS window that covers a DisplaySurface, plus the message
; handler that presents the frame buffer. The model (surfaces, runes) never sees an HWND,
; and this file never sees the model: it is handed an AREA (position and size) and a frame
; buffer pointer, and it passes key presses to the application through AppOnKey.
;
; Needs spatial.inc (AREA offsets) included first. All procs use the Win64 fastcall convention.
;
;   HostGetPrimaryBounds(area)        fills the AREA's x, y, w, h from the primary monitor.
;                                     eax = 1 on success, 0 on failure.
;   HostCreate(area, buffer, title)   registers the window class and creates a borderless window
;                                     covering the AREA that presents `buffer` (32 bits per pixel,
;                                     top-down, area w * h pixels). eax = 1 on success, 0 on failure.
;   HostRun()                         shows the window and runs the message loop until it closes.
;                                     eax = 0 on a normal exit, 1 if message retrieval failed.
;   HostClose()                       destroys the window, which ends HostRun.
;
; The application must provide:
;   AppOnKey(virtual_key)             called on every key press. Returns a HOST_KEY_* value.

DIB_RGB_COLORS = 0
SW_SHOWNORMAL  = 1
CS_VREDRAW     = 1
CS_HREDRAW     = 2
WM_KEYDOWN     = 0x0100

; What AppOnKey returns to the host.
HOST_KEY_IGNORED = 0        ; not handled: let Windows apply its default
HOST_KEY_REDRAW  = 1        ; handled: repaint the window
HOST_KEY_DONE    = 2        ; handled: nothing to repaint (for example the window was closed)

; MONITORINFO layout: cbSize followed by rcMonitor, rcWork, and dwFlags.
DS_MONITORINFO_LEFT   = 4
DS_MONITORINFO_TOP    = 8
DS_MONITORINFO_RIGHT  = 12
DS_MONITORINFO_BOTTOM = 16

section '.data' data readable writeable
    wc           WNDCLASSEX sizeof.WNDCLASSEX, CS_VREDRAW or CS_HREDRAW, WindowProc, 0, 0, 0, 0, 0, 0, 0, host_class, 0
    msg          MSG
    ps           PAINTSTRUCT
    hwnd         dq 0
    hinstance    dq 0
    host_buffer  dq 0           ; frame buffer to present (owned by the caller)
    host_w       dd 0           ; its width and height in pixels
    host_h       dd 0

    align 4
    monitor_info:
        dd 40                ; sizeof(MONITORINFO)
        dd 0, 0, 0, 0        ; rcMonitor: left, top, right, bottom
        dd 0, 0, 0, 0        ; rcWork
        dd 0                 ; dwFlags

    align 8
    bmi:
        dd 40
        dd 0          ; width,  filled by HostCreate from the AREA
        dd 0          ; height, filled by HostCreate from the AREA (negative = top-down)
        dw 1
        dw 32
        dd 0, 0, 0, 0, 0, 0, 0

    align 2
    host_class   TCHAR 'DHostClass', 0

section '.text' code readable executable

; Fills the AREA at `area` with the primary monitor's current bounds.
proc HostGetPrimaryBounds uses rbx, area
    mov rbx, rcx
    invoke MonitorFromPoint, 0, 1 ; POINT(0,0), MONITOR_DEFAULTTOPRIMARY
    test rax, rax
    jz .failed
    invoke GetMonitorInfo, rax, monitor_info
    test eax, eax
    jz .failed

    mov eax, [monitor_info + DS_MONITORINFO_LEFT]
    mov [rbx + AREA_X], eax
    mov eax, [monitor_info + DS_MONITORINFO_TOP]
    mov [rbx + AREA_Y], eax
    mov eax, [monitor_info + DS_MONITORINFO_RIGHT]
    sub eax, [monitor_info + DS_MONITORINFO_LEFT]
    test eax, eax
    jle .failed
    mov [rbx + AREA_W], eax
    mov eax, [monitor_info + DS_MONITORINFO_BOTTOM]
    sub eax, [monitor_info + DS_MONITORINFO_TOP]
    test eax, eax
    jle .failed
    mov [rbx + AREA_H], eax
    mov eax, 1
    ret
.failed:
    xor eax, eax
    ret
endp

; Registers the window class and creates the borderless window covering `area`.
proc HostCreate uses rbx rsi rdi r12 r13, area, buffer, title
    mov rdi, rcx                    ; area
    mov [host_buffer], rdx
    mov r13, r8                     ; window title

    ; Bitmap header: width, and negative height for top-down rows.
    mov eax, [rdi + AREA_W]
    mov [host_w], eax
    mov [bmi + 4], eax
    mov eax, [rdi + AREA_H]
    mov [host_h], eax
    neg eax
    mov [bmi + 8], eax

    invoke GetModuleHandle, 0
    mov [hinstance], rax
    mov [wc.hInstance], rax

    invoke LoadCursor, 0, IDC_ARROW
    mov [wc.hCursor], rax

    invoke RegisterClassEx, wc
    test rax, rax
    jz .failed

    movsxd rbx, dword [rdi + AREA_X]
    movsxd rsi, dword [rdi + AREA_Y]
    movsxd r11, dword [rdi + AREA_W]
    movsxd r12, dword [rdi + AREA_H]
    invoke CreateWindowEx, \
           0, host_class, r13, WS_POPUP or WS_VISIBLE, \
           rbx, rsi, r11, r12, \
           0, 0, [hinstance], 0
    test rax, rax
    jz .failed
    mov [hwnd], rax
    mov eax, 1
    ret
.failed:
    xor eax, eax
    ret
endp

; Shows the window and runs the message loop. Returns when the window closes.
proc HostRun
    invoke ShowWindow, [hwnd], SW_SHOWNORMAL
    invoke UpdateWindow, [hwnd]
.msg_loop:
    invoke GetMessage, msg, 0, 0, 0
    cmp eax, -1
    je .message_error
    test rax, rax
    jz .quit
    invoke TranslateMessage, msg
    invoke DispatchMessage, msg
    jmp .msg_loop
.quit:
    xor eax, eax
    ret
.message_error:
    mov eax, 1
    ret
endp

proc HostClose
    invoke DestroyWindow, [hwnd]
    ret
endp

proc WindowProc uses rbx rsi rdi r12, hwnd, uMsg, wParam, lParam
    mov [hwnd], rcx
    mov [uMsg], rdx
    mov [wParam], r8
    mov [lParam], r9

    cmp edx, WM_PAINT
    je .on_paint
    cmp edx, WM_KEYDOWN
    je .on_keydown
    cmp edx, WM_DESTROY
    je .on_destroy

    invoke DefWindowProc, [hwnd], [uMsg], [wParam], [lParam]
    jmp .done

.on_keydown:
    fastcall AppOnKey, [wParam]          ; supplied by the application: direct call, not invoke
    cmp eax, HOST_KEY_REDRAW
    je .key_redraw
    cmp eax, HOST_KEY_DONE
    je .key_done
    invoke DefWindowProc, [hwnd], [uMsg], [wParam], [lParam]
    jmp .done
.key_redraw:
    invoke InvalidateRect, [hwnd], 0, 0
    invoke UpdateWindow, [hwnd]
.key_done:
    xor rax, rax
    jmp .done

.on_paint:
    invoke BeginPaint, [hwnd], ps
    mov r12, rax
    movsxd rbx, dword [host_w]
    movsxd rsi, dword [host_h]
    invoke StretchDIBits, r12, 0, 0, rbx, rsi, 0, 0, rbx, rsi, [host_buffer], bmi, DIB_RGB_COLORS, SRCCOPY
    invoke EndPaint, [hwnd], ps
    xor rax, rax
    jmp .done

.on_destroy:
    invoke PostQuitMessage, 0
    xor rax, rax
.done:
    ret
endp
