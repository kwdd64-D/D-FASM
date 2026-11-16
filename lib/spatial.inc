; File: lib/spatial.inc
; The Area: the one record shape every layer of D shares.
;
;   position   x, y, z      where the near-top-left corner sits
;   size       w, h, d      how far it extends along each axis
;   color      0xAARRGGBB   what it looks like
;
; Position first, size second, always in x-y-z order. Seven dwords, padded
; to eight so that (index * 32) is a shift, not a multiply.
;
; Axes on a flat surface:
;   x, y  pixels, origin at the top-left of the thing's parent
;   z     layer. Higher z is nearer the viewer: drawn later, hit-tested first
;   d     number of layers the thing occupies (1 = a single layer)
;
; "d" is layer extent, NOT bits-per-pixel. Color depth is not stored here.
;
; The first triple (x, y, z) is the Anchor: a point. An Area is an Anchor
; plus an extent (w, h, d) and a color.
 
AREA_X     = 0
AREA_Y     = 4
AREA_Z     = 8
AREA_W     = 12
AREA_H     = 16
AREA_D     = 20
AREA_COLOR = 24
AREA_SPARE = 28      ; reserved, keeps the record at 32 bytes
AREA_SIZE  = 32
