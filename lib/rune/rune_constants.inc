; File: lib/rune/rune_constants.inc
; Constants shared by the rune files. Constants only: no code, no data.
; Key codes are not here: they live in lib/keys.inc.

; How large a rune is drawn on the surface (RUNE_VIEW_SIZE = 32 cells * RUNE_VIEW_SCALE).
RUNE_VIEW_SCALE   = 8
RUNE_VIEW_SIZE    = 256

; Banks and the fallback rune.
RUNE_BANK_COUNT   = 3
RUNE_BANK_DEFAULT = 0
RUNE_UNKNOWN_ID   = 0x02          ; bank 0: the one fallback rune for any unloaded mask

; Draw-time transform flags (see the PLANNED note in rune_draw.inc). Only mirror-x is wired up.
RUNE_XFORM_NONE     = 0
RUNE_XFORM_MIRROR_X = 1

; Bank indicator: the digit rune 0/1/2 (bank 0 digits, ID 0x30 + bank), drawn at 2x,
; top centre. Skipped on surfaces too short to keep it clear of the centred rune.
RUNE_DIGIT_BASE_ID    = 0x30
RUNE_INDICATOR_SCALE  = 2
RUNE_INDICATOR_PX     = 32 * RUNE_INDICATOR_SCALE
RUNE_INDICATOR_Y      = 24
RUNE_INDICATOR_MIN_H  = RUNE_INDICATOR_Y + RUNE_INDICATOR_PX + 24 + RUNE_VIEW_SIZE + 24
