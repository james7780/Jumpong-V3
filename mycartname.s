; Overrideen cartridge name
; (Replaces default CARTNAME segment in atari5200.lib)
; JH - 2023

.export         __CART_NAME__: absolute = 1

.macpack        atari

.segment        "CARTNAME"

;                scrcode "   zz"
;                .byte   '6' + 32, '5' + 32      ; use playfield 1
;                scrcode " zzzzzzzz"
                .byte   0, 0, 'j', 'u', 'm', 'p', 'o', 'n', 'g', 0, 'v', 19, 0, 8, 'c', 'c', 22, 21, 9, 0

