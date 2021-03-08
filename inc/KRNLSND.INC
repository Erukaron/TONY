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
; TONY Kernel sound module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Output sound
;
; input: 
;   ax -> Frequency (see COMMON.INC)
;      -> = 0 for default beep (GIS2, 250ms)
;
; output:
;   none
;-------------------------------------------------------------------------------
snd_output:
    sti ; allow hardware interrupts

    push ax
    push bx
        cmp ax, 0
        jnz .skip_default
            mov ax, NOTE_GIS2
        .skip_default:
        mov bx, ax

        mov al, 182
        cli
        out 43h, al     ; tell speaker we're about to play a note
        
        in al, 61h
        and al, 11111100b
        out 61h, al     ; clearing lowest two bits cancels note

        mov ax, bx
        
        out 42h, al     ; output low byte
        mov al, ah
        out 42h, al     ; output high byte
        
        in al, 61h
        or al, 00000011b
        out 61h, al     ; setting lowest two bits turns on note
        sti
    pop bx
    pop ax

    ; check for default beep
    cmp ax, 0
    jnz .done
        ; sleep for default time and stop speaker
        mov ax, 250
        int 0x81
        int 0x89

    .done:
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; stop sound output
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
snd_stop_output:
    in al, 61h
    and al, 11111100b
    out 61h, al     ; clearing lowest two bits cancels note
    
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Description
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
;name:
;    ret
;-------------------------------------------------------------------------------
