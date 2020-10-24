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
; TONY Kernel machine language terminal module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
MLT_BUFFER_SIZE     equ 80
MLT_CHAR            equ '*'
MLT_SEPARATOR_CHAR  equ ' '
MLT_READ            equ 'R'
MLT_WRITE           equ 'W'
MLT_EXECUTE         equ 'E'
MLT_NANI            equ '?'
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Starts the machine language terminal
; Address R returns the containing value
; Address R Address returns the containing values (span)
; Address W value sets the value of address to value
; Address W value value value ... sets the values of addresses to values
; Address E Sets execution to specified address
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
mlt_start:
    pusha
    pushf
    push es
    push ds

    mov ax, cs
    mov es, ax
    mov ds, ax

    .mlt_loop:
        ; putch
        mov al, MLT_CHAR
        int 0x90

        ; readln
        mov cx, MLT_BUFFER_SIZE
        mov di, .mlt_buffer
        int 0xb1

        ; split string
        mov ah, 0
        mov al, MLT_SEPARATOR_CHAR
        mov di, .mlt_buffer
        mov di, .mlt_split_buffer
        int 0xd4

        cmp byte [ds:di], MLT_READ
        je .mlt_read

        cmp byte [ds:di], MLT_WRITE
        je .mlt_write

        cmp byte [ds:di], MLT_EXECUTE
        je .mlt_execute

        ; putch
        mov al, MLT_NANI
        int 0x90

        jmp .mlt_loop

    .mlt_read:
        jmp .mlt_loop

    .mlt_write:
        jmp .mlt_loop

    .mlt_execute:
        jmp .mlt_loop

    .done:
        pop ds
        pop es
        popf
        popa
        iret

    .mlt_buffer       times MLT_BUFFER_SIZE db 0
    .mlt_split_buffer times MLT_BUFFER_SIZE db 0
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
;    iret / ret
;-------------------------------------------------------------------------------
