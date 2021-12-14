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
; Constants
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Entrypoint
;-------------------------------------------------------------------------------
MAIN:
    ; Load file (param)
    cmp byte [cs:argc], 0
    jz ERROR_NO_FILE

    mov si, argv
    mov di, program_data_start
    call load_file_to_buffer

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Errors
;-------------------------------------------------------------------------------
ERROR_NO_FILE:
    mov si, .msg
    mov al, 2
    int 0x97
    ret
    .msg db "Pass file as argument: basic <file>", 0
ERROR_FILE_NON_EXISTANT:
    mov si, .msg
    mov al, 2
    int 0x97
    ret
    .msg db "File does not exist!", 0
ERROR_FILE_TO_BIG:
    mov si, .msg
    mov al, 2
    int 0x97
    ret
    .msg db "File too big!", 0
ERROR_READING_FILE:
    mov si, .msg
    mov al, 2
    int 0x97
    ret
    .msg db "File not readable!", 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Loads the file specified by ds:si into the buffer at es:di
;-------------------------------------------------------------------------------
load_file_to_buffer:
    int 0xc1
    cmp ax, 0xffff
    jz  ERROR_FILE_NON_EXISTANT
    cmp dx, 0
    jnz  ERROR_FILE_TO_BIG
    cmp bx, program_max_size
    jae  ERROR_FILE_TO_BIG

    int 0xc0 ; read file to es:di
    jc ERROR_READING_FILE

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
program_data_start:
times (64*1024)-($-$$) 0
program_data_end:
program_max_size equ program_data_end - program_data_start
;-------------------------------------------------------------------------------