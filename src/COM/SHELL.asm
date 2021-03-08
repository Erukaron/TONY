;-------------------------------------------------------------------------------
; MIT License
;
; Copyright (c) 2020 Erukaron
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%include "ASCII.INC"
%include "COM_HEAD.INC"
jmp MAIN
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Vars
;-------------------------------------------------------------------------------
enable_hacking_game_ident db "-fallout_style"
enable_hacking_game       db FALSE

fo_mode_match_ident      db "-match", 0
fo_mode_keystroke_ident  db "-keystroke", 0
fo_mode_auto_ident       db "-auto", 0
fo_mode                  db 0 ; (0 = match, 1 = keystroke, 2 = auto)

fo_level_ident           db "-lv=", 0
fo_pw_level              db 0 ; 0 - 4 ; 5 characters, 8 characters, 10 characters, 12 characters, 15 characters

fo_sound_ident           db "-snd", 0
fo_use_sound             db FALSE

fo_fast_mode_ident       db "-fast", 0
fo_fast_mode             db FALSE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Program entry point
;-------------------------------------------------------------------------------
MAIN:
    xor cx, cx
    mov cl, [cs:argc]
    cmp cl, 0
    jz .no_params
        .extract_params:
            mov si, argv
            mov di, enable_hacking_game_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_hacking_game
                mov byte [cs:enable_hacking_game], TRUE
            .no_hacking_game:

            mov di, fo_mode_match_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_mode_match
                mov byte [cs:fo_mode], 0
            .no_mode_match:

            mov di, fo_mode_keystroke_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_mode_keystroke
                mov byte [cs:fo_mode], 1
            .no_mode_keystroke:

            mov di, fo_mode_auto_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_mode_auto
                mov byte [cs:fo_mode], 2
            .no_mode_auto:

            mov di, fo_level_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_level
                ; ax contains relative start address of "-lv="
                push si
                    add si, ax
                    add si, 4 ; characters for -lv= -> si now points to the level
                    lodsb
                pop si

                sub al, ASCII_0 ; ascii adjust
                cmp al, 4
                ja .no_level ; al is not between 0 and 4 -> use default values
                    mov [ds:fo_pw_level], al
            .no_level:

            mov di, fo_sound_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .fo_no_sound
                mov byte [cs:fo_use_sound], TRUE
            .fo_no_sound:

            mov di, fo_fast_mode_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .fo_no_fast_mode
                mov byte [cs:fo_fast_mode], TRUE
            .fo_no_fast_mode:
    .no_params:

    mov al, TRUE
    cmp byte [ds:enable_hacking_game], al
    jnz .skip_hacking_game
        call fallout_hacking_game
    .skip_hacking_game:



    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%include "SHELL_FO.INC"
;-------------------------------------------------------------------------------
