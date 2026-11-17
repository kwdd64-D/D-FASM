; File: lib/win64_host_data.inc
; Constants and data declarations for the host (lib/win64_host.inc). No code, and no section
; directive: include inside the .data section that d_fasm.asm opens, before win64_host.inc.
; Needs win64w.inc included first.

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

align 8
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
